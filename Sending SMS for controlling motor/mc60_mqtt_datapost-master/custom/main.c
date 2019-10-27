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

#ifdef __CUSTOMER_CODE__
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
#include "ril_bluetooth.h"
#include "ril_system.h"
#include "ql_system.h"

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




#define BL_POWERON        (1)
#define BL_POWERDOWN      (0)
#define MAX_BLDEV_CNT     (20)
#define DEFA_CON_ID       (0)
#define SCAN_TIME_OUT     (60)   //s
#define SERIAL_RX_BUFFER_LEN  (2048)
#define BL_RX_BUFFER_LEN       (1024+1)
#define BT_EVTGRP_NAME      "BT_EVETNGRP"
#define BT_EVENT_PAIR      (0x00000004)




static char bt_name[BT_NAME_LEN] = "chengxiaofei";
static ST_BT_DevInfo **g_dev_info = NULL;
static u8 m_RxBuf_Uart1[SERIAL_RX_BUFFER_LEN];
static u8 m_RxBuf_BL[BL_RX_BUFFER_LEN];
static u8 SppSendBuff[BL_RX_BUFFER_LEN];
static ST_BT_BasicInfo pSppRecHdl;

static u32  g_nEventGrpId = 0;

static bool g_pair_search = TRUE;
static bool g_paired_before = FALSE;
static char pinCode[BT_PIN_LEN] = {0};
static bool g_connect_ok = FALSE;
//static bool  g_passive_dir = FALSE;


static ST_BT_DevInfo cur_btdev = {0};
static ST_BT_DevInfo BTSppDev1 = {0};
static ST_BT_DevInfo BTSppDev2 = {0};

static void BT_COM_Demo(void);
static void BT_Callback(s32 event, s32 errCode, void* param1, void* param2);
static void BT_Dev_Append(ST_BT_BasicInfo* pstBtDev);
static void BT_Dev_Clean(void);

static void BT_DevMngmt_UpdatePairId(const u32 hdl, const s8 pairId);
static s32 ATResponse_Handler(char* line, u32 len, void* userData);

Enum_PinName pin1 = PINNAME_CTS; //declaring GPIO pins
Enum_PinName pin2 = PINNAME_RTS;//PINNAME_RTS
Enum_PinName pin3 = PINNAME_DTR;//PINNAME_DTR
Enum_PinName pin4 = PINNAME_DCD;//PINNAME_DCD
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
static ST_GprsConfig  m_gprsCfg;
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

//static u8 topic_update[MAX_MQTT_PARAM_LENGTH] = "devicesOut/test/autocmd";
//static u8 topic_publish[MAX_MQTT_PARAM_LENGTH] = "devicesIn/test/data";
//static u8 topic_get[MAX_MQTT_PARAM_LENGTH] = "/hDTjzV2bVeR/TEST1/get";

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

char devId[20] = "acb\0";

