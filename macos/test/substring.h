//
//  substring.h
//
//  Copyright (C) 2019-2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//

#ifndef substring_h
#define substring_h

#include <cstring>

#ifdef _WIN32
#define STRCASESTR StrStrI
#else
#define STRCASESTR strcasestr
#endif

// -----------------------------------------------------------------------------
// We're going to test size, copy characers, zero terminate,
// and update pointer and size a few times so make it a function
// Side effects: &dest updated
//               &size updated
// Returns: true  Substring was copied and zero terminated
//          false Error
inline bool
copy_substring(char *&dest, size_t &size, const char *src, size_t len)
{
    bool successful = false;
    
    if (   dest != nullptr
        && src  != nullptr
        && size  > len) {
        memcpy(dest, src, len);
        dest[len]   = 0;
        dest       += len;
        size       -= len;
        successful  = true;
    }
    
    return successful;
}

// -----------------------------------------------------------------------------
// Replace or remove a substring

bool replace_substring (char       *replacement,
                        size_t     size,
                        const char *original,
                        const char *find,
                        const char *replace = "");  // Empty to remove substr

inline bool
remove_substring (char       *replacement,
                  size_t     size,
                  const char *original,
                  const char *find) {
    return replace_substring(replacement, size, original, find);
}

#endif  /* substring_h */
