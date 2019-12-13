/****************************************************************************/
/*  Copyright 2019 Novatek. All rights reserved.                            */
/*  Use of this source code is governed by a BSD-style license that can be  */
/*  found in the LICENSE file.                                              */
/****************************************************************************/

#include "NVTSettingProc.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NVTAuxCtrl.h"

UCHAR* m_GoldenBuf;
struct GraphicConfig gConfig;

static const char* configItem[CONFIG_ITEMS] = {
    "TCON_ADDR",
    "EEPROM_ADDR",
    "EEPROM_SIZE",
    "AUX_REQUEST_BYTE",
    "FLASH_SECTOR_ERASE_TIME",
    "FLASH_PAGE_PROG_TIME",
    "FLASH_WRITE_STATUS_REG",
    "PATH",
    "DEVICE_TYPE",
};

INIT_STATUS LoadIni(char* path, bool RomPathEn, char* romPath) {
  printf("used mode:%s\n", path);

  // initialize config
  InitConfig();

  // load setting
  char* line = NULL;
  FILE* fp = fopen(path, "r");
  size_t lineLen;
  size_t size_ret;

  if (fp == NULL) {
    warn("can't open model file:%s", path);
    return INIT_STATUS_FAIL;
  }
  while ((getline(&line, &lineLen, fp)) > 0) {
    if ((line[0] >= 65) && (line[0] <= 90)) {
      char* item;
      char* value;
      item = strtok(line, "=");
      value = strtok(NULL, "\n");
      RemoveUnwantChar(item);
      RemoveUnwantChar(value);
      // printf("%s=%s\n",item,value);
      for (int i = 0; i < CONFIG_ITEMS; i++) {
        if (strcmp(item, configItem[i]) == 0) {
          // printf("%s=%s\n",item,value);
          SetConfig(i, value);
          break;
        }
      }
    }
  }
  fclose(fp);

  // change rom path to parameter defined
  if (RomPathEn) {
    strncpy(gConfig.m_GoldenFilePath, romPath, ROM_PATH_LEN_LIMIT);
    printf("changed rom path:%s\n", gConfig.m_GoldenFilePath);
  }

  // check setting
  fp = fopen(gConfig.m_GoldenFilePath, "rb");
  if (fp == NULL) {
    warn("%s not exist!!", gConfig.m_GoldenFilePath);
    return INIT_STATUS_FAIL;
  }

  if (fseek(fp, 0, SEEK_END) < 0) {
    warn("failed to seek on %s", path);
    fclose(fp);
    return INIT_STATUS_FAIL;
  }
  long fSize = ftell(fp);
  if (fSize != gConfig.m_BufferSize) {
    warnx("wrong Buffer Size");
    fclose(fp);
    return INIT_STATUS_FAIL;
  }
  if ((gConfig.m_DpInterface < 2) || (gConfig.m_DpInterface > 3)) {
    warnx("unknown DP Interface");
    fclose(fp);
    return INIT_STATUS_FAIL;
  }

  // initialize
  if (m_GoldenBuf != NULL) delete m_GoldenBuf;
  m_GoldenBuf = new UCHAR[fSize];
  if (fseek(fp, 0, SEEK_SET) < 0) {
    warn("failed to seek on %s", path);
    fclose(fp);
    return INIT_STATUS_FAIL;
  }
  size_ret = fread(m_GoldenBuf, fSize, 1, fp);
  fclose(fp);
  if (size_ret < 1) {
    warnx("failed to read from m_GoldenBuf");
    return INIT_STATUS_FAIL;
  }
  auxConfig.m_AuxRequestByte = gConfig.m_AuxRequestByte;

  printf("NVT Setting Initialize Success\n");
  return INIT_STATUS_OK;
}

void InitConfig() {
  m_GoldenBuf = NULL;

  gConfig.m_TCON_ADDR = 0xC0;
  gConfig.m_EEPROM_ADDR = 0xC4;
  gConfig.m_AuxRequestByte = AUX_REQ_BYTE_LIMIT;
  gConfig.m_BufferSize = 0;
  gConfig.m_FlashSectorEraseTime = 300;
  gConfig.m_FlashPageProgTime = 5;
  gConfig.m_FlashWriteStatusReg = DEFAULT_FLASH_WRITE_STATUS_REG;
  if (gConfig.m_GoldenFilePath) delete gConfig.m_GoldenFilePath;
  gConfig.m_GoldenFilePath = new char[ROM_PATH_LEN_LIMIT];
  strcpy(gConfig.m_GoldenFilePath, "goldenRom.bin");
  gConfig.m_DpInterface = 2;  // internal DP
}

void SetConfig(int item, char* inStr) {
  // the decode order need to maintain
  int tmp;

  switch (item) {
    case 0:
      tmp = strtol(inStr, NULL, 16);
      gConfig.m_TCON_ADDR = tmp;
      printf("TCON_ADDR:0x%02X\n", gConfig.m_TCON_ADDR);
      break;

    case 1:
      tmp = strtol(inStr, NULL, 16);
      gConfig.m_EEPROM_ADDR = tmp;
      printf("EEPROM_ADDR:0x%02X\n", gConfig.m_EEPROM_ADDR);
      break;

    case 2:
      tmp = strtol(inStr, NULL, 16);
      gConfig.m_BufferSize = tmp;
      printf("EEPROM_size:0x%X\n", gConfig.m_BufferSize);
      break;

    case 3:
      tmp = strtol(inStr, NULL, 10);
      gConfig.m_AuxRequestByte = tmp;
      printf("Aux Packet Size:%d\n", gConfig.m_AuxRequestByte);
      break;

    case 4:
      tmp = strtol(inStr, NULL, 10);
      gConfig.m_FlashSectorEraseTime = tmp;
      printf("Sector Erase Wait(ms):%d\n", gConfig.m_FlashSectorEraseTime);
      break;

    case 5:
      tmp = strtol(inStr, NULL, 10);
      gConfig.m_FlashPageProgTime = tmp;
      printf("Page Program Wait(ms):%d\n", gConfig.m_FlashPageProgTime);
      break;

    case 6:
      tmp = strtol(inStr, NULL, 16);
      gConfig.m_FlashWriteStatusReg = tmp;
      printf("Flash Status Reg:0x%02X\n", gConfig.m_FlashWriteStatusReg);
      break;

    case 7:
      strcpy(gConfig.m_GoldenFilePath, inStr);
      printf("rom path:%s\n", gConfig.m_GoldenFilePath);
      break;

    case 8:
      tmp = strtol(inStr, NULL, 10);
      gConfig.m_DpInterface = tmp;
      if (gConfig.m_DpInterface == 2) {
        printf("DP Interface:internal\n");
      } else if (gConfig.m_DpInterface == 3) {
        printf("DP Interface:external\n");
      } else
        printf("DP interface:unknown\n");
      break;
  }
}

void RemoveUnwantChar(char* str) {
  unsigned int wIdx, rIdx;
  for (wIdx = 0, rIdx = 0; rIdx < strlen(str); rIdx++) {
    if ((str[rIdx] != ' ') && (str[rIdx] > 31)) {
      str[wIdx] = str[rIdx];
      wIdx++;
    }
  }
  str[wIdx] = '\0';
}