u8 eepromInit = 'a';

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


    g_nEventGrpId = Ql_OS_CreateEvent(BT_EVTGRP_NAME);

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
            	  ret = Ql_SecureData_Read(1,(u8*)&eepromInit,sizeof(eepromInit));
            	               APP_DEBUG("<-- Read back data from security data region, ret=%d -->\r\n", ret);
            	               //APP_DEBUG("< - %c - >", eepromInit);
            	               if (eepromInit == '0')
            	               {
            	//            	APP_DEBUG("< - %d - >", sizeof(m_gprsCfg));
            	               	ret=Ql_SecureData_Read(13,  (u8*)&m_gprsCfg, sizeof(m_gprsCfg));
            	               	APP_DEBUG("<-- Read back data from security data region, ret=%d -->\r\n", ret);
            	               	APP_DEBUG("<-- New GPRS Configuration USED -->");
            	               	APP_DEBUG("\n %s, %s, %s \n",m_gprsCfg.apnName,m_gprsCfg.apnUserId,m_gprsCfg.apnPasswd);
            	               }
            	               else
            	               {
            	               	Ql_strcpy(m_gprsCfg.apnName, m_apn);
            	           		Ql_strcpy(m_gprsCfg.apnUserId, m_userid);
            	           		Ql_strcpy(m_gprsCfg.apnPasswd, m_passwd);
            	           		APP_DEBUG("<-- Default GPRS Configuration USED -->");

            	               }
            	            BT_COM_Demo();
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
	int request = 0;
	char *p = NULL;
	u8 mqtt_message[2048]={0};
	APP_DEBUG("<-- %s-->\r\n<-- %d-->\r\n<-- %s-->\r\n<-- %d-->\r\n",Ql_strstr(packet,"schcmd"),Ql_strstr(packet,"schcmd"),Ql_strstr(packet,"statecmd"),Ql_strstr(packet,"statecmd"));
	if(Ql_strstr(packet,"schcmd")>0)
	{
		request = 1;
	}
	else if(Ql_strstr(packet,"statecmd")>0)
	{
		request = 2;
	}
	p = Ql_strstr(packet,"{");

	if(p>=0)
	{
		APP_DEBUG("<-- RECV message:%s-->\r\n",p);

//		   p = Ql_strstr(mqtt_message,"{\"AMF\":");
//		   APP_DEBUG("<--%c-->\r\n",p[7]);
//		       if (p[7] == '0')
//		       {
//		    	   APP_DEBUG("<--  G P I O    O F F  -->\r\n");
//		       }
//		       else if (p[7] == '1')
//		       {
//		    	   APP_DEBUG("<--  G P I O    O N  -->\r\n");
//		       }



		       controlCmd:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

				//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

				if(Ql_GPIO_GetLevel(pin2) == 1)
				{
					Ql_Sleep(1000);
					if(request == 0)
					{
						APP_SERIAL1("%s\r\n",p);
					}
					else if(request == 1)
					{
						APP_SERIAL1("#%s\r\n",p);
					}
					else if(request == 2)
					{
						APP_SERIAL1("$%s\r\n",p);
					}

				}

				else
				{
					goto controlCmd;
				}

				ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
				Ql_Sleep(1000);
	}
   //QSDK_Get_Str(packet,mqtt_message,3);




}
static void Hdlr_Mqtt_Recv_State(u8* packet)
{
	char strTmp[10];
	s32 ret;

	Ql_Reset(0);

//	ResetCmd:   ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_HIGH);

							//APP_DEBUG("%d\r\n",Ql_GPIO_GetLevel(pin2));

//							if(Ql_GPIO_GetLevel(pin2) == 1)
//							{
//								Ql_Sleep(1000);
//							 APP_SERIAL1("RECV STAT");
//
//							}
//
//							else
//							{
//								goto ResetCmd;
//							}

//							ret=Ql_GPIO_SetLevel(pin1,PINLEVEL_LOW);
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
    BT_DEV_HDL BTDevHdl=0;
    s32 iret;
       u8 srvport[10];

       //command: Set_APN_Param=<APN>,<username>,<password>
       p = Ql_strstr(pData,"Set_APN_Param=");
       if (p)
       {
           Ql_memset(m_apn, 0, 10);
           if (Analyse_Command(pData, 1, '>', m_apn))
           {
               APP_DEBUG("<--APN Parameter Error.-->\r\n");
               return;
           }
           Ql_memset(m_userid, 0, 10);
           if (Analyse_Command(pData, 2, '>', m_userid))
           {
               APP_DEBUG("<--APN Username Parameter Error.-->\r\n");
               return;
           }
           Ql_memset(m_passwd, 0, 10);
           if (Analyse_Command(pData, 3, '>', m_passwd))
           {
               APP_DEBUG("<--APN Password Parameter Error.-->\r\n");
               return;
           }
//           u8 str[11] = {0};
//           BTDevHdl = strtoul(str,NULL,16);
//
//           APP_DEBUG("<--Set APN Parameter Successfully<%s>,<%s>,<%s>.-->\r\n",m_apn,m_userid,m_passwd);
//           iret = RIL_BT_SPP_Send(BTDevHdl,"<--Set APN Parameter Successfully",Ql_strlen("<--Set APN Parameter Successfully"),NULL);
           Ql_strcpy(m_gprsCfg.apnName, m_apn);
           Ql_strcpy(m_gprsCfg.apnUserId, m_userid);
           Ql_strcpy(m_gprsCfg.apnPasswd, m_passwd);
           iret = Ql_SecureData_Store(13,(u8*)&m_gprsCfg,sizeof(m_gprsCfg));
           if (iret !=QL_RET_OK) {
           	APP_DEBUG("<-- Fail to store critical data! Cause:%d -->\r\n", iret);
           }
           eepromInit = '0';
           iret = Ql_SecureData_Store(1,(u8*)&eepromInit,sizeof(eepromInit));
   		if (iret !=QL_RET_OK) {
   			APP_DEBUG("<-- Fail to store critical data! Cause:%d -->\r\n", iret);
   		}

           return;
       }

       //command: Set_Srv_Param=<srv ip>,<srv port>

       p = Ql_strstr(pData,"RESET");
           if (p)
           {
           	Ql_Reset(0);
           }



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
		char strImei[30];
		Ql_memset(strImei, 0x0, sizeof(strImei));

		ret = RIL_GetIMEI(strImei);
		APP_DEBUG("<-- IMEI:%s,ret=%d -->\r\n", strImei,ret);

		ret = RIL_MQTT_QMTSUB(0, 1,strImei, 2,NULL);
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
	  	char *p = NULL;
	  	char topic_sub[100];
	  	char topic_pub[100];
	  	char devIdLocal[20] = "\0";
        if ( flag >= 1 )
        	{
        	int subCount = 0;
        	RIL_NW_GetSignalQuality(&rssi, &ber);
        	Ql_sprintf(m_pCurrentPos + m_remain_len, ",{\"tag\":\"RSSI\", \"value\": %d}]}",rssi);



        	p = Ql_strstr(m_pCurrentPos,"device");
        	int iSearch = 8;
        	p = Ql_strstr(p + iSearch,"\"");

        	iSearch = 1;
        	int i = 0;
        	while(*(p + iSearch) != '\"')
        	{
        		devIdLocal[i] = *(p + iSearch);
        		i++;
        		iSearch++;
        	}
        	devIdLocal[i] = '\0';
			APP_DEBUG("<-- %s -->\r\n", p);
			APP_DEBUG("<-- %s -->\r\n", devIdLocal);

			if(Ql_strcmp(devIdLocal,devId))
				{
					p = topic_sub;
					Ql_sprintf(p, "devicesOut/%s/autocmd",devIdLocal);

					ret = RIL_MQTT_QMTSUB(0, 1,topic_sub, 2,NULL);
							APP_DEBUG("<-- mqtt subscribe, ret=%d -->\r\n",ret);
							if(ret ==0)
							{
								subCount++;
							 // m_tcp_state = STATE_MQTT_PUB;
							}
							else
							{
							   m_tcp_state = STATE_MQTT_CLOSE;
							}

					p = topic_sub;
					Ql_sprintf(p, "devicesOut/%s/schcmd",devIdLocal);

					ret = RIL_MQTT_QMTSUB(0, 1,topic_sub, 2,NULL);
							APP_DEBUG("<-- mqtt subscribe, ret=%d -->\r\n",ret);
							if(ret ==0)
							{
								subCount++;
							 // m_tcp_state = STATE_MQTT_PUB;
							}
							else
							{
							   m_tcp_state = STATE_MQTT_CLOSE;
							}
					p = topic_sub;
					Ql_sprintf(p, "devicesOut/%s/statecmd",devIdLocal);

					ret = RIL_MQTT_QMTSUB(0, 1,topic_sub, 2,NULL);
							APP_DEBUG("<-- mqtt subscribe, ret=%d -->\r\n",ret);
							if(ret ==0)
							{
								subCount++;
							 // m_tcp_state = STATE_MQTT_PUB;
							}
							else
							{
							   m_tcp_state = STATE_MQTT_CLOSE;
							}
				}


			if(subCount == 3)
			{
				Ql_sprintf(devId, devIdLocal);
				//devId = devIdLocal;
			}

			if(*m_pCurrentPos == '#')
			{
				m_pCurrentPos++;
				length = Ql_strlen(m_pCurrentPos);
				p = topic_pub;
				Ql_sprintf(p, "devicesIn/%s/schresp",devIdLocal);
				ret = RIL_MQTT_QMTPUB(0, 1,1,0, topic_pub,m_pCurrentPos,length);
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

			else if(*m_pCurrentPos == '$')
			{
				m_pCurrentPos++;
				length = Ql_strlen(m_pCurrentPos);
				p = topic_pub;
				Ql_sprintf(p, "devicesIn/%s/stateresp",devIdLocal);
				ret = RIL_MQTT_QMTPUB(0, 1,1,0, topic_pub,m_pCurrentPos,length);
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

			else
			{
				length = Ql_strlen(m_pCurrentPos);
				p = topic_pub;
				Ql_sprintf(p, "devicesIn/%s/data",devIdLocal);
				ret = RIL_MQTT_QMTPUB(0, 1,1,0, topic_pub,m_pCurrentPos,length);
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

static s32 ATResponse_Handler(char* line, u32 len, void* userData)
{
    Ql_UART_Write(UART_PORT1, (u8*)line, len);

    if (Ql_RIL_FindLine(line, len, "OK"))
    {
        return  RIL_ATRSP_SUCCESS;
    }
    else if (Ql_RIL_FindLine(line, len, "ERROR"))
    {
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CME ERROR"))
    {
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CMS ERROR:"))
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}


static void BT_Callback(s32 event, s32 errCode, void* param1, void* param2)
{
    ST_BT_BasicInfo *pstNewBtdev = NULL;
    ST_BT_BasicInfo *pstconBtdev = NULL;
    s32 ret = RIL_AT_SUCCESS;
    s32 connid = -1;

    switch(event)
    {
        case MSG_BT_SCAN_IND :

            if(URC_BT_SCAN_FINISHED== errCode)
            {
                APP_DEBUG("Scan is over.\r\n");
                //APP_DEBUG("Pair/Connect if need.\r\n");
                RIL_BT_GetDevListInfo();
                //here obtain ril layer table list for later use
                g_dev_info = RIL_BT_GetDevListPointer();
                g_pair_search = TRUE;
            }
            if(URC_BT_SCAN_FOUND == errCode)
            {
                pstNewBtdev = (ST_BT_BasicInfo *)param1;
                //you can manage the scan device here,or you don't need to manage it ,for ril layer already handle it
                APP_DEBUG("BTHdl[0x%08x] Addr[%s] Name[%s]\r\n",pstNewBtdev->devHdl,pstNewBtdev->addr,pstNewBtdev->name);

            }
            break;
         case MSG_BT_PAIR_IND :
              if(URC_BT_NEED_PASSKEY == errCode)
              {
                    //must ask for pincode;
                    pstconBtdev = (ST_BT_BasicInfo*)param1;
                    APP_DEBUG("Pair device BTHdl: 0x%08x\r\n",pstconBtdev->devHdl);
                    APP_DEBUG("Pair device addr: %s\r\n",pstconBtdev->addr);
                    APP_DEBUG("Waiting for pair confirm with pinCode...\r\n");
              }
              else if(URC_BT_NO_NEED_PASSKEY == errCode)
              {
                    //no need pincode
                    pstconBtdev = (ST_BT_BasicInfo*)param1;
                    Ql_strncpy(pinCode,(char*)param2,BT_PIN_LEN);
					pinCode[BT_PIN_LEN-1] = 0;
                    APP_DEBUG("Pair device BTHdl: 0x%08x\r\n",pstconBtdev->devHdl);
                    APP_DEBUG("Pair device addr: %s\r\n",pstconBtdev->addr);
                    APP_DEBUG("Pair pin code: %s\r\n",pinCode);
                    APP_DEBUG("pair confirm automatically\r\n");
                    RIL_BT_PairConfirm(TRUE,NULL);
              }

            break;
         case MSG_BT_PAIR_CNF_IND :
            if(URC_BT_PAIR_CNF_SUCCESS == errCode)
            {
                pstconBtdev = (ST_BT_BasicInfo*)param1;
                APP_DEBUG("Paired successful.\r\n");
            }
            else
            {
                APP_DEBUG("Paired failed.\r\n");
            }
            break;

         case MSG_BT_SPP_CONN_IND :

              if(URC_BT_CONN_SUCCESS == errCode )
              {

                Ql_memcpy(&(BTSppDev1.btDevice),(ST_BT_BasicInfo *)param1,sizeof(ST_BT_BasicInfo));

                APP_DEBUG("Connect successful.\r\n");
              }
              else
              {
                APP_DEBUG("Connect failed.\r\n");
              }

              break;

         case MSG_BT_RECV_IND :

             connid = *(s32 *)param1;
             pstconBtdev = (ST_BT_BasicInfo *)param2;
             Ql_memcpy(&pSppRecHdl,pstconBtdev,sizeof(pSppRecHdl));
             APP_DEBUG("SPP receive data from BTHdl[0x%08x].\r\n",pSppRecHdl.devHdl);
             u32 actualReadLen = 0;

			 Ql_memset(m_RxBuf_BL,0,sizeof(m_RxBuf_BL));
			 if(pSppRecHdl.devHdl == 0)
			 {
				 APP_DEBUG("No sender yet.\r\n");
				 break;
			 }
			 ret = RIL_BT_SPP_Read(pSppRecHdl.devHdl, m_RxBuf_BL, sizeof(m_RxBuf_BL),&actualReadLen);
			 if(RIL_AT_SUCCESS != ret)
			 {
				 APP_DEBUG("Read failed.ret=%d\r\n",ret);
				 break;
			 }
			 if(actualReadLen == 0)
			 {
				 APP_DEBUG("No more data.\r\n");
				 break;
			 }
			 APP_DEBUG("BTHdl[%x][len=%d]:\r\n%s\r\n",pSppRecHdl.devHdl,actualReadLen,m_RxBuf_BL);
			 proc_handle(m_RxBuf_BL,actualReadLen);
             break;

         case MSG_BT_PAIR_REQ:

              if(URC_BT_NEED_PASSKEY == errCode)
              {
                   //must ask for pincode;
                    pstconBtdev = (ST_BT_BasicInfo*)param1;
                    APP_DEBUG("Pair device BTHdl: 0x%08x\r\n",pstconBtdev->devHdl);
                    APP_DEBUG("Pair device addr: %s\r\n",pstconBtdev->addr);
                    APP_DEBUG("Waiting for pair confirm with pinCode...\r\n");
              }

              if(URC_BT_NO_NEED_PASSKEY == errCode)
              {
                    //no need pincode
                    pstconBtdev = (ST_BT_BasicInfo*)param1;
                    Ql_strncpy(pinCode,(char*)param2,BT_PIN_LEN);
					pinCode[BT_PIN_LEN-1] = 0;
                    APP_DEBUG("Pair device BTHdl: 0x%08x\r\n",pstconBtdev->devHdl);
                    APP_DEBUG("Pair device addr: %s\r\n",pstconBtdev->addr);
                    APP_DEBUG("Pair pin code: %s\r\n",pinCode);
                    APP_DEBUG("pair confirm automatically\r\n");
                    RIL_BT_PairConfirm(TRUE,NULL);
              }

            break;

        case  MSG_BT_CONN_REQ :

            pstconBtdev = (ST_BT_BasicInfo*)param1;
            APP_DEBUG("Get a connect req\r\n");
            APP_DEBUG("BTHdl: 0x%08x\r\n",pstconBtdev->devHdl);
            APP_DEBUG("Addr: %s\r\n",pstconBtdev->addr);
            APP_DEBUG("Name: %s\r\n",pstconBtdev->name);

            APP_DEBUG("Waiting connect accept.\r\n");
            ret = RIL_BT_ConnAccept(1,BT_SPP_CONN_MODE_BUF);

			if(RIL_AT_SUCCESS != ret)
			{
				APP_DEBUG("Send connect accept failed.\r\n");
			}
			else
			{
				APP_DEBUG("Send connect accept successful.\r\n");
			}

       case  MSG_BT_DISCONN_IND :

             if(URC_BT_DISCONNECT_PASSIVE == errCode || URC_BT_DISCONNECT_POSITIVE == errCode)
             {
                APP_DEBUG("Disconnect ok!\r\n");
             }



          break;


        default :
            break;
    }
}



static void BT_COM_Demo(void)
{
    s32 cur_pwrstate = 0 ;
    s32 ret = RIL_AT_SUCCESS ;
	s32 visb_mode = -1;

    //1 power on BT device
    ret = RIL_BT_GetPwrState(&cur_pwrstate);

    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("Get BT device power status failed.\r\n");
        //if run to here, some erros maybe occur, need reset device;
        return;
    }

    if(1 == cur_pwrstate)
    {
        APP_DEBUG("BT device already power on.\r\n");
    }
    else if(0 == cur_pwrstate)
    {
       ret = RIL_BT_Switch(BL_POWERON);
       if(RIL_AT_SUCCESS != ret)
       {
            APP_DEBUG("BT power on failed,ret=%d.\r\n",ret);
            return;
       }
       APP_DEBUG("BT device power on.\r\n");
    }

//    char strImei[30];
//	Ql_memset(strImei, 0x0, sizeof(strImei));
//
//	ret = RIL_GetIMEI(strImei);
//	if(RIL_AT_SUCCESS != ret)
//	    {
//	        APP_DEBUG("Get IMEI failed.\r\n");
//	        return;
//	    }
//	ret = RIL_BT_SetName(bt_name,BT_NAME_LEN);
//	if(RIL_AT_SUCCESS != ret)
//		    {
//		        APP_DEBUG("Set BT device name failed.\r\n");
//		        //return;
//		    }
    Ql_memset(bt_name,0,sizeof(bt_name));
    ret = RIL_BT_GetName(bt_name,BT_NAME_LEN);

    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("Get BT device name failed.\r\n");
        return;
    }

    APP_DEBUG("BT device name is: %s.\r\n",bt_name);



    //2. Register a callback function for BT operation.
    ret = RIL_BT_Initialize(BT_Callback);

    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("BT initialization failed.\r\n");
        return;
    }
    APP_DEBUG("BT callback function register successful.\r\n");

     ret = RIL_BT_GetVisble(&visb_mode);
	 if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("Get BT visble failed.\r\n");
        return;
    }
	APP_DEBUG("BT visble mode is: %d.\r\n",visb_mode);

    //3 Now the BT device is power on, start scan operation
    //parameters:           maxDevCount, class of device, timeout
   // ret = RIL_BT_StartScan(MAX_BLDEV_CNT, DEFA_CON_ID, 40);
//    if(RIL_AT_SUCCESS != ret)
//    {
//        APP_DEBUG("BT scan failed.\r\n");
//        return;
//    }
//    APP_DEBUG("BT scanning device...\r\n");
}




#endif // __EXAMPLE_TCPSSL__




