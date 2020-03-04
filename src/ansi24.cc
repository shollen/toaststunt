//
//  ansi24.cc
//
//  Copyright (C) 2019-2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//

#define MOO_BUILTINS 1  // Define builtin functions available to MOO code

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <algorithm>

#ifdef _WIN32
#include <shlwapi.h>
#endif

#include "ansi24.h"

#if MOO_BUILTINS
#include "functions.h"
#include "utils.h"
#endif

using namespace std;



// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Returns: Number of elements in an array
template <typename T, size_t N>
constexpr size_t numElements(const T(&)[N])
{
    return N;
}

// -----------------------------------------------------------------------------
// We're going to test size, copy characers, update pointer and size
// a few times so make it a function
// Returns: true  Substring was copied
//          false Error
inline bool
copy_substring(char *&dest, size_t &size, const char *src, size_t len)
{
    // This is a private function so its safe to require that
    // pointers have been checked for null by caller
    
    bool successful = false;
    
    if (size > len) {
        memcpy(dest, src, len);
        dest[len]   = 0;
        dest       += len;
        size       -= len;
        successful  = true;
    }
    
    return successful;
}

// ----------------------------------------------------------------------------
// Encode a unicode scalar as a zero terminated utf-8 string
// Returns: > 0 String was created
//          < 0 Error
static int
encode_utf8_string(uint8_t *dest, size_t size, int scalar)
{
    int len = -1;

    if (dest != nullptr && scalar >= 0 && scalar <= 0x10FFFF) {
        if (scalar == 0) {
            if (size >= 1) {
                dest[0] = 0;
                len     = 1;
            }
        }
        else if (scalar <= 0x7F) {
            if (size >= 2) {
                dest[0] = scalar;
                dest[1] = 0;
                len     = 2;
            }
        }
        else if (scalar <= 0x07FF) {
            if (size >= 3) {
                dest[0] = 0xC0 | ((scalar >>  6) & 0x1F);
                dest[1] = 0x80 | ( scalar        & 0x3F);
                dest[2] = 0;
                len     = 3;
            }
        }
        else if (scalar <= 0xFFFF) {
            if (size >= 4) {
                dest[0] = 0xE0 | ((scalar >> 12) & 0x0F);
                dest[1] = 0x80 | ((scalar >>  6) & 0x3F);
                dest[2] = 0x80 | ( scalar        & 0x3F);
                dest[3] = 0;
                len     = 4;
            }
        }
        else {  // <= 0x10FFFF
            if (size >= 5) {
                dest[0] = 0xF0 | ((scalar >> 18) & 0x07);
                dest[1] = 0x80 | ((scalar >> 12) & 0x3F);
                dest[2] = 0x80 | ((scalar >>  6) & 0x3F);
                dest[3] = 0x80 | ( scalar        & 0x3F);
                dest[4] = 0;
                len     = 5;
            }
        }
    }
    
    return len;
}


// -----------------------------------------------------------------------------
// Globals indicating default ansi color bits and foreground/background modes
// and functions to get and set them
// -----------------------------------------------------------------------------

static ansi_modes ansi_color_bits_mode = ansi_8;
static ansi_modes ansi_foreground_mode = ansi_fore;

// -----------------------------------------------------------------------------
// Returns: The default ansi color bits mode
ansi_modes
get_ansi_color_bits_mode(void)
{
    return ansi_color_bits_mode;
}

// -----------------------------------------------------------------------------
// Returns: The default ansi foreground/background mode
ansi_modes
get_ansi_foreground_mode(void)
{
    return ansi_foreground_mode;
}

// -----------------------------------------------------------------------------
// Returns: The default ansi color bits mode
ansi_modes
set_ansi_color_bits_mode(ansi_modes mode)
{
    switch (mode) {
        case ansi_4  :
        case ansi_8  :
        case ansi_24 :
            ansi_color_bits_mode = mode;
            break;
        default :
            break;  // Default unchanged for invalid paramter
    }
    
    return ansi_color_bits_mode;
}

// -----------------------------------------------------------------------------
// Returns: The default ansi foreground/background mode
ansi_modes
set_ansi_foreground_mode(ansi_modes mode)
{
    switch (mode) {
        case ansi_fore :
        case ansi_back :
            ansi_foreground_mode = mode;
            break;
        default :
            break;  // Default unchanged for invalid paramter
    }
    
    return ansi_foreground_mode;
}



// -----------------------------------------------------------------------------
// Define a lookup table with standard color definitions
// and a function that will convert a name into an index into this table.
// With a little feature creep we also have definitions for
// a name that changes the default color bits and foreground/background modes,
// turning a name into a predefined string,
// and for displaying the named colors.
// -----------------------------------------------------------------------------

typedef struct {
    const char *name;
    int        fg4,  bg4,   // 4-bit SGR codes
               pal,         // 8-bit palette entry
               rgb,         // 24-bit RGB
               color_bits,  // Ansi color bits mode
               foreground;  // Foreground/background mode
    const char *string;     // Predefined string
    bool       display;     // Color/Attribute can be displayed
} color_definition;

// IMPORTANT: The order of these colors must match the indicies returned by
//            the name_to_index function.
// -1 indicates undefined, a lower color bits mode will be used automatically.
//
// References:
//     https://en.wikipedia.org/wiki/ANSI_escape_code
//     https://en.wikipedia.org/wiki/List_of_software_palettes
//     https://en.wikipedia.org/wiki/X11_color_names
static color_definition standard_colors[] = {
//  name,       fg4 bg4  pal  rgb       color_bits foreground string   displayable
    "black",    30,  40,   0, 0x000000, -1,        -1,        nullptr, true,
    "red",      31,  41,   1, 0xbb0000, -1,        -1,        nullptr, true,
    "green",    32,  42,   2, 0x00bb00, -1,        -1,        nullptr, true,
    "yellow",   33,  43,   3, 0xbbbb00, -1,        -1,        nullptr, true,
    "blue",     34,  44,   4, 0x0000bb, -1,        -1,        nullptr, true,
    "magenta",  35,  45,   5, 0xbb00bb, -1,        -1,        nullptr, true,    // aka "purple"
    "cyan",     36,  46,   6, 0x00bbbb, -1,        -1,        nullptr, true,
    "white",    37,  47,   7, 0xbbbbbb, -1,        -1,        nullptr, true,
    "bblack",   90, 100,   8, 0x7f7f7f, -1,        -1,        nullptr, true,    // aka "gray", "grey"
    "bred",     91, 101,   9, 0xff0000, -1,        -1,        nullptr, true,
    "bgreen",   92, 102,  10, 0x00ff00, -1,        -1,        nullptr, true,
    "byellow",  93, 103,  11, 0xffff00, -1,        -1,        nullptr, true,
    "bblue",    94, 104,  12, 0x0000ff, -1,        -1,        nullptr, true,
    "bmagenta", 95, 105,  13, 0xff00ff, -1,        -1,        nullptr, true,    // aka "bpurple"
    "bcyan",    96, 106,  14, 0x00ffff, -1,        -1,        nullptr, true,
    "bwhite",   97, 107,  15, 0xffffff, -1,        -1,        nullptr, true,
    "normal",    0,  -1,  -1, -1,       -1,        -1,        nullptr, true,
    "bold",      1,  -1,  -1, -1,       -1,        -1,        nullptr, true,    // aka "bright"
    "faint",     2,  -1,  -1, -1,       -1,        -1,        nullptr, true,
    "under",     4,  -1,  -1, -1,       -1,        -1,        nullptr, true,    // aka "underline"
    "blink",     5,  -1,  -1, -1,       -1,        -1,        nullptr, true,
    "inverse",   7,  -1,  -1, -1,       -1,        -1,        nullptr, true,    // aka "reverse"
    "nobold",   22,  -1,  -1, -1,       -1,        -1,        nullptr, false,   // aka "nobright" "nofaint" "unbold", "unbright" "unfaint"nullptr,
    "nounder",  24,  -1,  -1, -1,       -1,        -1,        nullptr, false,
    "noblink",  25,  -1,  -1, -1,       -1,        -1,        nullptr, false,   // aka "unblink"
    "noinv",    27,  -1,  -1, -1,       -1,        -1,        nullptr, false,
    "4-bit",     0,  -1,  -1, -1,       ansi_4,    -1,        nullptr, false,
    "8-bit",     0,  -1,  -1, -1,       ansi_8,    -1,        nullptr, false,
    "24-bit",    0,  -1,  -1, -1,       ansi_24,   -1,        nullptr, false,
    "fg",       49,  -1,  -1, -1,       -1,        ansi_fore, nullptr, false,
    "bg",       -1,  -1,  -1, -1,       -1,        ansi_back, nullptr, false,
    "azure",    -1,  -1,  25, 0x0066bb, -1,        -1,        nullptr, true,
    "jade",     -1,  -1,  35, 0x00bb66, -1,        -1,        nullptr, true,
    "violet",   -1,  -1,  55, 0x6600bb, -1,        -1,        nullptr, true,
    "lime",     -1,  -1,  70, 0x66bb00, -1,        -1,        nullptr, true,
    "tan",      -1,  -1,  94, 0x886600, -1,        -1,        nullptr, true,
    "silver",   -1,  -1, 102, 0x888888, -1,        -1,        nullptr, true,
    "pink",     -1,  -1, 125, 0xbb0066, -1,        -1,        nullptr, true,
    "orange",   -1,  -1, 130, 0xbb6600, -1,        -1,        nullptr, true,
    "bazure",   -1,  -1,  33, 0x0088ff, -1,        -1,        nullptr, true,
    "bjade",    -1,  -1,  48, 0x00ff88, -1,        -1,        nullptr, true,
    "bviolet",  -1,  -1,  93, 0x8800ff, -1,        -1,        nullptr, true,
    "blime",    -1,  -1, 118, 0x88ff00, -1,        -1,        nullptr, true,
    "btan",     -1,  -1, 178, 0xddbb00, -1,        -1,        nullptr, true,
    "bsilver",  -1,  -1, 188, 0xdddddd, -1,        -1,        nullptr, true,
    "bpink",    -1,  -1, 198, 0xff0088, -1,        -1,        nullptr, true,
    "borange",  -1,  -1, 208, 0xff8800, -1,        -1,        nullptr, true,
    "esc",      -1,  -1,  -1, -1,       -1,        -1,        "\x1b",  false
};

