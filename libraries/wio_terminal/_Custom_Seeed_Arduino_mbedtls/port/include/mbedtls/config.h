/* This shim header is added so that any application code
   which includes "mbedtls/config.h" directly gets the correct
   config. */

/*
 * SPDX-License-Identifier: MIT
 * 
 * References:
 * https://github.com/Mbed-TLS/mbedtls/tree/development/include/mbedtls
 */

#pragma once
#if !defined(MBEDTLS_CONFIG_FILE)
#include_next "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif
