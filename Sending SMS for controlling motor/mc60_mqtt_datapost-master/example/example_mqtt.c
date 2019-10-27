/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2013
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   example_tcpssl.c
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   This example demonstrates how to use tcp ssl function with APIs in OpenCPU.
 *   
 * Usage:
 * ------
 *   Compile & Run:
 *
 *     Set "C_PREDEF=-D __EXAMPLE_TCPSSL__" in gcc_makefile file. And compile the 
 *     app using "make clean/new".
 *     Download image bin to module to run.
 * 
 *   Operation:
 *            
 * 
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/

#ifdef __EXAMPLE_MQTT__
#include "ql_stdlib.h"
#include "ql_common.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_uart.h"
#include "ql_gprs.h"
#include "ql_timer.h"
#include "ril_network.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_system.h"
#include "ril_sms.h"
#include "ql_wtd.h"

#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT1
#define DBG_BUF_LEN   1024
static char DBG_BUFFER[DBG_BUF_LEN];
#define APP_DEBUG(FORMAT,...) {\
    Ql_memset(DBG_BUFFER, 0, DBG_BUF_LEN);\
    Ql_sprintf(DBG_BUFFER,FORMAT,##__VA_ARGS__); \
    if (UART_PORT2 == (DEBUG_PORT)) \
    {\
        Ql_Debug_Trace(DBG_BUFFER);\
    } else {\
        Ql_UART_Write((Enum_SerialPort)(DEBUG_PORT), (u8*)(DBG_BUFFER), Ql_strlen((const char *)(DBG_BUFFER)));\
    }\
}
#else
#define APP_DEBUG(FORMAT,...) 
#endif


#define SERIAL_ENABLE 1
#if SERIAL_ENABLE > 0
#define SERIAL_PORT  UART_PORT1
#define SER_BUF_LEN   1024
static char SER_BUFFER[SER_BUF_LEN];
#define APP_SERIAL1(FORMAT,...) {\
    Ql_memset(SER_BUFFER, 0, SER_BUF_LEN);\
    Ql_sprintf(SER_BUFFER,FORMAT,##__VA_ARGS__); \
    if (UART_PORT2 == (SERIAL_PORT)) \
    {\
        Ql_Debug_Trace(SER_BUFFER);\
    } else {\
        Ql_UART_Write((Enum_SerialPort)(SERIAL_PORT), (u8*)(SER_BUFFER), Ql_strlen((const char *)(SER_BUFFER)));\
    }\
}
#else
#define APP_SERIAL1(FORMAT,...)
#endif


Enum_PinName pin1 = PINNAME_DCD; //declaring GPIO pins
Enum_PinName pin2 = PINNAME_DTR;
Enum_PinName pin3 = PINNAME_CTS;
Enum_PinName pin4 = PINNAME_RTS;
Enum_PinName pin5 = PINNAME_RI;


typedef struct{
u8 index;
u8* prefix;
s32 data;
u32 length;

}MQTT_Param;

/*****************************************************************
* timer param
******************************************************************/
#define MQTT_TIMER_ID         TIMER_ID_USER_START
#define TIMEOUT_90S_TIMER_ID TIMER_ID_USER_START + 1   //timeout
#define MQTT_TIMER_PERIOD     1000
#define TIMEOUT_90S_PERIOD   90000

static s32 timeout_90S_monitor = FALSE;

/*****************************************************************
* APN Param
******************************************************************/
static u8 m_apn[MAX_GPRS_APN_LEN] = "iot";
static u8 m_userid[MAX_GPRS_USER_NAME_LEN] = "";
static u8 m_passwd[MAX_GPRS_PASSWORD_LEN] = "";
/**********************************************************************************/

/*****************************************************************
* UART Param
******************************************************************/
#define SERIAL_RX_BUFFER_LEN  2048
static u8 m_RxBuf_Uart[SERIAL_RX_BUFFER_LEN];
/**********************************************************************************/

/*****************************************************************
* Server Param
******************************************************************/
#define SRVADDR_BUFFER_LEN  100
#define SEND_BUFFER_LEN     2048
#define RECV_BUFFER_LEN     2048

static u8 m_send_buf[SEND_BUFFER_LEN];
static u8 m_recv_buf[RECV_BUFFER_LEN];

static s32 m_remain_len = 0;     // record the remaining number of bytes in send buffer.
static char *m_pCurrentPos = NULL;
/**********************************************************************************/

#define MAX_MQTT_PARAM_LENGTH   2048
#define HOST_NAME       "faclon.com"
#define HOST_PORT            1883
#define CONTEXT         0

static u8 product_key[MAX_MQTT_PARAM_LENGTH] = "hap";
static u8 device_name[MAX_MQTT_PARAM_LENGTH] = "TEST1";
static u8 device_secret[MAX_MQTT_PARAM_LENGTH] = "lN9l09QesoiL4HXw7W9Z1XVJ0JzTJxhs";

static u8 topic_update[MAX_MQTT_PARAM_LENGTH] = "devicesOut/SV_TEST/autocmd";
static u8 topic_publish[MAX_MQTT_PARAM_LENGTH] = "devicesIn/SV_TEST/data";
static u8 topic_get[MAX_MQTT_PARAM_LENGTH] = "/hDTjzV2bVeR/TEST1/get";

/*************************************************************************************/
s32 wtdID;
u8 test_message[50] = "{ \"id\" : \"ABCD1\"}\0";
u32 rssi;
u32 ber;
typedef enum{
    STATE_NW_GET_SIMSTATE,
    STATE_NW_QUERY_STATE,
    STATE_GPRS_CONTEXT,
    STATE_GPRS_ACTIVATE,
    STATE_MQTT_CFG,
    STATE_MQTT_OPEN,
    STATE_MQTT_CONN,
    STATE_MQTT_SUB,
    STATE_MQTT_PUB,
    STATE_MQTT_CLOSE,
    STATE_GPRS_DEACTIVATE,
    STATE_TOTAL_NUM
}Enum_TCPSTATE;

static u8 m_tcp_state = STATE_NW_GET_SIMSTATE;
static s32 flag=0;
static MQTT_Param mqtt_param;
/*****************************************************************
* uart callback function
******************************************************************/
static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);
static void Callback_Timer(u32 timerId, void* param);
static void proc_handle(char *pData,s32 len);
static s32 ReadSerialPort(Enum_SerialPort port, /*[out]*/u8* pBuffer, /*[in]*/u32 bufLen);


static s32 ATResponse_mqtt_handler(char* line, u32 len, void* userdata);
static s32 ATResponse_mqtt_handler_pub(char* line, u32 len, void* userdata);
static s32 ATResponse_mqtt_handler_close(char* line, u32 len, void* userdata);
static void Hdlr_Mqtt_Recv_Data(u8* packet);
static void Hdlr_Mqtt_Recv_State(u8* packet);

s32 RIL_MQTT_QMTCFG_Ali( u8 connectID,u8* product_key,u8* device_name,u8* device_secret);
s32 RIL_MQTT_QMTOPEN( u8 connectID,u8* hostName, u32 port );
s32 RIL_MQTT_QMTCONN(u8 connectID, u8* clientID,u8* username,u8* password);
s32 RIL_MQTT_QMTSUB(u8 connectID, u8 msgId,u8* topic, u8 qos,u8* others);
s32 RIL_MQTT_QMTPUB(u8 connectID, u8 msgId,u8 qos,u8 retain, u8* topic,u8* data,s32 length);
s32 RIL_MQTT_QMTDISC(u8 connectID);
s32 RIL_MQTT_QMTCLOSE(u8 connectID);
s32 RIL_MQTT_QMTUNS(u8 connectID, u8 msgId,u8* topic,u8* others);

/*******************************************************************************/
#define CON_SMS_BUF_MAX_CNT   (1)
#define CON_SMS_SEG_MAX_CHAR  (160)
#define CON_SMS_SEG_MAX_BYTE  (4 * CON_SMS_SEG_MAX_CHAR)
#define CON_SMS_MAX_SEG       (7)

typedef struct
{
    u8 aData[CON_SMS_SEG_MAX_BYTE];
    u16 uLen;
} ConSMSSegStruct;

typedef struct
{
    u16 uMsgRef;
    u8 uMsgTot;

    ConSMSSegStruct asSeg[CON_SMS_MAX_SEG];
    bool abSegValid[CON_SMS_MAX_SEG];
} ConSMSStruct;

ConSMSStruct g_asConSMSBuf[CON_SMS_BUF_MAX_CNT];

u8 status = 'u';
u8 GNo[2][11] = {"9833429903\0","7021484163\0"};

static bool ConSMSBuf_IsIntact(ConSMSStruct *pCSBuf,u8 uCSMaxCnt,u8 uIdx,ST_RIL_SMS_Con *pCon);
static bool ConSMSBuf_AddSeg(ConSMSStruct *pCSBuf,u8 uCSMaxCnt,u8 uIdx,ST_RIL_SMS_Con *pCon,u8 *pData,u16 uLen);
static s8 ConSMSBuf_GetIndex(ConSMSStruct *pCSBuf,u8 uCSMaxCnt,ST_RIL_SMS_Con *pCon);
static bool ConSMSBuf_ResetCtx(ConSMSStruct *pCSBuf,u8 uCSMaxCnt,u8 uIdx);


/*******************************************************************************/

/*******************************************************************************/
/*****************************************************************************
 * FUNCTION
 *  ConSMSBuf_GetIndex
 *
 * DESCRIPTION
 *  This function is used to get available index in <pCSBuf>
 *
 * PARAMETERS
 *  <pCSBuf>     The SMS index in storage,it starts from 1
 *  <uCSMaxCnt>  TRUE: The module should reply a SMS to the sender; FALSE: The module only read this SMS.
 *  <pCon>       The pointer of 'ST_RIL_SMS_Con' data
 *
 * RETURNS
 *  -1:   FAIL! Can not get available index
 *  OTHER VALUES: SUCCESS.
 *
 * NOTE
 *  1. This is an internal function
 *****************************************************************************/
static s8 ConSMSBuf_GetIndex(ConSMSStruct *pCSBuf,u8 uCSMaxCnt,ST_RIL_SMS_Con *pCon)
{
	u8 uIdx = 0;

    if(    (NULL == pCSBuf) || (0 == uCSMaxCnt)
        || (NULL == pCon)
      )
    {
        APP_DEBUG("Enter ConSMSBuf_GetIndex,FAIL! Parameter is INVALID. pCSBuf:%x,uCSMaxCnt:%d,pCon:%x\r\n",pCSBuf,uCSMaxCnt,pCon);
        return -1;
    }

    if((pCon->msgTot) > CON_SMS_MAX_SEG)
    {
        APP_DEBUG("Enter ConSMSBuf_GetIndex,FAIL! msgTot:%d is larger than limit:%d\r\n",pCon->msgTot,CON_SMS_MAX_SEG);
        return -1;
    }

	for(uIdx = 0; uIdx < uCSMaxCnt; uIdx++)  //Match all exist records
	{
        if(    (pCon->msgRef == pCSBuf[uIdx].uMsgRef)
            && (pCon->msgTot == pCSBuf[uIdx].uMsgTot)
          )
        {
            return uIdx;
        }
	}

	for (uIdx = 0; uIdx < uCSMaxCnt; uIdx++)
	{
		if (0 == pCSBuf[uIdx].uMsgTot)  //Find the first unused record
		{
            pCSBuf[uIdx].uMsgTot = pCon->msgTot;
            pCSBuf[uIdx].uMsgRef = pCon->msgRef;

			return uIdx;
		}
	}

    APP_DEBUG("Enter ConSMSBuf_GetIndex,FAIL! No avail index in ConSMSBuf,uCSMaxCnt:%d\r\n",uCSMaxCnt);

	return -1;
}

/*****************************************************************************
 * FUNCTION
 *  ConSMSBuf_AddSeg
 *
 * DESCRIPTION
 *  This function is used to add segment in <pCSBuf>
 *
 * PARAMETERS
 *  <pCSBuf>     The SMS index in storage,it starts from 1
 *  <uCSMaxCnt>  TRUE: The module should reply a SMS to the sender; FALSE: The module only read this SMS.
 *  <uIdx>       Index of <pCSBuf> which will be stored
 *  <pCon>       The pointer of 'ST_RIL_SMS_Con' data
 *  <pData>      The pointer of CON-SMS-SEG data
 *  <uLen>       The length of CON-SMS-SEG data
 *
 * RETURNS
 *  FALSE:   FAIL!
 *  TRUE: SUCCESS.
 *
 * NOTE
 *  1. This is an internal function
 *****************************************************************************/
static bool ConSMSBuf_AddSeg(ConSMSStruct *pCSBuf,u8 uCSMaxCnt,u8 uIdx,ST_RIL_SMS_Con *pCon,u8 *pData,u16 uLen)
{
    u8 uSeg = 1;

    if(    (NULL == pCSBuf) || (0 == uCSMaxCnt)
        || (uIdx >= uCSMaxCnt)
        || (NULL == pCon)
        || (NULL == pData)
        || (uLen > (CON_SMS_SEG_MAX_CHAR * 4))
      )
    {
        APP_DEBUG("Enter ConSMSBuf_AddSeg,FAIL! Parameter is INVALID. pCSBuf:%x,uCSMaxCnt:%d,uIdx:%d,pCon:%x,pData:%x,uLen:%d\r\n",pCSBuf,uCSMaxCnt,uIdx,pCon,pData,uLen);
        return FALSE;
    }

    if((pCon->msgTot) > CON_SMS_MAX_SEG)
    {
        APP_DEBUG("Enter ConSMSBuf_GetIndex,FAIL! msgTot:%d is larger than limit:%d\r\n",pCon->msgTot,CON_SMS_MAX_SEG);
        return FALSE;
    }

    uSeg = pCon->msgSeg;
    pCSBuf[uIdx].abSegValid[uSeg-1] = TRUE;
    Ql_memcpy(pCSBuf[uIdx].asSeg[uSeg-1].aData,pData,uLen);
    pCSBuf[uIdx].asSeg[uSeg-1].uLen = uLen;

	return TRUE;
}

/*****************************************************************************
 * FUNCTION
 *  ConSMSBuf_IsIntact
 *
 * DESCRIPTION
 *  This function is used to check the CON-SMS is intact or not
 *
 * PARAMETERS
 *  <pCSBuf>     The SMS index in storage,it starts from 1
 *  <uCSMaxCnt>  TRUE: The module should reply a SMS to the sender; FALSE: The module only read this SMS.
 *  <uIdx>       Index of <pCSBuf> which will be stored
 *  <pCon>       The pointer of 'ST_RIL_SMS_Con' data
 *
 * RETURNS
 *  FALSE:   FAIL!
 *  TRUE: SUCCESS.
 *
 * NOTE
 *  1. This is an internal function
 *****************************************************************************/
static bool ConSMSBuf_IsIntact(ConSMSStruct *pCSBuf,u8 uCSMaxCnt,u8 uIdx,ST_RIL_SMS_Con *pCon)
{
    u8 uSeg = 1;

    if(    (NULL == pCSBuf)
        || (0 == uCSMaxCnt)
        || (uIdx >= uCSMaxCnt)
        || (NULL == pCon)
      )
    {
        APP_DEBUG("Enter ConSMSBuf_IsIntact,FAIL! Parameter is INVALID. pCSBuf:%x,uCSMaxCnt:%d,uIdx:%d,pCon:%x\r\n",pCSBuf,uCSMaxCnt,uIdx,pCon);
        return FALSE;
    }

    if((pCon->msgTot) > CON_SMS_MAX_SEG)
    {
        APP_DEBUG("Enter ConSMSBuf_GetIndex,FAIL! msgTot:%d is larger than limit:%d\r\n",pCon->msgTot,CON_SMS_MAX_SEG);
        return FALSE;
    }

	for (uSeg = 1; uSeg <= (pCon->msgTot); uSeg++)
	{
        if(FALSE == pCSBuf[uIdx].abSegValid[uSeg-1])
        {
            APP_DEBUG("Enter ConSMSBuf_IsIntact,FAIL! uSeg:%d has not received!\r\n",uSeg);
            return FALSE;
        }
	}

    return TRUE;
}

/*****************************************************************************
 * FUNCTION
 *  ConSMSBuf_ResetCtx
 *
 * DESCRIPTION
 *  This function is used to reset ConSMSBuf context
 *
 * PARAMETERS
 *  <pCSBuf>     The SMS index in storage,it starts from 1
 *  <uCSMaxCnt>  TRUE: The module should reply a SMS to the sender; FALSE: The module only read this SMS.
 *  <uIdx>       Index of <pCSBuf> which will be stored
 *
 * RETURNS
 *  FALSE:   FAIL!
 *  TRUE: SUCCESS.
 *
 * NOTE
 *  1. This is an internal function
 *****************************************************************************/
static bool ConSMSBuf_ResetCtx(ConSMSStruct *pCSBuf,u8 uCSMaxCnt,u8 uIdx)
{
    if(    (NULL == pCSBuf) || (0 == uCSMaxCnt)
        || (uIdx >= uCSMaxCnt)
      )
    {
        APP_DEBUG("Enter ConSMSBuf_ResetCtx,FAIL! Parameter is INVALID. pCSBuf:%x,uCSMaxCnt:%d,uIdx:%d\r\n",pCSBuf,uCSMaxCnt,uIdx);
        return FALSE;
    }

    //Default reset
    Ql_memset(&pCSBuf[uIdx],0x00,sizeof(ConSMSStruct));

    //TODO: Add special reset here

    return TRUE;
}

/*****************************************************************************
 * FUNCTION
 *  SMS_Initialize
 *
 * DESCRIPTION
 *  Initialize SMS environment.
 *
 * PARAMETERS
 *  VOID
 *
 * RETURNS
 *  TRUE:  This function works SUCCESS.
 *  FALSE: This function works FAIL!
 *****************************************************************************/
static bool SMS_Initialize(void)
{
    s32 iResult = 0;
    u8  nCurrStorage = 0;
    u32 nUsed = 0;
    u32 nTotal = 0;

    // Set SMS storage:
    // By default, short message is stored into SIM card. You can change the storage to ME if needed, or
    // you can do it again to make sure the short message storage is SIM card.
    #if 0
    {
        iResult = RIL_SMS_SetStorage(RIL_SMS_STORAGE_TYPE_SM,&nUsed,&nTotal);
        if (RIL_ATRSP_SUCCESS != iResult)
        {
            APP_DEBUG("Fail to set SMS storage, cause:%d\r\n", iResult);
            return FALSE;
        }
        APP_DEBUG("<-- Set SMS storage to SM, nUsed:%u,nTotal:%u -->\r\n", nUsed, nTotal);

        iResult = RIL_SMS_GetStorage(&nCurrStorage, &nUsed ,&nTotal);
        if(RIL_ATRSP_SUCCESS != iResult)
        {
            APP_DEBUG("Fail to get SMS storage, cause:%d\r\n", iResult);
            return FALSE;
        }
        APP_DEBUG("<-- Check SMS storage: curMem=%d, used=%d, total=%d -->\r\n", nCurrStorage, nUsed, nTotal);
    }
    #endif

    // Enable new short message indication
    // By default, the auto-indication for new short message is enalbed. You can do it again to
    // make sure that the option is open.
    #if 0
    {
        iResult = Ql_RIL_SendATCmd("AT+CNMI=2,1",Ql_strlen("AT+CNMI=2,1"),NULL,NULL,0);
        if (RIL_AT_SUCCESS != iResult)
        {
            APP_DEBUG("Fail to send \"AT+CNMI=2,1\", cause:%d\r\n", iResult);
            return FALSE;
        }
        APP_DEBUG("<-- Enable new SMS indication -->\r\n");
    }
    #endif

    // Delete all existed short messages (if needed)
    iResult = RIL_SMS_DeleteSMS(0, RIL_SMS_DEL_ALL_MSG);
    if (iResult != RIL_AT_SUCCESS)
    {
        APP_DEBUG("Fail to delete all messages, iResult=%d,cause:%d\r\n", iResult, Ql_RIL_AT_GetErrCode());
        return FALSE;
    }
    APP_DEBUG("Delete all existed messages\r\n");

    return TRUE;
}

