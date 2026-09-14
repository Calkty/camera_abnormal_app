#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <arpa/inet.h>
#include "opdevsdk_common_basic.h"
#include <net/if.h>
#include "alarm.h"
#include "audioplay.h"
#include "config.h"
#include <pthread.h>
#include "cJSON.h"

#define LISTENQ                 24

#define ALARM_SERVER_PORT  12345
#define LOCAL_INTERFACE  "eth0"

#define APP_IP_LEN  32

#define ALARM_MSG_MAGIC_NUM  0x414d4d4e
#define MAX_ALARM_MSG_LEN    4*1024*1024

struct EadAlarmRectStruct rectInfo; //rectInfo通过消息队列发送

#define DEFAULT_HTTP_ALARM_IP "10.184.142.15" //???????????
#define DEFAULT_HTTP_ALARM_PORT 7200

#define HEAD_FORMAT ("POST / HTTP/1.1\r\n"\
                     "Content-Type: multipart/form-data; boundary=boundary\r\n"\
                     "Host: %s:%d\r\n"\
                     "Connection: close\r\n"\
                     "Content-Length: %d\r\n\r\n")

#define JSON_FORMAT ("--boundary\r\n"\
                     "Content-Type: application/json\r\n"\
                     "Content-Length: %d\r\n\r\n")

#define PICTURE_FORMAT ("\r\n--boundary\r\n"\
                        "Content-Type: image/jpeg\r\n"\
                        "Content-Length: %d\r\n"\
                        "Content-ID: dogDetectBkPic\r\n\r\n")

#define BOUNDARY_FORMAT ("\r\n--boundary--\r\n")
#define BOUNDARY_SIZE (sizeof(BOUNDARY_FORMAT))

#ifndef SAFE_FREE_JSON
#define SAFE_FREE_JSON(x) do { if ((x) != NULL) { cJSON_Delete((x)); (x) = NULL;} } while(0)
#endif
int g_alarm_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_http_alarm_ip[APP_IP_LEN] = DEFAULT_HTTP_ALARM_IP;
static int g_http_alarm_port = DEFAULT_HTTP_ALARM_PORT;

void alarm_set_http_target(const char *ip, int port)
{
	if (ip != NULL && ip[0] != '\0')
	{
		snprintf(g_http_alarm_ip, sizeof(g_http_alarm_ip), "%s", ip);
	}
	if (port > 0)
	{
		g_http_alarm_port = port;
	}
}

int get_local_ipaddr(char *p_interface_name, char *p_ip, int len)
{
	int sock_fd = -1;
	struct in_addr addr_temp;
	struct ifconf ifconf;
	struct ifreq *ifr = NULL;
	unsigned char buf[512];
	int i = 0;
	
	memset(buf, 0, sizeof(buf));
	memset(&addr_temp, 0, sizeof(addr_temp));
	memset(&ifconf, 0, sizeof(ifconf));

	if ((NULL == p_interface_name) || (NULL == p_ip) || len < 16)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "get_ipaddr param err.\n");
		return -1;
	}

	if ((sock_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "socket failed! errno=%d\n", errno);
		return -1;
	}

	ifconf.ifc_len = 512;
	ifconf.ifc_buf = (char*)buf;

	/* Get all interfaces list */
	if (ioctl(sock_fd, SIOCGIFCONF, &ifconf) < 0)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "SIOCGIFCONF socket failed! errno=%d\n", errno);
		close(sock_fd);
		sock_fd = -1;
		return -1;
	}

	close(sock_fd);
	sock_fd = -1;

	ifr = (struct ifreq*)buf;
	for (i = (ifconf.ifc_len/sizeof(struct ifreq)); i > 0; i--)
	{
		if (AF_INET == ifr->ifr_flags && 0 == strncmp(ifr->ifr_name, p_interface_name, sizeof(ifr->ifr_name)))
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "Interface_name:[%s], IP_addr:[%s], ifr_size:[%u]\n", 
				ifr->ifr_name, inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr), sizeof(struct ifreq));
			break;
		}
		ifr++;
	}

	if (0 == i)
	{
    	return -1;
	}
	
	addr_temp = ((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr;
	
	(void)inet_ntop(AF_INET, &addr_temp, p_ip, len);
	
	return 0;
}

/** @fn	int readn(int connfd, void *pbuf, int nums)	  
 *  @brief	尽量从socket中读取最多nums个数据。  
 *  @param[in]  connfd 已经连接成功的连接的fd。范围:大于0
 *  @param[in]  pbuf 缓冲区。范围:非NULL
 *  @param[in]  nums 读取的个数。要读取的数据的个数。范围:大于0
 *  @param[out] pbuf 数据读到缓冲区内
 *  @return	  实际读取到数据的个数/ERROR
 */
int readn(int connfd, void *pbuf, int nums)
{
	int nleft = 0;
	int nread = 0;
	char *pread_buf = NULL;
	struct timeval select_timeout;
	fd_set rset;
	int ret = 0;
	
	memset(&select_timeout, 0, sizeof(select_timeout));

	if ((connfd <= 0) || (NULL == pbuf) || (nums <= 0))
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "readn: param err.\n");
		return -1;
	}

	pread_buf = (char *)pbuf;
	nleft = nums;

	while (nleft > 0)
	{
		select_timeout.tv_sec = 10;
		select_timeout.tv_usec = 0;

		FD_ZERO(&rset);
		FD_SET((unsigned int)connfd, &rset);
		ret = select(connfd + 1, &rset, NULL, NULL, &select_timeout);
		if (ret < 0)
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "readn: select failed, %s\n", strerror(errno));
			return -1;
		}
		else if (ret == 0)
		{
			break;
		}
		else
		{
			nread = recv(connfd, pread_buf, nleft, 0);
			if (nread < 0)
			{
				if (EINTR == errno)
				{
					nread = 0;
				}
				else
				{
					opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "recv, %s\n", strerror(errno));
					return -1;
				}
			}
			else if (0 == nread)
			{
				break;
			}
			else
			{
				nleft -= nread;
				pread_buf += nread;
			}
		}
	}
	
	return (nums - nleft);
}

