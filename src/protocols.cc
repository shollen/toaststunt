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
// This private function is used to implement both
// extract_telnet() and remove_telnet().
// Returns: true  Any existing subnegotiation were extracted or removed
//          false Error
bool
extract_remove_telnet(char       *replacement,
                      size_t     size,
                      const char *original,
                      bool       extract)
{
    bool successful = false;
    
    // Make sure we have both strings and that the replacemnt
    // is at least the size of the original with termination.
    if (   original    != nullptr
        && replacement != nullptr
        && size         > strlen(original)) {
        do {
            const char  *start, *end;

            // Search for subnegotiation start and end
            if ((start = strstr(original, "\xff\xfa")) != nullptr) {
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
            if ((end = strstr(start + 2, "\xff\xf0")) != nullptr) {
                if (extract) {
                    // Copy any leading characters
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

    return successful;
}



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
// Extract telnet subnegotiation
// Arguments: TYPE_STR string with telnet subnegotiation
// Returns:   TYPE_STR string with telnet subnegotiation body
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(telnet_extract(ansi24_printf("%c%ctelnet %c%cThe dog barks%c%cproto%c%c.%c%ccol%c%c", 255, 250, 255, 240, 255, 250, 255, 240, 255, 250, 255, 240)))
static package
bf_telnet_extract(Var arglist, Byte next, void *vdata, Objid progr) {
    // This is a private function so its safe to require that
    // pointers have been checked for null by the caller
    
    Var        rv;
    const char *original = arglist.v.list[1].v.str;
    char       replacement[256];
    
    if (extract_remove_telnet(replacement,
                              sizeof(replacement),
                              original,
                              true)) {
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
// Remove telnet subnegotiation
// Arguments: TYPE_STR string with telnet subnegotiation
// Returns:   TYPE_STR string with no telnet subnegotiation
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(telnet_remove(ansi24_printf("%c%ctelnet %c%cThe dog barks%c%cproto%c%c.%c%ccol%c%c", 255, 250, 255, 240, 255, 250, 255, 240, 255, 250, 255, 240)))
static package
bf_telnet_remove(Var arglist, Byte next, void *vdata, Objid progr) {
    // This is a private function so its safe to require that
    // pointers have been checked for null by the caller
    
    Var        rv;
    const char *original = arglist.v.list[1].v.str;
    char       replacement[256];
    
    if (extract_remove_telnet(replacement,
                              sizeof(replacement),
                              original,
                              false)) {
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
// Display the builtin functions, arguments and return values
// Returns: TYPE_STR string with builtin descriptions
//          TYPE_ERR E_RANGE Internal buffer too small
// Testing: ;player:tell(protocols_display_builtins())
static package
bf_protocols_display_builtins(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var        rv;
    const char info[] = "protocols Builtin Functions\n"
                        "\n"
                        "Information:\n"
                        "protocols_version          ()\n"
                        "                           --> TYPE_STR version\n"
                        "protocols_display_builtins ()\n"
                        "                           --> TYPE_STR available builtin functions\n"
                        "\n"
                        "Telnet subnegotiation extraction and removal:\n"
                        "telnet_extract             (TYPE_STR string with telnet subnegotiation)\n"
                        "                           --> TYPE_STR telnet subnegotiation\n"
                        "                           --> TYPE_ERR E_RANGE\n"
                        "telnet_remove              (TYPE_STR string with telnet subnegotiation)\n"
                        "                           --> TYPE_STR string with no telnet subnegotiation\n"
                        "                           --> TYPE_ERR E_RANGE\n";

    rv.type  = TYPE_STR;
    rv.v.str = str_dup(info);
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Make our public builtins accessible
void
register_protocols(void)
{
    register_function("protocols_version",          0,  0, bf_protocols_version);
    register_function("protocols_display_builtins", 0,  0, bf_protocols_display_builtins);
    register_function("telnet_extract",             1,  1, bf_telnet_extract, TYPE_STR);
    register_function("telnet_remove",              1,  1, bf_telnet_remove, TYPE_STR);
}

#endif  // NO_MOO_BUILTINS
