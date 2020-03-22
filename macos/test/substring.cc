//
//  substring.cc
//
//  Copyright (C) 2019-2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//

#include "substring.h"

#ifndef NO_MOO_BUILTINS
#include "functions.h"
#include "utils.h"
#endif

using namespace std;



// -----------------------------------------------------------------------------
// Replace one substring with another or remove the substring
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// This private function is used to implement both
// replace_substring() and remove_substring().
// Note removing a substring is really replacing it with "".
// Returns: true  Any existing substring was replaced or removed
//          false Error
static bool
replace_remove_substring(char       *replacement,
                         size_t     size,
                         const char *original,
                         const char *find,
                         const char *replace)
{
    bool successful = false;

    // Make sure we have all the strings and that the replacemnt buffer
    // is at least the size of the original with termination.
    // That's propbably not large enough if substitutions are made
    // but if there are none and the string is just a copy it will fit.
    // Also make sure we are searching for a non-empty substring.
    if (   replacement  != nullptr
        && original     != nullptr
        && find         != nullptr
        && replace      != nullptr
        && size          > strlen(original)
        && strlen(find)  > 0) {
        size_t     find_len    = strlen(find);
        size_t     replace_len = strlen(replace);
        const char *found;
        
        // Process the next match
        while ((found = STRCASESTR(original, find)) != nullptr) {
            // Copy the text before the match
            size_t leading_len = found - original;
            if (copy_substring(replacement, size, original, leading_len)) {
                // Move source pointer past the copied text
                // and the substring being searched for
                original += leading_len + find_len;
            }
            // Buffer too small
            else
                break;
            
            // Insert the replacement text
            if (copy_substring(replacement, size, replace, replace_len)) {
            }
            // Buffer too small
            else
                break;
        }
        
        // Copy the rest of the text
        successful = copy_substring(replacement,
                                    size,
                                    original,
                                    strlen(original));
    }
    
    return successful;
}

// -----------------------------------------------------------------------------
// Replace one substring with another.
// Note removing a substring is really replacing it with "".
// Returns: true  Any existing substring was replaced
//          false Error
bool
replace_substring(char       *replacement,
                  size_t     size,
                  const char *original,
                  const char *find,
                  const char *replace)
{
    bool successful = false;
    
    // Buffers are the same so create a temporary.
    // New substrings can be longer than old substrings so there is
    // an overwrite hazard for in place replacement.
    if (replacement == original) {
        char *buffer = new char[size];
        
        successful = replace_remove_substring(buffer,
                                              size,
                                              original,
                                              find,
                                              replace);
        strncpy(replacement, buffer, size);
        replacement[size - 1] = 0;
        
        delete[] buffer;
    }
    else
        successful = replace_remove_substring(replacement,
                                              size,
                                              original,
                                              find,
                                              replace);

    return successful;
}



#ifndef NO_MOO_BUILTINS

// -----------------------------------------------------------------------------
// Replace one substring with another
// Arguments: TYPE_STR orginal string
//            TYPE_STR substring to search for
//            TYPE_STR substring to replace matches with
// Returns:   TYPE_STR updated string
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(replace_substring(ansi24_replace_tags("The [red]red dog[normal] [bright][blink]barks[normal]."), "barks", "wags its tail"))
static package
bf_replace_substring(Var arglist, Byte next, void *vdata, Objid progr) {
    Var        rv;
    const char *original = arglist.v.list[1].v.str;
    const char *find     = arglist.v.list[2].v.str;
    const char *replace  = arglist.v.list[3].v.str;
    char       replacement[256];
    
    if (replace_substring(replacement,
                          sizeof(replacement),
                          original,
                          find,
                          replace)) {
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
register_substring(void)
{
    register_function("replace_substring", 3,  3, bf_replace_substring, TYPE_STR, TYPE_STR, TYPE_STR);
}

#endif  // NO_MOO_BUILTINS