static void Hdlr_RecvNewSMS(u32 nIndex, bool bAutoReply)
{
    s32 iResult = 0;
    s32 ret = 0;
    u32 uMsgRef = 0;
    ST_RIL_SMS_TextInfo *pTextInfo = NULL;
    ST_RIL_SMS_DeliverParam *pDeliverTextInfo = NULL;
    char aPhNum[RIL_SMS_PHONE_NUMBER_MAX_LEN] = {0,};
    const char aReplyCon[] = {"Module has received SMS."};
    bool bResult = FALSE;

    pTextInfo = Ql_MEM_Alloc(sizeof(ST_RIL_SMS_TextInfo));
    if (NULL == pTextInfo)
    {
        APP_DEBUG("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", sizeof(ST_RIL_SMS_TextInfo), __func__, __LINE__);
        return;
    }
    Ql_memset(pTextInfo, 0x00, sizeof(ST_RIL_SMS_TextInfo));
    iResult = RIL_SMS_ReadSMS_Text(nIndex, LIB_SMS_CHARSET_GSM, pTextInfo);
    if (iResult != RIL_AT_SUCCESS)
    {
        Ql_MEM_Free(pTextInfo);
        APP_DEBUG("Fail to read text SMS[%d], cause:%d\r\n", nIndex, iResult);
        return;
    }

    if ((LIB_SMS_PDU_TYPE_DELIVER != (pTextInfo->type)) || (RIL_SMS_STATUS_TYPE_INVALID == (pTextInfo->status)))
    {
        Ql_MEM_Free(pTextInfo);
        APP_DEBUG("WARNING: NOT a new received SMS.\r\n");
        return;
    }

    pDeliverTextInfo = &((pTextInfo->param).deliverParam);

    if(TRUE == pDeliverTextInfo->conPres)  //Receive CON-SMS segment
    {
        s8 iBufIdx = 0;
        u8 uSeg = 0;
        u16 uConLen = 0;

        iBufIdx = ConSMSBuf_GetIndex(g_asConSMSBuf,CON_SMS_BUF_MAX_CNT,&(pDeliverTextInfo->con));
        if(-1 == iBufIdx)
        {
            APP_DEBUG("Enter Hdlr_RecvNewSMS,WARNING! ConSMSBuf_GetIndex FAIL! Show this CON-SMS-SEG directly!\r\n");

            APP_DEBUG(
                "status:%u,type:%u,alpha:%u,sca:%s,oa:%s,scts:%s,data length:%u,cp:1,cy:%d,cr:%d,ct:%d,cs:%d\r\n",
                    (pTextInfo->status),
                    (pTextInfo->type),
                    (pDeliverTextInfo->alpha),
                    (pTextInfo->sca),
                    (pDeliverTextInfo->oa),
                    (pDeliverTextInfo->scts),
                    (pDeliverTextInfo->length),
                    pDeliverTextInfo->con.msgType,
                    pDeliverTextInfo->con.msgRef,
                    pDeliverTextInfo->con.msgTot,
                    pDeliverTextInfo->con.msgSeg
            );
            APP_DEBUG("data = %s\r\n",(pDeliverTextInfo->data));

            Ql_MEM_Free(pTextInfo);

            return;
        }

        bResult = ConSMSBuf_AddSeg(
                    g_asConSMSBuf,
                    CON_SMS_BUF_MAX_CNT,
                    iBufIdx,
                    &(pDeliverTextInfo->con),
                    (pDeliverTextInfo->data),
                    (pDeliverTextInfo->length)
        );
        if(FALSE == bResult)
        {
            APP_DEBUG("Enter Hdlr_RecvNewSMS,WARNING! ConSMSBuf_AddSeg FAIL! Show this CON-SMS-SEG directly!\r\n");

            APP_DEBUG(
                "status:%u,type:%u,alpha:%u,sca:%s,oa:%s,scts:%s,data length:%u,cp:1,cy:%d,cr:%d,ct:%d,cs:%d\r\n",
                (pTextInfo->status),
                (pTextInfo->type),
                (pDeliverTextInfo->alpha),
                (pTextInfo->sca),
                (pDeliverTextInfo->oa),
                (pDeliverTextInfo->scts),
                (pDeliverTextInfo->length),
                pDeliverTextInfo->con.msgType,
                pDeliverTextInfo->con.msgRef,
                pDeliverTextInfo->con.msgTot,
                pDeliverTextInfo->con.msgSeg
            );
            APP_DEBUG("data = %s\r\n",(pDeliverTextInfo->data));

            Ql_MEM_Free(pTextInfo);

            return;
        }

        bResult = ConSMSBuf_IsIntact(
                    g_asConSMSBuf,
                    CON_SMS_BUF_MAX_CNT,
                    iBufIdx,
                    &(pDeliverTextInfo->con)
        );
        if(FALSE == bResult)
        {
            APP_DEBUG(
                "Enter Hdlr_RecvNewSMS,WARNING! ConSMSBuf_IsIntact FAIL! Waiting. cp:1,cy:%d,cr:%d,ct:%d,cs:%d\r\n",
                pDeliverTextInfo->con.msgType,
                pDeliverTextInfo->con.msgRef,
                pDeliverTextInfo->con.msgTot,
                pDeliverTextInfo->con.msgSeg
            );

            Ql_MEM_Free(pTextInfo);

            return;
        }

        //Show the CON-SMS
        APP_DEBUG(
            "status:%u,type:%u,alpha:%u,sca:%s,oa:%s,scts:%s",
            (pTextInfo->status),
            (pTextInfo->type),
            (pDeliverTextInfo->alpha),
            (pTextInfo->sca),
            (pDeliverTextInfo->oa),
            (pDeliverTextInfo->scts)
        );

        uConLen = 0;
        for(uSeg = 1; uSeg <= pDeliverTextInfo->con.msgTot; uSeg++)
        {
            uConLen += g_asConSMSBuf[iBufIdx].asSeg[uSeg-1].uLen;
        }

        APP_DEBUG(",data length:%u",uConLen);
        APP_DEBUG("\r\n"); //Print CR LF

        for(uSeg = 1; uSeg <= pDeliverTextInfo->con.msgTot; uSeg++)
        {
            APP_DEBUG("data = %s ,len = %d",
                g_asConSMSBuf[iBufIdx].asSeg[uSeg-1].aData,
                g_asConSMSBuf[iBufIdx].asSeg[uSeg-1].uLen
            );
        }

        APP_DEBUG("\r\n"); //Print CR LF

        //Reset CON-SMS context
        bResult = ConSMSBuf_ResetCtx(g_asConSMSBuf,CON_SMS_BUF_MAX_CNT,iBufIdx);
        if(FALSE == bResult)
        {
            APP_DEBUG("Enter Hdlr_RecvNewSMS,WARNING! ConSMSBuf_ResetCtx FAIL! iBufIdx:%d\r\n",iBufIdx);
        }

        Ql_MEM_Free(pTextInfo);

        return;
    }

    APP_DEBUG("<-- RIL_SMS_ReadSMS_Text OK. eCharSet:LIB_SMS_CHARSET_GSM,nIndex:%u -->\r\n",nIndex);
    APP_DEBUG("status:%u,type:%u,alpha:%u,sca:%s,oa:%s,scts:%s,data length:%u\r\n",
        pTextInfo->status,
        pTextInfo->type,
        pDeliverTextInfo->alpha,
        pTextInfo->sca,
        pDeliverTextInfo->oa,
        pDeliverTextInfo->scts,
        pDeliverTextInfo->length);
    APP_DEBUG("data = %s\r\n",(pDeliverTextInfo->data));

    if ( status == 'u' )
     {
     	char *p=NULL;
     			s32 iRet = 0;
     			u8 MbNo[5][11];
     			u8 smsReceivedNo[11];
     			int flag = 0;
     			for ( int i = 0 ; i < 12 ; i++)
     			{
     				smsReceivedNo[i] = pDeliverTextInfo->oa[3+i];
     			}
     			smsReceivedNo[11] = '\0';
     			APP_DEBUG("<-- %s -->\r\n", smsReceivedNo);
     	//		for( int i = 0 ; i < 5 ; i++ )
     	//		{
     	//
     	//
     	//			iRet=Ql_SecureData_Read((i+1), (u8*)&MbNo[i], sizeof(MbNo[i]));
     	//			APP_DEBUG("<-- %s -->\r\n", MbNo[i]);
     	//
     				if( (strcmp(smsReceivedNo,GNo[0]) == 0) || (strcmp(smsReceivedNo,GNo[1]) == 0))
     				{
     					flag = 1;
     					APP_DEBUG("<-- V E R I F I E D -->\r\n");
     					//break;
     				}

     				else
     				{

     					flag = 2;
 //    					for( int i = 0 ; i < 5 ; i++ )
 //    										{
 //
 //
 //    											iRet=Ql_SecureData_Read((i+1), (u8*)&MbNo[i], sizeof(MbNo[i]));
 //    											APP_DEBUG("<-- %s -->\r\n", MbNo[i]);
 //
 //    											if( (strcmp(smsReceivedNo,GNo[0]) == 0) || (strcmp(smsReceivedNo,GNo[1]) == 0))
 //    											{
 //    												flag = 2;
 //    												APP_DEBUG("<-- V E R I F I E D -->\r\n", MbNo[i]);
 //    												break;
 //    											}
 //
 //    										}
     				}

     	//		}
     			ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
     			Ql_Sleep(200);
     			ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
     			Ql_Sleep(200);
     			ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
     			Ql_Sleep(200);
     			ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
     			Ql_Sleep(200);



     	//    	p = Ql_strstr(pDeliverTextInfo->data, "Secure,");
     			if( flag == 1)
     			{
     				ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
     				Ql_Sleep(200);
     				ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
     				Ql_Sleep(200);
     				ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
     				Ql_Sleep(200);
     				ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
     				Ql_Sleep(200);
     				if ( Ql_strstr(pDeliverTextInfo->data, "Secure,") )
     				{
     					status = 's';
     					u8 MbNo[5][11];
     					int xtext = 7;
     					iRet=Ql_SecureData_Store(6, (u8*)&status, sizeof(status));
     					if (iRet !=QL_RET_OK)
     					{
     					APP_DEBUG("<-- Fail to store critical data! Cause:%d -->\r\n", iRet);
     					}
     					for( int i = 0 ; i < 5 ; i++ )
     					{
     						for( int j = 0 ; j < 12 ; j++ )
     						{
     							if(pDeliverTextInfo->data[xtext+j] != ',')
     							{
     								MbNo[i][j] = pDeliverTextInfo->data[xtext+j];
     							}
     							else
     							{
     								MbNo[i][j] = '\0';
     								j++;
     								APP_DEBUG("---%d---\r\n",j);
     								xtext += j;
     								break;
     							}
     						}

     					}

     					for( int i = 0 ; i < 5 ; i++ )
     					{

     						APP_DEBUG("<-- %s -->\r\n", MbNo[i]);
     						iRet=Ql_SecureData_Store((i+1), (u8*)&MbNo[i], sizeof(MbNo[i]));

     					}




     				}

     				else if (Ql_strstr(pDeliverTextInfo->data, "Unsecure"))
     				{
     					status = 'u';
     					iRet=Ql_SecureData_Store(6, (u8*)&status, sizeof(status));
     				}

     				else
     				{
     					flag = 2;
     				}

     			}

     			if( flag == 2)
     					{
     						ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
     						Ql_Sleep(200);
     						ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
     						Ql_Sleep(200);
     						ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
     						Ql_Sleep(200);
     						ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
     						Ql_Sleep(200);
     									if (Ql_strstr(pDeliverTextInfo->data, "LED,ON"))
     									{
     										iRet=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
     									}
     									else if (Ql_strstr(pDeliverTextInfo->data, "LED,OFF"))
     									{
     										iRet=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
     									}

     									else if (Ql_strstr(pDeliverTextInfo->data, "Network"))
     									{
     										RIL_NW_GetSignalQuality(&rssi, &ber);

     										if(rssi>19)
     										{
     											APP_DEBUG("< - - - - H I G H - - - - - >");
     											u32 nMsgRef;
     											iResult = RIL_SMS_SendSMS_Text(pDeliverTextInfo->oa, pDeliverTextInfo->oa, LIB_SMS_CHARSET_GSM, "Network: HIGH", Ql_strlen("Network: HIGH"), &nMsgRef);
											   if (iResult != RIL_AT_SUCCESS)
											   {
												   APP_DEBUG("< Fail to send Text SMS, iResult=%d, cause:%d >\r\n", iResult, Ql_RIL_AT_GetErrCode());
												   //return;
											   }
											   else{
												   APP_DEBUG("< Send Text SMS successfully, MsgRef:%u >\r\n", nMsgRef);
											   }
     										}

     										else if(rssi>9 && rssi<20)
     										{
     											APP_DEBUG("< - - - - M E D I U M - - - - - >");
     											u32 nMsgRef;
												iResult = RIL_SMS_SendSMS_Text(pDeliverTextInfo->oa, pDeliverTextInfo->oa, LIB_SMS_CHARSET_GSM, "Network: MEDIUM", Ql_strlen("Network: MEDIUM"), &nMsgRef);
											   if (iResult != RIL_AT_SUCCESS)
											   {
												   APP_DEBUG("< Fail to send Text SMS, iResult=%d, cause:%d >\r\n", iResult, Ql_RIL_AT_GetErrCode());
												   //return;
											   }
											   else{
												   APP_DEBUG("< Send Text SMS successfully, MsgRef:%u >\r\n", nMsgRef);
											   }
     										}
     										else if(rssi<10)
     										{
     											APP_DEBUG("< - - - - L O W - - - - - >");
     											u32 nMsgRef;
												iResult = RIL_SMS_SendSMS_Text(pDeliverTextInfo->oa, pDeliverTextInfo->oa, LIB_SMS_CHARSET_GSM, "Network: LOW", Ql_strlen("Network: LOW"), &nMsgRef);
											   if (iResult != RIL_AT_SUCCESS)
											   {
												   APP_DEBUG("< Fail to send Text SMS, iResult=%d, cause:%d >\r\n", iResult, Ql_RIL_AT_GetErrCode());
												   //return;
											   }
											   else{
												   APP_DEBUG("< Send Text SMS successfully, MsgRef:%u >\r\n", nMsgRef);
											   }
     										}

     									}


     									else if (Ql_strstr(pDeliverTextInfo->data, "AUTO"))
										{
											uamf1:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

																		//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

																		if(Ql_GPIO_GetLevel(pin2) == 1)
																		{
																			Ql_Sleep(1000);
																		 APP_SERIAL1("{\"AMF\":1}\r\n");

																		}

																		else
																		{
																			goto uamf1;
																		}

																		ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
																		Ql_Sleep(1000);
										}

										else if (Ql_strstr(pDeliverTextInfo->data, "MANUAL"))
										{
											uamf0:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

																		//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

																		if(Ql_GPIO_GetLevel(pin2) == 1)
																		{
																			Ql_Sleep(1000);
																		 APP_SERIAL1("{\"AMF\":0}\r\n");

																		}

																		else
																		{
																			goto uamf0;
																		}

																		ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
																		Ql_Sleep(1000);
										}

										else if (Ql_strstr(pDeliverTextInfo->data, "STOP"))
										{
											uprf0:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

																		//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

																		if(Ql_GPIO_GetLevel(pin2) == 1)
																		{
																			Ql_Sleep(1000);
																		 APP_SERIAL1("{\"PRF\":0}\r\n");

																		}

																		else
																		{
																			goto uprf0;
																		}

																		ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
																		Ql_Sleep(1000);
										}

										else if (Ql_strstr(pDeliverTextInfo->data, "START"))
										{
											uprf1:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

																		//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

																		if(Ql_GPIO_GetLevel(pin2) == 1)
																		{
																			Ql_Sleep(1000);
																		 APP_SERIAL1("{\"PRF\":1}\r\n");

																		}

																		else
																		{
																			goto uprf1;
																		}

																		ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
																		Ql_Sleep(1000);
										}

     					}
     }

     else if ( status == 's' )
     {
     	char *p=NULL;
 		s32 iRet = 0;
 		u8 MbNo[5][11];
 		u8 smsReceivedNo[11];
 		int flag = 0;
 		for ( int i = 0 ; i < 12 ; i++)
 		{
 			smsReceivedNo[i] = pDeliverTextInfo->oa[3+i];
 		}
 		smsReceivedNo[11] = '\0';
 		APP_DEBUG("<-- %s -->\r\n", smsReceivedNo);
 //		for( int i = 0 ; i < 5 ; i++ )
 //		{
 //
 //
 //			iRet=Ql_SecureData_Read((i+1), (u8*)&MbNo[i], sizeof(MbNo[i]));
 //			APP_DEBUG("<-- %s -->\r\n", MbNo[i]);
 //
 			if( (strcmp(smsReceivedNo,GNo[0]) == 0) || (strcmp(smsReceivedNo,GNo[1]) == 0))
 			{
 				flag = 1;
 				APP_DEBUG("<-- V E R I F I E D -->\r\n");
 				//break;
 			}

 			else
 			{
 				for( int i = 0 ; i < 5 ; i++ )
 									{


 										iRet=Ql_SecureData_Read((i+1), (u8*)&MbNo[i], sizeof(MbNo[i]));
 										APP_DEBUG("<-- %s -->\r\n", MbNo[i]);

 										if( (strcmp(smsReceivedNo,MbNo[i]) == 0))
 										{
 											flag = 2;
 											APP_DEBUG("<-- V E R I F I E D -->\r\n");
 											break;
 										}

 									}
 			}

 //		}
 		ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
 		Ql_Sleep(200);
 		ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
 		Ql_Sleep(200);
 		ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
 		Ql_Sleep(200);
 		ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
 		Ql_Sleep(200);



 //    	p = Ql_strstr(pDeliverTextInfo->data, "Secure,");
 		if( flag == 1)
 		{
 			ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
 			Ql_Sleep(200);
 			ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
 			Ql_Sleep(200);
 			ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
 			Ql_Sleep(200);
 			ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
 			Ql_Sleep(200);
 			if ( Ql_strstr(pDeliverTextInfo->data, "Secure,") )
 			{
 				status = 's';
 				u8 MbNo[5][11];
 				int xtext = 7;
 				iRet=Ql_SecureData_Store(6, (u8*)&status, sizeof(status));
 				if (iRet !=QL_RET_OK)
 				{
 				APP_DEBUG("<-- Fail to store critical data! Cause:%d -->\r\n", iRet);
 				}
 				for( int i = 0 ; i < 5 ; i++ )
 				{
 					for( int j = 0 ; j < 12 ; j++ )
 					{
 						if(pDeliverTextInfo->data[xtext+j] != ',')
 						{
 							MbNo[i][j] = pDeliverTextInfo->data[xtext+j];
 						}
 						else
 						{
 							MbNo[i][j] = '\0';
 							j++;
 							APP_DEBUG("---%d---\r\n",j);
 							xtext += j;
 							break;
 						}
 					}

 				}

 				for( int i = 0 ; i < 5 ; i++ )
 				{

 					APP_DEBUG("<-- %s -->\r\n", MbNo[i]);
 					iRet=Ql_SecureData_Store((i+1), (u8*)&MbNo[i], sizeof(MbNo[i]));

 				}




 			}

 			else if (Ql_strstr(pDeliverTextInfo->data, "Unsecure"))
 			{
 				status = 'u';
 				iRet=Ql_SecureData_Store(6, (u8*)&status, sizeof(status));
 			}

 			else
 			{
 				flag = 2;
 			}

 		}

 		if( flag == 2)
 				{
 					ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
 					Ql_Sleep(200);
 					ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
 					Ql_Sleep(200);
 					ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
 					Ql_Sleep(200);
 					ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
 					Ql_Sleep(200);
 								if (Ql_strstr(pDeliverTextInfo->data, "LED,ON"))
 								{
 									iRet=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
 								}
 								else if (Ql_strstr(pDeliverTextInfo->data, "LED,OFF"))
 								{
 									iRet=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
 								}

 								else if (Ql_strstr(pDeliverTextInfo->data, "Network"))
								{
									RIL_NW_GetSignalQuality(&rssi, &ber);

									if(rssi>19)
									{
										APP_DEBUG("< - - - - H I G H - - - - - >");
										u32 nMsgRef;
										iResult = RIL_SMS_SendSMS_Text(pDeliverTextInfo->oa, pDeliverTextInfo->oa, LIB_SMS_CHARSET_GSM, "Network: HIGH", Ql_strlen("Network: HIGH"), &nMsgRef);
									   if (iResult != RIL_AT_SUCCESS)
									   {
										   APP_DEBUG("< Fail to send Text SMS, iResult=%d, cause:%d >\r\n", iResult, Ql_RIL_AT_GetErrCode());
										   //return;
									   }
									   else{
										   APP_DEBUG("< Send Text SMS successfully, MsgRef:%u >\r\n", nMsgRef);
									   }
									}

									else if(rssi>9 && rssi<20)
									{
										APP_DEBUG("< - - - - M E D I U M - - - - - >");
										u32 nMsgRef;
										iResult = RIL_SMS_SendSMS_Text(pDeliverTextInfo->oa, pDeliverTextInfo->oa, LIB_SMS_CHARSET_GSM, "Network: MEDIUM", Ql_strlen("Network: MEDIUM"), &nMsgRef);
									   if (iResult != RIL_AT_SUCCESS)
									   {
										   APP_DEBUG("< Fail to send Text SMS, iResult=%d, cause:%d >\r\n", iResult, Ql_RIL_AT_GetErrCode());
										   //return;
									   }
									   else{
										   APP_DEBUG("< Send Text SMS successfully, MsgRef:%u >\r\n", nMsgRef);
									   }
									}
									else if(rssi<10)
									{
										APP_DEBUG("< - - - - L O W - - - - - >");
										u32 nMsgRef;
										iResult = RIL_SMS_SendSMS_Text(pDeliverTextInfo->oa, pDeliverTextInfo->oa, LIB_SMS_CHARSET_GSM, "Network: LOW", Ql_strlen("Network: LOW"), &nMsgRef);
									   if (iResult != RIL_AT_SUCCESS)
									   {
										   APP_DEBUG("< Fail to send Text SMS, iResult=%d, cause:%d >\r\n", iResult, Ql_RIL_AT_GetErrCode());
										   //return;
									   }
									   else{
										   APP_DEBUG("< Send Text SMS successfully, MsgRef:%u >\r\n", nMsgRef);
									   }
									}

								}

 								else if (Ql_strstr(pDeliverTextInfo->data, "AUTO"))
 								{
 									samf1:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

 									       						//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

 									       						if(Ql_GPIO_GetLevel(pin2) == 1)
 									       						{
 									       							Ql_Sleep(1000);
 									       						 APP_SERIAL1("{\"AMF\":1}\r\n");

 									       						}

 									       						else
 									       						{
 									       							goto samf1;
 									       						}

 									       						ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
 									       						Ql_Sleep(1000);
 								}

 								else if (Ql_strstr(pDeliverTextInfo->data, "MANUAL"))
								{
									samf0:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

																//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

																if(Ql_GPIO_GetLevel(pin2) == 1)
																{
																	Ql_Sleep(1000);
																 APP_SERIAL1("{\"AMF\":0}\r\n");

																}

																else
																{
																	goto samf0;
																}

																ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
																Ql_Sleep(1000);
								}

 								else if (Ql_strstr(pDeliverTextInfo->data, "STOP"))
								{
									sprf0:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

																//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

																if(Ql_GPIO_GetLevel(pin2) == 1)
																{
																	Ql_Sleep(1000);
																 APP_SERIAL1("{\"PRF\":0}\r\n");

																}

																else
																{
																	goto sprf0;
																}

																ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
																Ql_Sleep(1000);
								}

 								else if (Ql_strstr(pDeliverTextInfo->data, "START"))
								{
									sprf1:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

																//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

																if(Ql_GPIO_GetLevel(pin2) == 1)
																{
																	Ql_Sleep(1000);
																 APP_SERIAL1("{\"PRF\":1}\r\n");

																}

																else
																{
																	goto sprf1;
																}

																ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
																Ql_Sleep(1000);
								}

 				}



 //			else if (Ql_strstr(pDeliverTextInfo->data, "LED,ON"))
 //			{
 //				//iRet=Ql_GPIO_SetLevel(LED,PINLEVEL_HIGH);
 //			}
 //			else if (Ql_strstr(pDeliverTextInfo->data, "LED,OFF"))
 //			{
 //				//iRet=Ql_GPIO_SetLevel(LED,PINLEVEL_LOW);
 //			}


 //			else if (Ql_strstr(pDeliverTextInfo->data, "Select,"))
 //					{
 //
 //						retry4:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);
 //
 //						//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));
 //
 //						if(Ql_GPIO_GetLevel(pin2) == 1)
 //						{
 //							Ql_Sleep(1000);
 //							p = Ql_strstr(pDeliverTextInfo->data, ",");
 //							p++;
 //							APP_SERIAL1("%s\r\n",p);
 //
 //						}
 //
 //						else
 //						{
 //							goto retry4;
 //						}
 //
 //						ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
 //						Ql_Sleep(1000);
 //
 //						}

 //			else
 //			{
 //				retry2:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);
 //
 //						//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));
 //
 //						if(Ql_GPIO_GetLevel(pin2) == 1)
 //						{
 //							Ql_Sleep(1000);
 //							APP_SERIAL1("%s\r\n",pDeliverTextInfo->data);
 //
 //						}
 //
 //						else
 //						{
 //							goto retry2;
 //						}
 //
 //						ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
 //						Ql_Sleep(1000);
 //
 //			}
 //			}
 		}


    Ql_strcpy(aPhNum, pDeliverTextInfo->oa);
    Ql_MEM_Free(pTextInfo);

//    if (bAutoReply)
//    {
//        if (!Ql_strstr(aPhNum, "10086"))  // Not reply SMS from operator
//        {
//            APP_DEBUG("<-- Replying SMS... -->\r\n");
//            iResult = RIL_SMS_SendSMS_Text(aPhNum, Ql_strlen(aPhNum),LIB_SMS_CHARSET_GSM,(u8*)aReplyCon,Ql_strlen(aReplyCon),&uMsgRef);
//            if (iResult != RIL_AT_SUCCESS)
//            {
//                APP_DEBUG("RIL_SMS_SendSMS_Text FAIL! iResult:%u\r\n",iResult);
//                return;
//            }
//            APP_DEBUG("<-- RIL_SMS_SendTextSMS OK. uMsgRef:%d -->\r\n", uMsgRef);
//        }
//    }
    return;
}

