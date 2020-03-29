//
//  utils.cc
//
//  Copyright (C) 2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli

#include <iostream>

#include "utils.h"

using namespace std;



// -------------------------------------------------------------------------
bool compare_to_expected(const char *expected, const char *actual) {
    bool same = strcmp(expected, actual) == 0;
    
    if (! same)
        cout << "  Expected: " << " \"" << expected << "\"" << endl
             << "  Actual:   " << " \"" << actual   << "\"" << endl;
    
    return same;
}

// -------------------------------------------------------------------------
void report_failure(const char *protocol, const char *input) {
    cout << "  " << protocol << " failed \"" << input << "\"" << endl;
}
