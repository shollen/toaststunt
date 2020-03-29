//
//  test-protocols.cc
//
//  Copyright (C) 2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//
// Building manually at the console:
//     macoS:   clang++ -std=c++11 -DNO_MOO_BUILTINS -I../../src/include test-protocols.cc ../../src/substring.cc ../../src/protocols.cc
//     linux:   g++ -std=c++11 -DNO_MOO_BUILTINS -I../../src/include test-protocols.cc ../../src/substring.cc ../../src/protocols.cc
//     windows: cl /EHsc /DNO_MOO_BUILTINS /I../../src/include test-protocols.cc ../../src/substring.cc ../../src/protocols.cc Shlwapi.lib
//              To change the code page of the console:
//                  chcp 65001

#include <iostream>

#include "substring.h"
#include "protocols.h"

using namespace std;



// -------------------------------------------------------------------------
int main(void) {

    cout << "Testing extraction of out-of-band data" << endl;

    return 0;
}
