/***************************************************************************
* note 2012-2019 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         hikflow_demo_priv.h
* @brief        hikflow demo private interface
*          
* @author       heoper
* @date         2023-8-30
* @version      2.0.0
* @note         1. defines log print, module statistics,  macro definitions, private handles
*****************************************************************************/

#ifndef _HIKFLOW_DEMO_H_
#define _HIKFLOW_DEMO_H_

/**
* @brief interface of C library
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <malloc.h>
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <dirent.h>
#include <elf.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>   
#include <sys/resource.h>
#include <sys/un.h>
#include <sys/prctl.h>
#include <linux/fb.h>
#include <libgen.h>
#include <getopt.h>
#include <stdarg.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <linux/sockios.h> 
#include <syslog.h>

/**
* @brief interface of bsc and scheduler library
*/
#include "opdevsdk_mediadrv_comm.h"
#include "opdevsdk_sys.h"
#include "opdevsdk_img.h"
#include "opdevsdk_mem.h"
#include "opdevsdk_vin.h"
#include "opdevsdk_video.h"
#include "opdevsdk_jpegenc.h"
#include "opdevsdk_jpegdec.h"
#include "opdevsdk_mscale.h"
#include "opdevsdk_pos.h"
#include "opdevsdk_sche.h"
#include "opdevsdk_isp.h"

/**
* @brief interface of opdevsdk library
*/
#include "alarm.h"
#include "opdevsdk_common_basic.h"
#include "stdbool.h"
#include "cJSON.h"

/**
* @brief interface of hikflow
*/
#include "opdevsdk_hikflow_lib.h"

/**
* @brief interface of other tools
*/
#include "stack_mng.h"
#include "json_proc.h"
#include "isfw_log.h"
#include "isfw_stat.h"
#include "isfw_debug.h"
#include "config.h"

/**
* @brief interface of hikflow_demo
*/
#include "hikflow_demo.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief hikflow demo print
*/
#define HIKFLOW_DBG(arg...)                 isfw_log_print("HFDemo",ISFW_LOG_LEVEL_DEBUG, __FILE__,__LINE__,##arg)
#define HIKFLOW_LOG(arg...)                 isfw_log_print("HFDemo",ISFW_LOG_LEVEL_INFO, __FILE__,__LINE__,##arg)
#define HIKFLOW_ERR(arg...)                 isfw_log_print("HFDemo",ISFW_LOG_LEVEL_ERROR, __FILE__,__LINE__,##arg)
#define HIKFLOW_WRN(arg...)                 isfw_log_print("HFDemo",ISFW_LOG_LEVEL_WARN, __FILE__,__LINE__,##arg)

/**
* @brief return value verification function
*/
#define HIKFLOW_RET(c,r)                    if(c){typeof(r) _ret_=r;HIKFLOW_ERR("ret failed %s ret=0x%x\n",#c,(int)_ret_); return _ret_;}
#define HIKFLOW_NORET(c,r)                  if(c){typeof(r) _ret_=r;HIKFLOW_ERR("noret failed %s ret=0x%x\n",#c,(int)_ret_); return;}
#define HIKFLOW_ASSER(c,r)                  if(c){typeof(r) _ret_=r;HIKFLOW_ERR("assert failed %s ret=0x%x\n",#c,(int)_ret_);}
#define HIKFLOW_KEY_RET(c,r,keys)           if(c){typeof(r) _ret_=r;HIKFLOW_ERR("key ret failed %s KEY[%s] ret=0x%x\n",#c,keys,(int)_ret_); return _ret_;}
#define HIKFLOW_EXIT(c,r,exit)              if(c){typeof(r) _ret_=r;HIKFLOW_ERR("exit failed %s ret=0x%x\n",#c,(int)_ret_); goto exit;}
#define HIKFLOW_KEY_EXIT(c,r,exit,keys)	    if(c){typeof(r) _ret_=r;HIKFLOW_ERR("key exit failed %s KEY[%s]  ret=0x%zx\n",#c,keys,(size_t)_ret_); goto exit;}

