/****************************************************************************/
/*  Copyright 2019 Novatek. All rights reserved.                            */
/*  Use of this source code is governed by a BSD-style license that can be  */
/*  found in the LICENSE file.                                              */
/****************************************************************************/

#include "NVTAuxCtrl.h"

#include <err.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "NVTSettingProc.h"

int fpNativeAux, fpI2CoverAux;
struct NvtAuxConfig auxConfig;

NVTAUX_STATUS NVTAUX_Open() {
  // search target DP interface

  FILE *fp;
  char auxPath[1024], i2cPath[1024], path[256], pollingPath[512];
  int openStat;
  char *chrret;

  if (gConfig.m_DpInterface == 2) {
    // assume there is only one eDP interface and fine eDP path
    fp = popen(
        "find /sys/class/drm/*eDP*/drm_dp_aux* -maxdepth 0 | sed 's/.*\\///'",
        "r");
    if (fp == NULL) {
      warnx("Fail to get eDP interface - DpInt==2 drm_dp_aux* popen() failed");
      return AUX_STATUS_FAIL;
    }
    chrret = fgets(path, sizeof(path) - 1, fp);
    pclose(fp);
    if (!chrret) {
      warnx("Fail to get eDP interface - DpInt==2 drm_dp_aux* fgets() failed");
      return AUX_STATUS_FAIL;
    }

    sprintf(auxPath, "/dev/%s", path);
    strtok(auxPath, "\n");
#if DEBUG_LOG
    printf("eDP aux device path: %s\n", auxPath);
#endif  // DEBUG_LOG
    fp = popen("find /sys/class/drm/*eDP*/i2c-* -maxdepth 0 | sed 's/.*\\///'",
               "r");
    if (fp == NULL) {
      warnx("Fail to get eDP interface - DpInt==2 i2c-* popen() failed");
      return AUX_STATUS_FAIL;
    }
    chrret = fgets(path, sizeof(path) - 1, fp);
    pclose(fp);
    if (!chrret) {
      warnx("Fail to get eDP interface - DpInt==2 i2c-* fgets() failed");
      return AUX_STATUS_FAIL;
    }

    sprintf(i2cPath, "/dev/%s", path);
    strtok(i2cPath, "\n");
#if DEBUG_LOG
    printf("eDP i2c device path: %s\n", i2cPath);
#endif  // DEBUG_LOG

  } else if (gConfig.m_DpInterface == 3) {
    // assume there is only 1 DP device connected and connect to card0 1st port
    int portNo = 0;
    for (portNo = 0; portNo < 8; portNo++) {
      sprintf(path, "/sys/class/drm/card0-DP-%d", portNo);
      openStat = open(path, O_RDONLY);
      if (openStat < 0) continue;
      if (close(openStat) < 0) {
        warn("failed when attempting to close %s", path);
        return AUX_STATUS_FAIL;
      }

      sprintf(pollingPath, "find %s/drm_dp_aux* -maxdepth 0 | sed 's/.*\\///'",
              path);
      fp = popen(pollingPath, "r");
      if (fp == NULL) {
        warnx("Fail to get eDP interface - DpInt==3 portNo==%d drm_dp_aux* "
              "popen() failed", portNo);
        return AUX_STATUS_FAIL;
      }
      chrret = fgets(pollingPath, sizeof(pollingPath) - 1, fp);
      pclose(fp);
      if (!chrret) {
        warnx("Fail to get eDP interface - DpInt==3 portNo==%d drm_dp_aux* "
              "fgets() failed", portNo);
        return AUX_STATUS_FAIL;
      }
      sprintf(auxPath, "/dev/%s", pollingPath);
      strtok(auxPath, "\n");
#if DEBUG_LOG
      printf("eDP aux device path: %s\n", auxPath);
#endif  // DEBUG_LOG
      sprintf(pollingPath, "find %s/i2c* -maxdepth 0 | sed 's/.*\\///'", path);
      fp = popen(pollingPath, "r");
      if (fp == NULL) {
        warnx("Fail to get eDP interface - DpInt==3 portNo==%d i2c-* popen() "
              "failed", portNo);
        return AUX_STATUS_FAIL;
      }
      chrret = fgets(pollingPath, sizeof(pollingPath) - 1, fp);
      pclose(fp);
      if (!chrret) {
        warnx("Fail to get eDP interface - DpInt==3 portNo==%d i2c-* fgets() "
              "failed", portNo);
        return AUX_STATUS_FAIL;
      }
      sprintf(i2cPath, "/dev/%s", pollingPath);
      strtok(i2cPath, "\n");
#if DEBUG_LOG
      printf("eDP i2c device path: %s\n", i2cPath);
#endif  // DEBUG_LOG
      break;
    }
  } else {
    warnx("unknown DP Interface");
    return AUX_STATUS_FAIL;
  }

  // open Native aux device
  if ((fpNativeAux = open(auxPath, O_RDWR)) < 0) {
    warn("open fpNativeAux device failed");
    return AUX_STATUS_FAIL;
  }

  // open I2CoverAux device
  if ((fpI2CoverAux = open(i2cPath, O_RDWR)) < 0) {
    warn("open I2CoverAux device failed");
    return AUX_STATUS_FAIL;
  }

  return AUX_STATUS_OK;
}

