# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

from __future__ import print_function
import sys

sys.path.append( "../glapi_parser" )
import apiutil


apiutil.CopyrightC()

print("""/* DO NOT EDIT!  THIS CODE IS AUTOGENERATED BY unpack_extend.py */

#ifndef UNPACK_EXTEND_H
#define UNPACK_EXTEND_H 1

#include "cr_unpack.h"

#ifdef __cplusplus
extern "C" {
#endif

""")


#
# Print extern declarations for all special unpacker functions
#
for func_name in apiutil.AllSpecials( "unpacker" ):
	if "extpack" in apiutil.ChromiumProps(func_name):
		print('extern void crUnpackExtend%s(PCrUnpackerState pState);' % func_name)
	else:
		print('extern void crUnpack%s(PCrUnpackerState pState);' % func_name)

print("""
#ifdef __cplusplus
}
#endif

#endif
""")