/** @fn	int writen(int connfd, void *pbuf, int nums)	  
 *  @brief	向socket中最多写入nums个字节	  
 *  @param[in]  connfd 已经连接成功的连接fd。范围:大于0
 *  @param[in]  pbuf 存放数据的缓冲区。范围:非NULL
 *  @param[in]  nums 要写入的数据的个数。范围:大于等于0
 *  @param[out] 无
 *  @return	  实际写入的字节数/ERROR
 */
int writen(int connfd, void *pbuf, int nums)
{
	int nleft = 0;
	int nwritten = 0;
	char *pwrite_buf = NULL;
	struct timeval select_timeout;
	fd_set rset;

	memset(&select_timeout, 0, sizeof(select_timeout));

	if ((connfd <= 0) || (NULL == pbuf) || (nums < 0))
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "writen: param err.\n");
		return -1;
	}

	pwrite_buf = (char *)pbuf;
	nleft = nums;

	while (nleft > 0)
	{
		select_timeout.tv_sec = 10;
		select_timeout.tv_usec = 0;

		FD_ZERO(&rset);
		FD_SET((unsigned int)connfd, &rset);
		if (select(connfd + 1, NULL, &rset, NULL, &select_timeout) <= 0)
		{	
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "readn: select failed, %s\n", strerror(errno));
			return -1;
		}

		if (-1 == (nwritten = send(connfd, pwrite_buf, nleft, MSG_NOSIGNAL|MSG_DONTWAIT)))
		{
			if (EINTR == errno)
			{
				opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "EINTR\n");
				nwritten = 0;
			}
			else 
			{
				opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "Send() error, 0x%x\n", errno);
				return -1;
			}
		}
		nleft -= nwritten;
		pwrite_buf += nwritten;
	}

	return(nums);
}

void inet_ntop4(const unsigned char *p_src, char *p_dst, int isize)
{
    static const char sz_fmt[] = "%u.%u.%u.%u";
    char sz_tmp[strlen("255.255.255.255")+1];

    if ((NULL == p_src) || (NULL == p_dst) || (isize <= 0))
    {
        return;
    }

    sprintf(sz_tmp, sz_fmt, p_src[0], p_src[1], p_src[2], p_src[3]);
    
    if ((int)strlen(sz_tmp) > isize-1)
    {
        return;
    }

    strcpy(p_dst, sz_tmp);
    return;
}

