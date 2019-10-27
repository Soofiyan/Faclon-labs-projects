
#ifdef __TEST_HTTPS_OCPU__3

#include "custom_feature_def.h"
#include "ql_stdlib.h"
#include "ql_common.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_uart.h"
#include "ql_uart.h"
#include "ql_gprs.h"
#include "ql_socket.h"
#include "ql_timer.h"
#include "ril_network.h"
#include "ril.h"
#include "ril_util.h"

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


/*****************************************************************
* timer param
******************************************************************/
#define TCP_TIMER_ID         TIMER_ID_USER_START
#define TCP_TIMER_PERIOD     2000

typedef struct{
char *prefix;
s32 data;
}ST_AT_Httparam;

#define RECV_DATA_LENGTH     1024
u8 recv_data [RECV_DATA_LENGTH];

static u8 *send_data = "123456";

/*****************************************************************
* APN Param
******************************************************************/

//#define HOST_NAME       "220.180.239.201"
#define HOST_NAME     "https://freeiotcloud.in/hello.htm"
#define PORT            443
#define CONTEXT          0

static u8 m_apn[10] = "WWW";
static u8 m_userid[10] = "";
static u8 m_passwd[10] = "";

static u8 ctxindex=  0;
static u8 ssl_version= 4;
static u8 *cipher_suite=  "0xFFFF";
static u8 sec_level= 2;
static u8 ssid= 0;
static u8 mode= 0;

static u8 write_flag = 0;

typedef enum{
	STATE_NW_GET_SIMSTATE,
	STATE_NW_QUERY_STATE,
	STATE_GPRS_CONTEXT,
	STATE_GPRS_ACTIVATE,
	STATE_SSL_CFG,
	STATE_SSL_CERTIFICATE_DEL,
	STATE_SSL_CERTIFICATE_WRITE,
	STATE_SSL_OPEN,
	STATE_SSL_SEND,
	STATE_SSL_CLOSE,
	STATE_GPRS_DEACTIVATE,
	STATE_TOTAL_NUM
}Enum_TCPSTATE;
static u8 m_tcp_state = STATE_NW_GET_SIMSTATE;


/*****************************CERTIFICATE*****************************/
#define http_cacert_len    1428
#define http_cacert   "-----BEGIN CERTIFICATE-----\n\
MIID8TCCAtmgAwIBAgIJAMeOoLh/N2yRMA0GCSqGSIb3DQEBBQUAMIGOMQswCQYD\n\
VQQGEwJDTjEOMAwGA1UECAwFQU5IVUkxDjAMBgNVBAcMBUhFRkVJMRAwDgYDVQQK\n\
DAdRVUVDVEVMMQwwCgYDVQQLDANUTFcxGDAWBgNVBAMMDzIyMC4xODAuMjM5LjIw\n\
MTElMCMGCSqGSIb3DQEJARYWZGFpc3kudGlhbkBxdWVjdGVsLmNvbTAeFw0xNjA1\n\
MTEwMjE4MzlaFw0yNjA1MDkwMjE4MzlaMIGOMQswCQYDVQQGEwJDTjEOMAwGA1UE\n\
CAwFQU5IVUkxDjAMBgNVBAcMBUhFRkVJMRAwDgYDVQQKDAdRVUVDVEVMMQwwCgYD\n\
VQQLDANUTFcxGDAWBgNVBAMMDzIyMC4xODAuMjM5LjIwMTElMCMGCSqGSIb3DQEJ\n\
ARYWZGFpc3kudGlhbkBxdWVjdGVsLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEP\n\
ADCCAQoCggEBAMM9MCHLhLDP0SKcwjUDjcNutth/7yfGA/E8vhD4o/9MQpX5NF4r\n\
4GPtM/gTLWFT4wKwM/e8JidMxNB0ZUc5g5H6igVeukkWukS4/NOWnIUtU7V1Xkgi\n\
q52xldWKNmmYomTRddtBJ5mXYjC9eyysOF7Cdo+1fpIeM7cYA/GDKzuP9H2Z+F3z\n\
tKGR4GDVdlF3puFJs3e/qRVeLcQg13NXnm9wTBnQofd6GdxEOv/LI81HcvioVbX9\n\
lYABOsX9DoaatCKhsQ+HWWca36VTj8xMI//AW0TqgWTe0RZe9yd7YJF/gP4zmNHG\n\
Qa2GIM6i4IqJXLQqq58liAoEZQ0M2xbeD5cCAwEAAaNQME4wHQYDVR0OBBYEFHay\n\
qBqjjhBmJoZz6YHPapmsSPzbMB8GA1UdIwQYMBaAFHayqBqjjhBmJoZz6YHPapms\n\
SPzbMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEFBQADggEBAGfzdwipMG+E6cCg\n\
H6OJgOD8Rv4bzvXO1muoYYa6eCCR0J0vvxJNptzpGEFKAA7FssXZ8kqGL4gLHS5l\n\
Vx7IeHQs+1rKbHbpHwQkm4X4ml5z1VWiEkeDM8tQYtzMHP7l4u5Q6zmZ4iAvN5np\n\
Dm0dbZWbXnnqykDW8atztdKTweceSt5TvMEFhqmN+9wWhbzU3FDO0BiXH74+EvXr\n\
WMpK0rRMezeGHgDuOFr/k/w9I9Tv3MngmIa0QqemymHCHLlG0+cfHGmmg0OnKHIx\n\
MdT33vcyindRL6+akbMcjmj+cpgnTQjIra3cHaxyjvCVM4UEIKQk9FG0S/9Mw7aX\n\
vh/Y4V8=\n\
-----END CERTIFICATE-----\n"

