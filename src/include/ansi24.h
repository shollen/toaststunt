//
//  ansi24.h
//
//  Copyright (C) 2019-2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//

#ifndef ansi24_h
#define ansi24_h

#include <cstdint>

enum ansi_modes { ansi_default,                 // Use global variable settings
                  ansi_fore, ansi_back,         // Foreground/background color
                  ansi_4  =  4,                 // Number of color bits
                  ansi_8  =  8,
                  ansi_24 = 24 };

typedef struct { char buf[32]; } ansi_string;   // Room for general strings too

// -----------------------------------------------------------------------------
// Get and set the ansi global variables

ansi_modes get_ansi_color_bits_mode (uint64_t id);
ansi_modes get_ansi_foreground_mode (uint64_t id);
ansi_modes set_ansi_color_bits_mode (uint64_t id, ansi_modes mode);
ansi_modes set_ansi_foreground_mode (uint64_t id, ansi_modes mode);

// -----------------------------------------------------------------------------
// Create an ansi escape sequence from a name or from a numeric value

bool create_ansi_string   (ansi_string &string,
                           const char  *name,
                           uint64_t    id         = 0,
                           ansi_modes  foreground = ansi_default,
                           ansi_modes  color_bits = ansi_default);

bool create_ansi4_string  (ansi_string &string,
                           uint8_t     sgr_code);

bool create_ansi8_string  (ansi_string &string,
                           uint8_t     palette_index,
                           uint64_t    id         = 0,
                           ansi_modes  foreground = ansi_default);

bool create_ansi24_string (ansi_string &string,
                           uint8_t     red,
                           uint8_t     green,
                           uint8_t     blue,
                           uint64_t    id         = 0,
                           ansi_modes  foreground = ansi_default);

inline
bool create_ansi24_string (ansi_string &string,
                           uint32_t    rgb,
                           uint64_t    id         = 0,
                           ansi_modes  foreground = ansi_default) {
    return create_ansi24_string(string,
                                (rgb >> 16) & 0xff,
                                (rgb >>  8) & 0xff,
                                 rgb        & 0xff,
                                id,
                                foreground);
}

// -----------------------------------------------------------------------------
// Replace ansi tags with ansi escape sequences
// or remove ansi tags or ansi escape sequences

bool replace_color_tags_with_ansi (char       *replacement,
                                   size_t     size,
                                   const char *original,
                                   uint64_t   id = 0);

bool remove_color_tags            (char       *replacement,
                                   size_t     size,
                                   const char *original,
                                   uint64_t);

bool remove_ansi_sequences        (char       *replacement,
                                   size_t     size,
                                   const char *original,
                                   uint64_t);

// -----------------------------------------------------------------------------
// Show the supported named colors and the 8-bit palette

bool display_colors (char *buffer, size_t size);

// -----------------------------------------------------------------------------
// Wrappers around snprintf

int format_string (char       *buffer,
                   size_t     size,
                   const char *format,
                   const char *string);
int format_int    (char       *buffer,
                   size_t     size,
                   const char *format,
                   int        number);
int format_double (char       *buffer,
                   size_t     size,
                   const char *format,
                   double     number);
int format_char   (char       *buffer,
                   size_t     size,
                   const char *format,
                   int        character);

#endif  /* ansi24_h */