NVTAUX_STATUS NVTAUX_Close() {
  NVTAUX_STATUS retval = AUX_STATUS_OK;
  if (close(fpNativeAux) < 0) {
    warn("failed when attempting to close fpNativeAux");
    retval = AUX_STATUS_FAIL;
  }
  if (close(fpI2CoverAux) < 0) {
    warn("failed when attempting to close fpI2CoverAux");
    retval = AUX_STATUS_FAIL;
  }
  return retval;
}

NVTAUX_STATUS NVTAUX_NativeAuxAccess(unsigned int op, unsigned int dpcdAddr,
                                     unsigned int len, unsigned char *buf) {
  int ret;

  if (op == NATIVE_AUX_WRITE) {
    ret = lseek(fpNativeAux, dpcdAddr, SEEK_SET);
    if (ret < 0) return AUX_STATUS_FAIL;

    ret = write(fpNativeAux, buf, len);
    if (ret < 0) return AUX_STATUS_FAIL;
  } else if (op == NATIVE_AUX_READ) {
    ret = lseek(fpNativeAux, dpcdAddr, SEEK_SET);
    if (ret < 0) return AUX_STATUS_FAIL;

    ret = read(fpNativeAux, buf, len);
    if (ret < 0) return AUX_STATUS_FAIL;
  } else {
    return AUX_STATUS_ERR_PARAM;
  }

  return AUX_STATUS_OK;
}