// Lets treat a short string as a 64-bit integer, this will make comparisons
// much faster.
// A union gives us a "free" conversion.
typedef union {
    char     name_as_string[8];
    uint64_t name_as_integer;
} name_converter;

// ----------------------------------------------------------------------------
// Convert a name into an index into the standard_colors table.
// IMPORTANT: The indices returned must match the standard_colors table order.
// Returns: >= 0 Index into the the standard_colors table
//            -1 Error
static int
name_to_index(const char *name)
{
    // Let's cheat, our longest name is "bmagenta". That's an 8 character name.
    // Treat the string as an 8 byte 64-bit integer, then we can do integer
    // comparisons rather than string comparisons. Much faster.
    
    int index = -1; // Indicates an error
    
    if (name != nullptr) {
        name_converter converter;
        
        // Zero the string buffer so all 8 bytes are defined,
        // note that we don't need a zero terminated string
        converter.name_as_integer = 0L;
        
        // Convert lower to upper case
        for (int i = 0;
             name[i] != 0 && i < sizeof(converter.name_as_string);
             ++i)
            converter.name_as_string[i] = toupper(name[i]);
        
        // Integers generated with:
        //     echo "NORMAL" | rev | hexdump -C
        //     00000000  4c 41 4d 52 4f 4e 0a  |LAMRON.|
        // Be sure to ignore the 0a on the end.
        //
        // TODO:
        // Note that characters are reversed, or more generally represent a
        // little endian memory layout. This is a problem if running this code
        // on a big endian machine. All that is needed for compatibility are
        // additional case statements with the bytes reversed in the hex
        // values. Ex:
        //     case 0x0000000000444552L :  // red
        //     case 0x5245440000000000L :
        //         index = 1;
        //         break;
        // Simple but not worth the time (programmer or game CPU cycles)
        // right now given that its likely no one will be using a big endian
        // machine. Apologies to anyone running this code on a 15+ year old
        // PowerPC based Macintosh. Someday if I fire up my G4 I may make such
        // an update (and test it) to future-proof the code in case there is a
        // big endian revival.
        switch(converter.name_as_integer) {
            case 0x0000004b43414c42L :  // black
                index = 0;
                break;
            case 0x0000000000444552L :  // red
                index = 1;
                break;
            case 0x0000004e45455247L :  // green
                index = 2;
                break;
            case 0x0000574f4c4c4559L :  // yellow
                index = 3;
                break;
            case 0x0000000045554c42L :  // blue
                index = 4;
                break;
            case 0x0041544e4547414dL :  // magenta
            case 0x0000454c50525550L :  // purple
                index = 5;
                break;
            case 0x000000004e415943L :  // cyan
                index = 6;
                break;
            case 0x0000004554494857L :  // white
                index = 7;
                break;
            case 0x00004b43414c4242L :  // bblack
            case 0x0000000059415247L :  // gray
            case 0x0000000059455247L :  // grey
                index = 8;
                break;
            case 0x0000000044455242L :  // bred
                index = 9;
                break;
            case 0x00004e4545524742L :  // bgreen
                index = 10;
                break;
            case 0x00574f4c4c455942L :  // byellow
                index = 11;
                break;
            case 0x00000045554c4242L :  // bblue
                index = 12;
                break;
            case 0x41544e4547414d42L :  // bmagenta
            case 0x00454c5052555042L :  // bpurple
                index = 13;
                break;
            case 0x0000004e41594342L :  // bcyan
                index = 14;
                break;
            case 0x0000455449485742L :  // bwhite
                index = 15;
                break;
            case 0x00004c414d524f4eL :  // normal
                index = 16;
                break;
            case 0x00000000444c4f42L :  // bold
            case 0x0000544847495242L :  // bright
                index = 17;
                break;
            case 0x000000544e494146L :  // faint
                index = 18;
                break;
            case 0x0000005245444e55L :  // under
            case 0x4e494c5245444e55L :  // underline
                index = 19;
                break;
            case 0x0000004b4e494c42L :  // blink
                index = 20;
                break;
            case 0x0045535245564e49L :  // inverse
            case 0x0045535245564552L :  // reverse
                index = 21;
                break;
            case 0x0000444c4f424f4eL :  // nobold
            case 0x5448474952424f4eL :  // nobright
            case 0x00544e4941464f4eL :  // nofaint
            case 0x0000444c4f424e55L :  // unbold
            case 0x5448474952424e55L :  // unbright
            case 0x00544e4941464e55L :  // unfaint
                index = 22;
                break;
            case 0x005245444e554f4eL :  // nounder
                index = 23;
                break;
            case 0x004b4e494c424f4eL :  // noblink
            case 0x004b4e494c424e55L :  // unblink
                index = 24;
                break;
            case 0x000000564e494f4eL :  // noinv
                index = 25;
                break;
            case 0x0000005449422d34L :  // 4-bit
                index = 26;
                break;
            case 0x0000005449422d38L :  // 8-bit
                index = 27;
                break;
            case 0x00005449422d3432L :  // 24-bit
                index = 28;
                break;
            case 0x0000000047424f4eL :  // nobg
            case 0x0000000000004746L :  // fg
                index = 29;
                break;
            case 0x0000000000004742L :  // bg
                index = 30;
                break;
            case 0x0000004552555a41L :  // azure
                index = 31;
                break;
            case 0x000000004544414aL :  // jade
                index = 32;
                break;
            case 0x000054454c4f4956L :  // violet
                index = 33;
                break;
            case 0x00000000454d494cL :  // lime
                index = 34;
                break;
            case 0x00000000004e4154L :  // tan
                index = 35;
                break;
            case 0x00005245564c4953L :  // silver
                index = 36;
                break;
            case 0x000000004b4e4950L :  // pink
                index = 37;
                break;
            case 0x000045474e41524fL :  // orange
                index = 38;
                break;
            case 0x00004552555a4142L :  // bazure
                index = 39;
                break;
            case 0x0000004544414a42L :  // bjade
                index = 40;
                break;
            case 0x0054454c4f495642L :  // bviolet
                index = 41;
                break;
            case 0x000000454d494c42L :  // blime
                index = 42;
                break;
            case 0x000000004e415442L :  // btan
                index = 43;
                break;
            case 0x005245564c495342L :  // bsilver
                index = 44;
                break;
            case 0x0000004b4e495042L :  // bpink
                index = 45;
                break;
            case 0x0045474e41524f42L :  // borange
                index = 46;
                break;
            case 0x0000000000435345L :  // esc
                index = 47;
                break;
        }
    }
    
    return index;
}



