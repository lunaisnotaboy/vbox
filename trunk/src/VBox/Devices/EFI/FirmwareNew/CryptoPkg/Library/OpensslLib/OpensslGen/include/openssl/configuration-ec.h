/*
 * WARNING: do not edit!
 * Generated by configdata.pm from Configurations/common0.tmpl, Configurations/unix-Makefile.tmpl
 * via Makefile.in
 *
 * Copyright 2016-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OPENSSL_CONFIGURATION_H
# define OPENSSL_CONFIGURATION_H
# pragma once

# ifdef  __cplusplus
extern "C" {
# endif

# ifdef OPENSSL_ALGORITHM_DEFINES
#  error OPENSSL_ALGORITHM_DEFINES no longer supported
# endif

/*
 * OpenSSL was configured with the following options:
 */

# ifndef OPENSSL_SYS_UEFI
#  define OPENSSL_SYS_UEFI 1
# endif
# define OPENSSL_CONFIGURED_API 10101
# ifndef OPENSSL_RAND_SEED_NONE
#  define OPENSSL_RAND_SEED_NONE
# endif
# ifndef OPENSSL_NO_ACVP_TESTS
#  define OPENSSL_NO_ACVP_TESTS
# endif
# ifndef OPENSSL_NO_AFALGENG
#  define OPENSSL_NO_AFALGENG
# endif
# ifndef OPENSSL_NO_APPS
#  define OPENSSL_NO_APPS
# endif
# ifndef OPENSSL_NO_ARIA
#  define OPENSSL_NO_ARIA
# endif
# ifndef OPENSSL_NO_ASAN
#  define OPENSSL_NO_ASAN
# endif
# ifndef OPENSSL_NO_ASYNC
#  define OPENSSL_NO_ASYNC
# endif
# ifndef OPENSSL_NO_AUTOERRINIT
#  define OPENSSL_NO_AUTOERRINIT
# endif
# ifndef OPENSSL_NO_AUTOLOAD_CONFIG
#  define OPENSSL_NO_AUTOLOAD_CONFIG
# endif
# ifndef OPENSSL_NO_BF
#  define OPENSSL_NO_BF
# endif
# ifndef OPENSSL_NO_BLAKE2
#  define OPENSSL_NO_BLAKE2
# endif
# ifndef OPENSSL_NO_CAMELLIA
#  define OPENSSL_NO_CAMELLIA
# endif
# ifndef OPENSSL_NO_CAPIENG
#  define OPENSSL_NO_CAPIENG
# endif
# ifndef OPENSSL_NO_CAST
#  define OPENSSL_NO_CAST
# endif
# ifndef OPENSSL_NO_CHACHA
#  define OPENSSL_NO_CHACHA
# endif
# ifndef OPENSSL_NO_CMAC
#  define OPENSSL_NO_CMAC
# endif
# ifndef OPENSSL_NO_CMP
#  define OPENSSL_NO_CMP
# endif
# ifndef OPENSSL_NO_CMS
#  define OPENSSL_NO_CMS
# endif
# ifndef OPENSSL_NO_CRMF
#  define OPENSSL_NO_CRMF
# endif
# ifndef OPENSSL_NO_CRYPTO_MDEBUG
#  define OPENSSL_NO_CRYPTO_MDEBUG
# endif
# ifndef OPENSSL_NO_CRYPTO_MDEBUG_BACKTRACE
#  define OPENSSL_NO_CRYPTO_MDEBUG_BACKTRACE
# endif
# ifndef OPENSSL_NO_CT
#  define OPENSSL_NO_CT
# endif
# ifndef OPENSSL_NO_DEPRECATED
#  define OPENSSL_NO_DEPRECATED
# endif
# ifndef OPENSSL_NO_DES
#  define OPENSSL_NO_DES
# endif
# ifndef OPENSSL_NO_DEVCRYPTOENG
#  define OPENSSL_NO_DEVCRYPTOENG
# endif
# ifndef OPENSSL_NO_DGRAM
#  define OPENSSL_NO_DGRAM
# endif
# ifndef OPENSSL_NO_DSA
#  define OPENSSL_NO_DSA
# endif
# ifndef OPENSSL_NO_DSO
#  define OPENSSL_NO_DSO
# endif
# ifndef OPENSSL_NO_DTLS
#  define OPENSSL_NO_DTLS
# endif
# ifndef OPENSSL_NO_DTLS1
#  define OPENSSL_NO_DTLS1
# endif
# ifndef OPENSSL_NO_DTLS1_METHOD
#  define OPENSSL_NO_DTLS1_METHOD
# endif
# ifndef OPENSSL_NO_DTLS1_2
#  define OPENSSL_NO_DTLS1_2
# endif
# ifndef OPENSSL_NO_DTLS1_2_METHOD
#  define OPENSSL_NO_DTLS1_2_METHOD
# endif
# ifndef OPENSSL_NO_EC2M
#  define OPENSSL_NO_EC2M
# endif
# ifndef OPENSSL_NO_EC_NISTP_64_GCC_128
#  define OPENSSL_NO_EC_NISTP_64_GCC_128
# endif
# ifndef OPENSSL_NO_EGD
#  define OPENSSL_NO_EGD
# endif
# ifndef OPENSSL_NO_ENGINE
#  define OPENSSL_NO_ENGINE
# endif
# ifndef OPENSSL_NO_ERR
#  define OPENSSL_NO_ERR
# endif
# ifndef OPENSSL_NO_EXTERNAL_TESTS
#  define OPENSSL_NO_EXTERNAL_TESTS
# endif
# ifndef OPENSSL_NO_FILENAMES
#  define OPENSSL_NO_FILENAMES
# endif
# ifndef OPENSSL_NO_FIPS_SECURITYCHECKS
#  define OPENSSL_NO_FIPS_SECURITYCHECKS
# endif
# ifndef OPENSSL_NO_FUZZ_AFL
#  define OPENSSL_NO_FUZZ_AFL
# endif
# ifndef OPENSSL_NO_FUZZ_LIBFUZZER
#  define OPENSSL_NO_FUZZ_LIBFUZZER
# endif
# ifndef OPENSSL_NO_GOST
#  define OPENSSL_NO_GOST
# endif
# ifndef OPENSSL_NO_IDEA
#  define OPENSSL_NO_IDEA
# endif
# ifndef OPENSSL_NO_KTLS
#  define OPENSSL_NO_KTLS
# endif
# ifndef OPENSSL_NO_LOADERENG
#  define OPENSSL_NO_LOADERENG
# endif
# ifndef OPENSSL_NO_MD2
#  define OPENSSL_NO_MD2
# endif
# ifndef OPENSSL_NO_MD4
#  define OPENSSL_NO_MD4
# endif
# ifndef OPENSSL_NO_MDC2
#  define OPENSSL_NO_MDC2
# endif
# ifndef OPENSSL_NO_MSAN
#  define OPENSSL_NO_MSAN
# endif
# ifndef OPENSSL_NO_MULTIBLOCK
#  define OPENSSL_NO_MULTIBLOCK
# endif
# ifndef OPENSSL_NO_NEXTPROTONEG
#  define OPENSSL_NO_NEXTPROTONEG
# endif
# ifndef OPENSSL_NO_OCB
#  define OPENSSL_NO_OCB
# endif
# ifndef OPENSSL_NO_OCSP
#  define OPENSSL_NO_OCSP
# endif
# ifndef OPENSSL_NO_PADLOCKENG
#  define OPENSSL_NO_PADLOCKENG
# endif
# ifndef OPENSSL_NO_POLY1305
#  define OPENSSL_NO_POLY1305
# endif
# ifndef OPENSSL_NO_POSIX_IO
#  define OPENSSL_NO_POSIX_IO
# endif
# ifndef OPENSSL_NO_PSK
#  define OPENSSL_NO_PSK
# endif
# ifndef OPENSSL_NO_RC2
#  define OPENSSL_NO_RC2
# endif
# ifndef OPENSSL_NO_RC4
#  define OPENSSL_NO_RC4
# endif
# ifndef OPENSSL_NO_RC5
#  define OPENSSL_NO_RC5
# endif
# ifndef OPENSSL_NO_RFC3779
#  define OPENSSL_NO_RFC3779
# endif
# ifndef OPENSSL_NO_RMD160
#  define OPENSSL_NO_RMD160
# endif
# ifndef OPENSSL_NO_SCRYPT
#  define OPENSSL_NO_SCRYPT
# endif
# ifndef OPENSSL_NO_SCTP
#  define OPENSSL_NO_SCTP
# endif
# ifndef OPENSSL_NO_SEED
#  define OPENSSL_NO_SEED
# endif
# ifndef OPENSSL_NO_SIPHASH
#  define OPENSSL_NO_SIPHASH
# endif
# ifndef OPENSSL_NO_SIV
#  define OPENSSL_NO_SIV
# endif
# ifndef OPENSSL_NO_SM2
#  define OPENSSL_NO_SM2
# endif
# ifndef OPENSSL_NO_SM4
#  define OPENSSL_NO_SM4
# endif
# ifndef OPENSSL_NO_SOCK
#  define OPENSSL_NO_SOCK
# endif
# ifndef OPENSSL_NO_SRP
#  define OPENSSL_NO_SRP
# endif
# ifndef OPENSSL_NO_SRTP
#  define OPENSSL_NO_SRTP
# endif
# ifndef OPENSSL_NO_SSL_TRACE
#  define OPENSSL_NO_SSL_TRACE
# endif
# ifndef OPENSSL_NO_SSL3
#  define OPENSSL_NO_SSL3
# endif
# ifndef OPENSSL_NO_SSL3_METHOD
#  define OPENSSL_NO_SSL3_METHOD
# endif
# ifndef OPENSSL_NO_STDIO
#  define OPENSSL_NO_STDIO
# endif
# ifndef OPENSSL_NO_TESTS
#  define OPENSSL_NO_TESTS
# endif
# ifndef OPENSSL_NO_TLS1_3
#  define OPENSSL_NO_TLS1_3
# endif
# ifndef OPENSSL_NO_TRACE
#  define OPENSSL_NO_TRACE
# endif
# ifndef OPENSSL_NO_TS
#  define OPENSSL_NO_TS
# endif
# ifndef OPENSSL_NO_UBSAN
#  define OPENSSL_NO_UBSAN
# endif
# ifndef OPENSSL_NO_UI_CONSOLE
#  define OPENSSL_NO_UI_CONSOLE
# endif
# ifndef OPENSSL_NO_UNIT_TEST
#  define OPENSSL_NO_UNIT_TEST
# endif
# ifndef OPENSSL_NO_UPLINK
#  define OPENSSL_NO_UPLINK
# endif
# ifndef OPENSSL_NO_WEAK_SSL_CIPHERS
#  define OPENSSL_NO_WEAK_SSL_CIPHERS
# endif
# ifndef OPENSSL_NO_WHIRLPOOL
#  define OPENSSL_NO_WHIRLPOOL
# endif
# ifndef OPENSSL_NO_DYNAMIC_ENGINE
#  define OPENSSL_NO_DYNAMIC_ENGINE
# endif


/* Generate 80386 code? */
# undef I386_ONLY

/*
 * The following are cipher-specific, but are part of the public API.
 */
# if !defined(OPENSSL_SYS_UEFI)
#  undef BN_LLONG
/* Only one for the following should be defined */
#  undef SIXTY_FOUR_BIT_LONG
#  undef SIXTY_FOUR_BIT
#  define THIRTY_TWO_BIT
# endif

# define RC4_INT unsigned int

# ifdef  __cplusplus
}
# endif

#endif                          /* OPENSSL_CONFIGURATION_H */