NVTAUX_STATUS NVTAUX_I2COverAuxWrite(unsigned int devAddr, unsigned int wrLen,
                                     unsigned char *wrBuf) {
  int ret;
  int auxSize = auxConfig.m_AuxRequestByte, lastPacketSize;
  struct i2c_rdwr_ioctl_data i2c_data;

  // i2c_rdwr_ioctl_data defined in i2c-dev.h
  // i2c_msg defined in i2c.h
#if 1
  // standard I2C interface
  i2c_data.nmsgs = ((wrLen - 1) / auxSize) + 1;
  lastPacketSize = wrLen % auxSize;
  i2c_data.msgs =
      (struct i2c_msg *)malloc(i2c_data.nmsgs * sizeof(struct i2c_msg));

#if DEBUG_LOG
  printf("msgs:%d, lastPacketSize:%d\n", i2c_data.nmsgs, lastPacketSize);
#endif  // DEBUG_LOG
  for (unsigned int i = 0; i < i2c_data.nmsgs - 1; i++) {
    (i2c_data.msgs[i]).len = auxSize;
    (i2c_data.msgs[i]).addr = ((devAddr >> 1) & 0x7F);
    (i2c_data.msgs[i]).flags = 0;  // write
    (i2c_data.msgs[i]).buf = (unsigned char *)malloc(auxSize);
    for (int j = 0; j < auxSize; j++) {
      (i2c_data.msgs[i]).buf[j] = wrBuf[i * auxSize + j];
#if DEBUG_LOG
      printf("%02X ", wrBuf[i * auxSize + j]);
#endif  // DEBUG_LOG
    }
  }

  if (lastPacketSize != 0) {
    (i2c_data.msgs[i2c_data.nmsgs - 1]).len = lastPacketSize;
    (i2c_data.msgs[i2c_data.nmsgs - 1]).addr = ((devAddr >> 1) & 0x7F);
    (i2c_data.msgs[i2c_data.nmsgs - 1]).flags = 0;  // write
    (i2c_data.msgs[i2c_data.nmsgs - 1]).buf =
        (unsigned char *)malloc(lastPacketSize);
    for (int j = 0; j < lastPacketSize; j++) {
      (i2c_data.msgs[i2c_data.nmsgs - 1]).buf[j] =
          wrBuf[(i2c_data.nmsgs - 1) * auxSize + j];
#if DEBUG_LOG
      printf("%02X ", wrBuf[(i2c_data.nmsgs - 1) * auxSize + j]);
#endif  // DEBUG_LOG
    }
  } else {
    (i2c_data.msgs[i2c_data.nmsgs - 1]).len = auxSize;
    (i2c_data.msgs[i2c_data.nmsgs - 1]).addr = ((devAddr >> 1) & 0x7F);
    (i2c_data.msgs[i2c_data.nmsgs - 1]).flags = 0;  // write
    (i2c_data.msgs[i2c_data.nmsgs - 1]).buf = (unsigned char *)malloc(auxSize);
    for (int j = 0; j < auxSize; j++) {
      (i2c_data.msgs[i2c_data.nmsgs - 1]).buf[j] =
          wrBuf[(i2c_data.nmsgs - 1) * auxSize + j];
#if DEBUG_LOG
      printf("%02X ", wrBuf[(i2c_data.nmsgs - 1) * auxSize + j]);
#endif  // DEBUG_LOG
    }
  }

#if DEBUG_LOG
  printf("\n");
#endif  // DEBUG_LOG

  ret = ioctl(fpI2CoverAux, I2C_RDWR, (unsigned long)&i2c_data);
  if (ret < 0) {
    warnx("i2c write fail");
    return AUX_STATUS_FAIL;
  }
#else
  // intel driver provided interface
  ret = write(fpI2CoverAux, wrBuf, wrLen);
  if (ret != wrLen) {
    warnx("i2c write fail");
    return AUX_STATUS_FAIL;
  }
#endif  // 0

  return AUX_STATUS_OK;
}

NVTAUX_STATUS NVTAUX_I2COverAuxRead(unsigned int devAddr, unsigned int rdLen,
                                    unsigned char *rdBuf) {
  int ret;
  struct i2c_rdwr_ioctl_data i2c_data;

  i2c_data.nmsgs = 1;
  i2c_data.msgs =
      (struct i2c_msg *)malloc(i2c_data.nmsgs * sizeof(struct i2c_msg));

  (i2c_data.msgs[0]).len = rdLen;
  (i2c_data.msgs[0]).addr = ((devAddr >> 1) & 0x7F);
  (i2c_data.msgs[0]).flags = I2C_M_RD;
  (i2c_data.msgs[0]).buf = (unsigned char *)malloc(rdLen);
  ret = ioctl(fpI2CoverAux, I2C_RDWR, (unsigned long)&i2c_data);

  if (ret < 0) {
    warnx("i2c read fail");
    return AUX_STATUS_FAIL;
  }

  for (unsigned int i = 0; i < rdLen; i++) rdBuf[i] = (i2c_data.msgs[0]).buf[i];

  return AUX_STATUS_OK;
}