// -----------------------------------------------------------------------------
// Create an ansi escape sequence from a name or from a numeric value
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Convert text to a numeric value. Text may be a regular number
// or a dotted triple (ex "191.127.255").
// Values may be 4-bit SGR codes, 8-bit palatte indices or 24-bit RGB values.
// Values in the range 0..255 are ambiguous. The red, green and blue variables
// passed by reference will be set to -1 for 4-bit and 8-bit values and
// 0..255 for 24-bit values.
// Side effects: &red   Component of an RGB value or -1
//               &green     "     "  "   "    "   "  "
//               &blue      "     "  "   "    "   "  "
// Returns:      >= 0   The numeric value (4-, 8- or 24-bit)
//                 -1   Error
static int
text_to_value(const char *text, int &red, int &green, int &blue)
{
    int           value     = -1;   // Indicates an error
    int           component = 0;    // Index to current component of the tripple
    unsigned long tripple[3];       // Values for the dotted triple

    red = green = blue = -1;        // Indicates not an RGB value

    // One iterations for a regular value,
    // three iterations for a dotted tripple
    while (text != nullptr) {
        char *next = (char *) text;
        
        // Make sure we converted a number, next should have advanced past text
        tripple[component] = strtoul(text, &next, 0);
        if (next <= text)
            break;
        
        // If we have three numbers we are done
        if (component == 2) {
            if (tripple[0] < 256 && tripple[1] < 256 && tripple[2] < 256) {
                red   = (int) tripple[0];
                green = (int) tripple[1];
                blue  = (int) tripple[2];
                value = (red << 16) | (green << 8) | blue;
            }
        }
        // If we have a dot then another number should follow it
        // so make another pass through the loop.
        // Be merciful and also accept some other characters
        // for those thinking argument list or escape sequence separators.
        else if (*next == '.' || *next == ',' || *next == ';' || *next == ':') {
            text = ++next;
            ++component;
            continue;
        }
        // We have a single value
        else if (component == 0) {
            if (tripple[0] < 256)
                value = (int) tripple[0];
        }
        
        break;
    }

    return value;
}

// -----------------------------------------------------------------------------
// Convert an RGB shade in the range 0..255
// to the range 0..n
// Return: Converted shade

inline int
shade_255_to_n(int shade, int n)
{
    return ((shade * n) + 128) / 255;
}

inline int rgb_256_to_24 (int shade) { return shade_255_to_n(shade, 23); }
inline int rgb_256_to_6  (int shade) { return shade_255_to_n(shade,  5); }
inline int rgb_256_to_3  (int shade) { return shade_255_to_n(shade,  2); }

// Map a 3 shade RGB value (0..2, 0..2, 0..2)
// to an SGR color code.
//     index = red * 9 + green * 3 + blue
static int shades_to_sgr[3 * 3 * 3] = {
    30, // 0.0.0 =  0 -> Black
    34, // 0.0.1 =  1 -> Blue
    94, // 0.0.2 =  2 -> Bright Blue
    32, // 0.1.0 =  3 -> Green
    36, // 0.1.1 =  4 -> Cyan
    96, // 0.1.2 =  5 -> Bright Cyan
    92, // 0.2.0 =  6 -> Bright Green
    96, // 0.2.1 =  7 -> Bright Cyan
    96, // 0.2.2 =  8 -> Bright Cyan
    31, // 1.0.0 =  9 -> Red
    35, // 1.0.1 = 10 -> Magenta
    95, // 1.0.2 = 11 -> Bright Magenta
    33, // 1.1.0 = 12 -> Yellow
    90, // 1.1.1 = 13 -> Bright Black
    90, // 1.1.2 = 14 -> Bright Black
    93, // 1.2.0 = 15 -> Bright Yellow
    90, // 1.2.1 = 16 -> Bright Black
    97, // 1.2.2 = 17 -> Bright White
    91, // 2.0.0 = 18 -> Bright Red
    95, // 2.0.1 = 19 -> Bright Magenta
    95, // 2.0.2 = 20 -> Bright Magenta
    93, // 2.1.0 = 21 -> Bright Yellow
    90, // 2.1.1 = 22 -> Bright Black
    97, // 2.1.2 = 23 -> Bright White
    93, // 2.2.0 = 24 -> Bright Yellow
    97, // 2.2.1 = 25 -> Bright White
    97  // 2.2.2 = 26 -> Bright White
};

// -----------------------------------------------------------------------------
// Create an ansi escape sequence from a name or a numeric value.
// The foreground and color_bits paramters are optional,
// defaults will be used if they are not set.
// Side effects:  &string Ansi escape sequence
// Returns:       true    Sequence was created
//                false   Error
bool
create_ansi_string (ansi_string &string,
                    const char  *name,
                    ansi_modes  foreground,
                    ansi_modes  color_bits)
{
    bool successful = false;
    int  color_index, color_value, red, green, blue;
    
    // Will return an empty string on error
    string.buf[0] = 0;
    
    // See if the default ansi modes are specified
    if (color_bits == ansi_default)
        color_bits = get_ansi_color_bits_mode();
    if (foreground == ansi_default)
        foreground = get_ansi_foreground_mode();

    // Maybe the color was specified by name
    if ((color_index = name_to_index(name)) != -1) {
        switch (color_bits) {
            case ansi_24: {
                // If the lookup table defines an RGB value
                // create a 24-bit ansi sequence
                int rgb = standard_colors[color_index].rgb;
                if (rgb != -1)
                    successful = create_ansi24_string(string, rgb, foreground);
                
                // If successful we are done,
                // otherwise fall through and try again with fewer color bits
                if (successful)
                    break;
            }
            
            case ansi_8: {
                // If the lookup table defines a palette index
                // create an 8-bit ansi sequence
                int pal = standard_colors[color_index].pal;
                if (pal != -1)
                    successful = create_ansi8_string(string, pal, foreground);
                
                // If successful we are done,
                // otherwise fall through and try again with fewer color bits
                if (successful)
                    break;
            }
            
            case ansi_4: {
                // Check for default modes being changed
                int cb = standard_colors[color_index].color_bits;
                int fg = standard_colors[color_index].foreground;
                if (cb != -1) {
                    // In case there is no escape sequence, just a state change
                    color_bits    = set_ansi_color_bits_mode((ansi_modes) cb);
                    string.buf[0] = 0;
                    successful    = true;
                }
                else if (fg != -1) {
                    // In case there is no escape sequence, just a state change
                    foreground    = set_ansi_foreground_mode((ansi_modes) fg);
                    string.buf[0] = 0;
                    successful    = true;
                }
                
                // If the sgr code is an invalid backgroud definition
                // use the foreground
                int sgr = standard_colors[color_index].fg4;
                int bg4 = standard_colors[color_index].bg4;
                if (foreground != ansi_fore && bg4 != -1)
                    sgr = bg4;
                if (sgr != -1)
                    successful = create_ansi4_string(string, sgr);

                // If successful we are done,
                // otherwise fall through and try again with the default
                if (successful)
                    break;
            }
            
            default: {
                // If the lookup table defines a replacement string
                // then copy that string
                const char *str = standard_colors[color_index].string;
                if (str != nullptr) {
                    strncpy(string.buf, str, sizeof(string.buf));
                    string.buf[sizeof(string.buf) - 1] = 0;
                    successful = true;
                }
                
                break;
            }
        }
    }
    
    // Maybe the color was specified by numeric value
    else if ((color_value = text_to_value(name, red, green, blue)) != -1) {
        switch (color_bits) {
            case ansi_24:
                if (red >= 0 && green >= 0 && blue >= 0)
                    successful = create_ansi24_string(string,
                                                      red,
                                                      green,
                                                      blue,
                                                      foreground);
                break;
                
            case ansi_8:
                // If we have a 24-bit RGB convert it to the
                // nearest palette entry
                if (red >= 0 && green >= 0 && blue >= 0) {
                    // When all components are the same
                    // use the specialized set of 24 shades of gray
                    if (red == green && red == blue) {
                        color_value = 232 + rgb_256_to_24(red);
                    }
                    // Use the 216 colors
                    else
                        color_value =   16
                                      + (rgb_256_to_6(red)   * 36)
                                      + (rgb_256_to_6(green) * 6)
                                      +  rgb_256_to_6(blue);
                }
                successful = create_ansi8_string(string,
                                                 color_value,
                                                 foreground);
                break;
                
            case ansi_4:
                // If we have a 24-bit RGB convert it to the
                // nearest SGR code (16 colors)
                if (red >= 0 && green >= 0 && blue >= 0) {
                    // Regular white is a special case
                    // since it does not fit into the 3 shade model
                    if (   red   >= 0xAA
                        && red   <= 0xD3
                        && green >= 0xAA
                        && green <= 0xD3
                        && blue  >= 0xAA
                        && blue  <= 0xD3)
                        color_value = 37;   // SGR code for white
                    else
                        color_value = shades_to_sgr[  (rgb_256_to_3(red)   * 9)
                                                    + (rgb_256_to_3(green) * 3)
                                                    +  rgb_256_to_3(blue)];
                    
                }
                successful = create_ansi4_string(string, color_value);
                break;
                
            default:
                break;
        }
    }
    
    return successful;
}

