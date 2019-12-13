/****************************************************************************/
/*  Copyright 2019 Novatek. All rights reserved.                            */
/*  Use of this source code is governed by a BSD-style license that can be  */
/*  found in the LICENSE file.                                              */
/****************************************************************************/

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#define DEBUG_LOG 0
#include "NVTAuxCtrl.h"
#include "NVTSettingProc.h"
#include "TconOperation.h"

/******************************************************************/
/*      Usage                                                     */
/*   -v: Show tool version                                        */
/*   model_file: update code                                      */
/*   -r model_file: read code and save to readBackRom.bin         */
/*   -p:rom_path: use user-defined rom path to replace golen rom  */
/*                path in model file                              */
/******************************************************************/

#define OPERATION_DONE 0
#define ERROR_PARA 1
#define LOAD_MODEL_INIT_FAIL 2
#define UPDATE_FAIL 3
#define READ_CODE_FAIL 4

static const char* tool_version = "V0.1.0.10";

// this is limited by i2c device nmsgs and FW limit
static const int oneWriteSize[] = {0,   32,  64,  64,  128, 128, 128, 256, 256,
                                   256, 256, 256, 256, 256, 256, 256, 256};

int main(int argc, char* argv[]) {
  struct timeval t1, t2;
  int status = -1;
  char paraRomPath[ROM_PATH_LEN_LIMIT];

  if (argc == 2) {
    if (strcmp(argv[1], "-v") == 0) {
      // check version
      printf("%s\n", tool_version);
      return OPERATION_DONE;
    } else {
      // update code
      status = LoadIni(argv[1], false, paraRomPath);
      if (status != INIT_STATUS_OK) {
        warnx("load model file fail");
        return LOAD_MODEL_INIT_FAIL;
      }
      gettimeofday(&t1, NULL);
      status = UpdateCode_WrAllChkAll_71870();
      gettimeofday(&t2, NULL);

      if (status == TCON_ACC_STATUS_OK)
        printf("time cost is %lu seconds\n", (t2.tv_sec - t1.tv_sec));
      else
        return UPDATE_FAIL;
    }
  } else if (argc == 3) {
    if (strcmp(argv[1], "-r") == 0) {
      // read code
      status = LoadIni(argv[2], false, paraRomPath);
      if (status != INIT_STATUS_OK) {
        warnx("load model file fail");
        return LOAD_MODEL_INIT_FAIL;
      }
      gettimeofday(&t1, NULL);
      status = ReadAndSaveRomcode();
      gettimeofday(&t2, NULL);
      if (status == TCON_ACC_STATUS_OK)
        printf("time cost is %lu seconds\n", (t2.tv_sec - t1.tv_sec));
      else
        return READ_CODE_FAIL;
    } else if (strncmp(argv[1], "-p:", 2) == 0) {
      // update code with rom path defined in parameter
      strncpy(paraRomPath, argv[1], ROM_PATH_LEN_LIMIT);
      for (int i = 0; i < ROM_PATH_LEN_LIMIT - 3; i++)
        paraRomPath[i] = paraRomPath[i + 3];
      printf("customer rom: %s\n", paraRomPath);
      status = LoadIni(argv[2], true, paraRomPath);

      if (status != INIT_STATUS_OK) {
        warnx("load model file fail");
        return LOAD_MODEL_INIT_FAIL;
      }
      gettimeofday(&t1, NULL);
      status = UpdateCode_WrAllChkAll_71870();
      gettimeofday(&t2, NULL);

      if (status == TCON_ACC_STATUS_OK)
        printf("time cost is %lu seconds\n", (t2.tv_sec - t1.tv_sec));
      else
        return UPDATE_FAIL;
    }
  } else {
    printf("Usage:TCON_DP_AUX_UPDATE_TOOL model_file\n");
    return ERROR_PARA;
  }

  return OPERATION_DONE;
}

TCON_OPERATION_STATUS TCON_Init() {
  unsigned char buf[16];
  int status;

  buf[0] = 0xC0;
  status = NVTAUX_NativeAuxAccess(NATIVE_AUX_WRITE, 0x00102, 1, buf);
  if (status != AUX_STATUS_OK) {
    warnx("Init TCON Fail(1.%d)", status);
    return TCON_ACC_STATUS_FAIL;
  }
  return TCON_ACC_STATUS_OK;
}