static int construct_json_buf(struct msbuf msg, char **p_buf, char *ip_str, unsigned short portNum, char *mac_str)
{
    cJSON *cjson_alarm = NULL;
    cjson_alarm = cJSON_CreateObject();
    if(NULL == cjson_alarm)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "cjson_alarm error.\n");
        return -1;
    }  
    cJSON *cjson_Rect = NULL;
    cjson_Rect = cJSON_CreateObject(); 
    if(NULL == cjson_Rect)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "cjson_Rect error.\n");
        SAFE_FREE_JSON(cjson_alarm);
        return -1;
    }
    cJSON_AddStringToObject(cjson_alarm, "ipAddress", ip_str);
    cJSON_AddNumberToObject(cjson_alarm, "portNo", portNum);
    cJSON_AddStringToObject(cjson_alarm, "protocol", "HTTP");
    cJSON_AddStringToObject(cjson_alarm, "macAddress", mac_str);
    cJSON_AddNumberToObject(cjson_alarm, "channelID", msg.mtext.channelID);
    cJSON_AddStringToObject(cjson_alarm, "eventType", msg.mtext.eventType);
    cJSON_AddNumberToObject(cjson_alarm, "absTime", msg.mtext.absTime);
    cJSON_AddNumberToObject(cjson_alarm, "millisecond", msg.mtext.millisecond);
    cJSON_AddNumberToObject(cjson_Rect, "x", msg.mtext.x);
    cJSON_AddNumberToObject(cjson_Rect, "y", msg.mtext.y);
    cJSON_AddNumberToObject(cjson_Rect, "w", msg.mtext.width);
    cJSON_AddNumberToObject(cjson_Rect, "h", msg.mtext.height);
    cJSON_AddItemToObject(cjson_alarm, "Rect", cjson_Rect);
    *p_buf = cJSON_Print(cjson_alarm);
    SAFE_FREE_JSON(cjson_alarm);
    return 0;
}

static void send_http_alarm(struct msbuf msg, char *p_buf)
{
    int c_sockfd;
    INT32 head_len = 0;
    INT32 json_len = 0;	
    INT32 p_buf_len = 0;	
    INT32 bounday_size = 0;
    INT32 picture_len = 0;
    char head_buf[512];
    char json_buf[512];
    char picture_buf[512];

    memset(head_buf, 0, sizeof(head_buf));
    memset(json_buf, 0, sizeof(json_buf));
    memset(picture_buf, 0, sizeof(picture_buf));

    c_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == c_sockfd)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "create socket failed\n");
        return;
    }
    struct sockaddr_in clientaddr; //在<netinet/in.h>中定义
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(g_http_alarm_port); //16位网络字节序
    clientaddr.sin_addr.s_addr = inet_addr(g_http_alarm_ip); //将点分十进制IP转换为无符号32位整数
    if (-1 == connect(c_sockfd, (struct sockaddr *)&clientaddr,  sizeof(clientaddr)))
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "connect failed\n");
        close(c_sockfd);
        return;
    }
	p_buf_len = strlen(p_buf);
	bounday_size = BOUNDARY_SIZE;
    json_len += snprintf(json_buf, 512, JSON_FORMAT, p_buf_len);
    picture_len += snprintf(picture_buf, 512, PICTURE_FORMAT, msg.mtext.piclen);
    head_len += snprintf(head_buf, 512, HEAD_FORMAT, g_http_alarm_ip, g_http_alarm_port, json_len + p_buf_len + picture_len + msg.mtext.piclen + bounday_size);

    if (head_len != writen(c_sockfd, head_buf, head_len)) //发送head
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "send head error\n");
        close(c_sockfd);
        return;
    }
    
    if (json_len != writen(c_sockfd, json_buf, json_len)) //发送json前的内容
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "send body error\n");
        close(c_sockfd);
        return;
    }
    
    if (strlen(p_buf) != writen(c_sockfd, p_buf, strlen(p_buf))) //发送json
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "send json error\n");
        close(c_sockfd);
        return;
    }
    
    if (picture_len != writen(c_sockfd, picture_buf, picture_len)) //发送图片前的内容
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "send picture_buf error\n");
        close(c_sockfd);
        return;
    }

    if (msg.mtext.piclen != writen(c_sockfd, msg.mtext.pic_buf, msg.mtext.piclen)) //发送图片
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "send pic error\n");
        close(c_sockfd);
        return;
    }
    
    if (BOUNDARY_SIZE != writen(c_sockfd, BOUNDARY_FORMAT, BOUNDARY_SIZE)) //发送结尾
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "send end error\n");
        close(c_sockfd);
        return;
    }
}