// -----------------------------------------------------------------------------
// Create an ansi escape sequence from a numeric value
// Side effects:  &string Ansi escape sequence
// Returns:       true    Sequence was created
//                false   Error

bool
create_ansi4_string(ansi_string &string, uint8_t sgr_code)
{
    string.buf[0] = 0;  // Just in case sprintf fails
    return sprintf(string.buf, "\x1b[%dm", sgr_code) >= 4;
}

bool
create_ansi8_string(ansi_string &string,
                    uint8_t     palette_index,
                    ansi_modes  foreground)
{
    string.buf[0] = 0;  // Just in case sprintf fails
    if (foreground == ansi_default)
        foreground = get_ansi_foreground_mode();
    
    return sprintf(string.buf,
                   "\x1b[%d;5;%dm",
                   foreground == ansi_fore ? 38 : 48,
                   palette_index) >= 9;
}

bool
create_ansi24_string (ansi_string &string,
                      uint8_t     red,
                      uint8_t     green,
                      uint8_t     blue,
                      ansi_modes  foreground)
{
    string.buf[0] = 0;  // Just in case sprintf fails
    if (foreground == ansi_default)
        foreground = get_ansi_foreground_mode();
    
    return sprintf(string.buf,
                   "\x1b[%d;2;%d;%d;%dm",
                   foreground == ansi_fore ? 38 : 48,
                   red,
                   green,
                   blue) >= 13;
}



// -----------------------------------------------------------------------------
// Replace ansi tags with ansi escape sequences
// or remove ansi tags or ansi escape sequences
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// This private function is used to implement both
// replace_color_tags_with_ansi() and remove_color_tags().
// Note that we can not find one tag and then use replace_substring() to replace
// all occurences. An earlier tag may change a state (color bits,
// foreground/background) that will cause a different escape sequence to be
// generated later.
// Returns: true  Any existing tags were replaced or removed
//          false Error
static bool
replace_remove_color_tags(char       *replacement,
                          size_t     size,
                          const char *original,
                          bool       remove)
{
    bool successful = false;
    
    // Make sure we have both strings and that the replacemnt buffer
    // is at least the size of the original with termination.
    // That's propbably not large enough if substitutions are made
    // but if there are none and the string is just a copy it will fit.
    if (   original    != nullptr
        && replacement != nullptr
        && size         > strlen(original)) {
        do {
            const char  *bracket_open, *bracket_close, *bracket;
            size_t      name_length;
            ansi_string name, ansi;

            // Search for a pair of brackets that will enclose a name
            if ((bracket_open  = strchr(original,         '[')) == nullptr)
                break;
            if ((bracket_close = strchr(bracket_open + 1, ']')) == nullptr)
                break;
            // If we have consecutive open brackets skip the first one
            while (   (bracket = strchr(bracket_open + 1, '[')) != nullptr
                   &&  bracket < bracket_close)
                bracket_open = bracket;
            
            // A name should be at most length 8 given our uint64_t hack
            // but a dotted tripple in an RGB value could be longer
            name_length = bracket_close - bracket_open - 1;
            if (name_length <= strlen("0xFF.0xFF.0xFF")) {
                // Extract the possible name
                // and "test" it by attempting a conversions to ansi
                memcpy(name.buf, bracket_open + 1, name_length);
                name.buf[name_length] = 0;
                if (create_ansi_string(ansi, name.buf)) {
                    // Copy any leading characters and then
                    // replace the bracketed name with ansi.
                    // Unless the remove flag is set then
                    // do not insert ansi, we are really just
                    // stripping the tags from the original.
                    if (copy_substring(replacement,
                                       size,
                                       original,
                                       bracket_open - original)) {
                        if (! remove) {
                            if (copy_substring(replacement,
                                               size,
                                               ansi.buf,
                                               strlen(ansi.buf))) {
                            }
                            // Buffer too small
                            else
                                break;
                        }
                    }
                    // Buffer too small
                    else
                        break;
                }
                else {
                    // Copy any leading characters and the mismatch
                    if (copy_substring(replacement,
                                       size,
                                       original,
                                       bracket_close - original + 1)) {
                    }
                    // Buffer too small
                    else
                        break;
                }
            }
            else {
                // Copy any leading characters and the mismatch
                if (copy_substring(replacement,
                                   size,
                                   original,
                                   bracket_close - original + 1)) {
                }
                // Buffer too small
                else
                    break;
            }

            // Continue the search after the closing bracket
            original = bracket_close + 1;
        } while(1);
        
        // Copy any remaining characters
        successful = copy_substring(replacement,
                                    size,
                                    original,
                                    strlen(original));
    }
    
    return successful;
}

// -----------------------------------------------------------------------------
// Replace tags with ansi escape sequqnces
// Returns: true  Any existing tags were replaced
//          false Error
bool
replace_color_tags_with_ansi(char       *replacement,
                             size_t     size,
                             const char *original)
{
    bool successful = false;
    
    // Buffers are the same so create a temporary.
    // Escape sequences can be longer than tags so there is
    // an overwrite hazard for in place replacement.
    if (replacement == original) {
        char *buffer = new char[size];
        
        successful = replace_remove_color_tags(buffer,
                                               size,
                                               original,
                                               false);
        strncpy(replacement, buffer, size);
        replacement[size - 1] = 0;
        
        delete[] buffer;
    }
    else
        successful = replace_remove_color_tags(replacement,
                                               size,
                                               original,
                                               false);
    
    return successful;
}

// -----------------------------------------------------------------------------
// Remove tags
// Returns: true  Any existing tags were removed
//          false Error
bool
remove_color_tags(char       *replacement,
                  size_t     size,
                  const char *original)
{
    return replace_remove_color_tags(replacement, size, original, true);
}

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
                case 'm' :
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
// Remove ansi escape sequences
// Returns: true  Any existing sequences were removed
//          false Error
bool
remove_ansi_sequences(char       *replacement,
                      size_t     size,
                      const char *original)
{
    bool successful = false;
    
    // Make sure we have both strings and that the replacemnt
    // is at least the size of the original with termination.
    if (   original    != nullptr
        && replacement != nullptr
        && size         > strlen(original)) {
        while (*original != 0) {
            // Found the beginning of an escape sequence
            if (   *original       == '\x1b'
                && *(original + 1) == '[') {
                const char *end = scan_for_terminator(original + 2);

                // Make sure we have the end of the sequence
                // and skip over it
                if (end != nullptr)
                    original += end - original + 1;
                // Sequence is malformed, just copy the pair
                else {
                    *replacement       =  *original;
                    *(replacement + 1) = *(original + 1);
                    replacement += 2;
                    original    += 2;
                }
            }
            // Next character
            else
                *replacement++ = *original++;
        };
        
        *replacement = 0;
        successful   = true;
    }
    
    return successful;
}



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



// -----------------------------------------------------------------------------
// Display the named colors and the 8-bit palette
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Display a named color in a particulear color bits mode,
// in both foreground and background modes
static void
display_color_for_name(ansi_modes color_bits,
                       char       *&buffer,
                       size_t     &size,
                       const char *spaces,  // Length defines width of output
                       const char *label,   // Name or palette index
                       int        fg4,
                       int        bg4,
                       int        pal,
                       int        rgb) {
    // This is a private function so its safe to require that
    // pointers have been checked for null by the caller
    
    // Determine how much padding the name will need
    size_t label_len   = strlen(label);
    size_t padding_len = strlen(spaces);
    padding_len        =   (padding_len > label_len)
                         ? padding_len - label_len
                         : 0;
    
    // Set the foreground color and add the padded name
    ansi_string ansi;
    create_ansi4_string (ansi, 0);  // Normal mode
    copy_substring      (buffer, size, ansi.buf, strlen(ansi.buf));
    switch (color_bits) {
        case ansi_4:
            create_ansi4_string  (ansi, fg4);
            break;
        case ansi_8:
            create_ansi8_string  (ansi, pal, ansi_fore);
            break;
        case ansi_24:
            create_ansi24_string (ansi, rgb, ansi_fore);
            break;
        default:
            ansi.buf[0] = 0;
            break;
    }
    copy_substring(buffer, size, ansi.buf, strlen(ansi.buf));
    copy_substring(buffer, size, label,    label_len);
    copy_substring(buffer, size, spaces,   padding_len);
    
    // Set the foreground color to black or white depending
    // on the background color,
    // then set the background color and add the padded name
    create_ansi4_string (ansi, 0);  // Normal mode
    copy_substring      (buffer, size, ansi.buf, strlen(ansi.buf));
    switch (color_bits) {
        case ansi_4:
            create_ansi4_string (ansi, rgb != 0 ? 30 : 37);
            copy_substring      (buffer, size, ansi.buf, strlen(ansi.buf));
            create_ansi4_string (ansi, bg4);
            break;
        case ansi_8:
            create_ansi8_string (ansi, rgb != 0 ? 0 : 7, ansi_fore);
            copy_substring      (buffer, size, ansi.buf, strlen(ansi.buf));
            create_ansi8_string (ansi, pal, ansi_back);
            break;
        case ansi_24:
            create_ansi24_string (ansi, rgb != 0 ? 0 : 0xbbbbbb, ansi_fore);
            copy_substring       (buffer, size, ansi.buf, strlen(ansi.buf));
            create_ansi24_string (ansi, rgb, ansi_back);
            break;
        default:
            ansi.buf[0] = 0;
            break;
    }
    copy_substring(buffer, size, ansi.buf, strlen(ansi.buf));
    copy_substring(buffer, size, label,    label_len);
    copy_substring(buffer, size, spaces,   padding_len);
    
    // Add normal
    create_ansi4_string (ansi, 0);
    copy_substring      (buffer, size, ansi.buf, strlen(ansi.buf));
}

