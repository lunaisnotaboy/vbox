/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef HEADER_OBJERR_H
# define HEADER_OBJERR_H

# ifndef HEADER_SYMHACKS_H
#  include <openssl/symhacks.h>
# endif

# ifdef  __cplusplus
extern "C"
# endif
int ERR_load_OBJ_strings(void);

/*
 * OBJ function codes.
 */
# define OBJ_F_OBJ_ADD_OBJECT                             105
# define OBJ_F_OBJ_ADD_SIGID                              107
# define OBJ_F_OBJ_CREATE                                 100
# define OBJ_F_OBJ_DUP                                    101
# define OBJ_F_OBJ_NAME_NEW_INDEX                         106
# define OBJ_F_OBJ_NID2LN                                 102
# define OBJ_F_OBJ_NID2OBJ                                103
# define OBJ_F_OBJ_NID2SN                                 104
# define OBJ_F_OBJ_TXT2OBJ                                108

/*
 * OBJ reason codes.
 */
# define OBJ_R_OID_EXISTS                                 102
# define OBJ_R_UNKNOWN_NID                                101

#endif