NVTAUX_STATUS NVTAUX_I2COverAuxBurstRead(unsigned int devAddr,
                                         unsigned int wrLen,
                                         unsigned char *wrBuf,
                                         unsigned int rdLen,
                                         unsigned char *rdBuf) {
  // this function is to compose the repeated start I2C read
  int ret;
  int auxSize = auxConfig.m_AuxRequestByte, lastPacketSize;
  struct i2c_rdwr_ioctl_data i2c_data;

  i2c_data.nmsgs = ((((wrLen - 1) / auxSize) + 1) + 1);
  lastPacketSize = wrLen % auxSize;
  i2c_data.msgs =
      (struct i2c_msg *)malloc(i2c_data.nmsgs * sizeof(struct i2c_msg));

#if DEBUG_LOG
  printf("msgs:%d, lastPacketSize:%d\n", i2c_data.nmsgs, lastPacketSize);
#endif  // DEBUG_LOG
  // compose write message
  for (unsigned int i = 0; i < i2c_data.nmsgs - 2; i++) {
    (i2c_data.msgs[i]).len = auxSize;
    (i2c_data.msgs[i]).addr = ((devAddr >> 1) & 0x7F);
    (i2c_data.msgs[i]).flags = 0;  // write
    (i2c_data.msgs[i]).buf = (unsigned char *)malloc(auxSize);
    for (int j = 0; j < auxSize; j++)
      (i2c_data.msgs[i]).buf[j] = wrBuf[i * auxSize + j];
  }

  if (lastPacketSize != 0) {
    (i2c_data.msgs[i2c_data.nmsgs - 2]).len = lastPacketSize;
    (i2c_data.msgs[i2c_data.nmsgs - 2]).addr = ((devAddr >> 1) & 0x7F);
    (i2c_data.msgs[i2c_data.nmsgs - 2]).flags = 0;  // write
    (i2c_data.msgs[i2c_data.nmsgs - 2]).buf =
        (unsigned char *)malloc(lastPacketSize);
    for (int j = 0; j < lastPacketSize; j++)
      (i2c_data.msgs[i2c_data.nmsgs - 2]).buf[j] =
          wrBuf[(i2c_data.nmsgs - 2) * auxSize + j];
  } else {
    (i2c_data.msgs[i2c_data.nmsgs - 2]).len = auxSize;
    (i2c_data.msgs[i2c_data.nmsgs - 2]).addr = ((devAddr >> 1) & 0x7F);
    (i2c_data.msgs[i2c_data.nmsgs - 2]).flags = 0;  // write
    (i2c_data.msgs[i2c_data.nmsgs - 2]).buf = (unsigned char *)malloc(auxSize);
    for (int j = 0; j < auxSize; j++)
      (i2c_data.msgs[i2c_data.nmsgs - 2]).buf[j] =
          wrBuf[(i2c_data.nmsgs - 2) * auxSize + j];
  }

  // compose read message
  (i2c_data.msgs[i2c_data.nmsgs - 1]).len = rdLen;
  (i2c_data.msgs[i2c_data.nmsgs - 1]).addr = ((devAddr >> 1) & 0x7F);
  (i2c_data.msgs[i2c_data.nmsgs - 1]).flags = I2C_M_RD;
  (i2c_data.msgs[i2c_data.nmsgs - 1]).buf = (unsigned char *)malloc(rdLen);

  ret = ioctl(fpI2CoverAux, I2C_RDWR, (unsigned long)&i2c_data);

  if (ret < 0) {
    warnx("i2c burst read fail");
    return AUX_STATUS_FAIL;
  }

  for (unsigned int i = 0; i < rdLen; i++)
    rdBuf[i] = (i2c_data.msgs[i2c_data.nmsgs - 1]).buf[i];

  return AUX_STATUS_OK;
}
