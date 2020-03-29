//
//  test-protocols.cc
//
//  Copyright (C) 2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//
// Building manually at the console:
//     macoS:   clang++ -std=c++11 -DNO_MOO_BUILTINS -I../../src/include test-protocols.cc utils.cc ../../src/substring.cc ../../src/protocols.cc
//     linux:   g++ -std=c++11 -DNO_MOO_BUILTINS -I../../src/include test-protocols.cc utils.cc ../../src/substring.cc ../../src/protocols.cc
//     windows: cl /EHsc /DNO_MOO_BUILTINS /I../../src/include test-protocols.cc utils.cc ../../src/substring.cc ../../src/protocols.cc Shlwapi.lib
//              To change the code page of the console:
//                  chcp 65001

#include <iostream>

#include "utils.h"
#include "substring.h"
#include "protocols.h"

using namespace std;



// -------------------------------------------------------------------------
void test_extract_remove_outofband(const char *expected,
                                   const char *original,
                                   uint8_t    protocol,
                                   bool       extract) {
    char replacement[256];

    bool result =   extract
                  ? protocol_extract (replacement,
                                      sizeof(replacement),
                                      original,
                                      protocol)
                  : protocol_remove  (replacement,
                                      sizeof(replacement),
                                      original,
                                      protocol);

    if (result) {
        compare_to_expected(expected, replacement);
    }
    else
        report_failure(  extract
                       ? "protocol_extract"
                       : "protocol_remove",
                       original);
}



// -------------------------------------------------------------------------
void test_create_outofband(const char *expected,
                           const char *body,
                           uint8_t    protocol,
                           uint8_t    tag) {
    char message[256];

    if (protocol_create(message,
                        sizeof(message),
                        body,
                        protocol,
                        tag)) {
        compare_to_expected(expected, message);
    }
    else
        report_failure("protocol_create", body);
}



// -------------------------------------------------------------------------
int main(void) {

    cout << "Testing extraction of out-of-band data" << endl;
    test_extract_remove_outofband("Out of band",
                                  "The dog\xff\xfaOut of band\xff\xf0 barks",
                                  0, true);
    test_extract_remove_outofband("Out of band",
                                  "\xff\xfaOut\xff\xf0The dog barks\xff\xfa of\xff\xf0\xff\xfa band\xff\xf0",
                                  0, true);
    test_extract_remove_outofband("Out of band",
                                  "The dog\xff\xfa\xc9Out of band\xff\xf0 barks",
                                  gmcp_id, true);
    test_extract_remove_outofband("Out of band",
                                  "\xff\xfa\xc9Out\xff\xf0The dog barks\xff\xfa\xc9 of\xff\xf0\xff\xfa\xc9 band\xff\xf0",
                                  gmcp_id, true);
    test_extract_remove_outofband("Out of band",
                                  "The dog\xff\xfa\x45Out of band\xff\xf0 barks",
                                  msdp_id, true);
    test_extract_remove_outofband("Out of band",
                                  "\xff\xfa\x45Out\xff\xf0The dog barks\xff\xfa\x45 of\xff\xf0\xff\xfa\x45 band\xff\xf0",
                                  msdp_id, true);
    test_extract_remove_outofband("Out of band",
                                  "The dog\x1b[Out of bandz barks",
                                  mxp_id, true);
    test_extract_remove_outofband("Out of band",
                                  "\x1b[OutzThe dog barks\x1b[ ofz\x1b[ bandz",
                                  mxp_id, true);

    cout << "Testing removal of out-of-band data" << endl;
    test_extract_remove_outofband("The dog barks",
                                  "The dog\xff\xfaOut of band\xff\xf0 barks",
                                  0, false);
    test_extract_remove_outofband("The dog barks",
                                  "\xff\xfaOut\xff\xf0The dog barks\xff\xfa of\xff\xf0\xff\xfa band\xff\xf0",
                                  0, false);
    test_extract_remove_outofband("The dog barks",
                                  "The dog\xff\xfa\xc9Out of band\xff\xf0 barks",
                                  gmcp_id, false);
    test_extract_remove_outofband("The dog barks",
                                  "\xff\xfa\xc9Out\xff\xf0The dog barks\xff\xfa\xc9 of\xff\xf0\xff\xfa\xc9 band\xff\xf0",
                                  gmcp_id, false);
    test_extract_remove_outofband("The dog barks",
                                  "The dog\xff\xfa\x45Out of band\xff\xf0 barks",
                                  msdp_id, false);
    test_extract_remove_outofband("The dog barks",
                                  "\xff\xfa\x45Out\xff\xf0The dog barks\xff\xfa\x45 of\xff\xf0\xff\xfa\x45 band\xff\xf0",
                                  msdp_id, false);
    test_extract_remove_outofband("The dog barks",
                                  "The dog\x1b[Out of bandz barks",
                                  mxp_id, false);
    test_extract_remove_outofband("The dog barks",
                                  "\x1b[OutzThe dog barks\x1b[ ofz\x1b[ bandz",
                                  mxp_id, false);

    cout << "Testing creation of message with out-of-band data" << endl;
    test_create_outofband("\xff\xfaThe dog barks\xff\xf0",
                          "The dog barks",
                          0, 0);
    test_create_outofband("\xff\xfa\xc9The dog barks\xff\xf0",
                          "The dog barks",
                          gmcp_id, 0);
    test_create_outofband("\xff\xfa\x45The dog barks\xff\xf0",
                          "The dog barks",
                          msdp_id, 0);
    test_create_outofband("\x1b[99zThe dog barks",
                          "The dog barks",
                          mxp_id, 99);

    return 0;
}
