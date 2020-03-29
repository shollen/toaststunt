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
// Get the the protocol starting and ending substrings
static bool get_protocol_start_end(char *start_buffer, size_t start_size,
                                   char *end_buffer,   size_t end_size,
                                   char protocol) {
    // This is a private function so its safe to require that
    // pointers have been checked for null by the caller
    
    bool successful = false;

    // Handle the MXP protocol
    if (protocol      == 27
        && start_size >= 3
        && end_size   >= 2) {
        start_buffer[0] = '\x1b';
        start_buffer[1] = '[';
        start_buffer[2] = 0;
        end_buffer[0]   = 'z';
        end_buffer[1]   = 0;
        successful      = true;
    }
    // Handle the telnet based protocols
    else if (   start_size >= 4
             && end_size   >= 3) {
        start_buffer[0] = '\xff';
        start_buffer[1] = '\xfa';
        start_buffer[2] = protocol;
        start_buffer[3] = 0;
        end_buffer[0]   = '\xff';
        end_buffer[1]   = '\xf0';
        end_buffer[2]   = 0;
        successful      = true;
    }

    return successful;
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
        char start_pattern[8], end_pattern[8];

        if (get_protocol_start_end(start_pattern, sizeof(start_pattern),
                                   end_pattern  , sizeof(end_pattern),
                                   protocol)) {
            size_t start_len = strlen(start_pattern),
                   end_len   = strlen(end_pattern);

            do {
                const char *start, *end;
                
                // Search for out-of-band start and end
                if ((start = strstr(original, start_pattern)) != nullptr) {
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
                if ((end = strstr(start + 2, end_pattern)) != nullptr) {
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
            
                // Continue the search after the out-of-band end
                original = end + end_len;
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
//                   {optional} TYPE_INT protocol = { 0, 201, 69, 27 })
//                                                    0 = telnet
//                                                  201 = GMCP
//                                                   69 = MSDP
//                                                   27 = MXP
//                   --> TYPE_STR out-of-band data
//                   --> TYPE_ERR E_RANGE
// protocols_remove  (TYPE_STR string with out-of-band data),
//                   {optional} TYPE_INT protocol = { 0, 201, 69, 27 })
//                                                    0 = telnet
//                                                  201 = GMCP
//                                                   69 = MSDP
//                                                   27 = MXP
//                   --> TYPE_STR string with out-of-band data removed
//                   --> TYPE_ERR E_RANGE
// protocols_create  (TYPE_STR message body,
//                    {optional} TYPE_INT protocol = { 0, 201, 69, 27 },
//                                                     0 = telnet
//                                                   201 = GMCP
//                                                    69 = MSDP
//                                                    27 = MXP
//                    {optional} TYPE_STR MXP protocol tag)
//                   --> TYPE_STR string with out-of-band data
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
// Arguments: TYPE_STR string with out-of-band data,
//            {optional} TYPE_INT protocol = { 0, 201, 69, 27 })
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
//            ;player:tell(protocols_extract(ansi24_printf("%c%cmxp %cThe dog barks%c%cproto%c.%c%ccol%c", 27, 91, 122, 27, 91, 122, 27, 91, 122), 27))
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
// Arguments: TYPE_STR string with out-of-band data,
//            {optional} TYPE_INT protocol = { 0, 201, 69, 27 })
//                                             0 = telnet
//                                           201 = GMCP
//                                            69 = MSDP
//                                            27 = MXP
// Returns:   TYPE_STR string with out-of-band data removed
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(protocols_remove(ansi24_printf("%c%ctelnet %c%cThe dog barks%c%cproto%c%c.%c%ccol%c%c", 255, 250, 255, 240, 255, 250, 255, 240, 255, 250, 255, 240)))
//            ;player:tell(protocols_remove(ansi24_printf("%c%ctelnet %c%cThe dog barks%c%cproto%c%c.%c%ccol%c%c", 255, 250, 255, 240, 255, 250, 255, 240, 255, 250, 255, 240), 0))
//            ;player:tell(protocols_remove(ansi24_printf("%c%c%cgmcp %c%cThe dog barks%c%c%cproto%c%c.%c%c%ccol%c%c", 255, 250, 201, 255, 240, 255, 250, 201, 255, 240, 255, 250, 201, 255, 240), 201))
//            ;player:tell(protocols_remove(ansi24_printf("%c%c%cmsdp %c%cThe dog barks%c%c%cproto%c%c.%c%c%ccol%c%c", 255, 250, 69, 255, 240, 255, 250, 69, 255, 240, 255, 250, 69, 255, 240), 69))
//            ;player:tell(protocols_remove(ansi24_printf("%c%cmxp %cThe dog barks%c%cproto%c.%c%ccol%c", 27, 91, 122, 27, 91, 122, 27, 91, 122), 27))
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
// Create a string containing out-of-band data
// Arguments: TYPE_STR message body,
//            {optional} TYPE_INT protocol = { 0, 201, 69, 27 },
//                                             0 = telnet
//                                           201 = GMCP
//                                            69 = MSDP
//                                            27 = MXP
//            {optional} TYPE_STR MXP protocol tag)
// Returns:   TYPE_STR string with out-of-band data
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(replace_substring(replace_substring(replace_substring(protocols_create("message"), chr(255), "{FF}"), chr(250), "{FA}"), chr(240), "{F0}"))
//            ;player:tell(replace_substring(replace_substring(replace_substring(protocols_create("message", 0), chr(255), "{FF}"), chr(250), "{FA}"), chr(240), "{F0}"))
//            ;player:tell(replace_substring(replace_substring(replace_substring(replace_substring(protocols_create("message", 201), chr(255), "{FF}"), chr(250), "{FA}"), chr(240), "{F0}"), chr(201), "{GMCP}"))
//            ;player:tell(replace_substring(replace_substring(replace_substring(replace_substring(protocols_create("message", 69), chr(255), "{FF}"), chr(250), "{FA}"), chr(240), "{F0}"), chr(69), "{MSDP}"))
//            ;player:tell(replace_substring(protocols_create("message", 27), chr(27), "{esc}"))
//            ;player:tell(replace_substring(protocols_create("message", 27, 99), chr(27), "{esc}"))
static package
bf_protocols_create(Var arglist, Byte next, void *vdata, Objid progr) {
    Var        rv;
    const int  nargs     = (int) arglist.v.list[0].v.num;
    const char *message  = arglist.v.list[1].v.str;
    const char protocol  = (nargs >= 2) ? arglist.v.list[2].v.num : 0;
    const int  tag       = (nargs >= 3) ? (int) arglist.v.list[3].v.num : 0;
    char       start_pattern[8], end_pattern[8];
    char       replacement[256];

    if (get_protocol_start_end(start_pattern, sizeof(start_pattern),
                               end_pattern  , sizeof(end_pattern),
                               protocol)) {
        char   *dest = replacement;
        size_t size  = sizeof(replacement);
        
        // Handle the MXP protocol
        if (protocol == 27) {
            char tag_str[32];
            
            snprintf(tag_str, sizeof(tag_str), "%d", tag);
            tag_str[sizeof(tag_str) - 1] = 0;
            if (   copy_substring(dest, size, start_pattern, strlen(start_pattern))
                && copy_substring(dest, size, tag_str,       strlen(tag_str))
                && copy_substring(dest, size, end_pattern,   strlen(end_pattern))
                && copy_substring(dest, size, message,       strlen(message))) {
            } else {
                free_var(arglist);
                return make_error_pack(E_RANGE);
            }
        }
        // Handle the telnet based protocols
        else {
            if (   copy_substring(dest, size, start_pattern, strlen(start_pattern))
                && copy_substring(dest, size, message,       strlen(message))
                && copy_substring(dest, size, end_pattern,   strlen(end_pattern))) {
            } else {
                free_var(arglist);
                return make_error_pack(E_RANGE);
            }
        }
    } else {
        free_var(arglist);
        return make_error_pack(E_RANGE);
    }

    rv.type  = TYPE_STR;
    rv.v.str = str_dup(replacement);

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
    register_function("protocols_create",  1,  3, bf_protocols_create, TYPE_STR, TYPE_INT, TYPE_INT);
}

#endif  // NO_MOO_BUILTINS