/**
* @brief some maximum values used by the hikflow demo
*/
#define HIKFLOW_DEMO_MAX_OUTPUT_BOX_NUM     (64)        /*!< maximum number of targets in demo*/
#define HIKFLOW_DEMO_MAX_STRING_LEN         (256)       /*!< maximum number of characters in a string */
#define HIKFLOW_DEMO_MAX_STRING_LEN_EX      (256+64)    /*!< maximum number of characters in a string expand*/
#define HIKFLOW_DEMO_MAX_JSON_LEN           (8*1024)    /*!< maximum size of json buffer */
#define HIKFLOW_DEMO_MAX_ATTR_NUM           (80)        /*!< maximum number of model attribute labels, for example:cat,dog,human */
#define HIKFLOW_DEMO_MAX_ABNORMAL_CLASS_NUM (32)        /*!< maximum number of configured abnormal classes */

#define HIKFLOW_DEMO_DEF_ALG_FPS            (12.5)      /*!< default processing frame rate */
#define HIKFLOW_DEMO_DEF_VB_CNT             (4)         /*!< default depth of video buffers */
#define HIKFLOW_DEMO_DEF_ALARM_INTER	    (1)         /*!< default alarm interval, unit is second */
#define HIKFLOW_DEMO_DEF_ALARM_QUALITY	    (90)        /*!< default alarm image quality */
#define HIKFLOW_DEMO_DEF_FILE_MODE	        (0644)      /*!< coredump used */


#define HIKFLOW_DEMO_VB_POOL_W_H_ALIGN      (16)                /*!< video buffer alignment */
#define HIKFLOW_DEMO_WAIT_TIME_OUT          (200*1000)          /*!< time threshold,int is used with stack_mng_proc_time which can print the cost when the threshold is be exceeded */
#define HIKFLOW_DEMO_CTRL_MAGIC             (0xFADACABA)        /*!< magic number */

#define HIKFLOW_DEMO_PROC_FROM_CAM          (0)                 /*!< camera input mode */
#define HIKFLOW_DEMO_PROC_FROM_FILE         (1)                 /*!< file-reading mode */
#define HIKFLOW_DEMO_MODEL_INFOLEN          (256)               /*!< specific value within the model  */

#define HIKFLOW_DEMO_CONFIG_FILE            "hikflow_config.json"       /*!< config file */
#define HIKFLOW_DEMO_ATTR_FILE              "hikflow_attr.json"         /*!< model property label file */
#define HIKFLOW_DEMO_VERSION                "hf_demo_v2.0.0"            /*!< demo version */
#define HIKFLOW_DEMO_APP_ID 				"19999" 					/*!< humandetect_detect_demo_v2 default appid */

/**
* @brief os memory request and release function
*/
#define HIKFLOW_DEMO_ALLOC(_SIZE_,_ALIGGN_) memalign(_ALIGGN_,_SIZE_)
#define HIKFLOW_DEMO_FREE(_PVOID_)          free(_PVOID_)
#define HIKFLOW_DEMO_MEM_ALIGN              (64)

/**
* @brief align forward and backward
*/
#define HIKFLOW_DEMO_ALIGN_BACK(val, align)  (((val) / (align)) * (align))                              /*!< align forward */
#define HIKFLOW_DEMO_ALIGN_FOWARD(val, align) (HIKFLOW_DEMO_ALIGN_BACK(((val) + (align)-1), (align)))   /*!< align backward */

/**
* @brief algorithm related input configuration parameters
*/
typedef struct _HIKFLOW_DEMO_NET_IN_ST_ 
{
    int     width;              /*!< image width, derived from the model */
    int     height;             /*!< image height, derived from the model */
    int     batch_cnt;          /*!< the number of data can be input in one time, derived from the model */
    int     core_proc_type;     /*!< processor type (8 is HIKFLOW_DL_PROC_TYPE_4 7 is HIKFLOW_DL_PROC_TYPE_3), derived from the model */
    int     channel_cnt;        /*!< channel num,such as the channel num of yuv or rgb is 3, derived from the model */
    int     data_type;          /*!< image type, 1 means BGR,2 means NV21, 0 meams NV12,derived from the model */ 
    int     fps;                /*!< frame rate,see OPDEVSDK_VIDEO_FRAME_RATE_EN,derived from the hikflow_config.json, defualt is OPDEVSDK_VIDEO_FRAME_RATE_12_5, corresponding to 12.5fps */
    int     vb_cnt;             /*!< video buffer counts, associated with opdevsdk_mem_createVbPoolGrp, store the scaled image depth for the mscale module,derived from the hikflow_config.json,defualt is 4 */
} HIKFLOW_DEMO_NET_IN_ST;

