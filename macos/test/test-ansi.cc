//
//  test-ansi.cc
//
//  Copyright (C) 2019-2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//
// Building manually at the console:
//     macoS:   clang++ -std=c++11 -DNO_MOO_BUILTINS -I../../src/include test-ansi.cc utils.cc ../../src/substring.cc ../../src/ansi24.cc
//     linux:   g++ -std=c++11 -DNO_MOO_BUILTINS -I../../src/include test-ansi.cc utils.cc ../../src/substring.cc ../../src/ansi24.cc
//     windows: cl /EHsc /DNO_MOO_BUILTINS /I../../src/include test-ansi.cc utils.cc ../../src/substring.cc ../../src/ansi24.cc Shlwapi.lib
//              To change the code page of the console:
//                  chcp 65001

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "utils.h"
#include "substring.h"
#include "ansi24.h"

using namespace std;



// -------------------------------------------------------------------------
// Output text once with foreground tags and a second time with background tags.
// Then set output back to normal.
// This output is for visual inspection, colored text, colored backgrounds.
void test_create_ansi(const char *fg1, const char *bg1,
                      const char *fg2, const char *bg2,
                      const char *text, ansi_modes color_bits) {
    ansi_string normal, foreground, background;

    normal.buf[0]     = 0;
    foreground.buf[0] = 0;
    background.buf[0] = 0;
    create_ansi_string  (normal,     "normal", ansi_fore, color_bits);
    create_ansi_string  (foreground, fg1,      ansi_fore, color_bits);
    create_ansi_string  (background, bg1,      ansi_back, color_bits);
    cout << foreground.buf << background.buf << text << normal.buf << "    ";
    
    normal.buf[0]     = 0;
    foreground.buf[0] = 0;
    background.buf[0] = 0;
    create_ansi4_string (normal,     0);
    create_ansi_string  (foreground, fg2,      ansi_fore, color_bits);
    create_ansi_string  (background, bg2,      ansi_back, color_bits);
    cout << foreground.buf << background.buf << text << normal.buf << "    ";
    
    cout << endl;
}

// -------------------------------------------------------------------------
// Test replacing ansi tags with ansi escape sequences,
// and test removing tags and escape sequences.
// The expected result is passed in for verification.
void test_replace_ansi(const char *expected,
                       const char *original,
                       bool       remove_tags = false,
                       bool       remove_ansi = false) {
    char replacement[256];
    
    if (remove_tags) {
        if (remove_color_tags(replacement,
                              sizeof(replacement),
                              original)) {
            compare_to_expected(expected, replacement);
        }
        else
            report_failure("remove_color_tags", original);
    }
    else if (remove_ansi) {
        if (remove_ansi_sequences(replacement,
                                  sizeof(replacement),
                                  original)) {
            compare_to_expected(expected, replacement);
        }
        else
            report_failure("remove_ansi_sequences", original);
    }
    else {
        if (replace_color_tags_with_ansi(replacement,
                                         sizeof(replacement),
                                         original)) {
            compare_to_expected(expected, replacement);
        }
        else
            report_failure("replace_color_tags_with_ansi", original);
    }
}

// -------------------------------------------------------------------------
// Test replacing and removing a substring.
// The expected result is passed in for verification.
void test_replace_string(const char *expected,
                         const char *original,
                         const char *find,
                         const bool caseless,
                         const char *replace = nullptr) {
    char replacement[256];
    
    if (replace == nullptr) {
        if (remove_substring(replacement,
                             sizeof(replacement),
                             original,
                             find,
                             caseless)) {
            compare_to_expected(expected, replacement);
        }
        else
            report_failure("remove_substring", original);
    }
    else {
        if (replace_substring(replacement,
                              sizeof(replacement),
                              original,
                              find,
                              caseless,
                              replace)) {
            compare_to_expected(expected, replacement);
        }
        else
            report_failure("replace_substring", original);
    }
}

// -------------------------------------------------------------------------
// Test the snprintf() wrappers.
// The expected result is passed in for verification.

void test_format_string(const char *expected, const char *format, const char *string) {
    char    buffer[256];
    int     written;

    written = format_string(buffer, sizeof(buffer), format, string);

    if (written > 0) {
        compare_to_expected(expected, buffer);
    }
    else
        report_failure("format_string", format);
}

void test_format_int(const char *expected, const char *format, int number) {
    char    buffer[256];
    int     written;

    written = format_int(buffer, sizeof(buffer), format, number);

    if (written > 0) {
        compare_to_expected(expected, buffer);
    }
    else
        report_failure("format_int", format);
}

void test_format_double(const char *expected, const char *format, double number) {
    char    buffer[256];
    int     written;

    written = format_double(buffer, sizeof(buffer), format, number);

    if (written > 0) {
        compare_to_expected(expected, buffer);
    }
    else
        report_failure("format_double", format);
}

void test_format_char(const char *expected, const char *format, int character) {
    char    buffer[256];
    int     written;

    written = format_char(buffer, sizeof(buffer), format, character);

    if (written > 0) {
        compare_to_expected(expected, buffer);
    }
    else
        report_failure("format_char", format);
}