TCON_OPERATION_STATUS UpdateCode_WrAllChkAll_71870() {
  int status;
  unsigned int fw_i2c_addr, sector;
  unsigned int one_wr_size = oneWriteSize[gConfig.m_AuxRequestByte];
  UINT slvAddr = gConfig.m_EEPROM_ADDR;
  UINT eraseWait = gConfig.m_FlashSectorEraseTime * 1000;
  UINT progWait = gConfig.m_FlashPageProgTime * 1000;
  UINT buf_size = gConfig.m_BufferSize;

  BYTE wBuf[512], rBuf[256];

#if DEBUG_LOG
  printf("one_wr_size is %d\n", one_wr_size);
#endif  // DEBUG_LOG
  //*******************************************
  // 1. Initialize
  //*******************************************
  status = NVTAUX_Open();
  if (status != AUX_STATUS_OK) {
    warnx("Open DP fail");
    NVTAUX_Close();
    return TCON_ACC_STATUS_FAIL;
  }
  TCON_Init();
  if (status != TCON_ACC_STATUS_OK) {
    warnx("Init TCON fail");
    NVTAUX_Close();
    return TCON_ACC_STATUS_FAIL;
  }

  // disable flash write protect
  Flash_FW_Protect_Key();
  Flash_Write_Status_Reg_Key(0x00);

  //*******************************************
  // 2. Update code
  //*******************************************
  printf("Writing......\n");
  ProgressPrintf(0, buf_size);
  for (unsigned int i = 0; i < buf_size; i += one_wr_size) {
    memset(wBuf, 0x00, sizeof(wBuf));
    memset(rBuf, 0x00, sizeof(rBuf));

    sector = (i / 0x1000) * 0x10;
    fw_i2c_addr = (i % 0x1000) + 0xD000;

#if DEBUG_LOG
    printf("\nwrite flash addr %06X\n", i);
#endif  // DEBUG_LOG
    ProgressPrintf(i, buf_size);

    if (fw_i2c_addr == 0xD000) {
      // sector erase
      Flash_FW_Protect_Key();
      wBuf[0] = 0xFF;
      wBuf[1] = 0x04;
      wBuf[2] = i / 0x10000;
      wBuf[3] = sector;
      NVTAUX_I2COverAuxWrite(slvAddr, 4, wBuf);
      usleep(eraseWait);
    }

    // change fw read flash address bias
    wBuf[0] = 0xFF;
    wBuf[1] = 0x03;
    wBuf[2] = i / 0x10000;
    wBuf[3] = sector;
    status = NVTAUX_I2COverAuxWrite(slvAddr, 4, wBuf);

    // write data to flash
    Flash_FW_Protect_Key();
    wBuf[0] = fw_i2c_addr >> 8;
    wBuf[1] = fw_i2c_addr;
    for (unsigned int j = 0; j < one_wr_size; j++)
      wBuf[j + 2] = m_GoldenBuf[i + j];
    status = NVTAUX_I2COverAuxWrite(slvAddr, one_wr_size + 2, wBuf);
    if (status != AUX_STATUS_OK) {
      warnx("\nWrite Fail");
      NVTAUX_Close();
      return TCON_ACC_STATUS_FAIL;
    }
    usleep(progWait);

    // verify
    wBuf[0] = fw_i2c_addr >> 8;
    wBuf[1] = fw_i2c_addr;
    status = NVTAUX_I2COverAuxBurstRead(slvAddr, 2, wBuf, one_wr_size, rBuf);
    if (status != AUX_STATUS_OK) {
      warnx("\nWrite fail");
      NVTAUX_Close();
      return TCON_ACC_STATUS_FAIL;
    }

    for (unsigned int j = 0; j < one_wr_size; j++) {
      if (m_GoldenBuf[i + j] != rBuf[j]) {
        warnx("\nWrite fail at address 0x%06X, write:%02X <=> read:%02X", i,
              m_GoldenBuf[i], rBuf[j]);
        NVTAUX_Close();
        return TCON_ACC_STATUS_FAIL;
      }
    }
  }
  ProgressPrintf(buf_size, buf_size);

  //*******************************************
  // 3. Verify
  //*******************************************
  printf("Verifying......\n");
  ProgressPrintf(0, buf_size);
  for (unsigned int i = 0; i < buf_size; i += one_wr_size) {
    sector = (i / 0x1000) * 0x10;
    fw_i2c_addr = (i % 0x1000) + 0xD000;

#if DEBUG_LOG
    printf("\nverify flash addr %06X\n", i);
#endif  // DEBUG_LOG
    ProgressPrintf(i, buf_size);

    // change fw read flash address bias
    wBuf[0] = 0xFF;
    wBuf[1] = 0x03;
    wBuf[2] = i / 0x10000;
    wBuf[3] = sector;
    NVTAUX_I2COverAuxWrite(slvAddr, 4, wBuf);

    wBuf[0] = fw_i2c_addr >> 8;
    wBuf[1] = fw_i2c_addr;
    status = NVTAUX_I2COverAuxBurstRead(slvAddr, 2, wBuf, one_wr_size, rBuf);
    if (status != AUX_STATUS_OK) {
      warnx("\nVerify fail on reading");
      return TCON_ACC_STATUS_FAIL;
    }

    for (unsigned int j = 0; j < one_wr_size; j++) {
      if (m_GoldenBuf[i + j] != rBuf[j]) {
        warnx("\nVerify fail at address 0x%06X, write:%02X <=> read:%02X", i,
              m_GoldenBuf[i], rBuf[j]);
        NVTAUX_Close();
        return TCON_ACC_STATUS_FAIL;
      }
    }
  }
  ProgressPrintf(buf_size, buf_size);

  //*******************************************
  // 4. Set flash status register to do protect
  //*******************************************
  if (gConfig.m_FlashWriteStatusReg != 0x00) {
    Flash_FW_Protect_Key();
    Flash_Write_Status_Reg_Key(gConfig.m_FlashWriteStatusReg);
  }

  NVTAUX_Close();
  printf("\nUpdate Code Success\n");
  return TCON_ACC_STATUS_OK;
}