/**
* @brief input configuration parameters
*/
typedef struct _HIKFLOW_DEMO_CONFIGURATION_ST_
{
    int                         data_mem_type;                                  /*!< memory alloced type,0 shows arm memory,1 means that memory is alloced from memory module in bsc library */   
    char                        model_path[HIKFLOW_DEMO_MAX_STRING_LEN_EX];     /*!< model path,include the input path of the main function and the relative path of the model, the relative path is derived from hikflow_config.json("model_path") */
    char                        image_list[HIKFLOW_DEMO_MAX_STRING_LEN_EX];     /*!< image_list includes some files which will takes effect in read file mode, the files will be sent to the algorithm processing thread, see hikflow_config.json */ 

    HIKFLOW_DEMO_NET_IN_ST      net_input;                                      /*!< algorithm related input configuration parameters */

    /*!< alarm related configuration */
    int                         b_alarm;                                        /*!< whether to alarm, the alarm process mainly includes displaying alarms target on the preview(red color), jpeg encoding, and uploading alarm messages. 0 represents done nothing, 1 represents processing alarms, derived from hikflow_config.json, default 1 */    
    int                         alarm_intrval;                                  /*!< alarm interval, unit is second. the demo use the param to calcalate the alarm time , derived from hikflow_config.json, default to 1 second */    
    int                         sel_class;                                      /*!< whether to filter targets for a specific attribute label,-1 represents  no filtering. For example, if filtering person targets,set the sel_class is 14 in hikflow_config.json */    
    int                         abnormal_class_count;                              /*!< abnormal class count configured by app.conf */
    int                         abnormal_classes[HIKFLOW_DEMO_MAX_ABNORMAL_CLASS_NUM]; /*!< abnormal class list configured by app.conf */

} HIKFLOW_DEMO_CONFIGURATION_ST;

/**
* @brief target rectangular box coordinates
*/
typedef struct _HIKFLOW_DEMO_BBOX_ST_
{
    float       x;          /*!< x-axis coordinates, uion is pixels */       
    float       y;          /*!< y-axis coordinates, uion is pixels */  
    float       width;      /*!< width, uion is pixels */  
    float       height;     /*!< height, uion is pixels */  
}HIKFLOW_DEMO_BBOX_ST;

/**
* @brief hikflow output blob information
*/
typedef struct _HIKFLOW_DEMO_BOX_INFO_ST_ 
{
    float                   class_type;     /*!< attribute label */   
    float                   score;          /*!< score */   
    HIKFLOW_DEMO_BBOX_ST    bbox;           /*!< target coordinates */   
    float                   batch_idx;      /*!< batch index */   
} HIKFLOW_DEMO_BOX_INFO_ST;

/**
* @brief normalization rules
*/
typedef struct _HIKFLOW_DEMO_RULE_
{
    unsigned int            point_num;                  		/*!< number of fixed points in the rule, with a maximum value of HIKFLOW_DEMO_MAX_POINT_NUM */        
    OPDEVSDK_POS_POINT_ST   point[HIKFLOW_DEMO_MAX_POINT_NUM];  /*!< regular vertex coordinate set, normalized value, range [0,1] */ 
}HIKFLOW_DEMO_RULE;

/**
* @brief single attribute label
*/
typedef struct _HIKFLOW_DEMO_MODEL_ATTR_
{
    int     idx;            /*!< index value, corresponding to the model output class_type */     
    char    name[64];       /*!< attribute string */                   		
}HIKFLOW_DEMO_MODEL_ATTR;

/**
* @brief attribute label list
*/
typedef struct _HIKFLOW_DEMO_MODEL_ATTR_LIST_
{
    int     attr_num;                                               /*!< number of attribute labels */ 
    HIKFLOW_DEMO_MODEL_ATTR  attr[HIKFLOW_DEMO_MAX_ATTR_NUM];       /*!< attribute labels */                          		
}HIKFLOW_DEMO_MODEL_ATTR_LIST;