#define  http_clientcert_len    1452
#define http_clientcert "-----BEGIN CERTIFICATE-----\n\
MIIEAzCCAuugAwIBAgIBATANBgkqhkiG9w0BAQUFADCBjjELMAkGA1UEBhMCQ04x\n\
DjAMBgNVBAgMBUFOSFVJMQ4wDAYDVQQHDAVIRUZFSTEQMA4GA1UECgwHUVVFQ1RF\n\
TDEMMAoGA1UECwwDVExXMRgwFgYDVQQDDA8yMjAuMTgwLjIzOS4yMDExJTAjBgkq\n\
hkiG9w0BCQEWFmRhaXN5LnRpYW5AcXVlY3RlbC5jb20wHhcNMTYwNTExMDIzODE5\n\
WhcNMTcwNTExMDIzODE5WjB+MQswCQYDVQQGEwJDTjEOMAwGA1UECAwFQU5IVUkx\n\
EDAOBgNVBAoMB1FVRUNURUwxDDAKBgNVBAsMA1RMVzEYMBYGA1UEAwwPMjIwLjE4\n\
MC4yMzkuMjAxMSUwIwYJKoZIhvcNAQkBFhZkYWlzeS50aWFuQHF1ZWN0ZWwuY29t\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzz7aYFvXcwwKlmYCiQwI\n\
K5SibMro1UgVKjiYF7uialJPQWDekI7E0DCFL+XZtXiNCvainkewqDEW4Y+UGu+v\n\
as1Qm2otuLzycm61WZyHEYxQbeq5guIiqQVMjwsHxrVDaEcep7ePBTyND3DxE9Up\n\
AZjXlcGEd0qbWZS8NXlSDs8IBENY0tjdXCk1QcRWnJcjxK/U84YHku9DRD42b+yg\n\
bwRZo3BDpYEX4V5oiDou8ZBgt5oFzzLaqUcaRpyT4ixaxjeh2wRCoNblo/583TEp\n\
x908Vz5mrD/rYl0J33ot+t2JGioqPuDEZM+yZ0wJ1WTPhmOKJKJOIjNjEvgtOiB4\n\
JQIDAQABo3sweTAJBgNVHRMEAjAAMCwGCWCGSAGG+EIBDQQfFh1PcGVuU1NMIEdl\n\
bmVyYXRlZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQUIKDlAbgehyy4xW0yp5a6/aS+\n\
Wj0wHwYDVR0jBBgwFoAUDpm2bWw+eaTHxXzo5s3vCsWvUPMwDQYJKoZIhvcNAQEF\n\
BQADggEBAH//fkngvl+WkCWP2oAay/UWyim9zYTUWJSBK1wvhWfz+BXOIpsRTLdG\n\
hxGT8vT09l81bypeDgM/baiz50fBfT6Z541ZBxC6AelIoG4jFwYuIqYRxD0k41V1\n\
qnx/2il6q7ZN1AgCOMB1FGUXnXMn/pwkzuEOBztt9YSoJIc8F3M1Vc0lW4ZyKAwm\n\
N0jxHW2/7wJjLTy8n3eRMEn9McpMO0zAzhV+oSIQvnvu1SrozcOpqT9l5p+mH1Vo\n\
LPk0vCsR4ZkYKbNo5cf+Jr6Yzal3KcRqaQZeTRoXZMHMSF2ZcDdrN+kiaYe4BkoK\n\
0VY+tVKmIkru1aGNQ18MDSCp7R8nqyw=\n\
-----END CERTIFICATE-----\n"