/*******************************************************************************/

/*******************************************************************************/


void proc_main_task(s32 taskId)
{
    s32 ret;
    ST_MSG msg;

    ret=Ql_GPIO_Init(pin1, PINDIRECTION_OUT, PINLEVEL_LOW,PINPULLSEL_DISABLE);
	ret=Ql_GPIO_Init(pin2, PINDIRECTION_IN, PINLEVEL_LOW,PINPULLSEL_PULLDOWN);
	ret=Ql_GPIO_Init(pin3, PINDIRECTION_OUT, PINLEVEL_LOW,PINPULLSEL_DISABLE);
	//ret=Ql_GPIO_Init(pin4, PINDIRECTION_OUT, PINLEVEL_LOW,PINPULLSEL_PULLUP);
	ret=Ql_GPIO_Init(pin5, PINDIRECTION_OUT, PINLEVEL_LOW,PINPULLSEL_DISABLE);

	wtdID = Ql_WTD_Start(120000);
    // Register & open UART port
    Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
    Ql_UART_Open(UART_PORT1, 9600, FC_NONE);

    APP_DEBUG("<--OpenCPU: mqtt test.-->\r\n");

    Ql_Timer_Register(MQTT_TIMER_ID, Callback_Timer, NULL);
    Ql_Timer_Start(MQTT_TIMER_ID, MQTT_TIMER_PERIOD, TRUE);

    Ql_Timer_Register(TIMEOUT_90S_TIMER_ID, Callback_Timer, NULL);
    timeout_90S_monitor = FALSE;

    while(TRUE)
    {
        s32 iResult = 0;

        Ql_OS_GetMessage(&msg);
        switch(msg.message)
       {
        case MSG_ID_RIL_READY:
            {
            	APP_DEBUG("<-- RIL is ready -->\r\n");
            	Ql_RIL_Initialize();
            	for(int i = 0; i < CON_SMS_BUF_MAX_CNT; i++)
				{
					ConSMSBuf_ResetCtx(g_asConSMSBuf,CON_SMS_BUF_MAX_CNT,i);
				}
				 //iResult=Ql_SecureData_Store(6, (u8*)&status, sizeof(status));
				 //if (iResult !=QL_RET_OK)
				 //{
				 // APP_DEBUG("<-- Fail to store critical data! Cause:%d -->\r\n", iResult);
				 //}
				status = 's';
				APP_DEBUG("< - - - - - %c - - - - - >\r\n", status);
				iResult=Ql_SecureData_Read(6, (u8*)&status, sizeof(status));
				APP_DEBUG("<-- Read back data from security data region, ret=%d -->\r\n", iResult);
				APP_DEBUG("< - - - - - %c - - - - - >\r\n", status);
				if (status!= 's'||status!= 'u')
				{
					status = 'u';
				}
				break;

            }
	    case MSG_ID_URC_INDICATION:
            switch (msg.param1)
            {
     	       case URC_MQTT_RECV_IND:
     	         APP_DEBUG("<-- RECV DATA:%s -->\r\n", msg.param2);
     			 Hdlr_Mqtt_Recv_Data(msg.param2);
     		    break;
     	       case URC_MQTT_STAT_IND:
     	         APP_DEBUG("<-- RECV STAT:%s-->\r\n", msg.param2);
     			 Hdlr_Mqtt_Recv_State(msg.param2);





     		   break;

     	      case URC_SYS_INIT_STATE_IND:
			  {
				  APP_DEBUG("<-- Sys Init Status %d -->\r\n", msg.param2);
				  if (SYS_STATE_SMSOK == msg.param2)
				  {
					  APP_DEBUG("\r\n<-- SMS module is ready -->\r\n");
					  APP_DEBUG("\r\n<-- Initialize SMS-related options -->\r\n");
					  iResult = SMS_Initialize();
					  if (!iResult)
					  {
						  APP_DEBUG("Fail to initialize SMS\r\n");
					  }
					  //ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
					 // SMS_TextMode_Send();
				  }
				  break;
			  }


		  case URC_NEW_SMS_IND:
			  {
				  APP_DEBUG("\r\n<-- New SMS Arrives: index=%d\r\n", msg.param2);


				  Hdlr_RecvNewSMS((msg.param2), FALSE);
				  break;
			  }
            }
		    break;
         default:
            break;
        }
   }
}