int GetRectInfoFromJsonStd(cJSON *p_rect, struct EadAlarmRectStruct *p_alarm_rect_info)
{
	cJSON *tmp_json = NULL;
	if ((NULL == p_rect) || (NULL == p_alarm_rect_info))
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "get_rect_info_from_json, point is NULL !\n");
		return -1;
	}

	tmp_json = cJSON_GetObjectItem(p_rect, "h");
	if(tmp_json == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "rect height lost.\n");
		return -1;
	}
	p_alarm_rect_info->height = tmp_json->valuedouble;

	tmp_json = cJSON_GetObjectItem(p_rect, "w");
	if(tmp_json == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "rect width lost.\n");
		return -1;
	}
	p_alarm_rect_info->width = tmp_json->valuedouble;

	tmp_json = cJSON_GetObjectItem(p_rect, "x");
	if(tmp_json == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "rect x lost.\n");
		return -1;
	}
	p_alarm_rect_info->x = tmp_json->valuedouble;

	tmp_json = cJSON_GetObjectItem(p_rect, "y");
	if(tmp_json == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "rect y lost.\n");
		return -1;
	}
	p_alarm_rect_info->y = tmp_json->valuedouble;

	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "get_rect_info_from_json_std, h:%f, w:%f, x:%f, y:%f !\n",
						p_alarm_rect_info->height,
						p_alarm_rect_info->width,
						p_alarm_rect_info->x,
						p_alarm_rect_info->y);
	return 0;
}

int stream_json_event_result(char*p_json_data)
{
    int ret = 0;
	cJSON *root = NULL;
	cJSON *head_info = NULL;
	cJSON *body_info = NULL;
	cJSON *tmp_json_info = NULL;

	memset(&rectInfo, 0, sizeof(rectInfo));
	if (NULL == p_json_data)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "stream_json_event_result, point is NULL !\n");
		return -1;
	}

	printf("stream_json_event_result, json_data is %s !\n",p_json_data);
	root = cJSON_Parse(p_json_data);
	if(NULL == root)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "cJSON_Parse root is NULL\n");
		return -1;
	}

	head_info = cJSON_GetObjectItem(root, "HeadInfo");
	if(head_info == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "root HeadInfo is NULL\n");
		cJSON_Delete(root);
		return -1;
	}
    
    tmp_json_info = cJSON_GetObjectItem(head_info, "eventType");	
	if(tmp_json_info == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "eventType is NULL\n");
		cJSON_Delete(root);
		return -1;
	}
    strncpy(rectInfo.eventType, tmp_json_info->valuestring,sizeof(rectInfo.eventType)-1);

    tmp_json_info = cJSON_GetObjectItem(head_info, "channel");	
	if(tmp_json_info == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "channel is NULL\n");
		cJSON_Delete(root);
		return -1;
	}
    rectInfo.channelID = tmp_json_info->valueint;

	body_info = cJSON_GetObjectItem(root, "BodyInfo");
	if(body_info == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "root BodyInfo is NULL\n");
		cJSON_Delete(root);
		return -1;
	}

	tmp_json_info = cJSON_GetObjectItem(body_info, "bodyType");	
	if(tmp_json_info == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "bodyType is NULL\n");
		cJSON_Delete(root);
		return -1;
	}	
	if(tmp_json_info->valuestring != NULL)
	{
		if(0 == strcmp(tmp_json_info->valuestring, "humanDetect_demo"))
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "################stream_json_event_result:humanDetect_demo################.\n");
		}
		else
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "stream_json_event_result  bodyType %s!\n", tmp_json_info->valuestring);
			cJSON_Delete(root);
			return -1;
		}
	}

	tmp_json_info = cJSON_GetObjectItem(body_info, "absTime");
    if (tmp_json_info == NULL)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "absTime is NULL\n");
        cJSON_Delete(root);
        return -1;
    }
    rectInfo.absTime = tmp_json_info->valueint;

    tmp_json_info = cJSON_GetObjectItem(body_info, "millisecond");
    if (tmp_json_info == NULL)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "millisecond is NULL\n");
        cJSON_Delete(root);
        return -1;
    }
    rectInfo.millisecond = tmp_json_info->valueint;
	
	tmp_json_info = cJSON_GetObjectItem(body_info, "Rect");	
	if(tmp_json_info == NULL)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "Rect is NULL\n");
		cJSON_Delete(root);
		return -1;
	}
	else
	{
		ret = GetRectInfoFromJsonStd(tmp_json_info, &rectInfo);
		if(ret != 0)
		{
			cJSON_Delete(root);
			return -1;
		}
	}
	
	cJSON_Delete(root);
	return 0;
}

