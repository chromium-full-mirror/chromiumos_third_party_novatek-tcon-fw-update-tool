#ifndef NVTSETTINGPROC_H_INCLUDED
#define NVTSETTINGPROC_H_INCLUDED

#define UCHAR unsigned char
#define UINT  unsigned int

#define AUX_REQ_BYTE_LIMIT 16
#define DEFAULT_FLASH_WRITE_STATUS_REG 0x00
#define CONFIG_ITEMS 9
#define ROM_PATH_LEN_LIMIT 1024

/**************************************************************************************************/
/*  Copyright (c) 2019, Novatek                                                                   */
/*  All rights reserved.                                                                          */
/*                                                                                                */
/*  Redistribution and use in source and binary forms, with or without                            */
/*  modification, are permitted provided that the following conditions are met:                   */
/*                                                                                                */
/*   * Redistributions of source code must retain the above copyright notice, this                */
/*     list of conditions and the following disclaimer.                                           */
/*   * Redistributions in binary form must reproduce the above copyright notice,                  */
/*     this list of conditions and the following disclaimer in the documentation                  */
/*     and/or other materials provided with the distribution.                                     */
/*   * Neither the name of Novatek nor the names of its contributors may be used                  */
/*     to endorse or promote products derived from this software without specific                 */
/*     prior written permission.                                                                  */
/*                                                                                                */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"                   */
/*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE                     */
/*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                */
/*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE                   */
/*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL                    */
/*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR                    */
/*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER                    */
/*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,                 */
/*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE                 */
/*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                          */
/**************************************************************************************************/

UCHAR   *m_GoldenBuf;

struct GraphicConfig
{
    // must maintain the struct item order
    UCHAR   m_TCON_ADDR;
    UCHAR   m_EEPROM_ADDR;
    UINT    m_BufferSize;
    UINT    m_AuxRequestByte;
    UINT    m_FlashSectorEraseTime;
    UINT    m_FlashPageProgTime;
    UINT    m_FlashWriteStatusReg;
    char *  m_GoldenFilePath;
    UINT    m_DpInterface;
} gConfig;

const char * configItem[CONFIG_ITEMS]={
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

typedef enum _init_status
{
    INIT_STATUS_OK = 0,
    INIT_STATUS_FAIL = -1,
    INIT_STATUS_ERR_PARAM = -2,
}INIT_STATUS;

INIT_STATUS LoadIni(char* path,bool romPathEn, char* romPath);
void InitConfig();
void SetConfig(int item, char * inStr);
void RemoveUnwantChar(char * str);
#endif // NVTSETTINGPROC_H_INCLUDED
