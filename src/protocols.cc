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
                                   uint8_t protocol) {
    // This is a private function so its safe to require that
    // pointers have been checked for null by the caller
    
    bool successful = false;

    // Handle the MXP protocol
    if (protocol      == mxp_id
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
    else if ((   protocol == 0
              || protocol == gmcp_id
              || protocol == msdp_id)
             && start_size >= 4
             && end_size   >= 3) {
        start_buffer[0] = telnet_interpret_as_command;
        start_buffer[1] = telnet_start_subnegotiation;
        start_buffer[2] = protocol;
        start_buffer[3] = 0;
        end_buffer[0]   = telnet_interpret_as_command;
        end_buffer[1]   = telnet_end_subnegotiation;
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
                        uint8_t    protocol,
                        bool       extract)
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
bool
protocol_request(uint8_t protocol)
{
    bool successful = false;

    if (   protocol == gmcp_id
        || protocol == msdp_id
        || protocol == mxp_id) {
        successful = true;
    }

    return successful;
}

// -----------------------------------------------------------------------------
bool
protocol_extract(char       *replacement,
                 size_t     size,
                 const char *original,
                 uint8_t    protocol)
{
    return extract_remove_protocol(replacement,
                                   size,
                                   original,
                                   protocol,
                                   true);
}

// -----------------------------------------------------------------------------
bool
protocol_remove(char       *replacement,
                size_t     size,
                const char *original,
                uint8_t    protocol)
{
    return extract_remove_protocol(replacement,
                                   size,
                                   original,
                                   protocol,
                                   false);
}

// -----------------------------------------------------------------------------
bool
protocol_create(char       *message,
                size_t     size,
                const char *body,
                uint8_t    protocol,
                uint8_t    tag)
{
    bool successful = false;
    char start_pattern[8], end_pattern[8];

    if (get_protocol_start_end(start_pattern, sizeof(start_pattern),
                               end_pattern  , sizeof(end_pattern),
                               protocol)) {
        
        // Handle the MXP protocol
        if (protocol == 27) {
            char tag_str[32];
            
            snprintf(tag_str, sizeof(tag_str), "%d", tag);
            tag_str[sizeof(tag_str) - 1] = 0;
            successful =    copy_substring(message,
                                           size,
                                           start_pattern,
                                           strlen(start_pattern))
                         && copy_substring(message,
                                           size,
                                           tag_str,
                                           strlen(tag_str))
                         && copy_substring(message,
                                           size,
                                           end_pattern,
                                           strlen(end_pattern))
                         && copy_substring(message,
                                           size,
                                           body,
                                           strlen(body));
        }
        // Handle the telnet based protocols
        else {
            successful =    copy_substring(message,
                                           size,
                                           start_pattern,
                                           strlen(start_pattern))
                         && copy_substring(message,
                                           size,
                                           body,
                                           strlen(body))
                         && copy_substring(message,
                                           size,
                                           end_pattern,
                                           strlen(end_pattern));
        }
    }

    return successful;
}



// -----------------------------------------------------------------------------
// Builtins for the moo code
//
// protocols_version ()
//                   --> TYPE_STR version
// protocols_request ({TYPE_INT protocol = { 201, 69, 27 })
//                                           201 = GMCP
//                                            69 = MSDP
//                                            27 = MXP}
//                   --> TYPE_INT 1
//                   --> TYPE_ERR E_RANGE
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
// Request the use of a protocol
// Arguments: TYPE_INT protocol = { 201, 69, 27 }
//                                201 = GMCP
//                                 69 = MSDP
//                                 27 = MXP
// Return:  TYPE_INT 1
//          TYPE_ERR E_RANGE
// Testing: ;player:tell(protocols_request(201))
//          ;player:tell(protocols_request(69))
//          ;player:tell(protocols_request(27))
static package
bf_protocols_request(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var           rv;
    const uint8_t protocol = arglist.v.list[1].v.num;

    if (protocol_request(protocol)) {
        rv.type  = TYPE_INT;
        rv.v.num = 1;
    }
    else {
        free_var(arglist);
        return make_error_pack(E_RANGE);
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Extract out-of-band data
// Arguments: TYPE_STR string with out-of-band data,
//            {optional} TYPE_INT protocol = { 0, 201, 69, 27 }
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
    uint8_t    protocol  = (nargs >= 2) ? arglist.v.list[2].v.num : 0;
    char       replacement[256];
    
    if (protocol_extract(replacement,
                         sizeof(replacement),
                         original,
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
//            {optional} TYPE_INT protocol = { 0, 201, 69, 27 }
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
    uint8_t    protocol  = (nargs >= 2) ? arglist.v.list[2].v.num : 0;
    char       replacement[256];
    
    if (protocol_remove(replacement,
                        sizeof(replacement),
                        original,
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
// Testing:   ;player:tell(replace_substring(replace_substring(replace_substring(protocols_create("message"), chr(255), "{IAC}"), chr(250), "{SB}"), chr(240), "{SE}"))
//            ;player:tell(replace_substring(replace_substring(replace_substring(protocols_create("message", 0), chr(255), "{IAC}"), chr(250), "{SB}"), chr(240), "{SE}"))
//            ;player:tell(replace_substring(replace_substring(replace_substring(replace_substring(protocols_create("message", 201), chr(201), "{GMCP}"), chr(255), "{IAC}"), chr(250), "{SB}"), chr(240), "{SE}"))
//            ;player:tell(replace_substring(replace_substring(replace_substring(replace_substring(protocols_create("message", 69), chr(69), "{MSDP}"), chr(255), "{IAC}"), chr(250), "{SB}"), chr(240), "{SE}"))
//            ;player:tell(replace_substring(protocols_create("message", 27), chr(27), "{esc}"))
//            ;player:tell(replace_substring(protocols_create("message", 27, 99), chr(27), "{esc}"))
static package
bf_protocols_create(Var arglist, Byte next, void *vdata, Objid progr) {
    Var        rv;
    const int  nargs     = (int) arglist.v.list[0].v.num;
    const char *body     = arglist.v.list[1].v.str;
    uint8_t    protocol  = (nargs >= 2) ? arglist.v.list[2].v.num : 0;
    uint8_t    tag       = (nargs >= 3) ? (int) arglist.v.list[3].v.num : 0;
    char       message[256];
    
    if (protocol_create(message, sizeof(message), body, protocol, tag)) {
        rv.type  = TYPE_STR;
        rv.v.str = str_dup(message);
    } else {
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
    register_function("protocols_request", 1,  1, bf_protocols_request, TYPE_INT);
    register_function("protocols_extract", 1,  2, bf_protocols_extract, TYPE_STR, TYPE_INT);
    register_function("protocols_remove",  1,  2, bf_protocols_remove, TYPE_STR, TYPE_INT);
    register_function("protocols_create",  1,  3, bf_protocols_create, TYPE_STR, TYPE_INT, TYPE_INT);
}

#endif  // NO_MOO_BUILTINS