void release_alarm_count(void)
{
	pthread_mutex_lock( &mutex);
	if(g_alarm_count > 0)
	{
		g_alarm_count--;
	}
	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, " msgsnd  g_alarm_count %d.\n", g_alarm_count);

	pthread_mutex_unlock(&mutex);
}

void *do_alarm_work(void *arg)
{
    int newfd = *(int *)arg;
    int ret = 0;
	int read_len = 0;
    char* recv_buf = NULL;
	char* pic_buf = NULL;
	ALARM_MSG_HEAD alarm_msg_head;

	key_t key;
    int s_msgid;
    struct msbuf msg;
    key = ftok(".", 'a');
    s_msgid = msgget(key, IPC_CREAT|0666);

	while (1)
	{
    	memset(&alarm_msg_head, 0, sizeof(alarm_msg_head));
		
		read_len = readn(newfd, &alarm_msg_head, sizeof(alarm_msg_head));
		if (-1 == read_len)
		{
        	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "client is disconnect.\n");
			break;
		}
		else if (read_len != sizeof(alarm_msg_head))
		{
			//opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "readn msg head error, read_len = %d\n", read_len);
			usleep(50*1000);
			continue;
		}

		if (ALARM_MSG_MAGIC_NUM != alarm_msg_head.magic || alarm_msg_head.len <= 0 || alarm_msg_head.len > MAX_ALARM_MSG_LEN)
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "recv msg error, magic[%d], len[%d].", alarm_msg_head.magic, alarm_msg_head.len);
			usleep(50*1000);
			continue;
		}

		//读取json
		recv_buf = malloc(alarm_msg_head.jsonLen);
		if (NULL == recv_buf)
		{
        	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "malloc failed, len[%d].", alarm_msg_head.len);
			usleep(50*1000);
			continue;
		}

		read_len = readn(newfd, recv_buf, alarm_msg_head.jsonLen); 
		if (-1 == read_len)
		{
        	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "client is disconnect.\n");
			free(recv_buf);
			break;
		}
		else if (read_len != alarm_msg_head.jsonLen)
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "readn json error, read_len = %d\n", read_len);
			free(recv_buf);
			usleep(50*1000);
			continue;
		}
		//读取图片
		pthread_mutex_lock( &mutex);
		if(g_alarm_count > 40)
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "%s g_alarm_count %d alarm_msg_head.picLen %d.\n",__FUNCTION__, g_alarm_count,alarm_msg_head.picLen);
			pthread_mutex_unlock(&mutex);
			pic_buf = malloc(alarm_msg_head.picLen);
			read_len = readn(newfd, pic_buf, alarm_msg_head.picLen);
			if (-1 == read_len)
			{
				opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "client is disconnect.\n");
				free(pic_buf);
				free(recv_buf);
				release_alarm_count();
				break;
			}	
			free(pic_buf);
			free(recv_buf);
            continue;
		}
		g_alarm_count++;
		pthread_mutex_unlock(&mutex);
		//读取图片
        pic_buf = malloc(alarm_msg_head.picLen);
        if (NULL == pic_buf)
        {
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "malloc failed, len[%d].", alarm_msg_head.len);
            free(recv_buf);
            usleep(50*1000);
			release_alarm_count();
            continue;
        }
        read_len = readn(newfd, pic_buf, alarm_msg_head.picLen);
		if (-1 == read_len)
		{
        	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "client is disconnect.\n");
			free(pic_buf);
            free(recv_buf);
			release_alarm_count();
			break;
		}
		else if (read_len != alarm_msg_head.picLen)
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "readn pic error, read_len = %d alarm_msg_head.picLen:%d\n", read_len,alarm_msg_head.picLen);
            free(recv_buf);
            free(pic_buf);
			usleep(50*1000);
			release_alarm_count();
			continue;
		}

		/* 解析dsp json 报警消息 */
		ret = stream_json_event_result(recv_buf);
		if (0 != ret)
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "parse alarm msg error, json = %s\n", recv_buf);
			free(pic_buf);
			free(recv_buf);
			usleep(50*1000);
			release_alarm_count();
			continue;
		}
		free(recv_buf);
        rectInfo.pic_buf = pic_buf;
        rectInfo.piclen=alarm_msg_head.picLen;

		/*组装消息队列并发送*/
        memset(&msg, 0, sizeof(msg));
        msg.mtype = 1;
        msg.mtext = rectInfo;
        ret = msgsnd(s_msgid, &msg, sizeof(struct EadAlarmRectStruct), IPC_NOWAIT);
        if (-1 == ret)
        {
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "send message error.\n");
			free(pic_buf);
			release_alarm_count();
            continue;
        }

		APP_CFG app_cfg;
		memset(&app_cfg, 0, sizeof(app_cfg));
		
		ret = get_app_cfg(&app_cfg);
		if (ret == 0)
		{
			if (app_cfg.voiceLinkage)
			{
				/* 联动语音播报 */
				audio_play();
			}
		}
	}  

    pthread_exit(NULL);
}

