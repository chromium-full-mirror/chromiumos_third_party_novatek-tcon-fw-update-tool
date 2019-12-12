/****************************************************************************/
/*  Copyright 2019 Novatek. All rights reserved.                            */
/*  Use of this source code is governed by a BSD-style license that can be  */
/*  found in the LICENSE file.                                              */
/****************************************************************************/

#ifndef NVTSETTINGPROC_H_INCLUDED
#define NVTSETTINGPROC_H_INCLUDED

#define UCHAR unsigned char
#define UINT unsigned int

#define AUX_REQ_BYTE_LIMIT 16
#define DEFAULT_FLASH_WRITE_STATUS_REG 0x00
#define CONFIG_ITEMS 9
#define ROM_PATH_LEN_LIMIT 1024

extern UCHAR* m_GoldenBuf;

extern struct GraphicConfig {
  // must maintain the struct item order
  UCHAR m_TCON_ADDR;
  UCHAR m_EEPROM_ADDR;
  UINT m_BufferSize;
  UINT m_AuxRequestByte;
  UINT m_FlashSectorEraseTime;
  UINT m_FlashPageProgTime;
  UINT m_FlashWriteStatusReg;
  char* m_GoldenFilePath;
  UINT m_DpInterface;
} gConfig;

typedef enum _init_status {
  INIT_STATUS_OK = 0,
  INIT_STATUS_FAIL = -1,
  INIT_STATUS_ERR_PARAM = -2,
} INIT_STATUS;

INIT_STATUS LoadIni(char* path, bool romPathEn, char* romPath);
void InitConfig();
void SetConfig(int item, char* inStr);
void RemoveUnwantChar(char* str);
#endif  // NVTSETTINGPROC_H_INCLUDED
