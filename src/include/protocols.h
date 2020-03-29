//
//  protocols.h
//
//  Copyright (C) 2020 Shan Hollen. All Rights Reserved.
//  Programming: Perpenso LLC, Tony Tribelli
//

#ifndef protocols_h
#define protocols_h

#include <cstdint>

enum protocol_codes { telnet_interpret_as_command = 255,
                      telnet_start_subnegotiation = 250,
                      telnet_end_subnegotiation   = 240,
                      telnet_option_will          = 251,
                      telnet_option_wont          = 252,
                      telnet_option_do            = 253,
                      telnet_option_dont          = 254,
                      gmcp_id                     = 201,
                      msdp_id                     = 69,
                      mxp_id                      = 27 };

// -----------------------------------------------------------------------------

bool protocol_request (uint8_t    protocol);
bool protocol_extract (char       *replacement,
                       size_t     size,
                       const char *original,
                       uint8_t    protocol);
bool protocol_remove  (char       *replacement,
                       size_t     size,
                       const char *original,
                       uint8_t    protocol);
bool protocol_create  (char       *message,
                       size_t     size,
                       const char *body,
                       uint8_t    protocol,
                       uint8_t    tag);

#endif /* protocols_h */