#define http_clientkey_len    1708
#define  http_clientkey   "-----BEGIN PRIVATE KEY-----\n\
MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQDPPtpgW9dzDAqW\n\
ZgKJDAgrlKJsyujVSBUqOJgXu6JqUk9BYN6QjsTQMIUv5dm1eI0K9qKeR7CoMRbh\n\
j5Qa769qzVCbai24vPJybrVZnIcRjFBt6rmC4iKpBUyPCwfGtUNoRx6nt48FPI0P\n\
cPET1SkBmNeVwYR3SptZlLw1eVIOzwgEQ1jS2N1cKTVBxFaclyPEr9TzhgeS70NE\n\
PjZv7KBvBFmjcEOlgRfhXmiIOi7xkGC3mgXPMtqpRxpGnJPiLFrGN6HbBEKg1uWj\n\
/nzdMSnH3TxXPmasP+tiXQnfei363YkaKio+4MRkz7JnTAnVZM+GY4okok4iM2MS\n\
+C06IHglAgMBAAECggEBAKTYiNk2lGQlgtJop8Gc+W641o2UxKjJolQoGgperGzH\n\
tdT6GW6AsVosDfSwboBjOEUtMuKVgZX3Hg0iqJrYZf6c+23zghS87lhJaSSzVdiG\n\
dH9Jwm+yMgGhfmkVTAUpr0llsKOVZUS0CjvrCUdOOUTU7z5mZFiC1pjlruMV8khl\n\
xJbohjSdoJ6KTSZyLVaiJqLNJCPCiwM7Bv5mxhL6GgWvRUUGKftkOdyLwy4msIcW\n\
nbgIyy6A/6/PJUKSIQdsX8JFBxs/n6QdjXONd69RiH3QpkOZlBalnO0W3EL5x75u\n\
kh3D0SfIf47F96PWamdtPTbjcMCH7bB6w4ogRAL4t+0CgYEA7NkFh7jnmqaUyHpL\n\
N9kTpuaFkivbE1eZ8NjKsnSWiARX+8mUuiiG2tDUcQfSWsD8/P6ayUGpjo0zl66m\n\
RJNs864LqOuIbQ7ISuTAi1+IquUwDowlHXnlCnbWCaQfQ8IgCd4jBeG42eiNEjKT\n\
fr4VH/k/bbpHEH2o9VzlridIjUMCgYEA4AEJZHd9qXHCvvlcfZ+c0l3iQLF8oXRx\n\
HxWEQmXkW+xCsvmZdgECwfgg9B5PpPAu8q40jFLwSuSI6h8eLGm55Eymj6tK2UAF\n\
BUJHndq+PLManOQrbubEmjgnMAkJtHD/dRtISlT8Y5d0Rx+PPxX/NTDctVdBLXbY\n\
+Sqak5raGncCgYEAhiSMS0hgdGiwj7Mj8ueRh8+8CwOnupa863n9o4EA2NyM8GBF\n\
SgI2DqyEBdiGPTxcjPWuuRnlbIVwmRIjvWc6J/GFTRDJXesnabORkd5zy0avJy4c\n\
v0sQfBK+OwunYXsLJkuXznb/ePuLGqlmfDwwPsGuOPlt0ls5XG5W74H0R30CgYA6\n\
hArQE+bfvLgC62Ed7/QngB76h2LnSmPCmvxR3Awrdyx1VH6iNOFjik4Rd1mW3Kdm\n\
/dr8TS44Yjrh7f8T2wqUePGJ1lVXK0IkfYv30IyhjqgFFBXEgsQZBVI3WiUO3fXd\n\
waLAyDKmUYouijABat2gJ4OAW6rLFaDYdiirJmiTQQKBgQDNwOVfQNWFxFIPnk2V\n\
+OpCpuLL3oFEglDM+0QRx9ZgoOCv72cJCs2pTipgFgftZaA/GFJ8z2PwPhiglLWc\n\
sdAUubkIdRoytvZ2HJ6DijL6EOvjvTAjwXXMzAYHHTeOtOz6x/QWcswWQB8O6TeI\n\
4wrCFdY87PN4YpI+ToLvWHIp0A==\n\
-----END PRIVATE KEY-----\n"