/**
* @brief single model related information
*/
typedef struct _HIKFLOW_DEMO_NET_INFO_ST_ 
{
    HIKFLOW_DEMO_NET_IN_ST              net_input;                              /*!< algorithm related input configuration */ 
    void                                *net_handle;                            /*!< algorithm handle, return by opdevsdk_hikflow_Creat e*/ 
    
    void                                *model_handle;                          /*!< model handle,return by opdevsdk_hikflow_CreateModel */ 
    void                                *model_buffer;                          /*!< model data buffer virtual address */ 
    int                                 model_size;                             /*!< model data buffer length */ 
    unsigned long long                  model_phy_base;                         /*!< model data buffer physical address */ 
    OPDEVSDK_HKA_MEM_TAB_ST             model_tab[OPDEVSDK_HKA_MEM_TAB_NUM];    /*!< model memory table that will be allocated,return by opdevsdk_hikflow_GetModelMemSize */ 
    OPDEVSDK_HKA_MEM_TAB_ST             mem_tab[OPDEVSDK_HKA_MEM_TAB_NUM];      /*!< net memory table that will be allocated,return by opdevsdk_hikflow_GetMemSize */ 
    OPDEVSDK_HIKFLOW_PARAM_ST           hf_info;                                /*!< opdevsdk_hikflow_Process input parameters,need to input information such as images, algorithm handles, etc */ 
    int                                 model_mem_used;                         /*!< model used memory */ 
    int                                 model_file_used;                        /*!< memory for storing model files requested */ 
    int                                 net_mem_used;                           /*!< memory requested by algorithm */ 
    HIKFLOW_DEMO_MODEL_ATTR_LIST        attr_list;                              /*!< attribute labels */              
    
} HIKFLOW_DEMO_NET_INFO_ST;

/**
* @brief  signal correlation processing
*/
typedef struct _HIKFLOW_DEMO_STATUS_EXIT_
{
	volatile int		signal_id; 			/*!< signal received */
	volatile siginfo_t  *info;              /*!< register and other stack information */
    volatile void       *ptr;               /*!< pc pointer */
	volatile int		signal_abnl; 	    /*!< oher signals outside of 15 is abnormal */
	volatile int		flg_atexit; 		/*!< received exit call flag */
	volatile int		flg_exit; 		    /*!< Process exit flag, sent to TSK_hikflow_destroy used,1 represents exit, 0 represents not exit */
	volatile int		flg_exit_re; 		/*!< resource release completion flag, returned to singal callback */	
	volatile int		res[6]; 		  
}HIKFLOW_DEMO_STATUS_EXIT;