// -----------------------------------------------------------------------------
// Display the named colors for a particulear color bits mode
// and the color palette if its 8-bit mode.
static void
display_colors_for_mode(ansi_modes color_bits, char *&buffer, size_t &size) {
    // This is a private function so its safe to require that
    // pointers have been checked for null by the caller
    
    // Preserve the current mode before changing it
    ansi_modes previous = get_ansi_color_bits_mode();
    set_ansi_color_bits_mode(color_bits);
    
    // Identifty the current mode in the output
    const char *label;
    switch (color_bits) {
        case ansi_4:
            label = "4-bit\n";
            break;
        case ansi_8:
            label = "8-bit\n";
            break;
        case ansi_24:
            label = "24-bit\n";
            break;
        default:
            label = "";
            break;
    }
    copy_substring(buffer, size, label, strlen(label));

    // Visit all the entries in the standard_colors table
    const char  *spaces     = "         ";  // Length defines width of output
    const char  *newline    = "\n";
    size_t      newline_len = strlen(newline);
    int         num_colors  = numElements(standard_colors);
    int         count       = 0;
    for (int i = 0; i < num_colors; ++i) {
        // Skip this table entry if there is nothing to display
        bool defined;
        switch (color_bits) {
            case ansi_4:
                defined = standard_colors[i].fg4 != -1;
                break;
            case ansi_8:
                defined = standard_colors[i].pal != -1;
                break;
            case ansi_24:
                defined = standard_colors[i].rgb != -1;
                break;
            default:
                defined = false;
                break;
        }
        if (   (! defined)
            || (! standard_colors[i].display))
            continue;
        
        display_color_for_name(color_bits, buffer, size, spaces,
                               standard_colors[i].name,
                               standard_colors[i].fg4,
                               standard_colors[i].bg4,
                               standard_colors[i].pal,
                               standard_colors[i].rgb);

        // Keep lines under 80 characters
        if ((count++ % 4) == 3)
            copy_substring(buffer, size, newline, newline_len);
    }
    
    // End the last line if it was a partial line
    if ((count % 4) != 0)
        copy_substring(buffer, size, newline, newline_len);
    
    // Show the 8-bit mode palette
    if (color_bits == ansi_8) {
        const char *spaces = "    ";    // Length defines width of output
        
        // Visit the entire palette
        for (int i = 0; i < 256; ++i) {
            // Identifty the palette index in the output
            char index[8];
            sprintf(index, "%d", i);
            
            display_color_for_name(ansi_8, buffer, size, spaces, index,
                                   // Only need the palette index
                                   -1, -1, i,
                                   // Set the rgb color to black or white
                                   // depending on the palette index
                                     (i != 0 && i != 16 && i != 232)
                                   ? 0xbbbbbb
                                   : 0);

            // Keep lines under 80 characters, unless its the last line
            if ((i % 8) == 7 && i != 255)
                copy_substring(buffer, size, newline, newline_len);
        }
        
        // End the last line
        copy_substring(buffer, size, newline, newline_len);
    }

    set_ansi_color_bits_mode(previous);
}

// -----------------------------------------------------------------------------
// IMPORTANT: The buffer needs to be huge to avoid truncation,
//            right now something about 20K in size for all
//            three modes concatenated.
bool
display_colors(char *buffer, size_t size)
{
    // Make sure we have a buffer and it will fit at least the terminating null
    if (   buffer != nullptr
        && size    > 0) {
        display_colors_for_mode(ansi_4,  buffer, size);
        display_colors_for_mode(ansi_8,  buffer, size);
        display_colors_for_mode(ansi_24, buffer, size);
    }
    
    return size != 0;
}



// -----------------------------------------------------------------------------
// Wrappers around snprintf.
// Note that we cannot use a va_list for variable arguments.
// We want to be callable by moo code and we can't convert a moo Var arglist
// to a va_list.
// So have a separate function for each supported moo type with one argument.
// Our builtin function will parse the format string and Var arglist
// and call these functions one at a time.
// Returns: Whatever snprintf returns
// -----------------------------------------------------------------------------

int
format_string(char       *buffer,
              size_t     size,
              const char *format,
              const char *string) {
    int len = -1;   // snprintf also returns negative on encoding errors
    
    if (   buffer != nullptr
        && format != nullptr
        && string != nullptr
        && size    > 0) {
        len = snprintf(buffer, size, format, string);
    }
    
    return len;
}

int
format_int(char       *buffer,
           size_t     size,
           const char *format,
           int        number) {
    int len = -1;   // snprintf also returns negative on encoding errors

    if (   buffer != nullptr
        && format != nullptr
        && size    > 0) {
        len = snprintf(buffer, size, format, number);
    }
    
    return len;
}

int
format_double(char       *buffer,
              size_t     size,
              const char *format,
              double     number) {
    int len = -1;   // snprintf also returns negative on encoding errors

    if (   buffer != nullptr
        && format != nullptr
        && size    > 0) {
        len = snprintf(buffer, size, format, number);
    }
    
    return len;
}

int
format_char(char       *buffer,
            size_t     size,
            const char *format,
            int        character) {
    int len = -1;   // snprintf also returns negative on encoding errors

    if (   buffer != nullptr
        && format != nullptr
        && size    > 0) {
        // If no formatting its safe to process the value
        // as a utf-8 codepoint
        if (   *format       == '%'
            && *(format + 1) == 'c') {
            uint8_t utf8[8];
            
            // Output the utf-8 multibyte sequence as a string
            if (encode_utf8_string(utf8, sizeof(utf8), character) > 0) {
                len = snprintf(buffer, size, "%s", utf8);
            }
        }
        // Don't attempt utf-8 since we have formatting
        else {
            len = snprintf(buffer, size, format, character);
        }
    }
    
    return len;
}