// -------------------------------------------------------------------------
int main(void) {
#ifdef _WIN32
    HANDLE hStdOut;
    DWORD  dwMode;
    BOOL   successful = FALSE;
    
    if ((hStdOut = GetStdHandle(STD_OUTPUT_HANDLE)) != INVALID_HANDLE_VALUE) {
        if (GetConsoleMode(hStdOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            successful = SetConsoleMode(hStdOut, dwMode);
        }
    }
    if (! successful) {
        cout << "Failed to set terminal processing" << endl;
    }
#endif

    // -------------------------------------------------------------------------
    // Show all the foreground and background colors and attributes
    // on the screen for visual inspection

    char buffer[1024 * 20];

    display_colors(buffer, sizeof(buffer));
    cout << buffer << endl;

    // -------------------------------------------------------------------------
    // Show all attributes being selectively turned on and off,
    // in other words turned off by something more specific that "normal",
    // on the screen for visual inspection

    ansi_string normal, color1, color2;

    cout << "Testing 4-bit attributes on and off" << endl;
    set_ansi_color_bits_mode(ansi_4);
    create_ansi_string (color1, "bold");
    create_ansi_string (color2, "nobold");
    cout << "normal    " << color1.buf << "bold"     << color2.buf << endl;
    create_ansi_string (color1, "bold");
    create_ansi_string (color2, "unbold");
    cout << "normal    " << color1.buf << "bold"     << color2.buf << endl;
    create_ansi_string (color1, "bright");
    create_ansi_string (color2, "nobright");
    cout << "normal    " << color1.buf << "bright"   << color2.buf << endl;
    create_ansi_string (color1, "bright");
    create_ansi_string (color2, "unbright");
    cout << "normal    " << color1.buf << "bright"   << color2.buf << endl;
    create_ansi_string (color1, "faint");
    create_ansi_string (color2, "nofaint");
    cout << "normal    " << color1.buf << "faint"    << color2.buf << endl;
    create_ansi_string (color1, "faint");
    create_ansi_string (color2, "unfaint");
    cout << "normal    " << color1.buf << "faint"    << color2.buf << endl;
    create_ansi_string (color1, "under");
    create_ansi_string (color2, "nounder");
    cout << "normal    " << color1.buf << "under"    << color2.buf << endl;
    create_ansi_string (color1, "blink");
    create_ansi_string (color2, "noblink");
    cout << "normal    " << color1.buf << "blink"    << color2.buf << endl;
    create_ansi_string (color1, "blink");
    create_ansi_string (color2, "unblink");
    cout << "normal    " << color1.buf << "blink"    << color2.buf << endl;
    create_ansi_string (color1, "reverse");
    create_ansi_string (color2, "noinv");
    cout << "normal    " << color1.buf << "reverse"  << color2.buf << endl;
    create_ansi_string (color1, "inverse");
    create_ansi_string (color2, "noinv");
    cout << "normal    " << color1.buf << "inverse"  << color2.buf << endl;
    cout << "normal    " << normal.buf << endl;
    
    // -------------------------------------------------------------------------
    // Testing using 24-bit tags in 8-bit mode
    cout << "Testing 24-bit RGB to 8-bit palette conversion" << endl;
    set_ansi_color_bits_mode(ansi_8);
    
    // Use components that are not exactly the same so we
    // end up in the 216 entry 6x6x6 space
    test_replace_ansi ("\x1b[38;5;16m 0.0.1\x1b[0m",        "[0.0.1] 0.0.1[normal]");
    test_replace_ansi ("\x1b[38;5;231m 255.255.254\x1b[0m", "[255.255.254] 255.255.254[normal]");
    test_replace_ansi ("\x1b[38;5;196m 255.0.0\x1b[0m",     "[255.0.0] 255.0.0[normal]");
    test_replace_ansi ("\x1b[38;5;46m 0.255.0\x1b[0m",      "[0.255.0] 0.255.0[normal]");
    test_replace_ansi ("\x1b[38;5;21m 0.0.255\x1b[0m",      "[0.0.255] 0.0.255[normal]");
    test_replace_ansi ("\x1b[38;5;226m 255.255.0\x1b[0m",   "[255.255.0] 255.255.0[normal]");
    test_replace_ansi ("\x1b[38;5;201m 255.0.255\x1b[0m",   "[255.0.255] 255.0.255[normal]");
    test_replace_ansi ("\x1b[38;5;51m 0.255.255\x1b[0m",    "[0.255.255] 0.255.255[normal]");
    test_replace_ansi ("\x1b[38;5;214m 255.153.0\x1b[0m",   "[255.153.0] 255.153.0[normal]");
    test_replace_ansi ("\x1b[38;5;214m 255.136.0\x1b[0m",   "[255.136.0] 255.136.0[normal]");
    test_replace_ansi ("\x1b[38;5;16m 25.25.24\x1b[0m",     "[25.25.24] 25.25.24[normal]");
    test_replace_ansi ("\x1b[38;5;59m 26.26.27\x1b[0m",     "[26.26.27] 26.26.27[normal]");
    test_replace_ansi ("\x1b[38;5;59m 76.76.75\x1b[0m",     "[76.76.75] 76.76.75[normal]");
    test_replace_ansi ("\x1b[38;5;102m 78.78.79\x1b[0m",    "[78.78.79] 78.78.79[normal]");
    test_replace_ansi ("\x1b[38;5;102m 127.127.126\x1b[0m", "[127.127.126] 127.127.126[normal]");
    test_replace_ansi ("\x1b[38;5;145m 128.128.129\x1b[0m", "[128.128.129] 128.128.129[normal]");
    test_replace_ansi ("\x1b[38;5;145m 178.178.177\x1b[0m", "[178.178.177] 178.178.177[normal]");
    test_replace_ansi ("\x1b[38;5;188m 179.179.180\x1b[0m", "[179,179,180] 179.179.180[normal]");
    test_replace_ansi ("\x1b[38;5;188m 229.229.228\x1b[0m", "[229;229;228] 229.229.228[normal]");
    test_replace_ansi ("\x1b[38;5;231m 230.230.231\x1b[0m", "[230:230:231] 230.230.231[normal]");
    
    // Use components that are exactly the same so we
    // end up in the 24 shades of gray
    test_replace_ansi ("\x1b[38;5;232m 0.0.0\x1b[0m",       "[0.0.0] 0.0.0[normal]");
    test_replace_ansi ("\x1b[38;5;255m 255.255.255\x1b[0m", "[255.255.255] 255.255.255[normal]");
    test_replace_ansi ("\x1b[38;5;232m 5.5.5\x1b[0m",       "[5.5.5] 5.5.5[normal]");
    test_replace_ansi ("\x1b[38;5;233m 6.6.6\x1b[0m",       "[6.6.6] 6.6.6[normal]");
    test_replace_ansi ("\x1b[38;5;233m 16.16.16\x1b[0m",    "[16.16.16] 16.16.16[normal]");
    test_replace_ansi ("\x1b[38;5;234m 17.17.17\x1b[0m",    "[17.17.17] 17.17.17[normal]");
    test_replace_ansi ("\x1b[38;5;234m 27.27.27\x1b[0m",    "[27.27.27] 27.27.27[normal]");
    test_replace_ansi ("\x1b[38;5;235m 28.28.28\x1b[0m",    "[28.28.28] 28.28.28[normal]");
    test_replace_ansi ("\x1b[38;5;235m 38.38.38\x1b[0m",    "[38.38.38] 38.38.38[normal]");
    test_replace_ansi ("\x1b[38;5;236m 39.39.39\x1b[0m",    "[39.39.39] 39.39.39[normal]");
    test_replace_ansi ("\x1b[38;5;236m 49.49.49\x1b[0m",    "[49.49.49] 49.49.49[normal]");
    test_replace_ansi ("\x1b[38;5;237m 50.50.50\x1b[0m",    "[50.50.50] 50.50.50[normal]");
    test_replace_ansi ("\x1b[38;5;237m 60.60.60\x1b[0m",    "[60.60.60] 60.60.60[normal]");
    test_replace_ansi ("\x1b[38;5;238m 61.61.61\x1b[0m",    "[61.61.61] 61.61.61[normal]");
    test_replace_ansi ("\x1b[38;5;238m 72.72.72\x1b[0m",    "[72.72.72] 72.72.72[normal]");
    test_replace_ansi ("\x1b[38;5;239m 73.73.73\x1b[0m",    "[73.73.73] 73.73.73[normal]");
    test_replace_ansi ("\x1b[38;5;239m 83.83.83\x1b[0m",    "[83.83.83] 83.83.83[normal]");
    test_replace_ansi ("\x1b[38;5;240m 84.84.84\x1b[0m",    "[84.84.84] 84.84.84[normal]");
    test_replace_ansi ("\x1b[38;5;240m 94.94.94\x1b[0m",    "[94.94.94] 94.94.94[normal]");
    test_replace_ansi ("\x1b[38;5;241m 95.95.95\x1b[0m",    "[95.95.95] 95.95.95[normal]");
    test_replace_ansi ("\x1b[38;5;241m 105.105.105\x1b[0m", "[105.105.105] 105.105.105[normal]");
    test_replace_ansi ("\x1b[38;5;242m 106.106.106\x1b[0m", "[106.106.106] 106.106.106[normal]");
    test_replace_ansi ("\x1b[38;5;242m 116.116.116\x1b[0m", "[116.116.116] 116.116.116[normal]");
    test_replace_ansi ("\x1b[38;5;243m 117.117.117\x1b[0m", "[117.117.117] 117.117.117[normal]");
    test_replace_ansi ("\x1b[38;5;243m 127.127.127\x1b[0m", "[127.127.127] 127.127.127[normal]");
    test_replace_ansi ("\x1b[38;5;244m 128.128.128\x1b[0m", "[128.128.128] 128.128.128[normal]");
    test_replace_ansi ("\x1b[38;5;244m 138.138.138\x1b[0m", "[138.138.138] 138.138.138[normal]");
    test_replace_ansi ("\x1b[38;5;245m 139.139.139\x1b[0m", "[139.139.139] 139.139.139[normal]");
    test_replace_ansi ("\x1b[38;5;245m 149.149.149\x1b[0m", "[149.149.149] 149.149.149[normal]");
    test_replace_ansi ("\x1b[38;5;246m 150.150.150\x1b[0m", "[150.150.150] 150.150.150[normal]");
    test_replace_ansi ("\x1b[38;5;246m 160.160.160\x1b[0m", "[160.160.160] 160.160.160[normal]");
    test_replace_ansi ("\x1b[38;5;247m 161.161.161\x1b[0m", "[161.161.161] 161.161.161[normal]");
    test_replace_ansi ("\x1b[38;5;247m 171.171.171\x1b[0m", "[171.171.171] 171.171.171[normal]");
    test_replace_ansi ("\x1b[38;5;248m 172.172.172\x1b[0m", "[172.172.172] 172.172.172[normal]");
    test_replace_ansi ("\x1b[38;5;248m 182.182.182\x1b[0m", "[182.182.182] 182.182.182[normal]");
    test_replace_ansi ("\x1b[38;5;249m 183.183.183\x1b[0m", "[183.183.183] 183.183.183[normal]");
    test_replace_ansi ("\x1b[38;5;249m 193.193.193\x1b[0m", "[193.193.193] 193.193.193[normal]");
    test_replace_ansi ("\x1b[38;5;250m 194.194.194\x1b[0m", "[194.194.194] 194.194.194[normal]");
    test_replace_ansi ("\x1b[38;5;250m 205.205.205\x1b[0m", "[205.205.205] 205.205.205[normal]");
    test_replace_ansi ("\x1b[38;5;251m 206.206.206\x1b[0m", "[206.206.206] 206.206.206[normal]");
    test_replace_ansi ("\x1b[38;5;251m 216.216.216\x1b[0m", "[216.216.216] 216.216.216[normal]");
    test_replace_ansi ("\x1b[38;5;252m 217.217.217\x1b[0m", "[217.217.217] 217.217.217[normal]");
    test_replace_ansi ("\x1b[38;5;252m 227.227.227\x1b[0m", "[227.227.227] 227.227.227[normal]");
    test_replace_ansi ("\x1b[38;5;253m 228.228.228\x1b[0m", "[228.228.228] 228.228.228[normal]");
    test_replace_ansi ("\x1b[38;5;253m 238.238.238\x1b[0m", "[238.238.238] 238.238.238[normal]");
    test_replace_ansi ("\x1b[38;5;254m 239.239.239\x1b[0m", "[239.239.239] 239.239.239[normal]");
    test_replace_ansi ("\x1b[38;5;254m 249.249.249\x1b[0m", "[249.249.249] 249.249.249[normal]");
    test_replace_ansi ("\x1b[38;5;255m 250.250.250\x1b[0m", "[250.250.250] 250.250.250[normal]");
    
    // -------------------------------------------------------------------------
    // Testing using 24-bit tags in 4-bit mode
    cout << "Testing 24-bit RGB to 4-bit SGR code conversion" << endl;
    set_ansi_color_bits_mode(ansi_4);
    test_replace_ansi ("\x1b[30m 0.0.0\x1b[0m",       "[0.0.0] 0.0.0[normal]");
    test_replace_ansi ("\x1b[34m 0.0.127\x1b[0m",     "[0.0.127] 0.0.127[normal]");
    test_replace_ansi ("\x1b[94m 0.0.255\x1b[0m",     "[0.0.255] 0.0.255[normal]");
    test_replace_ansi ("\x1b[32m 0.127.0\x1b[0m",     "[0.127.0] 0.127.0[normal]");
    test_replace_ansi ("\x1b[36m 0.127.127\x1b[0m",   "[0.127.127] 0.127.127[normal]");
    test_replace_ansi ("\x1b[96m 0.127.255\x1b[0m",   "[0.127.255] 0.127.255[normal]");
    test_replace_ansi ("\x1b[92m 0.255.0\x1b[0m",     "[0.255.0] 0.255.0[normal]");
    test_replace_ansi ("\x1b[96m 0.255.127\x1b[0m",   "[0.255.127] 0.255.127[normal]");
    test_replace_ansi ("\x1b[96m 0.255.255\x1b[0m",   "[0.255.255] 0.255.255[normal]");
    test_replace_ansi ("\x1b[31m 127.0.0\x1b[0m",     "[127.0.0] 127.0.0[normal]");
    test_replace_ansi ("\x1b[35m 127.0.127\x1b[0m",   "[127.0.127] 127.0.127[normal]");
    test_replace_ansi ("\x1b[95m 127.0.255\x1b[0m",   "[127.0.255] 127.0.255[normal]");
    test_replace_ansi ("\x1b[33m 127.127.0\x1b[0m",   "[127.127.0] 127.127.0[normal]");
    test_replace_ansi ("\x1b[90m 127.127.127\x1b[0m", "[127.127.127] 127.127.127[normal]");
    test_replace_ansi ("\x1b[90m 127.127.255\x1b[0m", "[127.127.255] 127.127.255[normal]");
    test_replace_ansi ("\x1b[93m 127.255.0\x1b[0m",   "[127.255.0] 127.255.0[normal]");
    test_replace_ansi ("\x1b[90m 127.255.127\x1b[0m", "[127.255.127] 127.255.127[normal]");
    test_replace_ansi ("\x1b[97m 127.255.255\x1b[0m", "[127.255.255] 127.255.255[normal]");
    test_replace_ansi ("\x1b[91m 255.0.0\x1b[0m",     "[255.0.0] 255.0.0[normal]");
    test_replace_ansi ("\x1b[95m 255.0.127\x1b[0m",   "[255.0.127] 255.0.127[normal]");
    test_replace_ansi ("\x1b[95m 255.0.255\x1b[0m",   "[255.0.255] 255.0.255[normal]");
    test_replace_ansi ("\x1b[93m 255.127.0\x1b[0m",   "[255.127.0] 255.127.0[normal]");
    test_replace_ansi ("\x1b[90m 255.127.127\x1b[0m", "[255.127.127] 255.127.127[normal]");
    test_replace_ansi ("\x1b[97m 255.127.255\x1b[0m", "[255.127.255] 255.127.255[normal]");
    test_replace_ansi ("\x1b[93m 255.255.0\x1b[0m",   "[255.255.0] 255.255.0[normal]");
    test_replace_ansi ("\x1b[97m 255.255.127\x1b[0m", "[255.255.127] 255.255.127[normal]");
    test_replace_ansi ("\x1b[97m 255.255.255\x1b[0m", "[255.255.255] 255.255.255[normal]");
    test_replace_ansi ("\x1b[37m 191.191.191\x1b[0m", "[191.191.191] 191.191.191[normal]");
    test_replace_ansi ("\x1b[37m 170.170.170\x1b[0m", "[170.170.170] 170.170.170[normal]");
    test_replace_ansi ("\x1b[37m 211.211.211\x1b[0m", "[211.211.211] 211.211.211[normal]");
    test_replace_ansi ("\x1b[93m 255.153.0\x1b[0m",   "[255.153.0] 255.153.0[normal]");

    // -------------------------------------------------------------------------
    // Test some of the parameter checking

    set_ansi_color_bits_mode(ansi_8);

    cout << "Testing paramter checking" << endl;
    if (create_ansi_string (normal, nullptr))
        cout << "  create_ansi_string() name nullptr test failed" << endl;
    
    if (replace_color_tags_with_ansi(nullptr, 1, ""))
        cout << "  replace_color_tags_with_ansi() buffer nullptr test failed" << endl;
    if (replace_color_tags_with_ansi(buffer, sizeof(buffer), nullptr))
        cout << "  replace_color_tags_with_ansi() original nullptr test failed" << endl;
    if (replace_color_tags_with_ansi(buffer, 0, ""))
        cout << "  replace_color_tags_with_ansi() buffer size 0 test failed" << endl;
    buffer[0] = 0;
    if (! replace_color_tags_with_ansi(buffer, sizeof(buffer), buffer))
        cout << "  replace_color_tags_with_ansi() destination is source test failed" << endl;
    buffer[0] = 1;
    if (   (! replace_color_tags_with_ansi(buffer, sizeof(buffer), ""))
        || buffer[0] != 0)
        cout << "  replace_color_tags_with_ansi() original empty test failed" << endl;
    if (remove_color_tags(nullptr, 1, ""))
        cout << "  remove_color_tags() buffer nullptr test failed" << endl;
    if (remove_color_tags(buffer, sizeof(buffer), nullptr))
        cout << "  remove_color_tags() original nullptr test failed" << endl;
    if (remove_color_tags(buffer, 0, ""))
        cout << "  remove_color_tags() buffer size 0 test failed" << endl;
    buffer[0] = 1;
    if (   (! remove_color_tags(buffer, sizeof(buffer), ""))
        || buffer[0] != 0)
        cout << "  remove_color_tags() original empty test failed" << endl;
    if (remove_ansi_sequences(nullptr, 1, ""))
        cout << "  remove_ansi_sequences() buffer nullptr test failed" << endl;
    if (remove_ansi_sequences(buffer, sizeof(buffer), nullptr))
        cout << "  remove_ansi_sequences() original nullptr test failed" << endl;
    if (remove_ansi_sequences(buffer, 0, ""))
        cout << "  remove_ansi_sequences() buffer size 0 test failed" << endl;
    buffer[0] = 1;
    if (   (! remove_ansi_sequences(buffer, sizeof(buffer), ""))
        || buffer[0] != 0)
        cout << "  remove_ansi_sequences() original empty test failed" << endl;

    if (replace_substring(nullptr, 1, "", "", false, ""))
        cout << "  replace_substring() buffer nullptr test failed" << endl;
    if (replace_substring(buffer, sizeof(buffer), nullptr, "", false, ""))
        cout << "  replace_substring() original nullptr test failed" << endl;
    if (replace_substring(buffer, sizeof(buffer), "", nullptr, false, ""))
        cout << "  replace_substring() find nullptr test failed" << endl;
    if (replace_substring(buffer, sizeof(buffer), "", "", false, nullptr))
        cout << "  replace_substring() replace nullptr test failed" << endl;
    if (replace_substring(buffer, 0, "", "", false, ""))
        cout << "  replace_substring() buffer size 0 test failed" << endl;
    if (replace_substring(buffer, sizeof(buffer), "", "", false, ""))
        cout << "  replace_substring() find size 0 test failed" << endl;
    buffer[0] = 0;
    if (! replace_substring(buffer, sizeof(buffer), buffer, "1", false, ""))
        cout << "  replace_substring() destination is source test failed" << endl;
    buffer[0] = 1;
    if (   (! replace_substring(buffer, sizeof(buffer), "", "1", false, ""))
        || buffer[0] != 0)
        cout << "  replace_substring() original empty test failed" << endl;
    if (remove_substring(nullptr, 1, "", "", false))
        cout << "  remove_substring() buffer nullptr test failed" << endl;
    if (remove_substring(buffer, sizeof(buffer), nullptr, "", false))
        cout << "  remove_substring() original nullptr test failed" << endl;
    if (remove_substring(buffer, sizeof(buffer), "", nullptr, false))
        cout << "  remove_substring() find nullptr test failed" << endl;
    if (remove_substring(buffer, 0, "", "", false))
        cout << "  remove_substring() buffer size 0 test failed" << endl;
    if (remove_substring(buffer, sizeof(buffer), "", "", false))
        cout << "  remove_substring() find size 0 test failed" << endl;
    buffer[0] = 1;
    if (   (! remove_substring(buffer, sizeof(buffer), "", "1", false))
        || buffer[0] != 0)
        cout << "  remove_substring() original empty test failed" << endl;

    // -------------------------------------------------------------------------
    // Test the conversion of all color and attribute tags to
    // ansi escape sequences under 4- 8- and 24-bit color modes
    // and in foreground and background modes.
    
    cout << "Testing replace ansi tags with ansi sequences" << endl;
    test_replace_ansi ("\x1b[49m",               "[fg]");           // Foreground
    test_replace_ansi ("\x1b[0m",                "[4-bit]");        // 4-bit
    test_replace_ansi ("\x1b[30m",               "[black]");
    test_replace_ansi ("\x1b[31m",               "[red]");
    test_replace_ansi ("\x1b[32m",               "[green]");
    test_replace_ansi ("\x1b[33m",               "[yellow]");
    test_replace_ansi ("\x1b[34m",               "[blue]");
    test_replace_ansi ("\x1b[35m",               "[magenta]");
    test_replace_ansi ("\x1b[35m",               "[purple]");
    test_replace_ansi ("\x1b[36m",               "[cyan]");
    test_replace_ansi ("\x1b[37m",               "[white]");
    test_replace_ansi ("\x1b[90m",               "[bblack]");
    test_replace_ansi ("\x1b[90m",               "[gray]");
    test_replace_ansi ("\x1b[90m",               "[grey]");
    test_replace_ansi ("\x1b[91m",               "[bred]");
    test_replace_ansi ("\x1b[92m",               "[bgreen]");
    test_replace_ansi ("\x1b[93m",               "[byellow]");
    test_replace_ansi ("\x1b[94m",               "[bblue]");
    test_replace_ansi ("\x1b[95m",               "[bmagenta]");
    test_replace_ansi ("\x1b[95m",               "[bpurple]");
    test_replace_ansi ("\x1b[96m",               "[bcyan]");
    test_replace_ansi ("\x1b[97m",               "[bwhite]");
    test_replace_ansi ("\x1b[0m",                "[normal]");
    test_replace_ansi ("\x1b[1m",                "[bold]");
    test_replace_ansi ("\x1b[1m",                "[bright]");
    test_replace_ansi ("\x1b[2m",                "[faint]");
    test_replace_ansi ("\x1b[4m",                "[under]");
    test_replace_ansi ("\x1b[4m",                "[underline]");
    test_replace_ansi ("\x1b[5m",                "[blink]");
    test_replace_ansi ("\x1b[7m",                "[inverse]");
    test_replace_ansi ("\x1b[7m",                "[reverse]");
    test_replace_ansi ("\x1b[22m",               "[nobold]");
    test_replace_ansi ("\x1b[22m",               "[nobright]");
    test_replace_ansi ("\x1b[22m",               "[nofaint]");
    test_replace_ansi ("\x1b[22m",               "[unbold]");
    test_replace_ansi ("\x1b[22m",               "[unbright]");
    test_replace_ansi ("\x1b[22m",               "[unfaint]");
    test_replace_ansi ("\x1b[24m",               "[nounder]");
    test_replace_ansi ("\x1b[25m",               "[noblink]");
    test_replace_ansi ("\x1b[25m",               "[unblink]");
    test_replace_ansi ("\x1b[27m",               "[noinv]");
    test_replace_ansi ("\x1b",                   "[esc]");
    test_replace_ansi ("",                       "[bg]");           // Background
    test_replace_ansi ("\x1b[40m",               "[black]");
    test_replace_ansi ("\x1b[41m",               "[red]");
    test_replace_ansi ("\x1b[42m",               "[green]");
    test_replace_ansi ("\x1b[43m",               "[yellow]");
    test_replace_ansi ("\x1b[44m",               "[blue]");
    test_replace_ansi ("\x1b[45m",               "[magenta]");
    test_replace_ansi ("\x1b[45m",               "[purple]");
    test_replace_ansi ("\x1b[46m",               "[cyan]");
    test_replace_ansi ("\x1b[47m",               "[white]");
    test_replace_ansi ("\x1b[100m",              "[bblack]");
    test_replace_ansi ("\x1b[100m",              "[gray]");
    test_replace_ansi ("\x1b[100m",              "[grey]");
    test_replace_ansi ("\x1b[101m",              "[bred]");
    test_replace_ansi ("\x1b[102m",              "[bgreen]");
    test_replace_ansi ("\x1b[103m",              "[byellow]");
    test_replace_ansi ("\x1b[104m",              "[bblue]");
    test_replace_ansi ("\x1b[105m",              "[bmagenta]");
    test_replace_ansi ("\x1b[105m",              "[bpurple]");
    test_replace_ansi ("\x1b[106m",              "[bcyan]");
    test_replace_ansi ("\x1b[107m",              "[bwhite]");
    test_replace_ansi ("\x1b",                   "[esc]");
    test_replace_ansi ("\x1b[49m",               "[fg]");           // Foreground
    test_replace_ansi ("\x1b[0m",                "[8-bit]");        // 8-bit
    test_replace_ansi ("\x1b[38;5;0m",           "[black]");
    test_replace_ansi ("\x1b[38;5;1m",           "[red]");
    test_replace_ansi ("\x1b[38;5;2m",           "[green]");
    test_replace_ansi ("\x1b[38;5;3m",           "[yellow]");
    test_replace_ansi ("\x1b[38;5;4m",           "[blue]");
    test_replace_ansi ("\x1b[38;5;5m",           "[magenta]");
    test_replace_ansi ("\x1b[38;5;5m",           "[purple]");
    test_replace_ansi ("\x1b[38;5;6m",           "[cyan]");
    test_replace_ansi ("\x1b[38;5;7m",           "[white]");
    test_replace_ansi ("\x1b[38;5;8m",           "[bblack]");
    test_replace_ansi ("\x1b[38;5;8m",           "[gray]");
    test_replace_ansi ("\x1b[38;5;8m",           "[grey]");
    test_replace_ansi ("\x1b[38;5;9m",           "[bred]");
    test_replace_ansi ("\x1b[38;5;10m",          "[bgreen]");
    test_replace_ansi ("\x1b[38;5;11m",          "[byellow]");
    test_replace_ansi ("\x1b[38;5;12m",          "[bblue]");
    test_replace_ansi ("\x1b[38;5;13m",          "[bmagenta]");
    test_replace_ansi ("\x1b[38;5;13m",          "[bpurple]");
    test_replace_ansi ("\x1b[38;5;14m",          "[bcyan]");
    test_replace_ansi ("\x1b[38;5;15m",          "[bwhite]");
    test_replace_ansi ("\x1b[0m",                "[normal]");
    test_replace_ansi ("\x1b[1m",                "[bold]");
    test_replace_ansi ("\x1b[1m",                "[bright]");
    test_replace_ansi ("\x1b[2m",                "[faint]");
    test_replace_ansi ("\x1b[4m",                "[under]");
    test_replace_ansi ("\x1b[4m",                "[underline]");
    test_replace_ansi ("\x1b[5m",                "[blink]");
    test_replace_ansi ("\x1b[7m",                "[inverse]");
    test_replace_ansi ("\x1b[7m",                "[reverse]");
    test_replace_ansi ("\x1b[22m",               "[nobold]");
    test_replace_ansi ("\x1b[22m",               "[nobright]");
    test_replace_ansi ("\x1b[22m",               "[nofaint]");
    test_replace_ansi ("\x1b[22m",               "[unbold]");
    test_replace_ansi ("\x1b[22m",               "[unbright]");
    test_replace_ansi ("\x1b[22m",               "[unfaint]");
    test_replace_ansi ("\x1b[24m",               "[nounder]");
    test_replace_ansi ("\x1b[25m",               "[noblink]");
    test_replace_ansi ("\x1b[25m",               "[unblink]");
    test_replace_ansi ("\x1b[27m",               "[noinv]");
    test_replace_ansi ("\x1b[38;5;25m",          "[azure]");
    test_replace_ansi ("\x1b[38;5;35m",          "[jade]");
    test_replace_ansi ("\x1b[38;5;55m",          "[violet]");
    test_replace_ansi ("\x1b[38;5;70m",          "[lime]");
    test_replace_ansi ("\x1b[38;5;94m",          "[tan]");
    test_replace_ansi ("\x1b[38;5;102m",         "[silver]");
    test_replace_ansi ("\x1b[38;5;125m",         "[pink]");
    test_replace_ansi ("\x1b[38;5;130m",         "[orange]");
    test_replace_ansi ("\x1b",                   "[esc]");
    test_replace_ansi ("",                       "[bg]");           // Background
    test_replace_ansi ("\x1b[48;5;0m",           "[black]");
    test_replace_ansi ("\x1b[48;5;1m",           "[red]");
    test_replace_ansi ("\x1b[48;5;2m",           "[green]");
    test_replace_ansi ("\x1b[48;5;3m",           "[yellow]");
    test_replace_ansi ("\x1b[48;5;4m",           "[blue]");
    test_replace_ansi ("\x1b[48;5;5m",           "[magenta]");
    test_replace_ansi ("\x1b[48;5;5m",           "[purple]");
    test_replace_ansi ("\x1b[48;5;6m",           "[cyan]");
    test_replace_ansi ("\x1b[48;5;7m",           "[white]");
    test_replace_ansi ("\x1b[48;5;8m",           "[bblack]");
    test_replace_ansi ("\x1b[48;5;8m",           "[gray]");
    test_replace_ansi ("\x1b[48;5;8m",           "[grey]");
    test_replace_ansi ("\x1b[48;5;9m",           "[bred]");
    test_replace_ansi ("\x1b[48;5;10m",          "[bgreen]");
    test_replace_ansi ("\x1b[48;5;11m",          "[byellow]");
    test_replace_ansi ("\x1b[48;5;12m",          "[bblue]");
    test_replace_ansi ("\x1b[48;5;13m",          "[bmagenta]");
    test_replace_ansi ("\x1b[48;5;13m",          "[bpurple]");
    test_replace_ansi ("\x1b[48;5;14m",          "[bcyan]");
    test_replace_ansi ("\x1b[48;5;15m",          "[bwhite]");
    test_replace_ansi ("\x1b[48;5;25m",          "[azure]");
    test_replace_ansi ("\x1b[48;5;35m",          "[jade]");
    test_replace_ansi ("\x1b[48;5;55m",          "[violet]");
    test_replace_ansi ("\x1b[48;5;70m",          "[lime]");
    test_replace_ansi ("\x1b[48;5;94m",          "[tan]");
    test_replace_ansi ("\x1b[48;5;102m",         "[silver]");
    test_replace_ansi ("\x1b[48;5;125m",         "[pink]");
    test_replace_ansi ("\x1b[48;5;130m",         "[orange]");
    test_replace_ansi ("\x1b",                   "[esc]");
    test_replace_ansi ("\x1b[49m",               "[fg]");           // Foreground
    test_replace_ansi ("\x1b[0m",                "[24-bit]");       // 24-bit
    test_replace_ansi ("\x1b[38;2;0;0;0m",       "[black]");
    test_replace_ansi ("\x1b[38;2;187;0;0m",     "[red]");
    test_replace_ansi ("\x1b[38;2;0;187;0m",     "[green]");
    test_replace_ansi ("\x1b[38;2;187;187;0m",   "[yellow]");
    test_replace_ansi ("\x1b[38;2;0;0;187m",     "[blue]");
    test_replace_ansi ("\x1b[38;2;187;0;187m",   "[magenta]");
    test_replace_ansi ("\x1b[38;2;187;0;187m",   "[purple]");
    test_replace_ansi ("\x1b[38;2;0;187;187m",   "[cyan]");
    test_replace_ansi ("\x1b[38;2;187;187;187m", "[white]");
    test_replace_ansi ("\x1b[38;2;127;127;127m", "[bblack]");
    test_replace_ansi ("\x1b[38;2;127;127;127m", "[gray]");
    test_replace_ansi ("\x1b[38;2;127;127;127m", "[grey]");
    test_replace_ansi ("\x1b[38;2;255;0;0m",     "[bred]");
    test_replace_ansi ("\x1b[38;2;0;255;0m",     "[bgreen]");
    test_replace_ansi ("\x1b[38;2;255;255;0m",   "[byellow]");
    test_replace_ansi ("\x1b[38;2;0;0;255m",     "[bblue]");
    test_replace_ansi ("\x1b[38;2;255;0;255m",   "[bmagenta]");
    test_replace_ansi ("\x1b[38;2;255;0;255m",   "[bpurple]");
    test_replace_ansi ("\x1b[38;2;0;255;255m",   "[bcyan]");
    test_replace_ansi ("\x1b[38;2;255;255;255m", "[bwhite]");
    test_replace_ansi ("\x1b[0m",                "[normal]");
    test_replace_ansi ("\x1b[1m",                "[bold]");
    test_replace_ansi ("\x1b[1m",                "[bright]");
    test_replace_ansi ("\x1b[2m",                "[faint]");
    test_replace_ansi ("\x1b[4m",                "[under]");
    test_replace_ansi ("\x1b[4m",                "[underline]");
    test_replace_ansi ("\x1b[5m",                "[blink]");
    test_replace_ansi ("\x1b[7m",                "[inverse]");
    test_replace_ansi ("\x1b[7m",                "[reverse]");
    test_replace_ansi ("\x1b[22m",               "[nobold]");
    test_replace_ansi ("\x1b[22m",               "[nobright]");
    test_replace_ansi ("\x1b[22m",               "[nofaint]");
    test_replace_ansi ("\x1b[22m",               "[unbold]");
    test_replace_ansi ("\x1b[22m",               "[unbright]");
    test_replace_ansi ("\x1b[22m",               "[unfaint]");
    test_replace_ansi ("\x1b[24m",               "[nounder]");
    test_replace_ansi ("\x1b[25m",               "[noblink]");
    test_replace_ansi ("\x1b[25m",               "[unblink]");
    test_replace_ansi ("\x1b[27m",               "[noinv]");
    test_replace_ansi ("\x1b[38;2;0;102;187m",   "[azure]");
    test_replace_ansi ("\x1b[38;2;0;187;102m",   "[jade]");
    test_replace_ansi ("\x1b[38;2;102;0;187m",   "[violet]");
    test_replace_ansi ("\x1b[38;2;102;187;0m",   "[lime]");
    test_replace_ansi ("\x1b[38;2;136;102;0m",   "[tan]");
    test_replace_ansi ("\x1b[38;2;136;136;136m", "[silver]");
    test_replace_ansi ("\x1b[38;2;187;0;102m",   "[pink]");
    test_replace_ansi ("\x1b[38;2;187;102;0m",   "[orange]");
    test_replace_ansi ("\x1b",                   "[esc]");
    test_replace_ansi ("",                       "[bg]");           // Background
    test_replace_ansi ("\x1b[48;2;0;0;0m",       "[black]");
    test_replace_ansi ("\x1b[48;2;187;0;0m",     "[red]");
    test_replace_ansi ("\x1b[48;2;0;187;0m",     "[green]");
    test_replace_ansi ("\x1b[48;2;187;187;0m",   "[yellow]");
    test_replace_ansi ("\x1b[48;2;0;0;187m",     "[blue]");
    test_replace_ansi ("\x1b[48;2;187;0;187m",   "[magenta]");
    test_replace_ansi ("\x1b[48;2;187;0;187m",   "[purple]");
    test_replace_ansi ("\x1b[48;2;0;187;187m",   "[cyan]");
    test_replace_ansi ("\x1b[48;2;187;187;187m", "[white]");
    test_replace_ansi ("\x1b[48;2;127;127;127m", "[bblack]");
    test_replace_ansi ("\x1b[48;2;127;127;127m", "[gray]");
    test_replace_ansi ("\x1b[48;2;127;127;127m", "[grey]");
    test_replace_ansi ("\x1b[48;2;255;0;0m",     "[bred]");
    test_replace_ansi ("\x1b[48;2;0;255;0m",     "[bgreen]");
    test_replace_ansi ("\x1b[48;2;255;255;0m",   "[byellow]");
    test_replace_ansi ("\x1b[48;2;0;0;255m",     "[bblue]");
    test_replace_ansi ("\x1b[48;2;255;0;255m",   "[bmagenta]");
    test_replace_ansi ("\x1b[48;2;255;0;255m",   "[bpurple]");
    test_replace_ansi ("\x1b[48;2;0;255;255m",   "[bcyan]");
    test_replace_ansi ("\x1b[48;2;255;255;255m", "[bwhite]");
    test_replace_ansi ("\x1b[48;2;0;102;187m",   "[azure]");
    test_replace_ansi ("\x1b[48;2;0;187;102m",   "[jade]");
    test_replace_ansi ("\x1b[48;2;102;0;187m",   "[violet]");
    test_replace_ansi ("\x1b[48;2;102;187;0m",   "[lime]");
    test_replace_ansi ("\x1b[48;2;136;102;0m",   "[tan]");
    test_replace_ansi ("\x1b[48;2;136;136;136m", "[silver]");
    test_replace_ansi ("\x1b[48;2;187;0;102m",   "[pink]");
    test_replace_ansi ("\x1b[48;2;187;102;0m",   "[orange]");
    test_replace_ansi ("\x1b",                   "[esc]");
    set_ansi_color_bits_mode (ansi_4);
    set_ansi_foreground_mode (ansi_fore);
    test_replace_ansi ("\x1b[32m",               "[green]");
    set_ansi_foreground_mode (ansi_back);
    test_replace_ansi ("\x1b[42m",               "[green]");
    set_ansi_color_bits_mode (ansi_8);
    set_ansi_foreground_mode (ansi_fore);
    test_replace_ansi ("\x1b[38;5;2m",           "[green]");
    set_ansi_foreground_mode (ansi_back);
    test_replace_ansi ("\x1b[48;5;2m",           "[green]");
    set_ansi_color_bits_mode (ansi_24);
    set_ansi_foreground_mode (ansi_fore);
    test_replace_ansi ("\x1b[38;2;0;187;0m",     "[green]");
    set_ansi_foreground_mode (ansi_back);
    test_replace_ansi ("\x1b[48;2;0;187;0m",     "[green]");
    test_replace_ansi ("\x1b[49m",               "[fg]");           // Foreground
    test_replace_ansi ("\x1b[0m",                "[4-bit]");        // 4-bit
    test_replace_ansi ("\x1b[32m",               "[green]");
    
    // Try some tests where the source and destination buffer is the same
    buffer[0] = 0;
    if (! replace_color_tags_with_ansi(buffer, sizeof(buffer), buffer))
        cout << "  replace_color_tags_with_ansi() destination is source test failed" << endl;

    // -------------------------------------------------------------------------
    // Test the replacement of tags with ansi escape sequences
    // when embedded in a line of text. Text that may be utf-8.
    
    set_ansi_color_bits_mode(ansi_4);
    test_replace_ansi("This is a test of 4-bit \x1b[31mred\x1b[0m, \x1b[2m\x1b[32mfaint green\x1b[0m and \x1b[4m\x1b[5m\x1b[34munderlined blinking blue\x1b[0m and a [mismatch].",
                      "This is a test of 4-bit [red]red[normal], [faint][green]faint green[normal] and [under][blink][blue]underlined blinking blue[normal] and a [mismatch].");
    set_ansi_color_bits_mode(ansi_8);
    test_replace_ansi("The embedded 8-bit palette index for \x1b[38;5;1mred\x1b[0m, \x1b[38;5;11mbright yellow\x1b[0m, \x1b[38;5;14mbright cyan\x1b[0m and \x1b[38;5;15mbright white\x1b[0m.",
                      "The embedded 8-bit palette index for [1]red[normal], [013]bright yellow[normal], [0xe]bright cyan[normal] and [0x0f]bright white[normal].");
    set_ansi_color_bits_mode(ansi_24);
    test_replace_ansi("The embedded 24-bit rgb values for \x1b[38;2;192;0;0mred\x1b[0m, \x1b[38;2;192;255;192mbright white with a green tint\x1b[0m and \x1b[38;2;255;255;191mbright white with a yellow tint\x1b[0m.",
                      "The embedded 24-bit rgb values for [192.0.0]red[normal], [192.255.192]bright white with a green tint[normal] and [0xff.0xff.0xbf]bright white with a yellow tint[normal].");
    set_ansi_color_bits_mode(ansi_4);
    test_replace_ansi("\x1b[31m\x1b[47mred on white \x1b[103mred on byellow\x1b[49m and plain red\x1b[0m.",
                      "[red][bg][white]red on white [byellow]red on byellow[fg] and plain red[normal].");
    test_replace_ansi("\x1b[0m\x1b[31m4-bit red\x1b[0m, \x1b[38;5;1m8-bit red \x1b[38;5;242mgray\x1b[0m, \x1b[38;2;187;0;0m24-bit red \x1b[38;2;79;255;79mgreenish\x1b[0m and normal.",
                      "[4-bit][red]4-bit red[8-bit], [red]8-bit red [242]gray[24-bit], [red]24-bit red [0x4f.255.0x4f]greenish[4-bit] and normal.");
    test_replace_ansi("How about some utf-8: 𐍈\x1b[31m𐍈red 𐍈\x1b[0m\xf0\x90\x8d\x88normal.",
                      "How about some utf-8: 𐍈[red]𐍈red 𐍈[normal]\xf0\x90\x8d\x88normal.");

    // Test using a common buffer for source and destination
    strncpy(buffer,
            "[4-bit][red]4-bit red[8-bit], [red]8-bit red [242]gray[24-bit], [red]24-bit red [0x4f.255.0x4f]greenish[4-bit] and normal.",
            sizeof(buffer));
    if (! replace_color_tags_with_ansi(buffer, sizeof(buffer), buffer))
        cout << "  replace_color_tags_with_ansi() destination is source failed" << endl;
    else if (strcmp(buffer,
                    "\x1b[0m\x1b[31m4-bit red\x1b[0m, \x1b[38;5;1m8-bit red \x1b[38;5;242mgray\x1b[0m, \x1b[38;2;187;0;0m24-bit red \x1b[38;2;79;255;79mgreenish\x1b[0m and normal.") != 0)
        cout << "  replace_color_tags_with_ansi() destination is source mismatch" << endl;

    // -------------------------------------------------------------------------
    // Test the removal of tags and ansi escape sequences
    // from a line of text. Text that may be utf-8.

    cout << "Testing remove ansi tags" << endl;
    test_replace_ansi("This is a test of 4-bit 𐍈𐍈red, faint green and underlined blinking blue and a [mismatch].",
                      "This is a test of 4-bit 𐍈[red]\xf0\x90\x8d\x88red[normal], [faint][green]faint green[normal] and [under][blink][blue]underlined blinking blue[normal] and a [mismatch].", true);
    cout << "Testing remove ansi sequences" << endl;
    test_replace_ansi("This is a test of 4-bit 𐍈𐍈red, 8-bit green and 24-bit blue ansi escape sequences.",
                      "This is a test of 4-bit 𐍈\x1b[31m\xf0\x90\x8d\x88red\x1b[0m, 8-bit \x1b[38;5;2mgreen\x1b[0m and 24-bit \x1b[38;2;0;0;191mblue\x1b[0m ansi escape sequences.", false, true);
    
    // Try removals again but let the source and destination buffers be the same
    strncpy(buffer,
            "This is a test of 4-bit 𐍈[red]\xf0\x90\x8d\x88red[normal], [faint][green]faint green[normal] and [under][blink][blue]underlined blinking blue[normal] and a [mismatch].",
            sizeof(buffer));
    if (   (! remove_color_tags(buffer, sizeof(buffer), buffer))
        || strcmp(buffer, "This is a test of 4-bit 𐍈𐍈red, faint green and underlined blinking blue and a [mismatch].") != 0)
        cout << "  remove_color_tags() destination is source test failed" << endl;
    strncpy(buffer,
            "This is a test of 4-bit 𐍈\x1b[31m\xf0\x90\x8d\x88red\x1b[0m, 8-bit \x1b[38;5;2mgreen\x1b[0m and 24-bit \x1b[38;2;0;0;191mblue\x1b[0m ansi escape sequences.",
            sizeof(buffer));
    if (   (! remove_ansi_sequences(buffer, sizeof(buffer), buffer))
        || strcmp(buffer, "This is a test of 4-bit 𐍈𐍈red, 8-bit green and 24-bit blue ansi escape sequences.") != 0)
        cout << "  remove_ansi_sequences() destination is source test failed" << endl;

    // -------------------------------------------------------------------------
    // Test some conditions where square brackets are not indicating a tag

    cout << "Testing ansi tag mismatches" << endl;
    test_replace_ansi("",
                      "");
    test_replace_ansi("red",
                      "red");
    test_replace_ansi("[red",
                      "[red");
    test_replace_ansi("]red",
                      "]red");
    test_replace_ansi("red[",
                      "red[");
    test_replace_ansi("red]",
                      "red]");
    test_replace_ansi("[mismatch]",
                      "[mismatch]");
    test_replace_ansi("[]Empty brackets.",
                      "[]Empty brackets.");
    test_replace_ansi("[The color magenta] is supported but that wasn't a proper name, and neither is [purple is magenta]",
                      "[The color magenta] is supported but that wasn't a proper name, and neither is [purple is magenta]");
    test_replace_ansi("[These are too far to even be considered]",
                      "[These are too far to even be considered]");
    test_replace_ansi("[red Red with no ending bracket",
                      "[red Red with no ending bracket");
    test_replace_ansi("red]Red with no starting bracket",
                      "red]Red with no starting bracket");
    test_replace_ansi("[ red]Red with an extra character",
                      "[ red]Red with an extra character");
    test_replace_ansi("Numeric parsing failure [c] no 0x [256] [0x100] [256.256.256] [0x100.0x100.0x100] too large and [-5] negative\x1b[0m.",
                      "Numeric parsing failure [c] no 0x [256] [0x100] [256.256.256] [0x100.0x100.0x100] too large and [-5] negative[normal].");
    test_replace_ansi("Does hex work in ansi sequences \x1b[38;2;0;0;0xbfmblue\x1b[0m ansi escape sequences.",
                      "Does hex work in ansi sequences \x1b[38;2;0;0;0xbfmblue\x1b[0m ansi escape sequences.");
    test_replace_ansi ("]]\x1b[31mRed\x1b[0m with leading closing bracket",
                       "]][red]Red[normal] with leading closing bracket");
    test_replace_ansi ("[[\x1b[31mRed\x1b[0m with extra opening bracket",
                       "[[[red]Red[normal] with extra opening bracket");

    // -------------------------------------------------------------------------
    // Test the replacement and removal of normal substrings
    // from a line of text. Substrings that may be utf-8.

    cout << "Testing replace substring" << endl;
    test_replace_string("This is the WRONG substring.",
                        "This is the WRONG substring.", "wrong", false, "correct");
    test_replace_string("This is the correct substring.",
                        "This is the WRONG substring.", "wrong", true, "correct");
    test_replace_string("This substring is correct correct correct",
                        "This substring is WRONG wrong Wrong", "wrong", true, "correct");
    test_replace_string("Correct I say.",
                        "Wrong I say.", "Wrong", false, "Correct");
    test_replace_string("This is the 𐍈correct\xf0\x90\x8d\x88 substring with utf-8.",
                        "This is the 𐍈WRONG\xf0\x90\x8d\x88 substring with utf-8.", "wrong", true, "correct");
    test_replace_string("This is utf-8 replaced with utf-8 \x24\xC2\xA2\xE2\x82\xAC\xF0\x90\x8D\x88",
                        "This is utf-8 replaced with utf-8 \x24\x24\xF0\x90\x8D\x88\xF0\x90\x8D\x88", "\x24\xF0\x90\x8D\x88", false, "\xC2\xA2\xE2\x82\xAC");
    test_replace_string("", "", "search", false, "replace");

    cout << "Testing remove substring" << endl;
    test_replace_string("This is the correctPLUS substring.",
                        "This is the correctPLUS substring.", "plus", false, nullptr);
    test_replace_string("This is the correct substring.",
                        "This is the correctPLUS substring.", "plus", true, nullptr);
    test_replace_string("This is the correct𐍈\xf0\x90\x8d\x88 substring with utf-8.",
                        "This is the correct𐍈PLUS\xf0\x90\x8d\x88 substring with utf-8.", "plus", true, nullptr);
    
    // Try removal again but let the source and destination buffers be the same
    strncpy(buffer,
            "This is the correct𐍈PLUS\xf0\x90\x8d\x88 substring with utf-8.",
            sizeof(buffer));
    if (   (! replace_substring(buffer, sizeof(buffer), buffer, "plus", true))
        || strcmp(buffer, "This is the correct𐍈\xf0\x90\x8d\x88 substring with utf-8.") != 0)
        cout << "  replace_substring() destination is source test failed" << endl;
    
    // Try removal again but use the inline remove function
    strncpy(buffer,
            "This is the correct𐍈PLUS\xf0\x90\x8d\x88 substring with utf-8.",
            sizeof(buffer));
    if (   (! remove_substring(buffer, sizeof(buffer), buffer, "plus", true))
        || strcmp(buffer, "This is the correct𐍈\xf0\x90\x8d\x88 substring with utf-8.") != 0)
        cout << "  remove_substring() destination is source test failed" << endl;

    // -------------------------------------------------------------------------
    // Test formatting with our snprintf wrappera

    cout << "Testing snprintf wrappers" << endl;
    test_format_string ("The utf-8 string is \x24\xC2\xA2\xE2\x82\xAC\xF0\x90\x8D\x88",
                        "The utf-8 string is %s", "\x24\xC2\xA2\xE2\x82\xAC\xF0\x90\x8D\x88");
    test_format_int    ("The number in a 5 character field is    10, right justified",
                        "The number in a 5 character field is %5d, right justified", 10);
    test_format_int    ("The number in a 5 character field is 10   , left justified",
                        "The number in a 5 character field is %-5d, left justified", 10);
    test_format_double ("The number in a 5 character field is   0.1, right justified",
                        "The number in a 5 character field is %5.1f, right justified", 0.1);
    test_format_double ("The number in a 5 character field is 0.1  , left justified",
                        "The number in a 5 character field is %-5.1f, left justified", 0.1);
    test_format_char   ("The character in a 5 character field is     A, right justified",
                        "The character in a 5 character field is %5c, right justified", 'A');
    test_format_char   ("The character in a 5 character field is A    , left justified",
                        "The character in a 5 character field is %-5c, left justified", 'A');
    test_format_char   ("\xF0\x90\x8D\x88",
                        "%c", 0x10348);

    return 0;
}