/*****************************************************************
* uart callback function
******************************************************************/
static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);
static void Callback_Timer(u32 timerId, void* param);

static s32 ATResponse_SSL_handler_open(char* line, u32 len, void* userdata);
static s32 ATResponse_SSL_handler_send(char* line, u32 len, void* userdata);
static s32 ATResponse_SSL_handler_recv(char* line, u32 len, void* userdata);
static s32 ATResponse_SSL_handler_close(char* line, u32 len, void* userdata);
static s32 ATResponse_SSL_handler_CERTIFICATE_WRITE(char* line, u32 len, void* userdata);
static s32 ATResponse_SSL_handler_CERTIFICATE_DEL(char* line, u32 len, void* userdata);
static s32 ATResponse_SSL_handler_CERTIFICATE_READ(char* line, u32 len, void* userdata);


s32 RIL_SSL_QSSLCERTIFICATE_WRITE(u8 ctxindex);
s32 RIL_SSL_QSSLCERTIFICATE_DEL(void);
s32 RIL_SSL_QSSLCERTIFICATE_READ(void);

s32 RIL_SSL_QSSLCFG(u8 ctxindex, u8 ssl_version,u8 *cipher_suite,u8 sec_level);
s32 RIL_SSL_QSSLOPEN(u8* hostName, u32 port,u8 ssid,u8 ctxindex, u8 mode) ;
s32 RIL_SSL_QSSLSEND(u8 ssid,u8 *send_data) ;
s32 RIL_SSL_QSSLCLOSE(u8 ssid) ;
s32 RIL_SSL_QSSLRECV(u8 ssid,u8 *recv_data) ;



void proc_main_task(s32 taskId)
{
    s32 ret;
    ST_MSG msg;


    // Register & open UART port
    Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
    Ql_UART_Register(UART_PORT2, CallBack_UART_Hdlr, NULL);
    Ql_UART_Open(UART_PORT1, 115200, FC_NONE);
    Ql_UART_Open(UART_PORT2, 115200, FC_NONE);


    APP_DEBUG("<--OpenCPU: HTTPS  test.-->\r\n");
    Ql_Timer_Register(TCP_TIMER_ID, Callback_Timer, NULL);
    Ql_Timer_Start(TCP_TIMER_ID, TCP_TIMER_PERIOD, TRUE);


    while(TRUE)
    {
        Ql_OS_GetMessage(&msg);
        switch(msg.message)
        {

        case MSG_ID_RIL_READY:
            APP_DEBUG("<-- RIL is ready -->\r\n");
            Ql_RIL_Initialize();
            break;
        case MSG_ID_URC_INDICATION:
        switch (msg.param1)
        {
		     case URC_SSL_RECV_IND:
		      APP_DEBUG("<-- SSL RECV DATA:ssid = %d -->\r\n", msg.param2); //ssid
			  ret =RIL_SSL_QSSLRECV(msg.param2,recv_data);
			  APP_DEBUG("<-- Set SSL RECV, recv_data = %s,ret=%d -->\r\n",recv_data, ret);
			 // m_tcp_state = STATE_SSL_CLOSE;
			  break;
        }
        default:
        break;
       }
    }
}

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{

}