//src_string="GPRMC,235945.799,V,,,,,0.00,0.00,050180,,,N" index =1  ·µ»ØTRUE ,dest_string="235945.799"; index =3£¬·µ»ØFALSE
char QSDK_Get_Str(char *src_string,  char *dest_string, unsigned char index)
{
    char SentenceCnt = 0;
    char ItemSum = 0;
    char ItemLen = 0, Idx = 0;
    char len = 0;
    unsigned int  i = 0;

    if (src_string ==NULL)
    {
        return FALSE;
    }
    len = Ql_strlen(src_string);
	for ( i = 0;  i < len; i++)
	{
		if (*(src_string + i) == ',')
		{
			ItemLen = i - ItemSum - SentenceCnt;
			ItemSum  += ItemLen;
            if (index == SentenceCnt )
            {
                if (ItemLen == 0)
                {
                    return FALSE;
                }
		        else
                {
                    Ql_memcpy(dest_string, src_string + Idx, ItemLen);
                    *(dest_string + ItemLen) = '\0';
                    return TRUE;
                }
            }
			SentenceCnt++;
			Idx = i + 1;
		}
	}
    if (index == SentenceCnt && (len - Idx) != 0)
    {
        Ql_memcpy(dest_string, src_string + Idx, len - Idx);
        *(dest_string + len) = '\0';
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}


static void Hdlr_Mqtt_Recv_Data(u8* packet)
{
	s32 ret=0;
	char *p = NULL;
	u8 mqtt_message[2048]={0};
   QSDK_Get_Str(packet,mqtt_message,3);
   APP_DEBUG("<-- RECV message:%s-->\r\n",mqtt_message);

   p = Ql_strstr(mqtt_message,"{\"AMF\":");
   APP_DEBUG("<--%c-->\r\n",p[7]);
       if (p[7] == '0')
       {
    	   APP_DEBUG("<--  G P I O    O F F  -->\r\n");
       }
       else if (p[7] == '1')
       {
    	   APP_DEBUG("<--  G P I O    O N  -->\r\n");
       }



       controlCmd:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

       						//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

       						if(Ql_GPIO_GetLevel(pin2) == 1)
       						{
       							Ql_Sleep(1000);
       						 APP_SERIAL1("%s\r\n",mqtt_message);

       						}

       						else
       						{
       							goto controlCmd;
       						}

       						ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
       						Ql_Sleep(1000);



}
static void Hdlr_Mqtt_Recv_State(u8* packet)
{
	char strTmp[10];
	s32 ret;

	ResetCmd:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

							//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

							if(Ql_GPIO_GetLevel(pin2) == 1)
							{
								Ql_Sleep(1000);
							 APP_SERIAL1("RECV STAT");

							}

							else
							{
								goto ResetCmd;
							}

							ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
							Ql_Sleep(1000);

    Ql_memset(strTmp, 0x0, sizeof(strTmp));
	QSDK_Get_Str(packet,strTmp,3);
	APP_DEBUG("<-- mqtt state = %d-->\r\n",Ql_atoi(strTmp));
}

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    switch (msg)
    {
    case EVENT_UART_READY_TO_READ:
        {

           s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart, sizeof(m_RxBuf_Uart));
           if (totalBytes > 0)
           {
               proc_handle(m_RxBuf_Uart,sizeof(m_RxBuf_Uart));
               flag = 1;
           }
           break;
        }
    case EVENT_UART_READY_TO_WRITE:
        break;
    default:
        break;
    }
}

