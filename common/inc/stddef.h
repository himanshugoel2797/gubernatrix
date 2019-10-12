// Copyright (c) 2019 Himanshu Goel
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef _STDDEF_H_
#define _STDDEF_H_

#include "types.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

#define offsetof(type, member) ((size_t)(&((type *)0)->member))

#endif