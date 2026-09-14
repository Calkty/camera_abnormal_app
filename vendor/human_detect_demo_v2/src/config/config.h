#ifndef __CONFIG_H_
#define __CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include "opdevsdk_common_basic.h"
#include "cJSON.h"

#ifndef INT32
typedef int  INT32;
#endif
#ifndef INT16
typedef short  INT16;
#endif
#ifndef INT8
typedef char INT8;
#endif
#ifndef UINT32
typedef unsigned int  UINT32;
#endif
#ifndef UINT16
typedef unsigned short UINT16;
#endif
#ifndef UINT8
typedef unsigned char UINT8;
#endif
#ifndef VOID
typedef void VOID;
#endif

#define OK		0
#define ERROR	-1
#define TRUE	1
#define FALSE	0
#define MIN(a,b)		((a) > (b) ? (b) : (a))
#define MAXCHAN 16
#define MAX_POLYGON_POINT_NUM   10  //多边形最多10个顶点
#define FIRST_CHAN_NO 1
#define HUMAN_APP_ID "19999"

/* 坐标结构体，使用归一化 */
typedef struct
{
    UINT16  x;  /* 横坐标 */
    UINT16  y;  /* 纵坐标 */
}POINT;

/* 多边型结构体 */
typedef struct
{
    UINT32 point_num;                  /* 有效点 */
    POINT  pos[MAX_POLYGON_POINT_NUM]; /* 多边形边界点,最多十个 */
}POLYGON;

typedef struct 
{
	bool enable;
	uint32_t id;
}ALARM_OUT_CFG;

typedef struct 
{
	bool enable;
	uint32_t id;
}RECORD_CFG;

typedef struct 
{
	bool enable;
	uint32_t id;
}CAPTURE_CFG;

typedef struct 
{
#define MAX_ALARMOUT_NUMS 16
#define MAX_RECORD_NUMS 16
#define MAX_CAPTURE_NUMS 16
    INT8 enable;         /* 智能总使能开关 0-关闭，1-开启 */
	INT8 osd_enable;     /* 智能OSD叠加 0-不叠加，1-叠加 */
	INT8 streamWithVca;  /* 码流叠加智能信息 0-不叠加，1-叠加 */
	INT8 voiceLinkage;   /* 报警联动语音播报 0-不联动，1-联动 */
	INT32 threshold;     /* 智能检测阈值[1-100] */
	POLYGON region;      /* 规则区域 */
	UINT8 voiceIndex;
	bool is_center_enable;
	bool is_storage_enable;
	int8_t res;
	ALARM_OUT_CFG alarmOutInfo[MAX_ALARMOUT_NUMS];
	RECORD_CFG alarmRecordInfo[MAX_RECORD_NUMS];
	CAPTURE_CFG alarmCapInfo[MAX_CAPTURE_NUMS];
	/*向后扩展*/
}APP_CFG;

typedef struct
{
	UINT32	magic_number;	/* 幻数 */ 
	UINT32	check_sum;		/* 检查和 */  
	UINT32	length;			/* 结构长度 */  
	UINT8	version;		/* 版本号，从1开始 */ 
	UINT8   res[3];
	APP_CFG app_cfg[MAXCHAN];        /* APP配置参数 */ 
}APP_CFG_SAVE;

INT32 set_app_cfg(APP_CFG *p_app_cfg);
INT32 get_app_cfg(APP_CFG *p_app_cfg);
INT32 set_app_cfg_V2(APP_CFG *p_app_cfg, int chan);
INT32 get_app_cfg_V2(APP_CFG *p_app_cfg, int chan);
INT32 multi_pdc_config_init(VOID);
INT32 get_app_chan(char *appID, int32_t chanInfo[], int32_t *validChanNum, void *data);

int32_t add_link_json_object_by_config(cJSON *linkageObj);

#ifdef __cplusplus
}
#endif

#endif //__CONFIG_H_

