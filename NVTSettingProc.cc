#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "NVTSettingProc.h"

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

INIT_STATUS LoadIni(char* path,bool RomPathEn, char* romPath)
{
    size_t returVal;
    
    printf("used mode:%s\n",path);

    //initialize config
    InitConfig();

    //load setting
    char *line = NULL;
    FILE *fp = fopen(path,"r");
    size_t lineLen;

    if(fp == NULL)
    {
        printf("can't open model file:%s\n",path);
        return INIT_STATUS_FAIL;
    }
    while( (getline(&line,&lineLen,fp)) > 0)
    {
        if( (line[0] >=65) && (line[0]<=90))
        {

            char * item;
            char * value;
            item = strtok(line,"=");
            value = strtok(NULL,"\n");
            RemoveUnwantChar(item);
            RemoveUnwantChar(value);
            //printf("%s=%s\n",item,value);
            for(int i=0; i<CONFIG_ITEMS; i++)
            {
                if( strcmp(item,configItem[i]) == 0)
                {
                    //printf("%s=%s\n",item,value);
                    SetConfig(i,value);
                    break;
                }
            }
        }
    }
    fclose(fp);

    //change rom path to parameter defined
    if(RomPathEn)
    {
        strncpy(gConfig.m_GoldenFilePath,romPath,ROM_PATH_LEN_LIMIT);
        printf("changed rom path:%s\n",gConfig.m_GoldenFilePath);
    }
    
    
    //check setting
    fp = fopen(gConfig.m_GoldenFilePath,"rb");
    if(fp == NULL)
    {
        printf("%s not exist!!\n",gConfig.m_GoldenFilePath);
        return INIT_STATUS_FAIL;
    }

    fseek(fp,0,SEEK_END);
    UINT fSize = ftell(fp);
    if(fSize != gConfig.m_BufferSize)
    {
        printf("wrong Buffer Size\n");
        return INIT_STATUS_FAIL;
    }
    if( (gConfig.m_DpInterface <2) || (gConfig.m_DpInterface>3))
    {
        printf("unknown DP Interface\n");
        return INIT_STATUS_FAIL;
    }

    //initialize
    if(m_GoldenBuf != NULL)
        delete m_GoldenBuf;
    m_GoldenBuf = new UCHAR[fSize];
    fseek(fp, 0, SEEK_SET);
    returVal = fread(m_GoldenBuf,fSize,1,fp);
    fclose(fp);
    auxConfig.m_AuxRequestByte = gConfig.m_AuxRequestByte;

    printf("NVT Setting Initialize Success\n");
    return INIT_STATUS_OK;
}

void InitConfig()
{
    m_GoldenBuf = NULL;

    gConfig.m_TCON_ADDR = 0xC0;
    gConfig.m_EEPROM_ADDR = 0xC4;
    gConfig.m_AuxRequestByte = AUX_REQ_BYTE_LIMIT;
    gConfig.m_BufferSize = 0;
    gConfig.m_FlashSectorEraseTime = 300;
    gConfig.m_FlashPageProgTime = 5;
    gConfig.m_FlashWriteStatusReg = DEFAULT_FLASH_WRITE_STATUS_REG;
    if(gConfig.m_GoldenFilePath)
        delete gConfig.m_GoldenFilePath;
    gConfig.m_GoldenFilePath = new char[ROM_PATH_LEN_LIMIT];
    strcpy(gConfig.m_GoldenFilePath,"goldenRom.bin");
    gConfig.m_DpInterface = 2;// internal DP
}

void SetConfig(int item, char * inStr)
{
    // the decode order need to maintain
    int tmp;

    switch(item)
    {
        case 0:
            tmp = strtol(inStr,NULL, 16);
            gConfig.m_TCON_ADDR = tmp;
            printf("TCON_ADDR:0x%02X\n",gConfig.m_TCON_ADDR);
        break;

        case 1:
            tmp = strtol(inStr,NULL, 16);
            gConfig.m_EEPROM_ADDR = tmp;
            printf("EEPROM_ADDR:0x%02X\n",gConfig.m_EEPROM_ADDR);
        break;

        case 2:
            tmp = strtol(inStr,NULL, 16);
            gConfig.m_BufferSize = tmp;
            printf("EEPROM_size:0x%X\n",gConfig.m_BufferSize);
        break;

        case 3:
            tmp = strtol(inStr,NULL, 10);
            gConfig.m_AuxRequestByte = tmp;
            printf("Aux Packet Size:%d\n",gConfig.m_AuxRequestByte);
        break;

        case 4:
            tmp = strtol(inStr,NULL, 10);
            gConfig.m_FlashSectorEraseTime = tmp;
            printf("Sector Erase Wait(ms):%d\n",gConfig.m_FlashSectorEraseTime);
        break;

        case 5:
            tmp = strtol(inStr,NULL, 10);
            gConfig.m_FlashPageProgTime = tmp;
            printf("Page Program Wait(ms):%d\n",gConfig.m_FlashPageProgTime);
        break;

        case 6:
            tmp = strtol(inStr,NULL, 16);
            gConfig.m_FlashWriteStatusReg = tmp;
            printf("Flash Status Reg:0x%02X\n",gConfig.m_FlashWriteStatusReg);
        break;

        case 7:
            strcpy(gConfig.m_GoldenFilePath,inStr);
            printf("rom path:%s\n",gConfig.m_GoldenFilePath);
        break;

        case 8:
            tmp = strtol(inStr,NULL, 10);
            gConfig.m_DpInterface = tmp;
            if(gConfig.m_DpInterface == 2)
            {
                printf("DP Interface:internal\n");
            }
            else if(gConfig.m_DpInterface == 3)
            {
                printf("DP Interface:external\n");
            }
            else
                printf("DP interface:unknown\n");

        default:
        break;
    }
}

void RemoveUnwantChar(char * str)
{
    unsigned int wIdx,rIdx;
    for( wIdx=0,rIdx=0; rIdx<strlen(str); rIdx++)
    {
        if( (str[rIdx] != ' ')  && (str[rIdx]>31) )
        {
            str[wIdx] = str[rIdx];
            wIdx++;
        }
    }
    str[wIdx] = '\0';
}
