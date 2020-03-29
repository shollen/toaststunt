//
//  utils.h
//
//  Copyright (C) 2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli

#ifndef utils_h
#define utils_h

bool compare_to_expected (int        expected,  int        actual);
bool compare_to_expected (const char *expected, const char *actual);
void report_failure      (const char *protocol, const char *input);

#endif /* utils_h */