TCON_OPERATION_STATUS ReadAndSaveRomcode() {
  int status;
  unsigned int fw_i2c_addr, sector;
  unsigned int slvAddr = gConfig.m_EEPROM_ADDR;
  unsigned int one_wr_size = oneWriteSize[gConfig.m_AuxRequestByte];
  unsigned int buf_size = gConfig.m_BufferSize;
  BYTE wBuf[256], rBuf[256];

  status = NVTAUX_Open();
  if (status != AUX_STATUS_OK) {
    warnx("Open DP fail");
    NVTAUX_Close();
    return TCON_ACC_STATUS_FAIL;
  }
  TCON_Init();
  if (status != TCON_ACC_STATUS_OK) {
    warnx("Init TCON fail");
    NVTAUX_Close();
    return TCON_ACC_STATUS_FAIL;
  }

  FILE* fp = fopen("readBackRom.bin", "wb");
  if (fp == NULL) {
    warn("can't open readBackRom.bin");
    return TCON_ACC_STATUS_FAIL;
  }

  printf("Reading......\n");
  ProgressPrintf(0, buf_size);
  for (unsigned int i = 0; i < buf_size; i += one_wr_size) {
    sector = (i / 0x1000) * 0x10;
    fw_i2c_addr = (i % 0x1000) + 0xD000;

    ProgressPrintf(i, buf_size);

    // change fw read flash address bias
    wBuf[0] = 0xFF;
    wBuf[1] = 0x03;
    wBuf[2] = i / 0x10000;
    wBuf[3] = sector;
    NVTAUX_I2COverAuxWrite(slvAddr, 4, wBuf);

    wBuf[0] = fw_i2c_addr >> 8;
    wBuf[1] = fw_i2c_addr;

    status = NVTAUX_I2COverAuxBurstRead(slvAddr, 2, wBuf, one_wr_size, rBuf);
    if (status != AUX_STATUS_OK) {
      warnx("Read Rom code fail");
      fclose(fp);
      NVTAUX_Close();
      return TCON_ACC_STATUS_FAIL;
    }
    if (fwrite(rBuf, one_wr_size, 1, fp) < 1) {
      warnx("Writing Rom code to file failed");
      fclose(fp);
      NVTAUX_Close();
      return TCON_ACC_STATUS_FAIL;
    }
  }

  ProgressPrintf(buf_size, buf_size);
  printf("\nRead Rom code and save to readBackRom.bin Success\n");
  fclose(fp);
  NVTAUX_Close();

  return TCON_ACC_STATUS_OK;
}

void Flash_FW_Protect_Key(void) {
  unsigned char fw_prt_key[5] = {0xFF, 0x3C, 0xC3, 0xAA, 0x55};
  unsigned int slvAddr = gConfig.m_EEPROM_ADDR;

  NVTAUX_I2COverAuxWrite(slvAddr, 5, fw_prt_key);
}

void Flash_Write_Status_Reg_Key(BYTE ucStatus) {
  unsigned char fw_prt_key[3] = {0xFF, 0x01, 0x00};
  unsigned int slvAddr = gConfig.m_EEPROM_ADDR;

  fw_prt_key[2] = ucStatus;

  NVTAUX_I2COverAuxWrite(slvAddr, 3, fw_prt_key);

  if (ucStatus) {
    fw_prt_key[0] = 0xFF;
    NVTAUX_NativeAuxAccess(NATIVE_AUX_WRITE, 0x00480, 1, fw_prt_key);
  }
}

void ProgressPrintf(UINT addr, UINT totalSize) {
  double percent = addr;
  percent = addr / ((double)totalSize);
  int progress = percent * 40;

  printf("\r[");
  for (int i = 0; i < progress; i++) printf("=");
  printf(">");
  for (int i = progress; i < 40; i++) printf(" ");
  int tmp = percent * 100;
  printf("]%d%%,%06X", tmp, addr);

  fflush(stdout);
  if (percent >= 1) printf("\n");
}