static s32 ReadSerialPort(Enum_SerialPort port, /*[out]*/u8* pBuffer, /*[in]*/u32 bufLen)
{
    s32 rdLen = 0;
    s32 rdTotalLen = 0;
    if (NULL == pBuffer || 0 == bufLen)
    {
        return -1;
    }
    Ql_memset(pBuffer, 0x0, bufLen);
    while (1)
    {
        rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
        if (rdLen <= 0)  // All data is read out, or Serial Port Error!
        {
            break;
        }
        rdTotalLen += rdLen;
        // Continue to read...
    }
    if (rdLen < 0) // Serial Port Error!
    {
        APP_DEBUG("<--Fail to read from port[%d]-->\r\n", port);
        return -99;
    }
    return rdTotalLen;
}

static void proc_handle(char *pData,s32 len)
{
    char *p = NULL;


    //if not command,send it to server
    m_pCurrentPos = m_send_buf;
    Ql_strcpy(m_pCurrentPos + m_remain_len, pData);

    m_remain_len = Ql_strlen(m_pCurrentPos);

}

static void Callback_Timer(u32 timerId, void* param)
{
  s32 ret ;
  if (MQTT_TIMER_ID == timerId)
  {
    switch(m_tcp_state)
    {
	  case STATE_NW_GET_SIMSTATE:
	  {
		  s32 simStat = 0;
		  RIL_SIM_GetSimState(&simStat);
		  if (simStat == SIM_STAT_READY)
		  {
			  m_tcp_state = STATE_NW_QUERY_STATE;
			  APP_DEBUG("<--SIM card status is normal!-->\r\n");
		  }else
		  {
			  APP_DEBUG("<--SIM card status is UNAVAILABLE!-->\r\n");
		  }
		  break;
	  }
	  case STATE_NW_QUERY_STATE:
	  {
		  s32 creg = 0;
		  s32 cgreg = 0;
		  ret = RIL_NW_GetGSMState(&creg);
		  ret = RIL_NW_GetGPRSState(&cgreg);
		  APP_DEBUG("<--Network State:creg=%d,cgreg=%d-->\r\n",creg,cgreg);
		  if((cgreg == NW_STAT_REGISTERED)||(cgreg == NW_STAT_REGISTERED_ROAMING))
		  {
			  m_tcp_state = STATE_GPRS_CONTEXT;
		  }
		  break;
	  }
	  case STATE_GPRS_CONTEXT:
	  {
        ret = RIL_NW_SetGPRSContext(CONTEXT);
        APP_DEBUG("<-- Set GPRS context, ret=%d -->\r\n", ret);

		if(ret ==0)
		{
	      m_tcp_state = STATE_GPRS_ACTIVATE;
	    }
		break;
	  }

	  case STATE_GPRS_ACTIVATE:
	  {
        ret = RIL_NW_SetAPN(1, m_apn, m_userid, m_passwd);
        APP_DEBUG("<-- Set GPRS APN, ret=%d -->\r\n", ret);

        ret = RIL_NW_OpenPDPContext();
        APP_DEBUG("<-- Open PDP context, ret=%d -->\r\n", ret);
		if(ret ==0)
		{
	      m_tcp_state = STATE_MQTT_CFG;
	    }
		break;
	  }
	  case STATE_MQTT_CFG:
	  {
        ret =  RIL_MQTT_QMTCFG_Ali( 0,product_key,device_name, device_secret);
        APP_DEBUG("<-- mqtt config, ret=%d -->\r\n", ret);
		if(ret ==0)
		{
	      m_tcp_state = STATE_MQTT_OPEN;
	    }
		break;
	  }

      case STATE_MQTT_OPEN:
	  {
       ret = RIL_MQTT_QMTOPEN( 0,HOST_NAME, HOST_PORT) ;
        APP_DEBUG("<-- mqtt open, ret=%d -->\r\n",ret);
		if(ret ==0)
		{
	      m_tcp_state = STATE_MQTT_CONN;
	    }
		else
		{
          m_tcp_state = STATE_GPRS_DEACTIVATE;
		}
		break;
	  }
	  case STATE_MQTT_CONN:
	  {
        char strImei[30];
        Ql_memset(strImei, 0x0, sizeof(strImei));

	    ret = RIL_GetIMEI(strImei);
        APP_DEBUG("<-- IMEI:%s,ret=%d -->\r\n", strImei,ret);

        ret = RIL_MQTT_QMTCONN(0,strImei,"admin","faclontest");
        APP_DEBUG("<-- mqtt connect,ret=%d -->\r\n", ret);
		if(ret ==0)
		{
	      m_tcp_state = STATE_MQTT_SUB;
	    }
        else 
	    {
		  m_tcp_state = STATE_GPRS_DEACTIVATE;
	    }
	   break;
	  }
	  case STATE_MQTT_SUB:
	  {
        ret = RIL_MQTT_QMTSUB(0, 1,topic_update, 2,NULL);
        APP_DEBUG("<-- mqtt subscribe, ret=%d -->\r\n",ret);
		if(ret ==0)
		{
			 ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_HIGH);
			 ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

			while(1)
				{
//				if(Ql_GPIO_GetLevel(pin2) == 1)
//
//			 							{
			 								//Ql_Sleep(1000);
				 ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
				 break;

//			 							}
//
				}
	      m_tcp_state = STATE_MQTT_PUB;
	    }
		else
	    {
		   m_tcp_state = STATE_MQTT_CLOSE;
	    }
		break;
	  }
	  case STATE_MQTT_PUB:
	  {
	  	u32 length =0;
	  	Ql_WTD_Feed(wtdID);

        if ( flag >= 1 )
        	{
        	RIL_NW_GetSignalQuality(&rssi, &ber);
        	Ql_sprintf(m_pCurrentPos + m_remain_len, ",{ \"tag\":\"RSSI\", \"value\": %d}]}",rssi);

        	length = Ql_strlen(m_pCurrentPos);
        	ret = RIL_MQTT_QMTPUB(0, 1,1,0, topic_publish,m_pCurrentPos,length);
        	APP_DEBUG("<-- mqtt publish, ret=%d -->\r\n", ret);
        	if(ret ==0)
        		{
        		m_remain_len = 0;
        		m_pCurrentPos = NULL;
        		flag = 0;
        		//m_nSentLen += ret;
        		m_tcp_state = STATE_MQTT_PUB;
				  //m_tcp_state = STATE_MQTT_CLOSE;
				}
				else
				{
				   m_tcp_state = STATE_MQTT_CLOSE;
				}
        	}
		break;
	  }
	   case STATE_MQTT_CLOSE:
	  {
		  ret=Ql_GPIO_SetLevel(pin5,PINLEVEL_LOW);
		  ret=Ql_GPIO_SetLevel(pin3,PINLEVEL_HIGH);
		  ret =RIL_MQTT_QMTCLOSE(0);

        APP_DEBUG("<--mqtt CLOSE, ret=%d -->\r\n", ret);
		if(ret ==0)
		{
	      m_tcp_state = STATE_GPRS_DEACTIVATE;
	    }
		else
		{
			m_tcp_state = STATE_GPRS_DEACTIVATE;
		}

		break;
	  }
	  case STATE_GPRS_DEACTIVATE:
	  {
        ret = RIL_NW_ClosePDPContext();
        APP_DEBUG("<-- Set GPRS DEACTIVATE, ret=%d -->\r\n", ret);
		if(ret ==0)
		{
	      m_tcp_state = STATE_NW_QUERY_STATE;
	    }
		break;
	  }
    }
   }

}

