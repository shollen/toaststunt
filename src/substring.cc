//
//  substring.cc
//
//  Copyright (C) 2019-2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//

#ifdef _WIN32
#include <shlwapi.h>
#endif

#include "substring.h"

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