void alarm_server_task(void)
{
	int sockfd = -1;
	int connfd = -1;
	int ret = -1;
	char loacl_ip[APP_IP_LEN];
	struct linger so_linger;
	struct sockaddr_in server_addr;
	int tmp = 1;
	pthread_t tid;
	
	pthread_attr_t attri;
	memset(&attri, 0, sizeof(attri));

	memset(&server_addr, 0, sizeof(server_addr));
    memset(loacl_ip, 0, sizeof(loacl_ip));

	(void)get_local_ipaddr(LOCAL_INTERFACE, loacl_ip, APP_IP_LEN);
	server_addr.sin_addr.s_addr = inet_addr(loacl_ip);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(ALARM_SERVER_PORT);
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "socket create failed.\n");
		return;
	}

	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&tmp, sizeof(tmp));
	if (0 != ret)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "setsockopt SO_REUSEADDR failed, ret=%d\n", ret);
	}

	ret = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (0 != ret)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "bind failed.\n");
		close(sockfd);
		return ;
	}

	if (0 != listen(sockfd, LISTENQ))
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "listen failed.\n");
		close(sockfd);
		return ;
	}

	while(1)
	{						
		if ((connfd = accept(sockfd, NULL, NULL)) < 0)
        {
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "accept() failed !\n"); 
            continue;
        }
		
		memset(&so_linger, 0, sizeof(so_linger));
		so_linger.l_onoff = 1;
		so_linger.l_linger = 0;
		ret = setsockopt(connfd, SOL_SOCKET, SO_LINGER, (void *)(&so_linger), sizeof(so_linger));
		if (0 != ret)
		{
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "setsockopt SO_LINGER error,iRet=%d!\n", ret);
			close(connfd);
			connfd = -1;
			continue;
		}
		memset(&attri, 0, sizeof(attri));
		pthread_attr_init(&attri);

		ret = pthread_attr_setschedpolicy(&attri, SCHED_OTHER);
		if(ret != 0)
		{
			pthread_attr_destroy(&attri);
			close(connfd);
			connfd = -1;
			continue;
		}

		
		ret = pthread_attr_setdetachstate(&attri, PTHREAD_CREATE_DETACHED);
		if(ret != 0)
		{
			pthread_attr_destroy(&attri);
			close(connfd);
			connfd = -1;
			continue;
		}

        ret = pthread_create(&tid, NULL, do_alarm_work, &connfd);
        if (0 != ret)
		{
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "pthread_create failed\n");
			pthread_attr_destroy(&attri);
            close(connfd);
			connfd = -1;
			continue;
        }
        pthread_detach(tid);
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "enter do_alarm_work\n");
	}

	close(sockfd);
	
	return;
}