/**
* @brief hikflow demo private struct
*/
typedef struct _HIKFLOW_DEMO_CTRL_
{
	unsigned int                        magic;                                      /*!< magic number */

    /*!< global param */    
    int                                 b_init;                                     /*!< initialize completion flag */
	int 								app_chan;		                            /*!< application channel */
	int 								idx;		                                /*!< channel index */
    HIKFLOW_DEMO_CONFIGURATION_ST       hikflow_config;                             /*!< configuration information, derived from hikflow_config.json and model */         
    OPDEVSDK_SYS_ABILITY_ST             ability;                                    /*!< device capacity */
    char                                proc_name[HIKFLOW_DEMO_MAX_STRING_LEN];     /*!< process name */
    char                                path[HIKFLOW_DEMO_MAX_STRING_LEN];          /*!< execution file path */
    char                                version[64];                                /*!< demo version */
    int                                 mem_used_all;                               /*!< total amount of cmm memory used */
    int                                 proc_type;                                  /*!< processing mode, 0 represents camera input mode, and 1 represents file-reading mode */
    int                                 b_start;                                    /*!< processing enable flag */
    unsigned long long                  proc_times;                                 /*!< target continuous existence time */

    int                                 json_max_size;                              /*!< json buffer maximum length */
    void                                *json_buf;                                  /*!< json buffer virtual address */
    void                                *json_buf_phy;                              /*!< json buffer physical address */
    pthread_mutex_t                     mutex;                                      /*!< locks for configuring rules */
    void                                *jpegenc_handle;                            /*!< jpeg encoder handle */
    int                                 jpegenc_max_size;                           /*!< jpeg encoder stream buffer maximum length */
    void                                *jpegenc_buf;                               /*!< jpeg encoder stream buffer virtual address */
    void                                *jpegenc_buf_phy;                           /*!< jpeg encoder stream buffer physical address */
    int                                 jpegenc_len;                                /*!< jpeg encoder stream length */

    OPDEVSDK_HKA_VERSION_ST             hikflow_ver;                                /*!< hikflow version */  
    OPDEVSDK_SYS_VERSION_ST             bsc_ver;                                    /*!< bsc version */  
    HIKFLOW_DEMO_RULE                   rule;                                       /*!< rule */    
    HIKFLOW_DEMO_ALARM_IMAGE_INFO       image;                                      /*!< alarm information */         

    /*!< single algorithm param */    
    HIKFLOW_DEMO_NET_INFO_ST            net_info;                                   /*!< single model parameters */  

    /*!< bsc scale param */    
    int                                 group_id;                                   /*!< video buffers group id, returned from opdevsdk_mem_createVbPoolGrp */  
    void                                *mscale_hdl;                                /*!< mscale handle is used for 1-IN-N-OUT scales,which is returned from opdevsdk_mscale_create */  
    HIKFLOW_DEMO_STATUS_EXIT            exit_stat;                                  /*!< exit signal information */  

    /*!< bsc vin yuv get thread records */    
    int                                 yuv_get_lost_times;                         /*!< number of failed counts to obtain vin images */  
    int                                 yuv_send_lost_times;                        /*!< number of failed counts to send vin images */  
    int                                 yuv_rel_lost_times;                         /*!< number of failed counts to release vin images */  
    int                                 yuv_suc_times;                              /*!< number of successful counts to vin_get thread */  
	ISFW_STAT_PROC_STAT	                vin_proc;                                   /*!< statidtics of vin_get thread */  
    int                                 vin_thread_exit;                            /*!< vin_get thread exit flag */  

    /*!< algorithm thread processing status */    
    int                                 mscale_get_lost_times;                      /*!< number of failed counts to obtain mscale images */  
    int                                 mscale_rel_lost_times;                      /*!< number of failed counts to release mscale images */
    int                                 mscale_get_suc_times;                       /*!< number of successful counts to obtain mscale images */  
    int                                 mscale_rel_suc_times;                       /*!< number of successful counts to release mscale images */
    int                                 net_suc_times;                              /*!< number of successful counts to process algorithm */  
    int                                 net_lost_times;                             /*!< number of failed counts to process algorithm */  
	ISFW_STAT_PROC_STAT	                net_proc;                                   /*!< net process thread,net processing time statistics */ 
	ISFW_STAT_PROC_STAT	                frame_proc;                                 /*!< statidtics of net processing thread */ 
    int                                 net_thread_exit;                            /*!< net processing thread exit flag */  

    /*!< algorithm thread alarm status */    
    ISFW_STAT_PROC_STAT                 alarm_proc;                                 /*!< statidtics of alarm processing */  
    int                                 alarm_lost_times;                           /*!< number of failed counts to make alarm processing */      
    int                                 alarm_suc_times;                            /*!< number of successful counts to make alarm processing */  
    ISFW_STAT_PROC_STAT                 jpegenc_proc;                               /*!< statidtics of jpeg encode processing */  
    int                                 jpegenc_lost_times;                         /*!< number of failed counts to make jpeg encoder */  
    int                                 jpegenc_suc_times;                          /*!< number of successful counts to make jpeg encoder */  

}HIKFLOW_DEMO_CTRL;

/** 
* @brief            hikflow net initialization funtion
*
* @param[in] 		pCtrl   handel     
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_proc_init(HIKFLOW_DEMO_CTRL* pCtrl);

/** 
* @brief            net processing function in camera input mode  
*
* @param[in] 		pCtrl       handel     
* @param[in] 		pfrm        image frame information     
* @param[out] 		ptarget     returned target information   
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_proc_alg_from_cam(HIKFLOW_DEMO_CTRL* pCtrl,OPDEVSDK_VIDEO_FRAME_INFO_ST *pfrm,OPDEVSDK_POS_TARGET_LIST_INFO_ST *ptarget);

/** 
* @brief            net processing function in file-reading mode
*
* @param[in] 		pCtrl       handel     
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_proc_alg_from_file(HIKFLOW_DEMO_CTRL* pCtrl);

#ifdef __cplusplus
}
#endif

#endif