// -----------------------------------------------------------------------------
// Builtins for the moo code
//
// ansi24_version           () --> TYPE_STR version
// ansi24_get_color_bits    () --> TYPE_INT color bits = { 4, 8, 24 }
// ansi24_is_foreground     () --> TYPE_INT foreground = 1, background = 0
// ansi24_set_color_bits    (TYPE_INT color bits = { 4, 8, 24 })
//                          --> TYPE_INT current setting = { 4, 8, 24 }
//                          --> TYPE_ERR E_RANGE
// ansi24_set_foreground    (TYPE_INT foreground = { true, false })
//                          --> TYPE_INT current setting = { 1, 0 }
// ansi24_named_sequence    (TYPE_STR color name,
//                           {optional} TYPE_INT foreground = { 1, 0 },
//                           {optional} TYPE_INT color bits = { 4, 8, 24 })
//                          --> TYPE_STR ansi escape sequence
//                          --> TYPE_ERR E_RANGE
// ansi24_4bit_sequence     (TYPE_INT ansi SGR code = { 0..255 })
//                          --> TYPE_STR ansi escape sequence
//                          --> TYPE_ERR E_RANGE
// ansi24_8bit_sequence     (TYPE_INT ansi palette index = { 0..255 },
//                           {optional} TYPE_INT foreground = { 1, 0 }),
//                          --> TYPE_STR ansi escape sequence
//                          --> TYPE_ERR E_RANGE
// ansi24_24bit_sequence    (TYPE_INT red   component = { 0..255 },
//                           TYPE_INT green component = { 0..255 },
//                           TYPE_INT blue  component = { 0..255 },
//                           {optional} TYPE_INT foreground = { 1, 0 }),
//                          --> TYPE_STR ansi escape sequence
//                          --> TYPE_ERR E_RANGE
// ansi24_replace_tags      (TYPE_STR string with color tags)
//                          --> TYPE_STR string with ansi escape sequences
//                          --> TYPE_ERR E_RANGE
// ansi24_remove_tags       (TYPE_STR string with color tags)
//                          --> TYPE_STR string with no color tags
//                          --> TYPE_ERR E_RANGE
// ansi24_remove_sequences  (TYPE_STR string with ansi escape sequences)
//                          --> TYPE_STR string with no ansi escape sequences
//                          --> TYPE_ERR E_RANGE
// ansi24_replace_substring (TYPE_STR orginal string
//                           TYPE_STR substring to search for
//                           TYPE_STR substring to replace matches with)
//                          --> TYPE_STR updated string
//                          --> TYPE_ERR E_RANGE
// ansi24_display_colors    () --> TYPE_STR string displaying all colors
//                          --> TYPE_ERR E_RANGE
// ansi24_printf            (TYPE_STR format string,
//                           ... { argument list })
//                          --> TYPE_STR output string
//                          --> TYPE_ERR E_ARGS  Argument type not supported
//                          --> TYPE_ERR E_EXEC  Encoding failed
//                          --> TYPE_ERR E_RANGE Output too long
//
// color names
// Note some are really attributes not colors themselves.
// Also note some have aliases
//   4-bit and above:
//     "black"
//     "red"
//     "green"
//     "yellow"
//     "blue"
//     "magenta", "purple"
//     "cyan"
//     "white"
//     "bblack", "gray", "grey"
//     "bred"
//     "bgreen"
//     "byellow"
//     "bblue"
//     "bmagenta", "bpurple"
//     "bcyan"
//     "bwhite"
//     "normal"
//     "bold", "bright"
//     "faint"
//     "under", "underline"
//     "blink"
//     "inverse", "reverse"
//     "nobold", "nobright" "nofaint" "unbold", "unbright" "unfaint"
//     "nounder"
//     "noblink", "unblink"
//     "noinv"
//     "4-bit"
//     "8-bit"
//     "24-bit"
//     "fg"
//     "bg"
//   8-bit and above:
//     "azure"
//     "jade"
//     "violet"
//     "lime"
//     "tan"
//     "silver"
//     "pink"
//     "orange"
//     "bazure"
//     "bjade"
//     "bviolet"
//     "blime"
//     "btan"
//     "bsilver"
//     "bpink"
//     "borange"
//   color tag = "[color name]"
//   Example: "[red]"
//
// string substitution names
//     "esc" --> "\x1b"
//   substitution tag = "[substitution name]"
//   Example: "[esc]"
// -----------------------------------------------------------------------------

#if MOO_BUILTINS

