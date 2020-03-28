//
//  protocols.cc
//
//  Copyright (C) 2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//

#include "substring.h"
#include "protocols.h"

#ifndef NO_MOO_BUILTINS
#include "functions.h"
#include "utils.h"
#endif

using namespace std;



// -----------------------------------------------------------------------------
// In an ansi escape sequence search for a terminator, 'm'.
// Characters passed over should be numbers or a separator, ';'.
// Returns: != nullptr Pointer to terminator
//          == nullptr Error
static const char *
scan_for_terminator(const char *src)
{
    const char *terminator = nullptr;
    
    if (src != nullptr) {
        while (*src != 0) {
            // TODO: This takes care of the sequences we generate
            //       but should we handle all CSI sequences?
            switch (*src) {
                case '0' :
                case '1' :
                case '2' :
                case '3' :
                case '4' :
                case '5' :
                case '6' :
                case '7' :
                case '8' :
                case '9' :
                case ';' :
                    ++src;
                    break;
                case 'z' :
                    terminator = src;
                    // Fall through to default, we're done
                default :
                    src = "";   // Will terminate while loop
                    break;
            }
        }
    }
    
    return terminator;
}

// -----------------------------------------------------------------------------
// This private function is used to implement both
// protocols_extract() and protocols_remove().
// Returns: true  Any existing out-of-band data was extracted or removed
//          false Error
bool
extract_remove_protocol(char       *replacement,
                        size_t     size,
                        const char *original,
                        bool       extract,
                        char       protocol)
{
    bool successful = false;
    
    // Make sure we have both strings and that the replacemnt
    // is at least the size of the original with termination.
    if (   original    != nullptr
        && replacement != nullptr
        && size         > strlen(original)) {
        const char *start, *end;

        // Handle the MXP protocol
        if (protocol == 27) {
            do {
                // Search for escape sequence start and end
                if ((start = strstr(original, "\x1b[")) != nullptr) {
                    if (! extract) {
                        // Copy any leading characters
                        copy_substring(replacement,
                                       size,
                                       original,
                                       start - original);
                    }
                }
                else {
                    break;
                }
                if ((end = scan_for_terminator(start + 2)) != nullptr) {
                    if (extract) {
                        // Copy out-of-band characters
                        successful = copy_substring(replacement,
                                                    size,
                                                    start + 2,
                                                    end - start - 2);
                    }
                }
                else {
                    break;
                }
            
                // Continue the search after the closing bracket
                original = end + 1;
            } while(1);
            
            if (! extract) {
                // Copy any remaining characters
                successful = copy_substring(replacement,
                                            size,
                                            original,
                                            strlen(original));
            }
        }
        
        // Handle the telnet based protocols
        else {
            char   start_subnegotiation[4] = "\xff\xfa\x00",    // May add protocol
                   end_subnegotiation[3]   = "\xff\xf0";
            size_t start_len               = 2;
            
            if (protocol != 0) {
                start_subnegotiation[2] = protocol;
                start_len               = 3;
            }
            
            do {
                // Search for subnegotiation start and end
                if ((start = strstr(original, start_subnegotiation)) != nullptr) {
                    if (! extract) {
                        // Copy any leading characters
                        copy_substring(replacement,
                                       size,
                                       original,
                                       start - original);
                    }
                }
                else {
                    break;
                }
                if ((end = strstr(start + 2, end_subnegotiation)) != nullptr) {
                    if (extract) {
                        // Copy out-of-band characters
                        successful = copy_substring(replacement,
                                                    size,
                                                    start + start_len,
                                                    end - start - start_len);
                    }
                }
                else {
                    break;
                }
            
                // Continue the search after the closing bracket
                original = end + 2;
            } while(1);
            
            if (! extract) {
                // Copy any remaining characters
                successful = copy_substring(replacement,
                                            size,
                                            original,
                                            strlen(original));
            }
        }
    }

    return successful;
}



// -----------------------------------------------------------------------------
// Builtins for the moo code
//
// protocols_version ()
//                   --> TYPE_STR version
// protocols_extract (TYPE_STR string with out-of-band data,
//                   {optional} TYPE_INT protocol = { 0, 201, 69 })
//                                                    0 = telnet
//                                                  201 = GMCP
//                                                   69 = MSDP
//                                                   27 = MXP
//                   --> TYPE_STR out-of-band data
//                   --> TYPE_ERR E_RANGE
// protocols_remove  (TYPE_STR string with out-of-band data)
//                   --> TYPE_STR string with out-of-band data removed
//                   --> TYPE_ERR E_RANGE
// -----------------------------------------------------------------------------

#ifndef NO_MOO_BUILTINS