s32 RIL_MQTT_QMTCFG_Ali( u8 connectID,u8* product_key,u8* device_name,u8* device_secret)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QMTCFG=\"SSL\",%d,0,2\n",connectID);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }
    return ret;
}


static s32 ATResponse_mqtt_handler(char* line, u32 len, void* userdata)
{
    MQTT_Param *mqtt_param = (MQTT_Param *)userdata;
    char *head = Ql_RIL_FindString(line, len, mqtt_param->prefix); //continue wait
    if(head)
    {
            char strTmp[10];
            char* p1 = NULL;
            char* p2 = NULL;
            Ql_memset(strTmp, 0x0, sizeof(strTmp));
            p1 = Ql_strstr(head, ":");
            if (p1)
            {
                QSDK_Get_Str((p1+1),strTmp, mqtt_param->index);
			    mqtt_param->data= Ql_atoi(strTmp);
            }
        return  RIL_ATRSP_SUCCESS;
    }
    head = Ql_RIL_FindString(line, len, "OK");
    if(head)
    {  
        return  RIL_ATRSP_CONTINUE;  
    }
    head = Ql_RIL_FindString(line, len, "ERROR");
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CMS ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}


static s32 ATResponse_mqtt_handler_pub(char* line, u32 len, void* userdata)
{
    u8 *head = NULL;
    u8 uCtrlZ = 0x1A;

	MQTT_Param *mqtt_param = (MQTT_Param *)userdata;

	head = Ql_RIL_FindString(line, len, "\r\n>");
    if(head)
    {
		Ql_RIL_WriteDataToCore (mqtt_param->prefix,mqtt_param->length);
        Ql_RIL_WriteDataToCore(&uCtrlZ,1);

		return RIL_ATRSP_CONTINUE;
    }
    head = Ql_RIL_FindString(line, len, "ERROR");
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "OK");
    if(head)
    {
        return  RIL_ATRSP_SUCCESS;
    }
    head = Ql_RIL_FindString(line, len, "+CMS ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_MQTT_QMTOPEN( u8 connectID,u8* hostName, u32 port) 
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
	mqtt_param.prefix="+QMTOPEN:";
	mqtt_param.index = 1;
    mqtt_param.data = 255;

    Ql_sprintf(strAT, "AT+QMTOPEN=%d,\"%s.%s\",%d\n", connectID,product_key,hostName,port);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_mqtt_handler,(void* )&mqtt_param,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    else if(0 != mqtt_param.data) 
    {
        APP_DEBUG("\r\n<--MQTT OPEN failure, =%d -->\r\n", mqtt_param.data);
        return QL_RET_ERR_RIL_MQTT_FAIL;
    }
    
    return ret;
}




s32 RIL_MQTT_QMTCONN(u8 connectID, u8* clientID,u8* username,u8* password)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
    mqtt_param.prefix="+QMTCONN:";
	mqtt_param.index = 1;
    mqtt_param.data = 255;

	if(NULL != username && NULL !=password)
	{
      Ql_sprintf(strAT, "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"\n", connectID,clientID,username,password);
	}
	else
	{
		Ql_sprintf(strAT, "AT+QMTCONN=%d,\"%s\"\n", connectID,clientID);
	}
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_mqtt_handler,(void* )&mqtt_param,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    else if(0 != mqtt_param.data) 
    {
        APP_DEBUG("\r\n<--MQTT connect failure, =%d -->\r\n", mqtt_param.data);
        return QL_RET_ERR_RIL_MQTT_FAIL;
    }
    
    return ret;
}



s32 RIL_MQTT_QMTSUB(u8 connectID, u8 msgId,u8* topic, u8 qos,u8* others)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
    mqtt_param.prefix="+QMTSUB:";
	mqtt_param.index = 2;
    mqtt_param.data = 255;

	if(NULL == others)
	{
      Ql_sprintf(strAT, "AT+QMTSUB=%d,%d,\"%s\",%d\n", connectID,msgId,topic,qos);
	}
	else
	{

	}
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_mqtt_handler,(void* )&mqtt_param,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    else if(0 != mqtt_param.data) 
    {
        APP_DEBUG("\r\n<--MQTT subscribe failure, =%d -->\r\n", mqtt_param.data);
        return QL_RET_ERR_RIL_MQTT_FAIL;
    }
    
    return ret;
}



s32 RIL_MQTT_QMTPUB(u8 connectID, u8 msgId,u8 qos,u8 retain, u8* topic,u8* data,s32 length)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
    mqtt_param.prefix=data;
	mqtt_param.length= length;
	Ql_sprintf(strAT, "AT+QMTPUB=%d,%d,%d,%d,\"%s\"\n", connectID,msgId,qos,retain,topic);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_mqtt_handler_pub,(void* )&mqtt_param,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    APP_DEBUG("<-- %s -->\r\n",mqtt_param.prefix);
    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    return ret;
}


s32 RIL_MQTT_QMTCLOSE(u8 connectID)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QMTCLOSE=%d\n",connectID);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }
    return ret;
}



s32 RIL_MQTT_QMTUNS(u8 connectID, u8 msgId,u8* topic,u8* others)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
    mqtt_param.prefix="+QMTUNS:";
	mqtt_param.index = 2;
    mqtt_param.data = 255;

	if(NULL == others)
	{
      Ql_sprintf(strAT, "AT+QMTUNS=%d,%d,\"%s\"\n", connectID,msgId,topic);
	}
	else
	{

	}
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_mqtt_handler,(void* )&mqtt_param,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    else if(0 != mqtt_param.data) 
    {
        APP_DEBUG("\r\n<--MQTT subscribe failure, =%d -->\r\n", mqtt_param.data);
        return QL_RET_ERR_RIL_MQTT_FAIL;
    }
    
    return ret;
}



#endif // __EXAMPLE_TCPSSL__



