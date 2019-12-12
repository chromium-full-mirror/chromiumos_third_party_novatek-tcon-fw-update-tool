/****************************************************************************/
/*  Copyright 2019 Novatek. All rights reserved.                            */
/*  Use of this source code is governed by a BSD-style license that can be  */
/*  found in the LICENSE file.                                              */
/****************************************************************************/

#ifndef TCONACCESS_H_INCLUDED
#define TCONACCESS_H_INCLUDED

#define BYTE unsigned char

typedef enum _tcon_operation_status {
  TCON_ACC_STATUS_OK = 0,
  TCON_ACC_STATUS_FAIL = -1,
  TCON_ACC_STATUS_ERR_PARAM = -2,
} TCON_OPERATION_STATUS;

TCON_OPERATION_STATUS TCON_Init();
TCON_OPERATION_STATUS UpdateCode_WrAllChkAll_71870();
TCON_OPERATION_STATUS ReadAndSaveRomcode();
void Flash_FW_Protect_Key(void);
void Flash_Write_Status_Reg_Key(BYTE ucStatus);
void ProgressPrintf(UINT addr, UINT totalSize);
#endif  // TCONACCESS_H_INCLUDED