static void Callback_Timer(u32 timerId, void* param)
{
  s32 ret ;

  if (TCP_TIMER_ID == timerId)
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
			  APP_DEBUG("<--SIM card status is unnormal!-->\r\n");
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
		 m_tcp_state = STATE_SSL_CERTIFICATE_DEL;
		}
		break;
	  }

	  case STATE_SSL_CERTIFICATE_DEL:
	  {
		ret =RIL_SSL_QSSLCERTIFICATE_DEL();
		APP_DEBUG("<-- DEL SSL CERTIFICATE, ret=%d -->\r\n", ret);
		if(ret ==0)
		{
		  APP_DEBUG("<-- DEL certificate is ok-->\r\n");
		  m_tcp_state = STATE_SSL_CERTIFICATE_WRITE;
		}
		else
		{
		   APP_DEBUG("<-- There is no certificate  plz write certificate-->\r\n");
		   m_tcp_state = STATE_SSL_CERTIFICATE_WRITE;
		}
		break;
	  }

	  case STATE_SSL_CERTIFICATE_WRITE:
	  {
		ret =RIL_SSL_QSSLCERTIFICATE_WRITE(100);
		APP_DEBUG("<-- Set SSL CERTIFICATE, ret=%d -->\r\n", ret);
		if(ret ==0)
		{
		  m_tcp_state = STATE_SSL_CFG;
		}
		else
		{
		   write_flag = 0;

		   APP_DEBUG("<-- Set SSL CERTIFICATE ERROR, ret=%d -->\r\n", ret);
		   m_tcp_state = STATE_TOTAL_NUM;
		}
		break;
	  }

	  case STATE_SSL_CFG:
	  {
        ret =RIL_SSL_QSSLCFG(ctxindex, ssl_version,cipher_suite,sec_level);
        APP_DEBUG("<-- Set SSL CFG, ret=%d -->\r\n", ret);
		if(ret ==0)
		{
	        m_tcp_state = STATE_SSL_OPEN;
	    }
		else
		{
			m_tcp_state = STATE_TOTAL_NUM;
		}
		break;
	  }

      case STATE_SSL_OPEN:
	  {
        ret = RIL_SSL_QSSLOPEN(HOST_NAME,PORT,ssid,ctxindex, mode);
        APP_DEBUG("<-- Set SSL OPEN, ret=%d -->\r\n", ret);
		if(ret ==0)
		{
	      m_tcp_state = STATE_SSL_SEND;
	    }
		else
		{
          m_tcp_state = STATE_GPRS_DEACTIVATE;
		}
		break;
	  }
	  case STATE_SSL_SEND:
	  {
        ret = RIL_SSL_QSSLSEND(ssid,send_data);
        APP_DEBUG("<-- Set SSL SEND, ret=%d -->\r\n", ret);
		if(ret < 0)
		{
	      m_tcp_state = STATE_TOTAL_NUM;
	    }
		break;
	  }
	  case STATE_SSL_CLOSE:
	  {
        ret = RIL_SSL_QSSLCLOSE(ssid);
        APP_DEBUG("<-- Set SSL CLOSE, ret=%d -->\r\n", ret);
	    m_tcp_state = STATE_GPRS_DEACTIVATE;
		break;
	  }
	  case STATE_GPRS_DEACTIVATE:
	  {
        ret = RIL_NW_ClosePDPContext();
        APP_DEBUG("<-- Set GPRS DEACTIVATE, ret=%d -->\r\n", ret);
		if(ret ==0)
		{
	      m_tcp_state = STATE_TOTAL_NUM;
	    }
		break;
	  }
    }
   }

}




s32 RIL_SSL_QSSLCERTIFICATE_DEL(void)
{
	s32  ret = RIL_AT_SUCCESS;
	s32  errCode = RIL_AT_FAILED;
	char strAT[200];

	 Ql_memset(strAT, 0, sizeof(strAT));
	 Ql_sprintf(strAT, "AT+QSECDEL=\"NVRAM:CA0\"\n");
	 ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_CERTIFICATE_DEL,&errCode,0);
	 APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
	 if (ret != RIL_AT_SUCCESS)
	 {
		if (RIL_AT_FAILED == errCode)
		{
			return ret;
		}
		else
		{
		   return errCode;
		}
	 }

	 Ql_memset(strAT, 0, sizeof(strAT));
	 Ql_sprintf(strAT, "AT+QSECDEL=\"NVRAM:CC0\"\n");
	 ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_CERTIFICATE_DEL,&errCode,0);
	 APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
	 if (ret != RIL_AT_SUCCESS)
	 {
		if (RIL_AT_FAILED == errCode)
		{
			return ret;
		}
		else
		{
		   return errCode;
		}
	 }

	 Ql_memset(strAT, 0, sizeof(strAT));
	 Ql_sprintf(strAT, "AT+QSECDEL=\"NVRAM:CK0\"\n");
	 ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_CERTIFICATE_DEL,&errCode,0);
	 APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
	 if (ret != RIL_AT_SUCCESS)
	 {
		if (RIL_AT_FAILED == errCode)
		{
			return ret;
		}
		else
		{
		   return errCode;
		}
	 }
	 return RIL_AT_SUCCESS;
}



