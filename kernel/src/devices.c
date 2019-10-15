/**
 * Copyright (c) 2019 Himanshu Goel
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "stdint.h"
#include "stddef.h"

void igfx_matchdevice(void);

void devices_load(void){
    igfx_matchdevice();
}