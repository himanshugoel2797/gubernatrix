/**
 * Copyright (c) 2019 Himanshu Goel
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "gtt.h"

//GTT config
//GTT offset = 2M MMADR + 6M rsvd
//Setup entries
//HWS_PGA (Hardware Status Page) needs a page allocated to it

//Disable PPGTT to avoid the logical rendering context
//CCID (Logical Rendering Context) needs space allocated to it, figure out how much space

//Ring Buffers
//Batch Buffers
//Ring Contexts

//TODO: Test MSI interrupts, learn to handle page faults