s32 RIL_SSL_QSSLCERTIFICATE_WRITE(u8 timeout)
{
	s32  ret = RIL_AT_SUCCESS;
	s32  errCode = RIL_AT_FAILED;
	char strAT[200];

	 Ql_memset(strAT, 0, sizeof(strAT));
	 write_flag= 1;
	 Ql_sprintf(strAT, "AT+QSECWRITE=\"NVRAM:CA0\",%d,%d\n",http_cacert_len,timeout);
	 ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_CERTIFICATE_WRITE,&errCode,0);
	 APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
	 if (ret != RIL_AT_SUCCESS)
	 {
		if (RIL_AT_FAILED == errCode)
		{
			return ret;
		}
		else
		{
		   return errCode;
		}
	 }

	 Ql_memset(strAT, 0, sizeof(strAT));
	 write_flag= 2;
	 Ql_sprintf(strAT, "AT+QSECWRITE=\"NVRAM:CC0\",%d,%d\n",http_clientcert_len,timeout);
	 ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_CERTIFICATE_WRITE,&errCode,0);
	 APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
	 if (ret != RIL_AT_SUCCESS)
	 {
		if (RIL_AT_FAILED == errCode)
		{
			return ret;
		}
		else
		{
		   return errCode;
		}
	 }

	 Ql_memset(strAT, 0, sizeof(strAT));
	 write_flag= 3;
	 Ql_sprintf(strAT, "AT+QSECWRITE=\"NVRAM:CK0\",%d,%d\n",http_clientkey_len,timeout);
	 ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_CERTIFICATE_WRITE,&errCode,0);
	 APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
	 if (ret != RIL_AT_SUCCESS)
	 {
		if (RIL_AT_FAILED == errCode)
		{
			return ret;
		}
		else
		{
		   return errCode;
		}
	 }
	 return RIL_AT_SUCCESS;
}

s32 RIL_SSL_QSSLCFG(u8 ctxindex, u8 ssl_version,u8 *cipher_suite,u8 sec_level)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];

	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSSLCFG=\"ignorertctime\",1\n");

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }

    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSSLCFG=\"sslversion\",%d,%d\n",ctxindex,ssl_version);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }

	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSSLCFG=\"ciphersuite\",%d,\"%s\"\n",ctxindex,cipher_suite);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }

	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSSLCFG=\"seclevel\",%d,%d\n",ctxindex,sec_level);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }

	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSSLCFG=\"cacert\",%d,\"NVRAM:CA0\"\n",ctxindex);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }

	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSSLCFG=\"clientcert\",%d,\"NVRAM:CC0\"\n",ctxindex);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }

	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSSLCFG=\"clientkey\",%d,\"NVRAM:CK0\"\n",ctxindex);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }

    return ret;
}



s32 RIL_SSL_QSSLCERTIFICATE_READ(void)
{
	s32  ret = RIL_AT_SUCCESS;
    ST_AT_Httparam httpParam;
	char strAT[200];
    httpParam.prefix="+QSECREAD:";
    httpParam.data = 255;
	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSECREAD=\"NVRAM:CA0\"\n");
	ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_CERTIFICATE_READ,&httpParam,0);
	APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
	if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    else if(1 != httpParam.data)
    {
        APP_DEBUG("\r\n<-- SSL READ failure, =%d -->\r\n", httpParam.data);
        return QL_RET_ERR_RIL_FTP_OPENFAIL;
    }

   	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSECREAD=\"NVRAM:CC0\"\n");
	ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_CERTIFICATE_READ,&httpParam,0);
	APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
	if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    else if(1 != httpParam.data)
    {
        APP_DEBUG("\r\n<-- SSL READ failure, =%d -->\r\n", httpParam.data);
        return QL_RET_ERR_RIL_FTP_OPENFAIL;
    }

		Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSECREAD=\"NVRAM:CK0\"\n");
	ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_CERTIFICATE_READ,&httpParam,0);
	APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
	if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    else if(1 != httpParam.data)
    {
        APP_DEBUG("\r\n<-- SSL READ failure, =%d -->\r\n", httpParam.data);
        return QL_RET_ERR_RIL_FTP_OPENFAIL;
    }

	 return RIL_AT_SUCCESS;
}