// -----------------------------------------------------------------------------
// Identify the package and version
// Return:  TYPE_STR version
// Testing: ;player:tell(protocols_version())
static package
bf_protocols_version(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var  rv;

    // Package informaion and version
    rv.type  = TYPE_STR;
    rv.v.str = str_dup("protocols 1.0.0");
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Extract out-of-band data
// Arguments: TYPE_STR string with out-of-band data
//            {optional} TYPE_INT protocol = { 0, 201, 69 })
//                                             0 = telnet
//                                           201 = GMCP
//                                            69 = MSDP
//                                            27 = MXP
// Returns:   TYPE_STR out-of-band data
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(protocols_extract(ansi24_printf("%c%ctelnet %c%cThe dog barks%c%cproto%c%c.%c%ccol%c%c", 255, 250, 255, 240, 255, 250, 255, 240, 255, 250, 255, 240)))
//            ;player:tell(protocols_extract(ansi24_printf("%c%ctelnet %c%cThe dog barks%c%cproto%c%c.%c%ccol%c%c", 255, 250, 255, 240, 255, 250, 255, 240, 255, 250, 255, 240), 0))
//            ;player:tell(protocols_extract(ansi24_printf("%c%c%cgmcp %c%cThe dog barks%c%c%cproto%c%c.%c%c%ccol%c%c", 255, 250, 201, 255, 240, 255, 250, 201, 255, 240, 255, 250, 201, 255, 240), 201))
//            ;player:tell(protocols_extract(ansi24_printf("%c%c%cmsdp %c%cThe dog barks%c%c%cproto%c%c.%c%c%ccol%c%c", 255, 250, 69, 255, 240, 255, 250, 69, 255, 240, 255, 250, 69, 255, 240), 69))
//            ;player:tell(protocols_extract(ansi24_printf("%c%c1%cThe dog barks%c%c2%c.%c%c3%c", 27, 91, 122, 27, 91, 122, 27, 91, 122), 27))
static package
bf_protocols_extract(Var arglist, Byte next, void *vdata, Objid progr) {
    Var        rv;
    const int  nargs     = (int) arglist.v.list[0].v.num;
    const char *original = arglist.v.list[1].v.str;
    char       protocol  = (nargs >= 2) ? arglist.v.list[2].v.num : 0;
    char       replacement[256];
    
    if (extract_remove_protocol(replacement,
                                sizeof(replacement),
                                original,
                                true,
                                protocol)) {
        rv.type  = TYPE_STR;
        rv.v.str = str_dup(replacement);
    }
    else {
        free_var(arglist);
        return make_error_pack(E_RANGE);
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Remove out-of-band data
// Arguments: TYPE_STR string with out-of-band data
// Returns:   TYPE_STR string with out-of-band data removed
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(protocols_remove(ansi24_printf("%c%ctelnet %c%cThe dog barks%c%cproto%c%c.%c%ccol%c%c", 255, 250, 255, 240, 255, 250, 255, 240, 255, 250, 255, 240)))
//            ;player:tell(protocols_remove(ansi24_printf("%c%ctelnet %c%cThe dog barks%c%cproto%c%c.%c%ccol%c%c", 255, 250, 255, 240, 255, 250, 255, 240, 255, 250, 255, 240), 0))
//            ;player:tell(protocols_remove(ansi24_printf("%c%c%cgmcp %c%cThe dog barks%c%c%cproto%c%c.%c%c%ccol%c%c", 255, 250, 201, 255, 240, 255, 250, 201, 255, 240, 255, 250, 201, 255, 240), 201))
//            ;player:tell(protocols_remove(ansi24_printf("%c%c%cmsdp %c%cThe dog barks%c%c%cproto%c%c.%c%c%ccol%c%c", 255, 250, 69, 255, 240, 255, 250, 69, 255, 240, 255, 250, 69, 255, 240), 69))
//            ;player:tell(protocols_remove(ansi24_printf("%c%c1%cThe dog barks%c%c2%c.%c%c3%c", 27, 91, 122, 27, 91, 122, 27, 91, 122), 27))
static package
bf_protocols_remove(Var arglist, Byte next, void *vdata, Objid progr) {
    Var        rv;
    const int  nargs     = (int) arglist.v.list[0].v.num;
    const char *original = arglist.v.list[1].v.str;
    char       protocol  = (nargs >= 2) ? arglist.v.list[2].v.num : 0;
    char       replacement[256];
    
    if (extract_remove_protocol(replacement,
                                sizeof(replacement),
                                original,
                                false,
                                protocol)) {
        rv.type  = TYPE_STR;
        rv.v.str = str_dup(replacement);
    }
    else {
        free_var(arglist);
        return make_error_pack(E_RANGE);
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Make our public builtins accessible
void
register_protocols(void)
{
    register_function("protocols_version", 0,  0, bf_protocols_version);
    register_function("protocols_extract", 1,  2, bf_protocols_extract, TYPE_STR, TYPE_INT);
    register_function("protocols_remove",  1,  2, bf_protocols_remove, TYPE_STR, TYPE_INT);
}

#endif  // NO_MOO_BUILTINS