void *alarm_process(void *arg)
{
    int ret = -1;
	unsigned short portNum = 0;
    char *p_buf = NULL;
    key_t key;
    int r_msgid;
    struct msbuf msg;
    OP_DEVSDK_NET_CFG net_cfg;
    OP_DEVSDK_DEVICE_INFO device_info;
  	char ip_str[64];
    char mac_str[64];
    key = ftok(".", 'a');
    r_msgid = msgget(key, IPC_CREAT|0666);

    memset(&net_cfg, 0, sizeof(net_cfg));
    memset(&device_info, 0, sizeof(device_info));
	memset(ip_str, 0, sizeof(ip_str));
    memset(mac_str, 0, sizeof(mac_str));

    (void)opdevsdk_get_network_cfg(&net_cfg);
    inet_ntop4((const unsigned char *)&net_cfg.net_addr.ip_address.v4, ip_str, 64);
    portNum = net_cfg.net_port.http_port;

    (void)opdevsdk_get_device_info(&device_info);
    memcpy(mac_str, device_info.mac_addr, sizeof(mac_str) - 1);
    
    while(1)
    {
        memset(&msg, 0, sizeof(struct msbuf));
        msg.mtype = 1;
        ret = msgrcv(r_msgid, &msg, sizeof(struct EadAlarmRectStruct), msg.mtype, 0);
        if (-1 == ret)
        {
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "send message error.\n");
            continue;
        }
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "alarm_process start.\n");

       	if(-1 == construct_json_buf(msg, &p_buf, ip_str, portNum, mac_str))
		{
            free(msg.mtext.pic_buf);
			release_alarm_count();
            continue;
        }
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "alarm_process send_http_alarm.\n");
        
        send_http_alarm(msg, p_buf);
		free(p_buf);
		free(msg.mtext.pic_buf);
		release_alarm_count();
    }
    
    pthread_exit(NULL);
}

/** @fn	int connect_with_timeout(int sock_fd, struct sockaddr* p_addrs, int adrslen, struct timeval* tm)	 
 *  @brief	连接服务器，若socket已经程序中且在超时时间内socket可写，则清除错误，返回成功	  
 *  @param[in]  sock_fd 网络socket句柄。范围:大于0
 *  @param[in]  p_addrs 包含服务器ip和端口号。范围:非NULL
 *  @param[in]  adrslen addrs长度.范围:大于0
 *  @param[in]  tm 连接超时时间。范围:非NULL
 *  @param[out] 无
 *  @return	 OK/ERROR 
 */
int connect_with_timeout(int sock_fd, struct sockaddr* p_addrs, int adrslen, struct timeval* tm)
{
	int err = 0;
	int len = sizeof(int);
	int block_or_not = 0; //将socket设置成阻塞或非阻塞
	int ret_val = -1;     //接收函数返回
	fd_set set;
	struct timeval mytm;

	if (NULL == p_addrs)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "connect_with_timeout para error\n");
		return -1;
	}

	memset(&mytm, 0, sizeof(struct timeval));

	if (tm != NULL)
	{
		memcpy(&mytm, tm, sizeof(struct timeval));
	}

	block_or_not = 1; //设置非阻塞
	if (ioctl(sock_fd, FIONBIO, &block_or_not) != 0)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "ioctl socket failed\n");
	}

	ret_val = connect(sock_fd, p_addrs, adrslen);
	if (-1 == ret_val)
	{
		if (EINPROGRESS == errno)
		{
			FD_ZERO(&set);
			FD_SET(sock_fd, &set);

			if (select(sock_fd + 1, NULL, &set, NULL, tm) > 0)
			{
				//清除错误
				(void)getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t*)&len);
				if (0 == err)
				{
					ret_val = 0;
				}
				else
				{
					ret_val = -1;
				}
			}
			else
			{
				ret_val = -1;
			}
		}
	}

	block_or_not = 0; //设置阻塞
	if (ioctl(sock_fd, FIONBIO, &block_or_not) != OK)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "ioctl socket failed\n");
	}

	if (tm != NULL)
	{
		memcpy(tm, &mytm, sizeof(struct timeval));
	}

	return ret_val;
}

int connect_alarm_server(void)
{
    int ret = -1;
	int sock_fd = -1;
	char loacl_ip[APP_IP_LEN];
	struct sockaddr_in servaddr;	
	struct timeval connect_time;	

	memset(loacl_ip, 0, sizeof(loacl_ip));
	(void)get_local_ipaddr(LOCAL_INTERFACE, loacl_ip, APP_IP_LEN);
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_addr.s_addr = inet_addr(loacl_ip);
	servaddr.sin_port = htons(ALARM_SERVER_PORT);
	servaddr.sin_family = AF_INET;
	
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sock_fd)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "create socket error!\n");
		return -1;
	}

	memset(&connect_time, 0, sizeof(connect_time));
	connect_time.tv_sec = 10;
    connect_time.tv_usec = 0;
	ret = connect_with_timeout(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr), &connect_time);
	if (0 != ret)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "connect to alarm server error!\n");
		close(sock_fd);
		return -1;
	}

	return sock_fd;
}