s32 RIL_SSL_QSSLOPEN(u8* hostName, u32 port,u8 ssid,u8 ctxindex, u8 mode)
{
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_Httparam httpParam;
    char strAT[200];
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSSLOPEN=%d,%d,\"%s\",%d,%d\n", ssid,ctxindex,hostName, port,mode);
    httpParam.prefix="+QSSLOPEN:";
    httpParam.data = 255;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_open,(void* )&httpParam,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    else if(0 != httpParam.data)
    {
        APP_DEBUG("\r\n<-- SSL OPEN  failure, =%d -->\r\n", httpParam.data);
        return QL_RET_ERR_RIL_FTP_OPENFAIL;
    }

    return ret;
}


s32 RIL_SSL_QSSLSEND(u8 ssid,u8 *send_data)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
	s32 length;
    Ql_memset(strAT, 0, sizeof(strAT));
	length = Ql_strlen(send_data);
    Ql_sprintf(strAT, "AT+QSSLSEND=%d,%d\n", ssid,length);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_send,send_data,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    return ret;
}


s32 RIL_SSL_QSSLRECV(u8 ssid,u8 *recv_data)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];


	Ql_memset(strAT, 0, sizeof(strAT));
	Ql_memset(recv_data, 0, RECV_DATA_LENGTH);
    Ql_sprintf(strAT, "AT+QSSLRECV=%d,%d,%d\n", ctxindex,ssid,RECV_DATA_LENGTH);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_recv,recv_data,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d ,recvparam=%s-->\r\n",strAT, ret,recv_data);
    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    return ret;
}

s32 RIL_SSL_QSSLCLOSE(u8 ssid)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSSLCLOSE=%d\n", ssid);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_SSL_handler_close,NULL,0);
    APP_DEBUG("<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        APP_DEBUG("\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    return ret;
}


static s32 ATResponse_SSL_handler_CERTIFICATE_DEL(char* line, u32 len, void* userdata)
{
	char* pHead = NULL;
	pHead = Ql_RIL_FindString(line, len, "OK");
	if (pHead)
	{
		return RIL_ATRSP_SUCCESS;
	}
	pHead = Ql_RIL_FindString(line, len, "ERROR");
	if (pHead)
	{
		if (userdata != NULL)
		{
			*((s32*)userdata) = RIL_AT_FAILED;
		}
		return RIL_ATRSP_FAILED;
	}
	pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");
	if (pHead)
	{
		if (userdata != NULL)
		{
			Ql_sscanf(line, "%*[^: ]: %d[^\r\n]", (s32*)userdata);
		}
		return RIL_ATRSP_FAILED;
	}
	return RIL_ATRSP_CONTINUE;	// Just wait for the specified results above
}


static s32 ATResponse_SSL_handler_CERTIFICATE_WRITE(char* line, u32 len, void* userdata)
{
	char* pHead = NULL;
	pHead = Ql_RIL_FindString(line, len, "CONNECT");
	if (pHead)
	{
		Ql_Sleep(1000);//Do not move
		if (write_flag == 1)
		{
			Ql_RIL_WriteDataToCore((u8*)http_cacert, http_cacert_len);
		}
		else if (write_flag == 2)
		{
			Ql_RIL_WriteDataToCore((u8*)http_clientcert, http_clientcert_len);
		}
		else if (write_flag == 3)
		{
			 Ql_RIL_WriteDataToCore((u8*)http_clientkey, http_clientkey_len);
		}
		return RIL_ATRSP_CONTINUE;	// wait for OK
	}
	pHead = Ql_RIL_FindString(line, len, "OK");
	if (pHead)
	{
		return RIL_ATRSP_SUCCESS;
	}
	pHead = Ql_RIL_FindString(line, len, "ERROR");
	if (pHead)
	{
		if (userdata != NULL)
		{
			*((s32*)userdata) = RIL_AT_FAILED;
		}
		return RIL_ATRSP_FAILED;
	}
	pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");
	if (pHead)
	{
		if (userdata != NULL)
		{
			Ql_sscanf(line, "%*[^: ]: %d[^\r\n]", (s32*)userdata);
		}
		return RIL_ATRSP_FAILED;
	}
	return RIL_ATRSP_CONTINUE;	// Just wait for the specified results above
}

