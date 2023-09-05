/*
 * Copyright 2017-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OSSL_INTERNAL_NELEM_H
# define OSSL_INTERNAL_NELEM_H
# ifndef RT_WITHOUT_PRAGMA_ONCE                                                                         /* VBOX */
# pragma once
# endif                                                                                                 /* VBOX */

# define OSSL_NELEM(x)    (sizeof(x)/sizeof((x)[0]))
#endif
