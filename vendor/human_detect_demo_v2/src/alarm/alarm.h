/****************************************************************************** 
 *  @file alarm.h
 *  @note HangZhou Hikvision Digital Technology Co., Ltd. All Rights Reserved.
 *  @brief 
 *
 *  @author   zhouwenjie7@hikvision.com.cn
 *  @date     2022-1-17
 *
 *  @note History:
 *  @note
 ******************************************************************************/
#ifndef __ALARM_H_
#define __ALARM_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INT32
typedef int INT32;
#endif

/**
 * @brief 报警数据区域信息
 */
struct EadAlarmRectStruct
{
	char *pic_buf;
    INT32 channelID;
    char eventType[32];
    INT32 absTime;
    INT32 millisecond;
	float height;
	float width;
	float x;
	float y;
	INT32 piclen;
};

/*消息队列结构体*/
struct msbuf
{
    long mtype;
    struct EadAlarmRectStruct mtext;
};

typedef struct
{
   int magic;     //0x414d4d4e
   int len;       //消息长度
   int jsonLen;
   int picLen;
   char res[8];
}ALARM_MSG_HEAD;

void alarm_server_task(void);
void *alarm_process(void *arg);
int connect_alarm_server(void);
void alarm_set_http_target(const char *ip, int port);
int writen(int connfd, void *pbuf, int nums);

#ifdef __cplusplus
}
#endif

#endif //__ALARM_H_

