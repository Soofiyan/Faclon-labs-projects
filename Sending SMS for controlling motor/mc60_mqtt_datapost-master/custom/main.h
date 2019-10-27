/*
 * main.h
 *
 *  Created on: 04-Apr-2017
 *      Author: Viral Shah`
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "custom_feature_def.h"
#include "ql_trace.h"
#include "ril_gps.h"
#include "ql_system.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_telephony.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ril.h"
#include "ril_network.h"
#include "ril_sim.h"
#include "ril_location.h"
#include "ql_gprs.h"
#include "gsm.h"
#include "adc.h"
#include "ql_uart.h"
#include "ql_system.h"
#include "ql_adc.h"
#include "ql_clock.h"
#include "ql_common.h"
#include "ql_gprs.h"
#include "ql_uart.h"
#include "ql_wtd.h"
#include "ql_type.h"
#include "ql_timer.h"
#include "ql_gpio.h"
#include "stdlib.h"
#include <string.h>

typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;

/************************************************************************
 * Faclon Feature Definitions
 ************************************************************************/
/*#include "modes.h"*/

/*#include "modbus.h"*/

#define uint8_t u8
#define uint16_t u16
#define uint32_t u32
#define uint64_t u64

#define sint8_t s8
#define sint16_t s16
#define sint32_t s32
#define sint64_t s64

#define MEMSET(x,...) Ql_memset(x,'\0',sizeof(x));
#define MEMSETPTR(x,...) Ql_memset(x,'\0',sizeof(*x))

#define RIL_CGATT_ACT_SUCCESS 10
#define RIL_UPLOAD_CONNECT_SUCCESS 20
#define RIL_UPLOAD_PROMPT_SUCCESS  21
#define RIL_UPLOAD_CLOSE_SUCCESS  22
#define GPS_POWER_OFF 23
#define GPS_POWER_ON  24
#define SERIAL_RX_BUFFER_LEN  2048





#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT1
#define DBG_BUF_LEN   512
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



/*Function Prototypes*/





#endif /* MAIN_H_ */