static s32 ATResponse_SSL_handler_CERTIFICATE_READ(char* line, u32 len, void* userdata)
{
    ST_AT_Httparam *httpParam = (ST_AT_Httparam *)userdata;
    char *head = Ql_RIL_FindString(line, len, httpParam->prefix); //continue wait
    //+QSECREAD: 1,6640
    if(head)
    {
      char strTmp[10];
      char* p1 = NULL;
      char* p2 = NULL;
      Ql_memset(strTmp, 0x0, sizeof(strTmp));
      p1 = Ql_strstr(head, ":");
      p2 = Ql_strstr(p1 + 1, ",");
      if (p1 && p2)
      {
          Ql_memcpy(strTmp, p1 + 2, p2 - p1 - 2);
          httpParam->data= Ql_atoi(strTmp);
      }
      return  RIL_ATRSP_CONTINUE;
    }
    head = Ql_RIL_FindString(line, len, "OK");
    if(head)
    {
		return  RIL_ATRSP_SUCCESS;
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


static s32 ATResponse_SSL_handler_open(char* line, u32 len, void* userdata)
{
    ST_AT_Httparam *httpParam = (ST_AT_Httparam *)userdata;
    char *head = Ql_RIL_FindString(line, len, httpParam->prefix); //continue wait
    //+QSSLOPEN: 0,0
    if(head)
    {
       char strTmp[10];
       char* p1 = NULL;
       char* p2 = NULL;
       Ql_memset(strTmp, 0x0, sizeof(strTmp));
       p1 = Ql_strstr(head, ":");
       p2 = Ql_strstr(p1 + 1, "\r\n");
       if (p1 && p2)
       {
           //Ql_memcpy(strTmp, p1 + 2, p2 - p1 - 2);
           Ql_memcpy(strTmp, p1 + 4, p2 - p1 - 4);
           httpParam->data= Ql_atoi(strTmp);
       }
      return  RIL_ATRSP_SUCCESS; ;
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

static s32 ATResponse_SSL_handler_recv(char* line, u32 len, void* userdata)
{

    char *head ;
	head = Ql_RIL_FindString(line, len, "\r\n+QSSLRECV:");
    if(head)
    {
		return RIL_ATRSP_CONTINUE;
    }
    head = Ql_RIL_FindString(line, len, "ERROR");
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "OK");//fail
    if(head)
    {
        return  RIL_ATRSP_SUCCESS;
    }
    head = Ql_RIL_FindString(line, len, "+CMS ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }

	{
       Ql_memcpy(userdata,line,len);
	}
    return RIL_ATRSP_CONTINUE; //continue wait
}



static s32 ATResponse_SSL_handler_send(char* line, u32 len, void* userdata)
{

    char *head ;
	s32 length;
	length = Ql_strlen((char*)userdata);
	head = Ql_RIL_FindString(line, len, "\r\n>");
    if(head)
    {
		//APP_DEBUG("\r\n<-- send_data str = %s,%d -->\r\n",userdata,length);
		Ql_RIL_WriteDataToCore (userdata,length);
		return RIL_ATRSP_CONTINUE;
    }
    head = Ql_RIL_FindString(line, len, "ERROR");
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "SEND OK");//fail
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

static s32 ATResponse_SSL_handler_close(char* line, u32 len, void* userdata)
{

		char *head = Ql_RIL_FindString(line, len, "CLOSE OK");
		if(head)
		{
			return	RIL_ATRSP_SUCCESS;
		}
		head = Ql_RIL_FindString(line, len, "ERROR:");//fail
		if(head)
		{
			return	RIL_ATRSP_FAILED;
		}

		return RIL_ATRSP_FAILED; //continue wait
}


#endif // __EXAMPLE_TCPCLIENT__