// -----------------------------------------------------------------------------
// Identify the package and version
// Return:  TYPE_STR version
// Testing: ;player:tell(ansi24_version())
static package
bf_ansi24_version(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var  rv;

    // Package informaion and version
    rv.type  = TYPE_STR;
    rv.v.str = str_dup("ansi24 1.0.0");
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Get the default ansi color bits mode
// Return:  TYPE_INT color bits = { 4, 8, 24 }
// Testing: ;player:tell(tostr(ansi24_set_color_bits(4)))
//          ;player:tell(tostr(ansi24_get_color_bits()))
//          ;player:tell(tostr(ansi24_set_color_bits(24)))
//          ;player:tell(tostr(ansi24_get_color_bits()))
//          ;player:tell(tostr(ansi24_set_color_bits(8)))
//          ;player:tell(tostr(ansi24_get_color_bits()))
//          ;player:tell(tostr(ansi24_set_color_bits(4)))
//          ;player:tell(tostr(ansi24_get_color_bits()))
//          ;player:tell(tostr(ansi24_set_color_bits(42)))
//          ;player:tell(tostr(ansi24_get_color_bits()))
static package
bf_ansi24_get_color_bits(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var rv;
    
    switch (get_ansi_color_bits_mode()) {
        case ansi_4  :
            rv.type  = TYPE_INT;
            rv.v.num = 4;
            break;
        case ansi_8  :
            rv.type  = TYPE_INT;
            rv.v.num = 8;
            break;
        case ansi_24 :
            rv.type  = TYPE_INT;
            rv.v.num = 24;
            break;
        default :
            free_var(arglist);                  // Should never happen since
            return make_error_pack(E_RANGE);    // set_ansi verifies parameter
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Get the default ansi foreground/background mode
// Return:  TYPE_INT foreground = 1, background = 0
// Testing: ;player:tell(tostr(ansi24_set_foreground(1)))
//          ;player:tell(tostr(ansi24_is_foreground()))
//          ;player:tell(tostr(ansi24_set_foreground(0)))
//          ;player:tell(tostr(ansi24_is_foreground()))
//          ;player:tell(tostr(ansi24_set_foreground(42)))
//          ;player:tell(tostr(ansi24_is_foreground()))
static package
bf_ansi24_is_foreground(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var  rv;
    
    switch (get_ansi_foreground_mode()) {
        case ansi_fore :
            rv.type  = TYPE_INT;
            rv.v.num = 1;
            break;
        case ansi_back :
            rv.type  = TYPE_INT;
            rv.v.num = 0;
            break;
        default :
            free_var(arglist);                  // Should never happen since
            return make_error_pack(E_RANGE);    // set_ansi verifies parameter
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Set the default ansi color bits mode
// Arguments: TYPE_INT color bits = { 4, 8, 24 }
// Returns:   TYPE_INT current setting = { 4, 8, 24 }
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(tostr(ansi24_set_color_bits(4)))
//            ;player:tell(tostr(ansi24_get_color_bits()))
//            ;player:tell(tostr(ansi24_set_color_bits(24)))
//            ;player:tell(tostr(ansi24_get_color_bits()))
//            ;player:tell(tostr(ansi24_set_color_bits(8)))
//            ;player:tell(tostr(ansi24_get_color_bits()))
//            ;player:tell(tostr(ansi24_set_color_bits(4)))
//            ;player:tell(tostr(ansi24_get_color_bits()))
//            ;player:tell(tostr(ansi24_set_color_bits(42)))
//            ;player:tell(tostr(ansi24_get_color_bits()))
static package
bf_ansi24_set_color_bits(Var arglist, Byte next, void *vdata, Objid progr)
{
    uint8_t color_bits = (uint8_t) arglist.v.list[1].v.num;

    switch (color_bits) {
        case 4 :
            set_ansi_color_bits_mode(ansi_4);
            break;
        case 8 :
            set_ansi_color_bits_mode(ansi_8);
            break;
        case 24 :
            set_ansi_color_bits_mode(ansi_24);
            break;
        default :
            free_var(arglist);
            return make_error_pack(E_RANGE);
    }

    // Don't trust that the set_ansi calls worked,
    // use our "get" builtin to get the actual state.
    return bf_ansi24_get_color_bits(arglist, next, vdata, progr);
}

// -----------------------------------------------------------------------------
// Get the default ansi foreground/background mode
// Arguments: TYPE_INT foreground = { true, false }
// Return:    TYPE_INT current setting = { 1, 0 }
// Testing:   ;player:tell(tostr(ansi24_set_foreground(1)))
//            ;player:tell(tostr(ansi24_is_foreground()))
//            ;player:tell(tostr(ansi24_set_foreground(0)))
//            ;player:tell(tostr(ansi24_is_foreground()))
//            ;player:tell(tostr(ansi24_set_foreground(42)))
//            ;player:tell(tostr(ansi24_is_foreground()))
static package
bf_ansi24_set_foreground(Var arglist, Byte next, void *vdata, Objid progr)
{
    set_ansi_foreground_mode(  ((bool) arglist.v.list[1].v.num)
                             ? ansi_fore : ansi_back);

    // Don't trust that the set_ansi calls worked,
    // use our "is" builtin to get the actual state.
    return bf_ansi24_is_foreground(arglist, next, vdata, progr);
}

// -----------------------------------------------------------------------------
// Create an ansi escape sequence from a name or a numeric value
// Arguments: (TYPE_STR color name or numeric value
//            {optional} TYPE_INT foreground = { true, false }
//            {optional} TYPE_INT color bits = { 4, 8, 24 }
// Returns:   TYPE_STR ansi escape sequence
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(tostr(ansi24_set_color_bits(4)))
//            ;player:tell(tostr(ansi24_set_foreground(1)))
//            ;player:tell(ansi24_replace_substring(ansi24_named_sequence("red"), chr(27), "{esc}"))
//            ;player:tell(tostr(ansi24_set_color_bits(8)))
//            ;player:tell(ansi24_replace_substring(ansi24_named_sequence("red", 0), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_named_sequence("red", 1, 24), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_named_sequence("red", 1, 42), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_named_sequence("1", 1, 4), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_named_sequence("1", 1, 8), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_named_sequence("191.0.0", 1, 24), chr(27), "{esc}"))
static package
bf_ansi24_named_sequence(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var         rv;
    const int   nargs      = (int) arglist.v.list[0].v.num;
    const char  *name      = arglist.v.list[1].v.str;
    ansi_modes  foreground =   (nargs >= 2)
                             ? (arglist.v.list[2].v.num ? ansi_fore : ansi_back)
                             : ansi_default;
    ansi_modes  color_bits =   (nargs >= 3)
                             ? (ansi_modes) arglist.v.list[3].v.num
                             : ansi_default;
    ansi_string sequence;

    if (create_ansi_string(sequence, name, foreground, color_bits)) {
        rv.type  = TYPE_STR;
        rv.v.str = str_dup(sequence.buf);
    }
    else {
        free_var(arglist);
        return make_error_pack(E_RANGE);
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Create an ansi escape sequence from a numeric value
// Arguments: TYPE_INT ansi SGR code = { 0..255 }
// Returns:   TYPE_STR ansi escape sequence
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(tostr(ansi24_set_foreground(1)))
//            ;player:tell(ansi24_replace_substring(ansi24_4bit_sequence(32), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_4bit_sequence(256), chr(27), "{esc}"))

static package
bf_ansi24_4bit_sequence(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var         rv;
    const int   sgr_code = (int) arglist.v.list[1].v.num;
    ansi_string sequence;
    
    if (   sgr_code >= 0
        && sgr_code <= 255
        && create_ansi4_string(sequence, (uint8_t) sgr_code)) {
        rv.type  = TYPE_STR;
        rv.v.str = str_dup(sequence.buf);
    }
    else {
        free_var(arglist);
        return make_error_pack(E_RANGE);
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Create an ansi escape sequence from a numeric value
// Arguments: TYPE_INT ansi palette index = { 0..255 }
//            {optional} TYPE_INT foreground = { true, false }
// Returns:   TYPE_STR ansi escape sequence
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(tostr(ansi24_set_foreground(1)))
//            ;player:tell(ansi24_replace_substring(ansi24_8bit_sequence(2), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_8bit_sequence(2, 42), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_8bit_sequence(2, 0), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_8bit_sequence(256), chr(27), "{esc}"))
static package
bf_ansi24_8bit_sequence(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var         rv;
    const int   nargs      = (int) arglist.v.list[0].v.num;
    const int   pal        = (int) arglist.v.list[1].v.num;
    ansi_modes  foreground =   (nargs >= 2)
                             ? (arglist.v.list[2].v.num ? ansi_fore : ansi_back)
                             : ansi_default;
    ansi_string sequence;

    if (    pal >= 0
         && pal <= 255
         && create_ansi8_string(sequence, (uint8_t) pal, foreground)) {
        rv.type  = TYPE_STR;
        rv.v.str = str_dup(sequence.buf);
    }
    else {
        free_var(arglist);
        return make_error_pack(E_RANGE);
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Create an ansi escape sequence from a numeric value
// Arguments: TYPE_INT red   component = { 0..255 }
//            TYPE_INT green component = { 0..255 }
//            TYPE_INT blue  component = { 0..255 }
//            {optional} TYPE_INT foreground = { true, false }
// Returns:   TYPE_STR ansi escape sequence
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(tostr(ansi24_set_foreground(1)))
//            ;player:tell(ansi24_replace_substring(ansi24_24bit_sequence(191, 0, 0), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_24bit_sequence(0, 191, 0, 42), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_24bit_sequence(0, 0, 191, 0), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_24bit_sequence(256, 0, 0), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_24bit_sequence(0, 256, 0), chr(27), "{esc}"))
//            ;player:tell(ansi24_replace_substring(ansi24_24bit_sequence(0, 0, 256), chr(27), "{esc}"))
static package
bf_ansi24_24bit_sequence(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var         rv;
    const int   nargs      = (int) arglist.v.list[0].v.num;
    const int   red        = (int) arglist.v.list[1].v.num;
    const int   green      = (int) arglist.v.list[2].v.num;
    const int   blue       = (int) arglist.v.list[3].v.num;
    ansi_modes  foreground =   (nargs >= 4)
                             ? (arglist.v.list[4].v.num ? ansi_fore : ansi_back)
                             : ansi_default;
    ansi_string sequence;

    if (    red   >= 0
         && red   <= 255
         && green >= 0
         && green <= 255
         && blue  >= 0
         && blue  <= 255
         && create_ansi24_string(sequence,
                                 (uint8_t) red,
                                 (uint8_t) green,
                                 (uint8_t) blue,
                                 foreground)) {
        rv.type  = TYPE_STR;
        rv.v.str = str_dup(sequence.buf);
    }
    else {
        free_var(arglist);
        return make_error_pack(E_RANGE);
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Several of the tag functions differ only by a function called,
// implement them with common code
// Arguments: TYPE_STR string to perform func() on
// Returns:   TYPE_STR string returned by func()
//            TYPE_ERR E_RANGE
static package
replace_remove_tags(Var arglist, Byte next, void *vdata, Objid progr,
                    bool (*func) (char *, size_t, const char *)) {
    // This is a private function so its safe to require that
    // pointers have been checked for null by the caller
    
    Var        rv;
    const char *original = arglist.v.list[1].v.str;
    char       replacement[256];
    
    if (func(replacement, sizeof(replacement), original)) {
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
// Replace tags with ansi escape sequqnces
// Arguments: TYPE_STR string with color tags
// Returns:   TYPE_STR string with ansi escape sequences
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(ansi24_replace_tags("The [red]red dog[normal] [bright][blink]barks[normal]."))
static package
bf_ansi24_replace_tags(Var arglist, Byte next, void *vdata, Objid progr)
{
    return replace_remove_tags(arglist, next, vdata, progr,
                               replace_color_tags_with_ansi);
}

// -----------------------------------------------------------------------------
// Remove tags
// Arguments: TYPE_STR string with color tags
// Returns:   TYPE_STR string with no color tags
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(ansi24_remove_tags("The [red]red dog[normal] [bright][blink]barks[normal]."))
static package
bf_ansi24_remove_tags(Var arglist, Byte next, void *vdata, Objid progr)
{
    return replace_remove_tags(arglist, next, vdata, progr,
                               remove_color_tags);
}

// -----------------------------------------------------------------------------
// Remove ansi escape sequences
// Arguments: TYPE_STR string with ansi escape sequences
// Returns:   TYPE_STR string with no ansi escape sequences
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(ansi24_remove_sequences(ansi24_replace_tags("The [red]red dog[normal] [bright][blink]barks[normal].")))
static package
bf_ansi24_remove_sequences(Var arglist, Byte next, void *vdata, Objid progr)
{
    return replace_remove_tags(arglist, next, vdata, progr,
                               remove_ansi_sequences);
}

// -----------------------------------------------------------------------------
// Replace one substring with another
// Arguments: TYPE_STR orginal string
//            TYPE_STR substring to search for
//            TYPE_STR substring to replace matches with
// Returns:   TYPE_STR updated string
//            TYPE_ERR E_RANGE
// Testing:   ;player:tell(ansi24_replace_substring(ansi24_replace_tags("The [red]red dog[normal] [bright][blink]barks[normal]."), "barks", "wags its tail"))
static package
bf_ansi24_replace_substring(Var arglist, Byte next, void *vdata, Objid progr) {
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
// Display the named colors and the 8-bit palette
// Returns: TYPE_STR string displaying all colors
//          TYPE_ERR E_RANGE
// Testing: ;player:tell(ansi24_display_colors())
static package
bf_ansi24_display_colors(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var  rv;
    char buffer[1024 * 20]; // Yes, nearly 20K needed to display it all
    
    if (display_colors(buffer, sizeof(buffer))) {
        rv.type  = TYPE_STR;
        rv.v.str = str_dup(buffer);
    }
    else {
        free_var(arglist);
        return make_error_pack(E_RANGE);
    }
    
    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Wrapper around snprintf
// Arguments: TYPE_STR format string
//            ... { argument list }
// Returns:   TYPE_STR output string
//            TYPE_ERR E_ARGS  Argument type not supported
//            TYPE_ERR E_EXEC  Encoding failed
//            TYPE_ERR E_RANGE Output too long
// Testing:   ;player:tell(ansi24_printf(""))
//            ;player:tell(ansi24_printf("Convert the lone %%."))
//            ;player:tell(ansi24_printf("Also convert the leading %%%s", "."))
//            ;player:tell(ansi24_printf("And of course convert the trailing %% %d%% of the time.", 100))
//            ;player:tell(ansi24_printf("Pi is %f.", 3.141592653589793))
//            ;player:tell(ansi24_printf("Consider extra arguments to be %s.", "an", "error"))
//            ;player:tell(ansi24_printf("A missing argument is an %s."))
//            ;player:tell(ansi24_printf("It was nighttime."))
//            ;player:tell(tostr(ansi24_printf("It was a [gray]%s[normal] and %s night.", "dark", "stormy")))
//            ;player:tell(ansi24_printf("%%c UTF-8 test = 1 byte: %c  2 byte: %c  3 byte: %c  4 byte: %c  Not UTF-8: %3c", 36, 162, 8364, 66376, 36))
static package
bf_ansi24_printf(Var arglist, Byte next, void *vdata, Objid progr) {
    Var        rv;
    const int  nargs   = (int) arglist.v.list[0].v.num;
    const char *format = arglist.v.list[1].v.str;
    size_t     size    = 256;
    char       buffer[size];
    char       *dest = buffer;
    
    // Process any additional arguments.
    // Note that "%%" is not associated with an argument so
    // we need to handle it manually, snprintf is only
    // called for certain argument types.
    int i = 2;
    while (i <= nargs) {
        // Make sure we have a format specifier
        const char *start = strchr(format, '%');
        if (start == nullptr) {
            free_var(arglist);
            return make_error_pack(E_ARGS);
        }

        // If we have two percents copy one of them
        // and continue searching for the specifier
        // matching the argument
        if (*(start + 1) == '%') {
            copy_substring(dest, size, format, start - format + 1);
            format = start + 2;
            continue;
        }

        // Copy everything up to the specifier
        copy_substring(dest, size, format, start - format);

        // Start searching for the end of the specifier
        // and note the type when it is found
        const char *end  = start + 1,
                   *type = nullptr;
        
        // https://en.wikipedia.org/wiki/Printf_format_string
        // http://www.cplusplus.com/reference/cstdio/printf/
        // Check for flags, note multiple flags are allowed
        bool done = false;
        while (! done) {
            switch (*end) {
                case '-'  :
                case '+'  :
                case ' '  :
                case '0'  :
                case '\'' :
                case '#'  :
                    ++end;
                    break;
                default:
                    done = true;    // Found a non-flag
                    break;
            }
        }
        
        // Check for width and an optional precision
        int iteration = 1;
        while (iteration <= 2) {
            if (*end == '*') {
                ++end;
            }
            else {
                while (*end >= '0' && *end <= '9')
                    ++end;
            }
            
            // Do one more iteration of loop if a precision is defined
            if (iteration++ == 1 && *end == '.')
                ++end;
            else
                break;
        };
        
        // Check for length
        switch (*end) {
            case 'h' :
            case 'l' :
                if (*end == *(end + 1)) // "hh" and "ll"
                    ++end;
                ++end;
                break;
            case 'L' :
            case 'z' :
            case 'j' :
            case 't' :
            case 'q' :
                ++end;
                break;
            case 'I' :
                if (   (   *(end + 1) == '3'
                        && *(end + 2) == '2')   // "I32"
                    || (   *(end + 1) == '6'
                        && *(end + 2) == '4'))  // "I64"
                    end += 2;
                ++end;
                break;
            default:
                break;
        }
        
        // Check for type
        switch (*end) {
            case 'd' :
            case 'i' :
            case 'u' :
            case 'f' :
            case 'F' :
            case 'e' :
            case 'E' :
            case 'g' :
            case 'G' :
            case 'x' :
            case 'X' :
            case 'o' :
            case 's' :
            case 'c' :
            case 'p' :
            case 'a' :
            case 'A' :
            case 'n' :
                type = end;
                break;
            default:
                break;
        }

        // Make sure we found a type
        if (type == nullptr) {
            free_var(arglist);
            return make_error_pack(E_ARGS);
        }

        // Make sure the specifier is a reasonable length
        size_t specifier_len = end - start + 1;
        char   specifier[32];
        if (specifier_len >= sizeof(specifier)) {
            free_var(arglist);
            return make_error_pack(E_ARGS);
        }

        // Turn the specifier into a string to pass to snprintf
        strncpy(specifier, start, specifier_len);
        specifier[specifier_len] = 0;
        
        // Make sure the type is supported and matches the specifier.
        // If so use snprintf to format the argument.
        int output_len;
        switch (arglist.v.list[i].type) {
            case TYPE_STR :
                if (*type == 's') {
                    output_len = format_string(dest, size, specifier,
                                               arglist.v.list[i].v.str);
                    ++i;
                }
                else {
                    free_var(arglist);
                    return make_error_pack(E_ARGS);
                }
                break;
                
            case TYPE_INT :
                switch (*type) {
                    case 'd' :
                    case 'i' :
                    case 'u' :
                    case 'x' :
                    case 'X' :
                    case 'o' :
                        output_len = format_int(dest, size, specifier,
                                                (int) arglist.v.list[i].v.num);
                        ++i;
                        break;
                    case 'c' : {
                        output_len = format_char(dest, size, specifier,
                                                 (int) arglist.v.list[i].v.num);
                        ++i;
                        break;
                    }
                    default:
                        free_var(arglist);
                        return make_error_pack(E_ARGS);
                }
                break;
                
            case TYPE_FLOAT :
                switch (*type) {
                    case 'f' :
                    case 'F' :
                    case 'e' :
                    case 'E' :
                    case 'g' :
                    case 'G' :
                    case 'a' :
                    case 'A' :
                        output_len = format_double(dest, size, specifier,
                                                   arglist.v.list[i].v.fnum);
                        ++i;
                        break;
                    default:
                        free_var(arglist);
                        return make_error_pack(E_ARGS);
                }
                break;
                
            default :
                free_var(arglist);
                return make_error_pack(E_ARGS);
        }

        if (output_len < 0) {            // Encoding failed
            free_var(arglist);
            return make_error_pack(E_EXEC);
        }
        else if (output_len >= size) {   // Buffer too small
            free_var(arglist);
            return make_error_pack(E_RANGE);
        }
        
        // We had a successful snprintf call,
        // continue after the format specifier
        dest   += output_len;
        size   -= output_len;
        format  = end + 1;
    };

    // Remember that we are handling "%%" manually
    const char *percent;
    do {
        percent = strchr(format, '%');
        if (percent != nullptr) {
            // Copy any leading text and one of the percents
            if (*(percent + 1) == '%') {
                copy_substring(dest, size, format, percent - format + 1);
                format = percent + 2;
            }
            // We have an extra format specifier, an argument seems missing
            else {
                free_var(arglist);
                return make_error_pack(E_ARGS);
            }
        }
    } while (percent != nullptr);
    
    // Copy the remainder of the format string
    if (copy_substring(dest, size, format, strlen(format))) {
        rv.type  = TYPE_STR;
        rv.v.str = str_dup(buffer);
    }
    else {
        free_var(arglist);
        return make_error_pack(E_EXEC);
    }

    free_var(arglist);
    return make_var_pack(rv);
}

// -----------------------------------------------------------------------------
// Make our public builtins accessible
void
register_ansi24(void)
{
    register_function("ansi24_version",           0,  0, bf_ansi24_version);
    register_function("ansi24_display_colors",    0,  0, bf_ansi24_display_colors);
    register_function("ansi24_get_color_bits",    0,  0, bf_ansi24_get_color_bits);
    register_function("ansi24_is_foreground",     0,  0, bf_ansi24_is_foreground);
    register_function("ansi24_set_color_bits",    1,  1, bf_ansi24_set_color_bits, TYPE_INT);
    register_function("ansi24_set_foreground",    1,  1, bf_ansi24_set_foreground, TYPE_INT);
    register_function("ansi24_named_sequence",    1,  3, bf_ansi24_named_sequence, TYPE_STR, TYPE_INT, TYPE_INT);
    register_function("ansi24_4bit_sequence",     1,  1, bf_ansi24_4bit_sequence, TYPE_INT);
    register_function("ansi24_8bit_sequence",     1,  2, bf_ansi24_8bit_sequence, TYPE_INT, TYPE_INT);
    register_function("ansi24_24bit_sequence",    3,  4, bf_ansi24_24bit_sequence, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT);
    register_function("ansi24_replace_tags",      1,  1, bf_ansi24_replace_tags, TYPE_STR);
    register_function("ansi24_remove_tags",       1,  1, bf_ansi24_remove_tags, TYPE_STR);
    register_function("ansi24_remove_sequences",  1,  1, bf_ansi24_remove_sequences, TYPE_STR);
    register_function("ansi24_replace_substring", 3,  3, bf_ansi24_replace_substring, TYPE_STR, TYPE_STR, TYPE_STR);
    register_function("ansi24_printf",            1, -1, bf_ansi24_printf, TYPE_STR);
}

#endif  // MOO_BUILTINS
