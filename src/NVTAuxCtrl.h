/****************************************************************************/
/*  Copyright 2019 Novatek. All rights reserved.                            */
/*  Use of this source code is governed by a BSD-style license that can be  */
/*  found in the LICENSE file.                                              */
/****************************************************************************/

#ifndef AUXCTRLLIB_H_INCLUDED
#define AUXCTRLLIB_H_INCLUDED
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_AUX_REQUEST_BYTE 16

extern struct NvtAuxConfig {
  uint m_AuxRequestByte;  // must consider with transistion length because
                          // I2C_RDWR_IOCTL_MAX_MSGS = 42
} auxConfig;

typedef enum _native_aux_operation {
  NATIVE_AUX_WRITE = 0,
  NATIVE_AUX_READ = 1,
} NATIVE_AUX_OPERATION;

typedef enum _nvtaux_status {
  AUX_STATUS_OK = 0,
  AUX_STATUS_FAIL = -1,
  AUX_STATUS_ERR_PARAM = -2,
} NVTAUX_STATUS;

NVTAUX_STATUS NVTAUX_Open();
NVTAUX_STATUS NVTAUX_Close();
NVTAUX_STATUS NVTAUX_NativeAuxAccess(unsigned int op, unsigned int dpcdAddr,
                                     unsigned int len, unsigned char* buf);
NVTAUX_STATUS NVTAUX_I2COverAuxWrite(unsigned int devAddr, unsigned int wrLen,
                                     unsigned char* wrBuf);
NVTAUX_STATUS NVTAUX_I2COverAuxRead(unsigned int devAddr, unsigned int rdLen,
                                    unsigned char* rdBuf);
NVTAUX_STATUS NVTAUX_I2COverAuxBurstRead(unsigned int devAddr,
                                         unsigned int wrLen,
                                         unsigned char* wrBuf,
                                         unsigned int rdLen,
                                         unsigned char* rdBuf);
#endif  // AUXCTRLLIB_H_INCLUDED
