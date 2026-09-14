/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         hikflow_demo.c
* @brief        hikflow demo main code
*
* @author       guanjiajun
*
* @date         2023/8/24
* @version      2.0.0
* @note         1. supports to set static param,such as: video buffer depth,fps....
*               2. supports to set printf level on line
*               3. supports to print status statistics
*               4. supports alarm process,include: jpeg encoding,upload json message
*               5. supports to select CAMERA mode or FILE-reading mode
*****************************************************************************/

#include "hikflow_demo_priv.h"
#include "event_bridge.h"
#include "module_flags.h"
#include <sys/time.h>

/**
* @brief  hikflow demo ctrol struct
*/
static HIKFLOW_DEMO_CTRL hikflow_ctrl = {0};
static int g_hikflow_infer_interval_ms = 0;
static char g_hikflow_model_path_override[HIKFLOW_DEMO_MAX_STRING_LEN_EX] = {0};
static char g_hikflow_abnormal_classes_override[128] = {0};

void hikflow_demo_set_infer_interval_ms(int interval_ms)
{
    if (interval_ms < 0)
    {
        interval_ms = 0;
    }
    g_hikflow_infer_interval_ms = interval_ms;
}

void hikflow_demo_set_runtime_options(const char *model_path, const char *abnormal_classes)
{
    if (model_path != NULL && model_path[0] != '\0')
    {
        snprintf(g_hikflow_model_path_override, sizeof(g_hikflow_model_path_override), "%s", model_path);
    }
    else
    {
        g_hikflow_model_path_override[0] = '\0';
    }

    if (abnormal_classes != NULL && abnormal_classes[0] != '\0')
    {
        snprintf(g_hikflow_abnormal_classes_override, sizeof(g_hikflow_abnormal_classes_override), "%s", abnormal_classes);
    }
    else
    {
        g_hikflow_abnormal_classes_override[0] = '\0';
    }
}

static void hikflow_demo_apply_runtime_options(HIKFLOW_DEMO_CTRL *pCtrl)
{
    char classes[sizeof(g_hikflow_abnormal_classes_override)];
    char *token = NULL;
    char *saveptr = NULL;

    if (pCtrl == NULL)
    {
        return;
    }

    if (g_hikflow_model_path_override[0] != '\0')
    {
        snprintf(pCtrl->hikflow_config.model_path, sizeof(pCtrl->hikflow_config.model_path), "%s", g_hikflow_model_path_override);
        HIKFLOW_LOG("runtime override model_path %s\n", pCtrl->hikflow_config.model_path);
    }

    pCtrl->hikflow_config.abnormal_class_count = 0;
    memset(pCtrl->hikflow_config.abnormal_classes, 0, sizeof(pCtrl->hikflow_config.abnormal_classes));
    if (g_hikflow_abnormal_classes_override[0] == '\0')
    {
        return;
    }

    snprintf(classes, sizeof(classes), "%s", g_hikflow_abnormal_classes_override);
    token = strtok_r(classes, ",", &saveptr);
    while (token != NULL && pCtrl->hikflow_config.abnormal_class_count < HIKFLOW_DEMO_MAX_ABNORMAL_CLASS_NUM)
    {
        while (*token == ' ' || *token == '\t')
        {
            token++;
        }
        if (*token != '\0')
        {
            pCtrl->hikflow_config.abnormal_classes[pCtrl->hikflow_config.abnormal_class_count++] = atoi(token);
        }
        token = strtok_r(NULL, ",", &saveptr);
    }
    HIKFLOW_LOG("runtime abnormal_class_count %d\n", pCtrl->hikflow_config.abnormal_class_count);
}

static long long hikflow_demo_now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
}

static int hikflow_demo_should_skip_infer(OPDEVSDK_VIDEO_FRAME_INFO_ST *frame, long long *last_infer_ms)
{
    long long now_ms;
    if (g_hikflow_infer_interval_ms <= 0)
    {
        return 0;
    }

    now_ms = frame->timeStamp > 0 ? (long long)frame->timeStamp / 1000LL : hikflow_demo_now_ms();
    if (*last_infer_ms == 0 || now_ms - *last_infer_ms >= g_hikflow_infer_interval_ms)
    {
        *last_infer_ms = now_ms;
        return 0;
    }
    return 1;
}

static void hikflow_demo_print(const char *modulename, ISFW_LOG_LEVEL_ENUM level, const char *filename, const int line, const char *str)
{
	int print_level = 0xF;
	switch (level)
	{
		case ISFW_LOG_LEVEL_DEBUG:
			print_level = LOG_DEBUG;
			break;
		case ISFW_LOG_LEVEL_INFO:
			print_level = LOG_NOTICE;
			break;
		case ISFW_LOG_LEVEL_WARN:
			print_level = LOG_CRIT;
			break;
		case ISFW_LOG_LEVEL_ERROR:
			print_level = LOG_CRIT;	
			break;
		case ISFW_LOG_LEVEL_DPT:
			print_level = LOG_ALERT;
			break;
		default:
			break;
	}

    if(print_level == 0xF)
	{
		return;
	}

	syslog(LOG_LOCAL2|print_level,str);
}

/*!< initialize the syslog,it will save the logs in the path /var/log/conaintrt name/dsp.log */
static void hikflow_demo_init_syslog()
{
    openlog("hikflow_demo",LOG_CONS,LOG_LOCAL2);
	isfw_log_set_cb_fxn(hikflow_demo_print);  
    isfw_log_init();

    /*!< set LOG level */
    isfw_log_set_level_all(0);
}

static void hikflow_demo_deinit_syslog()
{
    isfw_log_deinit();
    closelog();
}

/*!< set coredump */
static int hikflow_demo_open_core_dump(char *pCorePid, char *pCorePath)
{
    int ret = -1, fd = -1;
    struct rlimit limit;
    struct rlimit limit_set;

    do{
        /*!< 1, set core ulimit */
        if (getrlimit(RLIMIT_CORE, &limit))
        {
            HIKFLOW_ERR("get resource limit fail!\n");
            break;
        }

        limit_set.rlim_cur = limit_set.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &limit_set))
        {
            limit_set.rlim_cur = limit_set.rlim_max = limit.rlim_max;
            if (limit.rlim_max != RLIM_INFINITY)
            {
                //HIKFLOW_ERR( "CORE: cur=0x%x, max=0x%x\n",
                //limit.rlim_cur, limit.rlim_max);
            }

            if (setrlimit(RLIMIT_CORE, &limit_set))
            {
                HIKFLOW_ERR( "set core ulimited fail!\n");
                break;
            }
        }

        /*!< 2, set core use pid */
        if (pCorePid && strlen(pCorePid) > 0)
        {
            fd = open("/proc/sys/kernel/core_uses_pid", O_RDWR|O_NDELAY|O_TRUNC, HIKFLOW_DEMO_DEF_FILE_MODE);
            if (fd < 0)
            {
                HIKFLOW_ERR( "open core_uses_pid fail!\n");
                break;
            }
            if (strlen(pCorePid) != write(fd, pCorePid, strlen(pCorePid)))
            {
                HIKFLOW_ERR("set core_uses_pid fail!\n");
                close(fd);
                break;               
            }
            close(fd);
        }        

        /*!< 3, set core dump open succ */
        ret = 0;
        HIKFLOW_LOG("set core dump open succ!\n"); 
    }while(0);

    return ret;
}

/*!< set core dump param */
static void hikflow_demo_set_core_dump(void)
{
    char szCorePid[32];
    char szCorePath[128]={"hf_core_dump"};

    strcpy(szCorePid, "1");
    strcpy(szCorePath, "/coredump/");
    strcat(szCorePath, "core-%s-%e-%p-%t");

    if (0 != hikflow_demo_open_core_dump(szCorePid, szCorePath))
    {
        HIKFLOW_ERR("<hf> ========= gdb core dump open fail! =========\n\n");
    }
    else
    {
        HIKFLOW_ERR("<hf> sys gdb core open succ ~~~ \n");
    }
}

static void hikflow_demo_sig_proc(int signum, siginfo_t* info, void*ptr, int times)
{
    int end_times = times;
    int start_times = 0;
        
    /*!< def */
    if(signum != 0)
    {
        signal(signum,SIG_DFL);
        if(signum != SIGTERM)
        {
            hikflow_ctrl.exit_stat.signal_abnl = 1;
        }
        
        hikflow_ctrl.exit_stat.signal_id = signum;
		hikflow_ctrl.exit_stat.info = info;
		hikflow_ctrl.exit_stat.ptr = ptr;
		hikflow_ctrl.exit_stat.flg_exit = 1;
    }

    /*!< wait relase resource,now wait 5s */
    while(hikflow_ctrl.exit_stat.flg_exit_re != 1 && (start_times < end_times))
    {
        usleep(100*1000);
        start_times++;
    };    
}

static void hikflow_demo_signal_call_back_exit()
{
    /*!< pay attention: the function can't include printf function */
    //dprint("\n hikflow_demo_signal_call_back_exit start\n"); 
    int signum = 0;
    hikflow_ctrl.exit_stat.flg_atexit = 1;
    hikflow_demo_sig_proc(signum, NULL, NULL, 50);
    
    if(0 == hikflow_ctrl.exit_stat.signal_id)
    {
        _exit(0);
    }
    
    //dprint("\nhikflow_demo_signal_call_back_exit end\n"); 
}

static void hikflow_demo_sig_handler(int signum, siginfo_t* info, void*ptr)
{
    //dprint("\n hikflow_demo_sig_handler start signum %d\n",signum);	
    /*!< pay attention: the function can't include printf function */

    /*!< give signal to tsk_destroy,and wait 5s at most*/
    hikflow_demo_sig_proc(signum, info, ptr, 50);
    
    /*!< after tsk_hikflow_destroy,exit*/
    if(hikflow_ctrl.exit_stat.signal_abnl == 0)
    {
        //dprint("\n hikflow_demo_sig_handler\n");    
        _exit(0);
    }
    //dprint("\n###########hikflow_demo_sig_handler signum %d signal_abnl %d end\n",signum,hikflow_ctrl.exit_stat.signal_abnl);    
}

void  hikflow_demo_catch_sigsegv(void)
{
    struct sigaction action;

    /*!< set signal callback,the function will release the resource */
    /*!< signal SIGTERM is used for exit in container */
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = hikflow_demo_sig_handler;
    action.sa_flags = SA_SIGINFO;

    /*!< memory lack */
    if(sigaction(SIGSEGV, &action, NULL) < 0)
    {
        perror("sigaction");
    }

    /*!< math calcalate err */
    if(sigaction(SIGFPE, &action, NULL) < 0)
    {
        perror("sigaction");
    }

    /*!< exit in container */
    if(sigaction(SIGTERM, &action, NULL) < 0)
    {
        perror("sigaction");
    }

    /*!< ctrl+c */
    if(sigaction(SIGINT, &action, NULL) < 0)
    {
        perror("sigaction");
    }

    /*!< double free,assert or abort */
	if(sigaction(SIGABRT, &action, NULL) < 0)
    {
        perror("sigaction");
    }

    /*!< bus err */
	if(sigaction(SIGBUS, &action, NULL) < 0)
	{
		perror("sigaction");
	}

    /*!< signal 9-directly exit */
	if(sigaction(SIGKILL, &action, NULL) < 0)
	{
		perror("sigaction");
	}

    struct sigaction actionPipe;
    memset(&actionPipe, 0, sizeof(actionPipe));
    actionPipe.sa_handler = SIG_IGN;
    actionPipe.sa_flags = 0;

	if(sigaction(SIGPIPE, &actionPipe, 0) < 0)
	{
		perror("sigaction");
	}
}

/*!< initialize monitor */
static void hikflow_demo_init_monitor()
{
    /*!< set core dump */
    hikflow_demo_set_core_dump();

    /*!< catch the key information after process exit,such as pc pointer,registers informations and so on */
    hikflow_demo_catch_sigsegv();

    /*!< [ATTENTION] catch exit signal for release resource */
    atexit(hikflow_demo_signal_call_back_exit);        
}

/*!< de-initialize demo */
static int hikflow_demo_deinit(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);
    /*!<  release resource */    
    stack_mng_deinit();   
    hikflow_demo_deinit_syslog();
    memset(pCtrl,0,sizeof(HIKFLOW_DEMO_CTRL));
    printf("hikflow_demo_deint ok\n");
    return HIKFLOW_DEMO_OK;
}

/*!<  destroy pthread for exit */  
static void TSK_hikflow_destroy(void *arg)
{
    HIKFLOW_DEMO_CTRL* pCtrl = (HIKFLOW_DEMO_CTRL*)arg;
    HIKFLOW_NORET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);

    while(1)
    {
        /*!< wait exit flag */
        if(1 == hikflow_ctrl.exit_stat.flg_exit || 1 == hikflow_ctrl.exit_stat.flg_atexit)
        {
            isfw_log_dprint("\n hikflow_demo_deint begin quit_c %d quit_s %d\n",hikflow_ctrl.exit_stat.flg_atexit,hikflow_ctrl.exit_stat.signal_id);

			/*!< print memory */
            stack_mng_mem_stat_prt();
			/*!< destroy function */
            _stack_mng_proc_time_(int ret = hikflow_demo_deinit(pCtrl),HIKFLOW_DEMO_WAIT_TIME_OUT);
            isfw_log_dprint("\n hikflow_demo_deint end\n");   
            /*!< send back finishing flag */
            hikflow_ctrl.exit_stat.flg_exit_re = 1;
            //hikflow_ctrl.exit_stat.flg_exit  =0;
            return;
        }   
        
        usleep(40 * 1000);
    }
}

/*!< initialization for alarm */
static int hikflow_demo_init_alarm(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_RET(0 == pCtrl->hikflow_config.b_alarm,HIKFLOW_DEMO_OK); /*!< retrun ok directly if not support */

    /*!< initialize json buffer for store alarm json message,pay attention on: type,size and proc_type */
    STACK_MNG_MEM_INFO mem_info = {NULL,NULL,HIKFLOW_DEMO_MAX_JSON_LEN,STACK_MNG_MEM_TYPE_CMM,0,STACK_MNG_MEM_PROC_TYPE_ALLOC,"alarm_json"};
    mem_info.size = HIKFLOW_DEMO_MAX_JSON_LEN;

    /*!< allocate cmm memory */
    int ret = stack_mng_mem_get(&mem_info);
    HIKFLOW_EXIT(ret != STACK_MNG_OK,ret,err0);

    pCtrl->json_max_size = mem_info.size;
    pCtrl->json_buf = mem_info.vaddr;
    pCtrl->json_buf_phy = mem_info.paddr;   
    pCtrl->mem_used_all += pCtrl->json_max_size;    

    /*!< initialize jpeg stream buffer for store stream after encoder,pay attention on: type,size and proc_type */
    OPDEVSDK_VIN_CHN_ST *pchn_info = &pCtrl->ability.vinAbili.chnInfo[pCtrl->idx];
    STACK_MNG_MEM_INFO mem_info_stream = {NULL,NULL,0,STACK_MNG_MEM_TYPE_CMM,0,STACK_MNG_MEM_PROC_TYPE_ALLOC,"jpegenc"};
    mem_info_stream.size = pchn_info->width * pchn_info->height * 3 / 2;
    ret = stack_mng_mem_get(&mem_info_stream);
    HIKFLOW_EXIT(ret != STACK_MNG_OK,ret,err1);
    pCtrl->jpegenc_max_size = mem_info_stream.size;
    pCtrl->jpegenc_buf = mem_info_stream.vaddr;
    pCtrl->jpegenc_buf_phy = mem_info_stream.paddr;   
    pCtrl->mem_used_all += pCtrl->jpegenc_max_size;    

    /*!< creat jpeg encode handle */
    ret = opdevsdk_jpegenc_create(&pCtrl->jpegenc_handle,0);
    HIKFLOW_KEY_EXIT((ret != OPDEVSDK_S_OK || pCtrl->jpegenc_handle == NULL),ret,err2,"opdevsdk_jpegenc_create err");
    stack_mng_push((void *)pCtrl->jpegenc_handle,opdevsdk_jpegenc_destroy);

    /*!< initialize  mutex for set param */
    pthread_mutex_init(&pCtrl->mutex, NULL);
    HIKFLOW_DBG("hikflow_demo_init_alarm pCtrl->mutex %p ok\n",&pCtrl->mutex);
    return HIKFLOW_DEMO_OK;
err2:
    /*!< give address,proc_type when free  */
    mem_info_stream.proc_type = STACK_MNG_MEM_PROC_TYPE_FREE;
    ret = stack_mng_mem_release(&mem_info_stream);
    pCtrl->jpegenc_buf_phy = NULL;
    pCtrl->jpegenc_buf = NULL;    
    pCtrl->mem_used_all -= pCtrl->jpegenc_max_size;    
err1:
    mem_info.proc_type = STACK_MNG_MEM_PROC_TYPE_FREE;
    ret = stack_mng_mem_release(&mem_info);
    pCtrl->json_buf_phy = NULL;
    pCtrl->json_buf = NULL;    
    pCtrl->mem_used_all -= pCtrl->json_max_size;    
err0:
    HIKFLOW_ERR("hikflow_demo_init_alarm err\n");
    return HIKFLOW_DEMO_OK;
}

/*!< de-initialize alarm */
static int hikflow_demo_deinit_alarm(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);

    /*!< destroy  mutex*/
   	pthread_mutex_destroy(&pCtrl->mutex);

    /*!< free json buffer */
    STACK_MNG_MEM_INFO mem_info = {NULL,NULL,HIKFLOW_DEMO_MAX_JSON_LEN,STACK_MNG_MEM_TYPE_CMM,0,STACK_MNG_MEM_PROC_TYPE_FREE,"alarm_json"};
    mem_info.size = HIKFLOW_DEMO_MAX_JSON_LEN;
    mem_info.paddr = pCtrl->json_buf_phy;
    mem_info.vaddr = pCtrl->json_buf;
    int ret = stack_mng_mem_release(&mem_info);
    HIKFLOW_ASSER(ret != STACK_MNG_OK,ret);  
    pCtrl->json_buf_phy = NULL;
    pCtrl->json_buf = NULL;
    pCtrl->mem_used_all -= pCtrl->json_max_size;    

    /*!< free jpegenc buffer */
    STACK_MNG_MEM_INFO mem_info_stream = {NULL,NULL,HIKFLOW_DEMO_MAX_JSON_LEN,STACK_MNG_MEM_TYPE_CMM,0,STACK_MNG_MEM_PROC_TYPE_FREE,"jpegenc"};
    mem_info_stream.size = pCtrl->jpegenc_max_size;
    mem_info_stream.paddr = pCtrl->jpegenc_buf_phy;
    mem_info_stream.vaddr = pCtrl->jpegenc_buf;
    ret = stack_mng_mem_release(&mem_info_stream);
    HIKFLOW_ASSER(ret != STACK_MNG_OK,ret);  
    pCtrl->jpegenc_buf_phy = NULL;
    pCtrl->jpegenc_buf = NULL;
    pCtrl->mem_used_all -= pCtrl->jpegenc_max_size;    
    HIKFLOW_LOG("hikflow_demo_deinit_alarm ok\n");
    
    return HIKFLOW_DEMO_OK;
}

/*!< initialize the hikflow lib */
static int hikflow_demo_init_alg(HIKFLOW_DEMO_CTRL* pCtrl)
{
    // int ret = 0;
    // HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);

    // /*!< get hikflow version */
    // ret=opdevsdk_hikflow_GetVersion(&pCtrl->hikflow_ver);
    // HIKFLOW_ASSER(ret != OPDEVSDK_SCHE_S_OK,HIKFLOW_DEMO_ERR_FAILED);
    
    // /*!< hikflow initilization  */
    // ret = hikflow_proc_init(pCtrl);
    // HIKFLOW_KEY_RET(HIKFLOW_DEMO_OK != ret,ret,"hikflow_proc_init err");

    // HIKFLOW_LOG("hikflow_demo_init_alg(ver:%s) ok\n", pCtrl->hikflow_ver.version);
    // return HIKFLOW_DEMO_OK;

    HIKFLOW_LOG("******************************hikflow_demo_init_alg 0\n");
    int ret = 0;
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);

    /*!< get hikflow version */
    ret=opdevsdk_hikflow_GetVersion(&pCtrl->hikflow_ver);
    HIKFLOW_ASSER(ret != OPDEVSDK_SCHE_S_OK,HIKFLOW_DEMO_ERR_FAILED);
    
    HIKFLOW_LOG("******************************hikflow_demo_init_alg 1\n");
    /*!< hikflow initilization  */
    ret = hikflow_proc_init(pCtrl);
    HIKFLOW_KEY_RET(HIKFLOW_DEMO_OK != ret,ret,"hikflow_proc_init err");
	HIKFLOW_LOG("******************************hikflow_demo_init_alg 2\n");
    HIKFLOW_LOG("hikflow_demo_init_alg(ver:%s) ok\n", pCtrl->hikflow_ver.version);
    return HIKFLOW_DEMO_OK;

}

/*!< initialize the scheduler lib,which is the basic lib of hikflow */
static int hikflow_demo_init_sche(HIKFLOW_DEMO_CTRL* pCtrl)
{
    int ret = 0;
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);

    /*!< the function must be called befor hikflow initialization  */
    ret = opdevsdk_sche_init();
    HIKFLOW_KEY_RET(OPDEVSDK_SCHE_S_OK != ret,ret,"opdevsdk_sche_init err");
	stack_mng_push(NULL,(void *)opdevsdk_sche_deinit);

    HIKFLOW_LOG("hikflow_demo_init_sche ok\n");
    return HIKFLOW_DEMO_OK;
}

/*!< initialize the bsc library */
static int hikflow_demo_get_chan(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);
	int ret = 0, valid_chan_num = 1;
	int chanInfo[MAXCHAN] = {0};
	void *data = NULL;
	
    /*!< call function form bll */
	ret = get_app_chan(HIKFLOW_DEMO_APP_ID, chanInfo, &valid_chan_num, data);
	if(HIKFLOW_DEMO_OK!=ret || valid_chan_num>MAXCHAN || valid_chan_num<1 || chanInfo[0]<1)
	{
	    HIKFLOW_ERR("get_app_chan err ret %d, valid_chan_num %d, chanInfo[0] %d\n",ret, valid_chan_num, chanInfo[0]);
    	pCtrl->app_chan = 1;
	}
	else
	{
		pCtrl->app_chan = chanInfo[0];
	}
    HIKFLOW_LOG("pCtrl->app_chan %d\n",pCtrl->app_chan);
    return HIKFLOW_DEMO_OK;
}

/*!< initialize the bsc library */
static int hikflow_demo_init_bsc(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);
    int index = -1;
    /*!< it will initialize the bsc library */
    int ret = opdevsdk_sys_init(); 
    HIKFLOW_KEY_RET(OPDEVSDK_S_OK != ret,ret,"opdevsdk_sys_init err");
	stack_mng_push(NULL,(void *)opdevsdk_sys_deinit);

    /*!< get bsc library capabilities  */
    ret = opdevsdk_sys_getAbility(&pCtrl->ability);
    HIKFLOW_ASSER(ret != OPDEVSDK_S_OK,HIKFLOW_DEMO_ERR_FAILED);
    ret = hikflow_demo_get_chan(pCtrl);
    HIKFLOW_ASSER(ret != OPDEVSDK_S_OK,HIKFLOW_DEMO_ERR_FAILED);

    /*!< print the abilities */
    HIKFLOW_LOG("opdevsdk_sys_init mmSize %dKB cacheSize %dKB noCacheSize %dKB, jpegEncAbili %d jpegDecAbili %d scaleAbili %d\n", \
        pCtrl->ability.mmSize, pCtrl->ability.cacheSize, pCtrl->ability.noCacheSize, pCtrl->ability.jpegEncAbili, pCtrl->ability.jpegDecAbili, pCtrl->ability.scaleAbili);

    for(int i = 0;i < pCtrl->ability.vinAbili.chnNum;i++)
    {
        HIKFLOW_LOG("opdevsdk_init chan_id %d w %d h %d ,fps %f\n", \
            pCtrl->ability.vinAbili.chnInfo[i].chan, pCtrl->ability.vinAbili.chnInfo[i].width, \
            pCtrl->ability.vinAbili.chnInfo[i].height, pCtrl->ability.vinAbili.chnInfo[i].fps);
        if(pCtrl->app_chan == pCtrl->ability.vinAbili.chnInfo[i].chan)
        {
            index = i;
        }
    }

    /*!< can't find,to use the first channel */
    if(index == -1)
    {
        pCtrl->app_chan = pCtrl->ability.vinAbili.chnInfo[0].chan;
        index = 0;
    }

    pCtrl->idx = index;/*!< record the index of chnInfo in current app_chan */

    /*!< get bsc version */
    OPDEVSDK_SYS_VERSION_ST version = {0};
    ret=opdevsdk_sys_getVersion(&pCtrl->bsc_ver);
    HIKFLOW_ASSER(ret != OPDEVSDK_S_OK,HIKFLOW_DEMO_ERR_FAILED);

    HIKFLOW_LOG("hikflow_demo_init_bsc(ver:%s) ok\n", pCtrl->bsc_ver.version);
    return HIKFLOW_DEMO_OK;
}

/*!< de-initialize the vin module in bsc library */
static int hikflow_demo_deinit_bsc_vin(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);
    int ret = opdevsdk_vin_deinit(pCtrl->app_chan);
    HIKFLOW_KEY_RET(OPDEVSDK_S_OK!= ret,HIKFLOW_DEMO_ERR_FAILED,"opdevsdk_vin_deinit err");    
    return HIKFLOW_DEMO_OK;
}

/*!< de-initialize the mem module in bsc library */
static int hikflow_demo_destroy_group(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);
    int ret = opdevsdk_mem_destroyVbPoolGrp(pCtrl->group_id);
    HIKFLOW_KEY_RET(OPDEVSDK_S_OK!= ret,HIKFLOW_DEMO_ERR_FAILED,"opdevsdk_mem_destroyVbPoolGrp err");    
    return HIKFLOW_DEMO_OK;
}

/*!< initialize the mscale module */
static int hikflow_demo_init_mscale(HIKFLOW_DEMO_CTRL* pCtrl)
{
    int i, ret;
    int width = 0, height = 0;
    
    /*!< first step: initialize vin */
    ret =  opdevsdk_vin_init(pCtrl->app_chan);
    HIKFLOW_KEY_RET(OPDEVSDK_S_OK!= ret,HIKFLOW_DEMO_ERR_FAILED,"opdevsdk_vin_init err");
    stack_mng_push((void *)pCtrl,hikflow_demo_deinit_bsc_vin);
    
    HIKFLOW_LOG("opdevsdk_vin_init chan %d\n", pCtrl->app_chan);

    /*!< second step: set vin frame rate */
    ret = opdevsdk_vin_setFrameRate(pCtrl->app_chan, pCtrl->net_info.net_input.fps);
    HIKFLOW_ASSER(OPDEVSDK_S_OK!= ret,HIKFLOW_DEMO_ERR_FAILED);

    OPDEVSDK_VIN_CHN_ST *pchn_info = &pCtrl->ability.vinAbili.chnInfo[pCtrl->idx];

    /*!< creat vb pool,for example, we need 1920x1080 and 416x416 pixels images,1080p for capture,416x416 for algorithm process */
    /*!< firstly,it is neccery to creat two buffer pools with blkCnt(depth) * block_size(w*h*3/2) size,which will be saved yuv scaled in msacle */
    /*!< pay attation:the blkCnt(depth) must be enoungh for algorithm process fluctuation,for example, vin frame rate is 12.5fps,algorithm process cost average 60ms with max 130ms, so you can set the depth is  2+1 at best */
    
    OPDEVSDK_MEM_VB_POOL_GRP_ST vb_pool_grp = {0};
    vb_pool_grp.poolCnt = 2;

    /*!< pool 0 : alarm image pool,buffer use maximum width and height */
    width =  HIKFLOW_DEMO_ALIGN_FOWARD(pchn_info->width, HIKFLOW_DEMO_VB_POOL_W_H_ALIGN);
    height =  HIKFLOW_DEMO_ALIGN_FOWARD(pchn_info->height, HIKFLOW_DEMO_VB_POOL_W_H_ALIGN);
    snprintf(vb_pool_grp.pool[0].name, sizeof(vb_pool_grp.pool[0].name), "cap_img");
    vb_pool_grp.pool[0].poolId = 0; /*!< pool id 0 */
    vb_pool_grp.pool[0].blkCnt = pCtrl->net_info.net_input.vb_cnt;
    vb_pool_grp.pool[0].blkSize = width * height * 3 / 2;/*!< yuv size */

    /*!< pool 1 : alg(net) image pool */
    width =  HIKFLOW_DEMO_ALIGN_FOWARD(pCtrl->net_info.net_input.width, HIKFLOW_DEMO_VB_POOL_W_H_ALIGN);
    height =  HIKFLOW_DEMO_ALIGN_FOWARD(pCtrl->net_info.net_input.height, HIKFLOW_DEMO_VB_POOL_W_H_ALIGN);
    snprintf(vb_pool_grp.pool[1].name, sizeof(vb_pool_grp.pool[1].name), "net_img");
    vb_pool_grp.pool[1].poolId = 1;/*!< pool id 1 */
    vb_pool_grp.pool[1].blkCnt = pCtrl->net_info.net_input.vb_cnt;
    vb_pool_grp.pool[1].blkSize = width * height * 3 / 2;/*!< yuv size */

    int grpId = opdevsdk_mem_createVbPoolGrp(&vb_pool_grp);
    HIKFLOW_LOG("opdevsdk_mem_createVbPoolGrp grpId %d\n",grpId);
    HIKFLOW_KEY_RET((grpId < 0),HIKFLOW_DEMO_ERR_FAILED,"opdevsdk_mem_createVbPoolGrp err");
    pCtrl->group_id = grpId;
    stack_mng_push((void *)pCtrl,hikflow_demo_destroy_group);
    
    for(i = 0; i<vb_pool_grp.poolCnt; i++)
    {
        HIKFLOW_LOG("vbpool create idx %d name %s poolId %d blkCnt %d blkSize %d\n",\
            i, vb_pool_grp.pool[i].name, vb_pool_grp.pool[i].poolId, vb_pool_grp.pool[i].blkCnt, vb_pool_grp.pool[i].blkSize);
    }

    /*!< secondly,call opdevsdk_mscale_create to creat mscale handle,the input param includes output width and height */
    void *ms_hdl = NULL;
    OPDEVSDK_MSCALE_INIT_ST mscale_info = {0};
    mscale_info.srcImg.width = pchn_info->width;/*!< input image width */
    mscale_info.srcImg.height = pchn_info->height;/*!< input image height */
    mscale_info.srcImg.pitch = pchn_info->width;
    mscale_info.dstImgCnt = 2;
    mscale_info.dstImg[0].width = pchn_info->width;
    mscale_info.dstImg[0].height = pchn_info->height;
    mscale_info.dstImg[0].pitch = pchn_info->width;
    mscale_info.dstImg[1].width = pCtrl->net_info.net_input.width;
    mscale_info.dstImg[1].height = pCtrl->net_info.net_input.height;
    mscale_info.dstImg[1].pitch = pCtrl->net_info.net_input.width;
    ret = opdevsdk_mscale_create(&ms_hdl,&mscale_info);
    HIKFLOW_LOG("opdevsdk_mscale_create handle %p\n",ms_hdl);
    HIKFLOW_KEY_RET((ret != OPDEVSDK_S_OK || NULL == ms_hdl),ret,"opdevsdk_mem_createVbPoolGrp err");
    pCtrl->mscale_hdl = ms_hdl;
    stack_mng_push((void *)pCtrl->mscale_hdl,opdevsdk_mscale_destroy);

    /*!< thirdly,bin mscale with group id */
    ret = opdevsdk_mscale_bindVbPoolGrp(pCtrl->mscale_hdl, pCtrl->group_id);
    HIKFLOW_KEY_RET((ret != OPDEVSDK_S_OK),ret,"opdevsdk_mscale_bindVbPoolGrp err");
    stack_mng_push((void *)pCtrl->mscale_hdl,opdevsdk_mscale_unbindVbPoolGrp);

    pCtrl->image.width = pchn_info->width;
    pCtrl->image.height = pchn_info->height;
    pCtrl->image.quality = HIKFLOW_DEMO_DEF_ALARM_QUALITY;

    HIKFLOW_LOG("hikflow_demo_init_mscale ok\n");
    return HIKFLOW_DEMO_OK;
}

/*!< jpeg encode process */
static int hikflow_demo_proc_jpegenc(HIKFLOW_DEMO_CTRL* pCtrl,OPDEVSDK_POS_TARGET_ST *target,OPDEVSDK_VIDEO_FRAME_INFO_ST *frame,HIKFLOW_DEMO_ALARM_IMAGE_INFO *image)
{
    int ret = 0,i = 0;
    
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == target,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == frame,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == image,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(0 == image->width,HIKFLOW_DEMO_OK);
    HIKFLOW_RET(0 == image->height,HIKFLOW_DEMO_OK);
    HIKFLOW_RET(0 == image->quality,HIKFLOW_DEMO_OK);
    HIKFLOW_RET(NULL == pCtrl->jpegenc_handle,HIKFLOW_DEMO_ERR_NULL_PTR);

    int chan = pCtrl->app_chan;

    /*!< draw alarm rect */
    OPDEVSDK_IMG_RECT_PARAM_ST param = {0};
    
    param.point.x = target->region.point[0].x * frame->yuvFrame.width;
    param.point.y = target->region.point[0].y * frame->yuvFrame.height;
    param.width = (target->region.point[1].x - target->region.point[0].x) * frame->yuvFrame.width;
    param.height = (target->region.point[3].y - target->region.point[0].y) * frame->yuvFrame.height;

    /*!< align forward */
    param.point.x = HIKFLOW_DEMO_ALIGN_BACK(param.point.x,2);
    param.point.y = HIKFLOW_DEMO_ALIGN_BACK(param.point.y,2);
    param.width = HIKFLOW_DEMO_ALIGN_BACK(param.width,4);
    param.height = HIKFLOW_DEMO_ALIGN_BACK(param.height,4);  

    /*!< flush cache,because opdevsdk_img_drawRect will draw rect with cpu */
    int size = frame->yuvFrame.width * frame->yuvFrame.height * 3 / 2;
    opdevsdk_mem_flushCache((void *)(size_t)frame->yuvFrame.pPhyAddr[0],(void *)(size_t)frame->yuvFrame.pVirAddr[0],size);
    ret = opdevsdk_img_drawRect(&frame->yuvFrame,param,0x0000FF00,4);
    HIKFLOW_ASSER((ret != OPDEVSDK_S_OK),ret);
    opdevsdk_mem_flushCache((void *)(size_t)frame->yuvFrame.pPhyAddr[0],(void *)(size_t)frame->yuvFrame.pVirAddr[0],size);

    /*!< jpeg encode */
    OPDEVSDK_JPEGENC_PARAM_ST  pParam = {0};
    if(image->width == 0 || image->height == 0)
    {
        pParam.encWidth = frame->yuvFrame.width;
        pParam.encHeight = frame->yuvFrame.height;
    }
    else
    {
        pParam.encWidth = image->width;
        pParam.encHeight = image->height;
    }
    pParam.quality = image->quality;
    pParam.bufSize = pCtrl->jpegenc_max_size;
    pParam.pBuf = (size_t)pCtrl->jpegenc_buf;
   
    /*!< hardware jpeg encode */
    pCtrl->jpegenc_len = 0;
    ret = opdevsdk_jpegenc_proc(pCtrl->jpegenc_handle,&frame->yuvFrame,&pParam);
    HIKFLOW_KEY_RET((ret != OPDEVSDK_S_OK),ret,"opdevsdk_jpegenc_proc err");
    pCtrl->jpegenc_len = pParam.jpegSize;

    /*!< flush cache,because the jpeg will be upload with cpu */
    opdevsdk_mem_flushCache((void *)(size_t)pCtrl->jpegenc_buf_phy,(void *)(size_t)pCtrl->jpegenc_buf,pCtrl->jpegenc_len);
    
    HIKFLOW_DBG("hikflow_demo_proc_jpegenc ok\n");
    return HIKFLOW_DEMO_OK;
}

/*!< write data to file */
static int hikflow_demo_save_data(char *name,void *vir,void *phy,int size)
{
    HIKFLOW_RET(NULL == name,HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_RET(NULL == vir,HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_RET(NULL == phy,HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_RET(0 == size,HIKFLOW_DEMO_ERR_FAILED);

    opdevsdk_mem_flushCache((void *)phy,(void *)vir,size);
    int ret = stack_mng_write_file(name,vir,size);
    HIKFLOW_ASSER(ret!=0,ret);
    return HIKFLOW_DEMO_OK;
}

/*!< alarm process */
static int hikflow_demo_proc_alarm(HIKFLOW_DEMO_CTRL* pCtrl,OPDEVSDK_POS_TARGET_ST *alarm_target,OPDEVSDK_VIDEO_FRAME_INFO_ST *frame,OPDEVSDK_POS_RULE_ST *rule_info)
{
    int ret = 0, i = 0, j = 0, k =0;
    int alarm_flag = -1;

    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == alarm_target,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == frame,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == rule_info,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_DEMO_ALARM_IMAGE_INFO             image = {0};

    /*!< return when not enable */
    if(0 == pCtrl->hikflow_config.b_alarm)
    {
        return HIKFLOW_DEMO_OK;
    }

    int chan = pCtrl->app_chan;
    
    HIKFLOW_DBG("hikflow_demo_proc_alarm begin\n");

    /*!< id !=0 shows we can jurge interval time */
    if(alarm_target->id >= 1)
    {
        unsigned long long curent_time =  stack_mng_get_time();
        if(pCtrl->hikflow_config.alarm_intrval == 0)
        {
            HIKFLOW_ERR("pay attention: pCtrl->alarm_intrval is 0\n");
            pCtrl->hikflow_config.alarm_intrval = HIKFLOW_DEMO_DEF_ALARM_INTER;
        }

        /*!< interval >= X ns setted */
        if((curent_time-pCtrl->proc_times) > pCtrl->hikflow_config.alarm_intrval * 1000 * 1000)
        {
            alarm_flag = 1;
            HIKFLOW_LOG("model alarm triggered: id %d class %d curent_time %llu last %llu cut %llu interval %d s flg %d\n",
                alarm_target->id, alarm_target->res[0], curent_time, pCtrl->proc_times,
                curent_time-pCtrl->proc_times, pCtrl->hikflow_config.alarm_intrval, alarm_flag);
            pCtrl->proc_times = curent_time;
        }
    }
    else
    {
        pCtrl->proc_times = stack_mng_get_time();
    }

    HIKFLOW_DBG("alarm_flag %d,pCtrl->b_alarm %d, id %d\n",alarm_flag,pCtrl->hikflow_config.b_alarm,alarm_target->id);

    /*!< alarm flg=1 shows that it is time to make alarm */
    if(1 == alarm_flag)
    {
        camera_abnormal_on_human_alarm((int64_t)frame->timeStamp / 1000, 1.0f);
        /* POS targets/text are emitted separately by hikflow_demo_proc_pos(). */
        if (!CA_ENABLE_LEGACY_ALARM) return HIKFLOW_DEMO_OK;

        OPDEVSDK_POS_ALERT_INFO_ST alarm = {0};
        alarm.timeType = OPDEVSDK_POS_TIME_TYPE_1K;
        alarm.timeStamp = frame->timeStamp / 1000;
        alarm.alert = 1;
        alarm.ruleInfo.id = rule_info->id;
        alarm.ruleInfo.enable = 1;
        alarm.ruleInfo.ruleType = rule_info->ruleType;
        alarm.ruleInfo.polygon.pointNum = rule_info->polygon.pointNum;
        for(i = 0; i < rule_info->polygon.pointNum; i++)
        {
            alarm.ruleInfo.polygon.point[i].x = rule_info->polygon.point[i].x;
            alarm.ruleInfo.polygon.point[i].y = rule_info->polygon.point[i].y;
            HIKFLOW_DBG("r i %d x %f y %f\n",i,rule_info->polygon.point[i].x,rule_info->polygon.point[i].y);
        }
        
        /*!< the first target is alarm target */
        alarm.target.id = alarm_target->id;
        alarm.target.color = 0;
        alarm.target.region.pointNum = alarm_target->region.pointNum;
        for(i = 0; i < alarm_target->region.pointNum; i++)
        {
            alarm.target.region.point[i].x = alarm_target->region.point[i].x;
            alarm.target.region.point[i].y = alarm_target->region.point[i].y;
            HIKFLOW_DBG("t i %d x %f y %f\n",i,alarm_target->region.point[i].x,alarm_target->region.point[i].y);
        }
    
        /*!< alarm show */
        ret = opdevsdk_pos_procAlarm(chan, &alarm);
        HIKFLOW_ASSER((ret != OPDEVSDK_S_OK),ret);
        
        pthread_mutex_lock(&pCtrl->mutex);
        memcpy(&image,&pCtrl->image,sizeof(HIKFLOW_DEMO_ALARM_IMAGE_INFO));
        pthread_mutex_unlock(&pCtrl->mutex);

        /*!< draw rect and jpeg encode */
        isfw_stat_time_enter(&pCtrl->jpegenc_proc);        
        ret = hikflow_demo_proc_jpegenc(pCtrl,&alarm.target,frame,&image);
        isfw_stat_time_exit(&pCtrl->jpegenc_proc);        
        HIKFLOW_KEY_EXIT((ret != HIKFLOW_DEMO_OK),(pCtrl->jpegenc_lost_times++,ret),proc_end,"hikflow_demo_proc_jpegenc");
        pCtrl->jpegenc_suc_times++;

#ifdef  JPEG_ENC_TEST
        char name[128];
        static int jpeg_proc_idx = 0;
        snprintf(name,128, "./data/jpeg_%d_%d",jpeg_proc_idx,pCtrl->jpegenc_len);
        if(jpeg_proc_idx < 4)
        {
            ret = hikflow_demo_save_data(name,pCtrl->jpegenc_buf,pCtrl->jpegenc_buf_phy,pCtrl->jpegenc_len);
            HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,ret);
        }
        jpeg_proc_idx++;
#endif
        /*!< add json and upload*/
        isfw_stat_time_enter(&pCtrl->alarm_proc);        
        ret = json_proc_add_content_json(&alarm.target,pCtrl->json_buf,pCtrl->json_max_size,&frame->dateTime,pCtrl->jpegenc_buf,pCtrl->jpegenc_len);
        isfw_stat_time_exit(&pCtrl->alarm_proc);        
        HIKFLOW_ASSER((ret != HIKFLOW_DEMO_OK),(pCtrl->alarm_lost_times++,ret));
        pCtrl->alarm_suc_times++;

        //HIKFLOW_DBG("hikflow_demo_proc_alarm ok\n");
    } 
proc_end:
    return HIKFLOW_DEMO_OK;
}

/*!< get attribute name throug idx */
static void* hikflow_demo_get_attr_name(HIKFLOW_DEMO_CTRL* pCtrl,int index)
{
    if(NULL == pCtrl)
    {
        return NULL;
    }

    for(int i = 0; i < pCtrl->net_info.attr_list.attr_num;i++)
    {
        if(index != pCtrl->net_info.attr_list.attr[i].idx)
        {
            continue;
        }
        //HIKFLOW_DBG("hikflow_demo_get_attr(%d)_name i %d idx %d name %s ok\n",index,i,pCtrl->net_info.attr_list.attr[i].idx,pCtrl->net_info.attr_list.attr[i].name);

        return pCtrl->net_info.attr_list.attr[i].name;
    }

    //HIKFLOW_ERR("hikflow_demo_get_attr(%d)_name i %d idx %d name %s err\n",index);
    return NULL;
}

/*!< pack pos into stream,so you can see the target rects in web */
static int hikflow_demo_proc_pos(HIKFLOW_DEMO_CTRL* pCtrl,OPDEVSDK_POS_TARGET_LIST_INFO_ST *pack_target,unsigned long long time_stamp,OPDEVSDK_POS_TARGET_ST *alarm_target,OPDEVSDK_POS_RULE_ST *rule_info)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == alarm_target,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == pack_target,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == rule_info,HIKFLOW_DEMO_ERR_NULL_PTR);
    
    int ret = 0,i = 0;
	OPDEVSDK_POS_STRING_ST str[HIKFLOW_DEMO_MAX_OUTPUT_BOX_NUM] = {0};
	OPDEVSDK_POS_TEXT_PARAM_ST text_param = {0};
	char tmp_str[HIKFLOW_DEMO_MAX_OUTPUT_BOX_NUM][256];
    char pos_buf[4096] = {0};
    unsigned int total_len = 0;
    HIKFLOW_DEMO_RULE                rule = {0};
	int in_rule = 0,j =0;
    int alarm_flag = 0;
    int chan = pCtrl->app_chan;

    /*!< pack target */
    pack_target->timeType = OPDEVSDK_POS_TIME_TYPE_1K;
    pack_target->packId = 0;
    pack_target->timeStamp = time_stamp / 1000;
    pack_target->attribute = OPDEVSDK_POS_POLYGON_ATTRI_LINEBR_TWO_THIRD;
    ret = opdevsdk_pos_procTarget(chan, pack_target);		
    HIKFLOW_ASSER((ret != OPDEVSDK_S_OK),ret);

    memset(str,0,HIKFLOW_DEMO_MAX_OUTPUT_BOX_NUM* sizeof(OPDEVSDK_POS_STRING_ST));

    /*!< pack text */
    HIKFLOW_DBG("pack_target->tgtList.tgtNum %d\n",pack_target->tgtList.tgtNum);
    if(pack_target->tgtList.tgtNum > 0)
    {
        /*
         * Keep the first filtered model target as the alarm target before
         * res[0] is cleared for POS display. alarm_flag below is then driven
         * by a real HIKFlow target id, not by a standalone timer.
         */
        memcpy(alarm_target, &pack_target->tgtList.pTgt[0], sizeof(OPDEVSDK_POS_TARGET_ST));
        HIKFLOW_LOG("alarm candidate from model: id %d class %d tgtNum %d\n",
            alarm_target->id, alarm_target->res[0], pack_target->tgtList.tgtNum);
    }
    else
    {
        memset(alarm_target, 0, sizeof(OPDEVSDK_POS_TARGET_ST));
    }
    for(i = 0; i < pack_target->tgtList.tgtNum; i++)
    {
        str[i].point.x = pack_target->tgtList.pTgt[i].region.point[0].x;
        str[i].point.y = pack_target->tgtList.pTgt[i].region.point[0].y;
        int class_type = pack_target->tgtList.pTgt[i].res[0];
        //HIKFLOW_DBG("i %d x %f y %f\n",i,str[i].point.x,str[i].point.y);

        /*!< get attribute name from hikflow_attr.json */
        char *name = NULL;
        name = hikflow_demo_get_attr_name(pCtrl,class_type);
        if(name != NULL)
        {
            snprintf(tmp_str[i],256,"%s id:%d",name,pack_target->tgtList.pTgt[i].id);    
        }
        else
        {
            snprintf(tmp_str[i],256,"id:%d",pack_target->tgtList.pTgt[i].id);    
        }
        pack_target->tgtList.pTgt[i].res[0] = 0;     
        str[i].str = tmp_str[i];
        str[i].strLen = strlen(tmp_str[i]);
    }
    
    /*!< size control,16 or 32 pixels*/
    int size = 16;
    if(pCtrl->ability.vinAbili.chnInfo[pCtrl->idx].width >= 2560)
    {
        size = 32;
    }

    /*!< text shows*/
    text_param.textBuf = pos_buf;
    text_param.textLen = &total_len;
    text_param.textBufLen= sizeof(pos_buf);
    text_param.encWidth = pCtrl->net_info.net_input.width;
    text_param.encHeight = pCtrl->net_info.net_input.height;
    text_param.charSize = size;
    text_param.strList.strNum = pack_target->tgtList.tgtNum;
    text_param.strList.str = str;
    ret = opdevsdk_pos_initText(chan, &text_param);
    HIKFLOW_KEY_RET(ret != OPDEVSDK_S_OK,ret,"opdevsdk_pos_initText err");

    OPDEVSDK_POS_TEXT_INFO_ST text_info = {0};
    text_info.timeType = OPDEVSDK_POS_TIME_TYPE_1K;
    text_info.timeStamp = time_stamp / 1000;
    text_info.textBuf = pos_buf;
    text_info.textLen = total_len;
    ret = opdevsdk_pos_procText(chan,&text_info);
    HIKFLOW_KEY_RET(ret != OPDEVSDK_S_OK,ret,"opdevsdk_pos_procText err");
    //HIKFLOW_DBG("hikflow_demo_proc_pos ok\n");

    /*!< pack rule */
    pthread_mutex_lock(&pCtrl->mutex);
    memcpy(&rule,&pCtrl->rule,sizeof(HIKFLOW_DEMO_RULE));
    pthread_mutex_unlock(&pCtrl->mutex);
    rule_info->id = 1;
    rule_info->enable = 1;
    rule_info->ruleType = OPDEVSDK_POS_RULE_TYPE_REGION;
    rule_info->polygon.pointNum = rule.point_num;
    for(i = 0; i < rule_info->polygon.pointNum; i++)
    {
        rule_info->polygon.point[i].x = rule.point[i].x;
        rule_info->polygon.point[i].y = rule.point[i].y;
    }
    
    OPDEVSDK_POS_RULE_LIST_INFO_ST rule_list;
    rule_list.timeType = OPDEVSDK_POS_TIME_TYPE_1K;
    rule_list.timeStamp = time_stamp / 1000;
    rule_list.attribute = OPDEVSDK_POS_POLYGON_ATTRI_NORMAL;
    rule_list.ruleList.ruleNum = 1;
    rule_list.ruleList.pRule = rule_info;
    ret = opdevsdk_pos_procRule( chan, &rule_list);
    HIKFLOW_ASSER((ret != OPDEVSDK_S_OK),ret);

    return HIKFLOW_DEMO_OK;
}

/*!< algorithm thread in FILE mode */
static int hikflow_demo_alg_thread_from_file(void *arg)
{
    int ret = 0;
    char name[16];
    HIKFLOW_DEMO_CTRL* pCtrl = (HIKFLOW_DEMO_CTRL*)arg;
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);

    /*!< set pthread name */
    snprintf(name,16,"%s","hf_net_file");
	prctl(PR_SET_NAME , (unsigned long)name);

    /*!< alg proc */
    ret = hikflow_proc_alg_from_file(pCtrl);
    HIKFLOW_ASSER(HIKFLOW_DEMO_OK != ret,ret);

    ret = hikflow_demo_deinit(pCtrl);
    HIKFLOW_ASSER(HIKFLOW_DEMO_OK != ret,ret);
    _exit(0);
    return HIKFLOW_DEMO_OK;
}

/*!< get yuv from vin module and send to mscale module */
static int hikflow_demo_vin_get_thread(void *arg)
{
    HIKFLOW_DEMO_CTRL* pCtrl = (HIKFLOW_DEMO_CTRL*)arg;
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);

    /*!< set pthread name */
    char name[16];
    snprintf(name,16,"%s","hf_vin_get");
	prctl(PR_SET_NAME , (unsigned long)name);

    int ret = 0;
    int frm_num = 0;
    
    int chan = pCtrl->app_chan;
    OPDEVSDK_VIDEO_FRAME_INFO_ST vin_frame = {0};
    memset(&vin_frame, 0, sizeof(OPDEVSDK_VIDEO_FRAME_INFO_ST));

	pCtrl->vin_proc.hist_grid = 10*1000;
    pCtrl->vin_thread_exit = 1;

    while(pCtrl->magic == HIKFLOW_DEMO_CTRL_MAGIC)
    {
        if(0 == pCtrl->b_start)
        {
            usleep(40 *1000);
            continue;
        }

        isfw_stat_time_enter(&pCtrl->vin_proc);

        /*!< firstly,get yuv ,wait forever */
        ret = opdevsdk_vin_getFrame(chan, &vin_frame, -1);
        if(ret != OPDEVSDK_S_OK)
        {
            HIKFLOW_LOG("opdevsdk_vin_getFrame error ret = 0x%x\n",ret);
            pCtrl->yuv_get_lost_times++;
            usleep(20 * 1000);
            isfw_stat_time_exit(&pCtrl->vin_proc);
            continue;
        }
                
        /*!< secondly,send yuv  to mscale module,make sure vb is enough */
        ret = opdevsdk_mscale_sendFrame(pCtrl->mscale_hdl, &vin_frame);
        if(ret != OPDEVSDK_S_OK)
        {
            HIKFLOW_LOG("opdevsdk_mscale_sendFrame  ret = 0x%x\n",ret);
            pCtrl->yuv_send_lost_times++;
        }

        /*!< thirdly,release vin frame */
        ret = opdevsdk_vin_releaseFrame(chan, &vin_frame);
        if(ret != OPDEVSDK_S_OK)
        {
            HIKFLOW_LOG("opdevsdk_vin_releaseFrame error ret = 0x%x\n",ret);
            pCtrl->yuv_rel_lost_times++;
        }

        pCtrl->yuv_suc_times++;
        memset(&vin_frame, 0, sizeof(OPDEVSDK_VIDEO_FRAME_INFO_ST));
        isfw_stat_time_exit(&pCtrl->vin_proc);
    }
    
    HIKFLOW_LOG("hikflow_demo_vin_get_thread exit\n");
    pCtrl->vin_thread_exit = 0;
    return ret;
}

/*!< quit all threads */
static int hikflow_demo_quit_thread(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_LOG("hikflow_demo_quit_thread enter\n");
    
    /*!< set all thread exit */
    pCtrl->magic = ~HIKFLOW_DEMO_CTRL_MAGIC;

	int waitcnt = 0;
	int quit = 0;
	while(1)
	{
		quit = pCtrl->vin_thread_exit | pCtrl->net_thread_exit;
		waitcnt++;		

        /*!< wait untill all threads exit */
		if(!quit)
		{
			HIKFLOW_LOG("hikflow_demo_quit_thread ok\n");
			break;
		}
		
		usleep(10*1000);
	}
    HIKFLOW_LOG("hikflow_demo_quit_thread exit\n");
    return HIKFLOW_DEMO_OK;
}

/*!< algorithm processing and upload alarm */
static int hikflow_demo_alg_thread_from_cam(void *arg)
{
    int ret = 0;
    char name[16];
    int proc_err = 0;
    int mscale_rel_err = 0;
    OPDEVSDK_POS_TARGET_LIST_INFO_ST pack_target = {0};

    HIKFLOW_DEMO_CTRL* pCtrl = (HIKFLOW_DEMO_CTRL*)arg;
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);

    /*!< set pthread name */
    snprintf(name,16,"%s","hf_net_cam");
	prctl(PR_SET_NAME , (unsigned long)name);
    static int avg = 0,total = 0, frm_num = 0;
    
    OPDEVSDK_POS_TARGET_ST target[HIKFLOW_DEMO_MAX_OUTPUT_BOX_NUM] = {0};
    int chan = pCtrl->app_chan;
    OPDEVSDK_VIDEO_FRAME_INFO_ST vin_frame = {0};
    memset(&vin_frame, 0, sizeof(OPDEVSDK_VIDEO_FRAME_INFO_ST));

    OPDEVSDK_MSCALE_RES_ST cap_img = {0};
    OPDEVSDK_MSCALE_RES_ST net_img = {0};
    cap_img.width = pCtrl->ability.vinAbili.chnInfo[pCtrl->idx].width;
    cap_img.pitch = pCtrl->ability.vinAbili.chnInfo[pCtrl->idx].width;
    cap_img.height = pCtrl->ability.vinAbili.chnInfo[pCtrl->idx].height;
    net_img.width = pCtrl->net_info.net_input.width;
    net_img.pitch = pCtrl->net_info.net_input.width;
    net_img.height = pCtrl->net_info.net_input.height;

    OPDEVSDK_VIDEO_FRAME_INFO_ST cap_frame = {0};
    OPDEVSDK_VIDEO_FRAME_INFO_ST net_frame = {0};
   
    pCtrl->frame_proc.hist_grid = 10*1000;
    pCtrl->net_proc.hist_grid = 10*1000;
    pCtrl->net_thread_exit = 1;

    int idx = 0;
    long long last_infer_ms = 0;
    while(pCtrl->magic == HIKFLOW_DEMO_CTRL_MAGIC)
    {
        if(0 == pCtrl->b_start)
        {
            usleep(40 *1000);
            continue;
        }
        
        isfw_stat_time_enter(&pCtrl->frame_proc);
        
        /*!< first,get cap yuv and net yuv ,wait forever */
        ret = opdevsdk_mscale_getFrame(pCtrl->mscale_hdl, cap_img, &cap_frame, -1);
        if(ret != OPDEVSDK_S_OK)
        {
            HIKFLOW_LOG("opdevsdk_mscale_getFrame cap_img error  ret = 0x%x\n",ret);
            pCtrl->mscale_get_lost_times++; 
        }
        
        ret = opdevsdk_mscale_getFrame(pCtrl->mscale_hdl, net_img, &net_frame, -1);
        if(ret != OPDEVSDK_S_OK)
        {
            HIKFLOW_LOG("opdevsdk_mscale_getFrame net_img error  ret = 0x%x\n",ret);
            pCtrl->mscale_get_lost_times++;
        }
        
        pCtrl->mscale_get_suc_times++;
        idx++;
        if (hikflow_demo_should_skip_infer(&net_frame, &last_infer_ms))
        {
            ret = opdevsdk_mscale_releaseFrame(pCtrl->mscale_hdl, &cap_frame);
            if(ret != OPDEVSDK_S_OK)
            {
                HIKFLOW_ERR("opdevsdk_mscale_releaseFrame error  ret = 0x%x\n",ret);
                pCtrl->mscale_rel_lost_times++;
            }
            ret = opdevsdk_mscale_releaseFrame(pCtrl->mscale_hdl,&net_frame);
            if(ret != OPDEVSDK_S_OK)
            {
                HIKFLOW_ERR("opdevsdk_mscale_releaseFrame error  ret = 0x%x\n",ret);
                pCtrl->mscale_rel_lost_times++;
            }
            pCtrl->mscale_rel_suc_times++;
            memset(&net_frame, 0, sizeof(OPDEVSDK_VIDEO_FRAME_INFO_ST));
            memset(&cap_frame, 0, sizeof(OPDEVSDK_VIDEO_FRAME_INFO_ST));
            isfw_stat_time_exit(&pCtrl->frame_proc);
            continue;
        }
        
#ifdef TEST_WRITE_YUV
        char name[128];
        snprintf(name,128, "./data/yuv_%d_%dx%d",idx,net_frame.yuvFrame.width,net_frame.yuvFrame.height);
        ret = hikflow_demo_save_data(name,(void *)(size_t)net_frame.yuvFrame.pVirAddr[0],(void *)(size_t)net_frame.yuvFrame.pPhyAddr[0],net_frame.yuvFrame.width*net_frame.yuvFrame.height * 3 / 2);
        HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,ret);
#endif    

        /*!< second,algorithm processing and return target list */
        memset(&pack_target, 0, sizeof(pack_target));
        memset(target, 0x0, sizeof(OPDEVSDK_POS_TARGET_ST)*HIKFLOW_DEMO_MAX_OUTPUT_BOX_NUM);
        pack_target.tgtList.pTgt = &target[0];
        
        proc_err = 0;
        isfw_stat_time_enter(&pCtrl->net_proc);
        ret = hikflow_proc_alg_from_cam(pCtrl,&net_frame, &pack_target);
        isfw_stat_time_exit(&pCtrl->net_proc);        
        if(ret != HIKFLOW_DEMO_OK)
        {
            HIKFLOW_LOG("demo_alg_process error  ret = 0x%x\n",ret);
            pCtrl->net_lost_times++;
            proc_err++;
        }
        else
        {
            pCtrl->net_suc_times++;
            OPDEVSDK_POS_TARGET_ST alarm_target = {0};
            OPDEVSDK_POS_RULE_ST rule_list= {0};
            /*!< third,pack pos into stream,so you can see the target rects in web */
            ret = hikflow_demo_proc_pos(pCtrl,&pack_target,net_frame.timeStamp,&alarm_target,&rule_list);
            HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,ret);
            
            /*!< fourth,process alarm information */
            ret = hikflow_demo_proc_alarm(pCtrl,&alarm_target,&cap_frame,&rule_list);
            HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,ret);
        }

        /*!< fifth,release cap_frame  */
        ret = opdevsdk_mscale_releaseFrame(pCtrl->mscale_hdl, &cap_frame);
        if(ret != OPDEVSDK_S_OK)
        {
            HIKFLOW_ERR("opdevsdk_mscale_releaseFrame error  ret = 0x%x\n",ret);
            pCtrl->mscale_rel_lost_times++;
        }
        
        /*!< sixth,release net_frame  */
        ret = opdevsdk_mscale_releaseFrame(pCtrl->mscale_hdl,&net_frame);
        if(ret != OPDEVSDK_S_OK)
        {
            HIKFLOW_ERR("opdevsdk_mscale_releaseFrame error  ret = 0x%x\n",ret);
            pCtrl->mscale_rel_lost_times++;
        }
        pCtrl->mscale_rel_suc_times++;
        
        memset(&net_frame, 0, sizeof(OPDEVSDK_VIDEO_FRAME_INFO_ST));
        memset(&cap_frame, 0, sizeof(OPDEVSDK_VIDEO_FRAME_INFO_ST));

        isfw_stat_time_exit(&pCtrl->frame_proc);        
        if(proc_err)
        {
            HIKFLOW_ERR("hikflow_proc_alg_from_cam  err\n");
            //return HIKFLOW_DEMO_ERR_FAILED;
        }
    }
     
    HIKFLOW_LOG("hikflow_demo_alg_proc_thread_ex  exit\n");
    pCtrl->net_thread_exit = 0;
    
    return HIKFLOW_DEMO_OK;
}

/*!< set process name and version */
static int hikflow_demo_init_proc(HIKFLOW_DEMO_CTRL* pCtrl, char *argv[])
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_RET(NULL == argv[0],HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_RET(NULL == argv[1],HIKFLOW_DEMO_ERR_FAILED);

    int pid = getpid();
    int ret = 0;

    /*!< set process name and version */
    memset(pCtrl,0,sizeof(HIKFLOW_DEMO_CTRL));
	pCtrl->magic = HIKFLOW_DEMO_CTRL_MAGIC;

    /*!< process name include pid and container name */
    char procName[HIKFLOW_DEMO_MAX_STRING_LEN];
    memset(procName,'\0',sizeof(procName));
    snprintf(procName,HIKFLOW_DEMO_MAX_STRING_LEN,"hf%d",pid);

    /*!< container name */
    char hostName[HIKFLOW_DEMO_MAX_STRING_LEN];
    memset(hostName,'\0',HIKFLOW_DEMO_MAX_STRING_LEN);
    ret = gethostname(hostName,sizeof(hostName));
    if((ret != 0) || (strlen(hostName) == 0) || (0 == strcmp("(none)", hostName)))
    {
        snprintf(hostName,sizeof(hostName),"%s","cont*");
    }

    snprintf(procName+strlen(procName),strlen(hostName)+2,"_%s",hostName);
	prctl(PR_SET_NAME , (unsigned long)procName);

    /*!< process name,version and file path */
    snprintf(pCtrl->proc_name,HIKFLOW_DEMO_MAX_STRING_LEN,"%s",procName);
    snprintf(pCtrl->version,64,"%s",HIKFLOW_DEMO_VERSION);
    snprintf(pCtrl->path,HIKFLOW_DEMO_MAX_STRING_LEN,"%s",argv[1]);

    pCtrl->hikflow_config.sel_class = -1;
    pCtrl->proc_type = HIKFLOW_DEMO_PROC_FROM_CAM;

    /*!< record the mode */
    if (0 == strcmp("FILE", argv[2]))
    {
        pCtrl->proc_type = HIKFLOW_DEMO_PROC_FROM_FILE;
    }

    HIKFLOW_LOG("hikflow_demo name %s path %s ver %s proc mode %s\n",pCtrl->proc_name,pCtrl->path,pCtrl->version,argv[2]);
    return HIKFLOW_DEMO_OK;
}

/*!< print config information */
static void hikflow_demo_print_config(HIKFLOW_DEMO_CONFIGURATION_ST *config_data)
{
    HIKFLOW_NORET(NULL == config_data,HIKFLOW_DEMO_ERR_FAILED);
    isfw_log_dprint("config info:     \n");
    isfw_log_dprint("  core_proc_type    %d\n", config_data->net_input.core_proc_type);
    isfw_log_dprint("  data_type         %d\n", config_data->net_input.data_type);
    isfw_log_dprint("  data_mem_type     %d\n", config_data->data_mem_type);
    isfw_log_dprint("  model path:       %s\n", config_data->model_path);
    isfw_log_dprint("  image_list        %s\n", config_data->image_list);
    isfw_log_dprint("  batch_cnt         %d\n", config_data->net_input.batch_cnt);
    isfw_log_dprint("  channel_cnt       %d\n", config_data->net_input.channel_cnt);
    isfw_log_dprint("  height            %d\n", config_data->net_input.height);
    isfw_log_dprint("  width             %d\n", config_data->net_input.width);
    isfw_log_dprint("  fps[idx]          %d\n", config_data->net_input.fps);
    isfw_log_dprint("  vb_cnt            %d\n", config_data->net_input.vb_cnt);
    isfw_log_dprint("  b_alarm           %d\n", config_data->b_alarm);
    isfw_log_dprint("  alarm_intrval     %d\n", config_data->alarm_intrval);
    isfw_log_dprint("  sel_class         %d\n", config_data->sel_class);
    isfw_log_dprint("\n");
    return;
}

/*!< print algorithm information */
static void hikflow_demo_print_net_info(HIKFLOW_DEMO_NET_INFO_ST *net_info)
{
    HIKFLOW_NORET(NULL == net_info,HIKFLOW_DEMO_ERR_FAILED);
    isfw_log_dprint("net info:     \n");
    isfw_log_dprint("  core_proc_type    %d\n", net_info->net_input.core_proc_type);
    isfw_log_dprint("  data_type         %d\n", net_info->net_input.data_type);
    isfw_log_dprint("  batch_cnt         %d\n", net_info->net_input.batch_cnt);
    isfw_log_dprint("  channel_cnt       %d\n", net_info->net_input.channel_cnt);
    isfw_log_dprint("  height            %d\n", net_info->net_input.height);
    isfw_log_dprint("  width             %d\n", net_info->net_input.width);
    isfw_log_dprint("  fps               %d\n", net_info->net_input.fps);
    isfw_log_dprint("  vb_cnt            %d\n", net_info->net_input.vb_cnt);


    isfw_log_dprint("  net_handle        %p\n", net_info->net_handle);
    isfw_log_dprint("  model_handle      %p\n", net_info->model_handle);
    isfw_log_dprint("  model_size        %d\n", net_info->model_size);
    isfw_log_dprint("  model_buffer      %p\n", net_info->model_buffer);
    isfw_log_dprint("  model_phy_base    0x%x\n", net_info->model_phy_base);
    isfw_log_dprint("  net_mem_used      %dkB\n", net_info->net_mem_used/1024);
    isfw_log_dprint("  model_mem_used    %dkB\n", net_info->model_mem_used/1024);
    isfw_log_dprint("  model_file_used   %dkB\n", net_info->model_file_used/1024);

    isfw_log_dprint("\n");

    return;
}

static int hikflow_demo_is_num_str(char *str)
{
	int ret = 0;
	while('0' <= *str && *str <= '9')
	{
		str++;
		if(*str == 0)
		{
			return 1;
		}
	}
	return 0;
}

static int hikflow_demo_str_2_int(char *str)
{
	int ret = 0;
	while('0' <= *str && *str <= '9')
	{
		ret = ret * 10 + str[0] - '0';
		str++;
	}
	return ret;
}

static void hikflow_demo_print_mem(void *arg,int argc, char *argv[])
{
    stack_mng_mem_stat_prt();
    return;
}

/*!< debug callback,online debug */
static void hikflow_demo_set_debug(void *arg,int argc, char *argv[])
{
    HIKFLOW_DEMO_CTRL* pCtrl = (HIKFLOW_DEMO_CTRL*)arg;
    HIKFLOW_NORET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_NORET(NULL == argv[0],HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_NORET(HIKFLOW_DEMO_CTRL_MAGIC != pCtrl->magic,HIKFLOW_DEMO_ERR_FAILED);
    int ret = 0;
    
    /*!< set rule */
    if(strcmp(argv[0], "rule") == 0)
    {
        HIKFLOW_DEMO_POLYGON polygen = {0};
        polygen.point_num = 4;
        polygen.point[0].x = 150;
        polygen.point[0].y = 150;
        polygen.point[1].x = 850;
        polygen.point[1].y = 150;
        polygen.point[2].x = 850;
        polygen.point[2].y = 850;
        polygen.point[3].x = 150;
        polygen.point[3].y = 850;
        
        ret = hikflow_demo_param_set(&polygen,HIKFLOW_DEMO_PARAM_SET_TYPE_RULE);
        HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,ret);
    }
    else if(strcmp(argv[0], "enable") == 0 )/*!< set enable */
    {
        HIKFLOW_NORET(argc != 2,HIKFLOW_DEMO_ERR_FAILED);
        HIKFLOW_NORET(NULL == argv[1],HIKFLOW_DEMO_ERR_FAILED);

        int enable = 0;
        if(hikflow_demo_is_num_str(argv[1]))
        {
            enable = hikflow_demo_str_2_int(argv[1]);
        }   
                
        ret = hikflow_demo_param_set(&enable,HIKFLOW_DEMO_PARAM_SET_TYPE_EN);
        HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,ret);
    }
    else if(strcmp(argv[0], "alarm") == 0 )/*!< set alarm */
    {
        HIKFLOW_DEMO_ALARM_IMAGE_INFO image = {0};
        image.width = 1280;
        image.height = 720;
        image.quality = 80;
        ret = hikflow_demo_param_set(&image,HIKFLOW_DEMO_PARAM_SET_TYPE_ALARM_IMAG);
        HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,ret);
    }
    else if(strcmp(argv[0], "set_all") == 0 )/*!< set all */
    {
        HIKFLOW_NORET(argc != 2,HIKFLOW_DEMO_ERR_FAILED);
        HIKFLOW_NORET(NULL == argv[1],HIKFLOW_DEMO_ERR_FAILED);

        HIKFLOW_DEMO_POLYGON polygen = {0};
        polygen.point_num = 4;
        polygen.point[0].x = 150;
        polygen.point[0].y = 150;
        polygen.point[1].x = 850;
        polygen.point[1].y = 150;
        polygen.point[2].x = 850;
        polygen.point[2].y = 850;
        polygen.point[3].x = 150;
        polygen.point[3].y = 850;
        ret = hikflow_demo_param_set(&polygen,HIKFLOW_DEMO_PARAM_SET_TYPE_RULE);

        HIKFLOW_DEMO_ALARM_IMAGE_INFO image = {0};
        image.width = 1280;
        image.height = 720;
        image.quality = 80;
        ret = hikflow_demo_param_set(&image,HIKFLOW_DEMO_PARAM_SET_TYPE_ALARM_IMAG);
        HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,ret);

        int enable = 0;
        if(hikflow_demo_is_num_str(argv[1]))
        {
            enable = hikflow_demo_str_2_int(argv[1]);
        }   
        
        ret = hikflow_demo_param_set(&enable,HIKFLOW_DEMO_PARAM_SET_TYPE_EN);
        HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,ret);
    }
    
	isfw_log_dprint("hikflow_demo_set_debug ok\n");
    return;
}

/*!< print hikflow demo status */
static void hikflow_demo_print_stat(void *arg,int argc, char *argv[])
{
    HIKFLOW_DEMO_CTRL* pCtrl = (HIKFLOW_DEMO_CTRL*)arg;
    HIKFLOW_NORET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);
    HIKFLOW_NORET(HIKFLOW_DEMO_CTRL_MAGIC != pCtrl->magic,HIKFLOW_DEMO_ERR_FAILED);
    
	isfw_log_dprint("=================  hikflow_demo prt info start ==================\n");

    isfw_log_dprint("init:           %d\n",pCtrl->b_init);
    isfw_log_dprint("start:          %d\n",pCtrl->b_start);
    isfw_log_dprint("app_chan:       %d\n",pCtrl->app_chan);
    isfw_log_dprint("version:        %s\n",pCtrl->version);
    isfw_log_dprint("proc_name:      %s\n",pCtrl->proc_name);
    isfw_log_dprint("proc_path:      %s\n",pCtrl->path);
    isfw_log_dprint("proc_type:      %s\n",pCtrl->proc_type == 0 ? "CAMERA":"FILE");

    isfw_log_dprint("proc_version:   %s\n",pCtrl->version);
    isfw_log_dprint("bsc_version:    %s\n",pCtrl->bsc_ver.version);
    isfw_log_dprint("hf_version:     %s\n",pCtrl->hikflow_ver.version);
    pCtrl->mem_used_all += pCtrl->net_info.model_mem_used + pCtrl->net_info.model_file_used + pCtrl->net_info.net_mem_used;
    isfw_log_dprint("cmm mem:        %dkB\n",pCtrl->mem_used_all/1024);

    isfw_log_dprint("\n");

    int i = 0,j= 0;
    
    /*!<vin  ability */    
    isfw_log_dprint("vin info:     \n");
    for(i = 0;i < pCtrl->ability.vinAbili.chnNum;i++)
    {
        isfw_log_dprint("  idx %d :\n",i);    
        isfw_log_dprint("      chan:      %d\n",pCtrl->ability.vinAbili.chnInfo[i].chan);    
        isfw_log_dprint("      med_chan:  %d\n",pCtrl->ability.vinAbili.chnInfo[i].mediaChan);    
        isfw_log_dprint("      width:     %d\n",pCtrl->ability.vinAbili.chnInfo[i].width);    
        isfw_log_dprint("      height:    %d\n",pCtrl->ability.vinAbili.chnInfo[i].height);    
        isfw_log_dprint("      fps:       %f\n",pCtrl->ability.vinAbili.chnInfo[i].fps);    
        isfw_log_dprint("      extType:   %d\n",pCtrl->ability.vinAbili.chnInfo[i].extType);    
    }
    isfw_log_dprint("\n");
    
    /*!< config param */    
    hikflow_demo_print_config(&pCtrl->hikflow_config);
    /*!< algorithm param */    
    hikflow_demo_print_net_info(&pCtrl->net_info);

    /*!< scale param */    
    isfw_log_dprint("mscale info:     \n");
    isfw_log_dprint("  group_id:       %d\n",pCtrl->group_id);
    isfw_log_dprint("  mscale_hdl:     %p\n",pCtrl->mscale_hdl);
    isfw_log_dprint("\n");

    /*!< vin yuv get thread */    
    isfw_log_dprint("vin thread data:     \n");
    isfw_log_dprint("  get_lost:      %d\n",pCtrl->yuv_get_lost_times);
    isfw_log_dprint("  send_lost:     %d\n",pCtrl->yuv_send_lost_times);
    isfw_log_dprint("  rel_lost:      %d\n",pCtrl->yuv_rel_lost_times);
    isfw_log_dprint("  suc_cnt:       %d\n",pCtrl->yuv_suc_times);
    isfw_log_dprint("  exit_flg:      %s\n",pCtrl->vin_thread_exit == 1 ? "runing":"exit");
    isfw_log_dprint("\n");
    
    /*!< algorithm thread */    
    isfw_log_dprint("net_proc thread data:     \n");
    isfw_log_dprint("  get_lost:      %d\n",pCtrl->mscale_get_lost_times);
    isfw_log_dprint("  rel_lost:      %d\n",pCtrl->mscale_rel_lost_times);
    isfw_log_dprint("  get_suc:       %d\n",pCtrl->mscale_get_suc_times);
    isfw_log_dprint("  rel_suc:       %d\n",pCtrl->mscale_rel_suc_times);
    isfw_log_dprint("  net_suc:       %d\n",pCtrl->net_suc_times);
    isfw_log_dprint("  net_lost:      %d\n",pCtrl->net_lost_times);
    isfw_log_dprint("  exit_flg:      %s\n",pCtrl->net_thread_exit == 1 ? "runing":"exit");
    isfw_log_dprint("\n");

    /*!< algorithm rule */    
    isfw_log_dprint("net_rule data:     \n");
    isfw_log_dprint("  point_num:     %d\n",pCtrl->rule.point_num);
    for(i = 0;i < pCtrl->rule.point_num && i < HIKFLOW_DEMO_MAX_POINT_NUM;i++)
    {
        isfw_log_dprint("rect_%d     x=%f\n",i,pCtrl->rule.point[i].x);
        isfw_log_dprint("rect_%d     y=%f\n",i,pCtrl->rule.point[i].y);
    }
    isfw_log_dprint("\n");

    /*!< algorithm attr */    
    isfw_log_dprint("attr data:     \n");
    isfw_log_dprint("  attr_num:      %d\n",pCtrl->net_info.attr_list.attr_num);
    for(int i = 0; i < pCtrl->net_info.attr_list.attr_num;i++)
    {
        isfw_log_dprint("  i %d idx %d name %s \n",i,pCtrl->net_info.attr_list.attr[i].idx,pCtrl->net_info.attr_list.attr[i].name);
    }
    isfw_log_dprint("\n");

    /*!< alarn info */    
    isfw_log_dprint("alarm data:     \n");
    isfw_log_dprint("  json_buf:      %p\n",pCtrl->json_buf);
    isfw_log_dprint("  json_buf_phy:  %p\n",pCtrl->json_buf_phy);
    isfw_log_dprint("  json_buf_size: %d\n",pCtrl->json_max_size);

    isfw_log_dprint("  jpegenc_handle:   %p\n",pCtrl->jpegenc_handle);
    isfw_log_dprint("  jpegenc_width:    %d\n",pCtrl->image.width);
    isfw_log_dprint("  jpegenc_height:   %d\n",pCtrl->image.height);
    isfw_log_dprint("  jpegenc_quality:  %d\n",pCtrl->image.quality);
    isfw_log_dprint("  jpegenc_buf:      %p\n",pCtrl->jpegenc_buf);
    isfw_log_dprint("  jpegenc_buf_phy:  %p\n",pCtrl->jpegenc_buf_phy);
    isfw_log_dprint("  jpegenc_buf_size: %d\n",pCtrl->jpegenc_max_size);

    isfw_log_dprint("  alarm_lost:       %d\n",pCtrl->alarm_lost_times);
    isfw_log_dprint("  alarm_suc:        %d\n",pCtrl->alarm_suc_times);
    isfw_log_dprint("  jpegenc_lost:     %d\n",pCtrl->jpegenc_lost_times);
    isfw_log_dprint("  jpegenc_suc:      %d\n",pCtrl->jpegenc_suc_times);
    isfw_log_dprint("\n");


    /*!< thread recode */    
	ISFW_STAT_PROC_STAT *stat[] =
	{
		&pCtrl->vin_proc,
        &pCtrl->net_proc,
        &pCtrl->frame_proc,
        &pCtrl->jpegenc_proc,
        &pCtrl->alarm_proc,
            
	};
    
	char *names[] = 
	{
		"vin_proc","net_proc","frame_proc","jpegenc","upload"	};
	
	isfw_stat_proc_stat_print(stat, names, sizeof(stat)/sizeof(ISFW_STAT_PROC_STAT *));
	isfw_log_dprint("================= hikflow_demo prt info start end ==================\n\n");
    return;
}

/*!< initialize the status,register set_prt_lvl hf_stat hf_test hf_mem and initialize the online debug server */
static int hikflow_demo_init_stat(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);

    /*!< initialize status moduler */
    int ret = isfw_stat_init(NULL,-1);
    HIKFLOW_RET(ret != ISFW_STAT_OK,HIKFLOW_DEMO_ERR_FAILED);
	stack_mng_push(NULL,(void *)isfw_stat_deinit);

    /*!< register log ,print level setting,debug */
    ISFW_STAT_REG_ST prt_lvl = {stack_mng_set_prt_lvl,   NULL, 2, "set print level",\
            "set_prt_lvl [mod_name] [lvl] \n\t\toption lvl can be:"
            "\n\t\t0 or NON or NONE or none"
            "\n\t\t1 or DBG or DEBUG or debug"
            "\n\t\t2 or LOG or log"
            "\n\t\t3 or WRN or WARNNING or warnning"
            "\n\t\t4 or ERR or ERROR or error","hf_dbg pid set_print_level HFDemo DBG"};

    ret = isfw_stat_register("set_prt_lvl",&prt_lvl);
    HIKFLOW_RET(ret != ISFW_LOG_OK,HIKFLOW_DEMO_ERR_FAILED);
    
    ISFW_STAT_REG_ST hf_stat = {hikflow_demo_print_stat,   NULL, 2, "print hikflow demo status",\
        "\n\t\t hf_stat","hf_dbg pid hf_stat"};
    hf_stat.arg = pCtrl;
    ret = isfw_stat_register("hf_stat",&hf_stat);
    HIKFLOW_RET(ret != ISFW_LOG_OK,HIKFLOW_DEMO_ERR_FAILED);

    ISFW_STAT_REG_ST hf_test = {hikflow_demo_set_debug,   NULL, 2, "test hikflow demo",\
        "\n\t\t hf_test rule"
        "\n\t\t hf_test enable 1/0"
        "\n\t\t hf_test alarm"
        "\n\t\t hf_test set_all 1/0","hf_dbg pid hf_test"};
    hf_test.arg = pCtrl;
    ret = isfw_stat_register("hf_test",&hf_test);
    HIKFLOW_RET(ret != ISFW_LOG_OK,HIKFLOW_DEMO_ERR_FAILED);

    ISFW_STAT_REG_ST hf_mem = {hikflow_demo_print_mem,   NULL, 2, "print mem",\
        "\n\t\t hf_mem","hf_dbg pid hf_mem"};
    hf_mem.arg = pCtrl;
    ret = isfw_stat_register("hf_mem",&hf_mem);
    HIKFLOW_RET(ret != ISFW_LOG_OK,HIKFLOW_DEMO_ERR_FAILED);

    /*!< online debug server,and hf_dbg can be used */
    ret = isfw_debug_server_init(pCtrl->proc_name);
    HIKFLOW_RET(ret != 0,HIKFLOW_DEMO_ERR_FAILED);
	stack_mng_push(NULL,(void *)isfw_debug_server_deinit);

    HIKFLOW_DBG("hikflow_demo_init_stat ok\n");
    return HIKFLOW_DEMO_OK;
}

/*!< os  memory callback function */
static int hikflow_demo_os_mem_proc(STACK_MNG_MEM_INFO *mem)
{
    HIKFLOW_RET(NULL == mem,HIKFLOW_DEMO_ERR_FAILED);
    void *addr = NULL;
    if(mem->proc_type == STACK_MNG_MEM_PROC_TYPE_ALLOC)
    {
        addr = HIKFLOW_DEMO_ALLOC(mem->size,HIKFLOW_DEMO_MEM_ALIGN);
        HIKFLOW_DBG("hikflow_demo_os_mem_proc alloc %p,size %d\n",addr,mem->size);
        HIKFLOW_RET(NULL == addr,HIKFLOW_DEMO_ERR_FAILED);
        mem->vaddr = addr;
    }
    else
    {
        HIKFLOW_RET(NULL == mem->vaddr,HIKFLOW_DEMO_ERR_FAILED);
        HIKFLOW_DEMO_FREE(mem->vaddr);
        HIKFLOW_DBG("hikflow_demo_os_mem_proc free %p,size %d\n",addr,mem->size);
    }
    return HIKFLOW_DEMO_OK;
}

/*!< cmm memory callback function */
static int hikflow_demo_cmm_mem_proc(STACK_MNG_MEM_INFO *mem)
{
    HIKFLOW_RET(NULL == mem,HIKFLOW_DEMO_ERR_FAILED);
    if(mem->proc_type == STACK_MNG_MEM_PROC_TYPE_ALLOC)
    {
        unsigned long long  u64PhyAddr = 0;
        void    *ppVirAddr  = NULL;
        int ret = opdevsdk_mem_allocCache((void *)&u64PhyAddr, (void **)&ppVirAddr, (const char *)mem->name, mem->size);
        HIKFLOW_RET(OPDEVSDK_S_OK != ret || NULL == ppVirAddr ,HIKFLOW_DEMO_ERR_FAILED);
        HIKFLOW_DBG("hikflow_demo_os_mem_proc alloc p-%p v-%p,size %d\n",mem->paddr,mem->vaddr,mem->size);
        mem->vaddr = ppVirAddr;
        mem->paddr = (void *)(size_t)u64PhyAddr;
    }
    else
    {
        HIKFLOW_RET(NULL == mem->vaddr,HIKFLOW_DEMO_ERR_FAILED);
        HIKFLOW_RET(NULL == mem->paddr,HIKFLOW_DEMO_ERR_FAILED);
        opdevsdk_mem_free(mem->paddr,mem->vaddr);
        HIKFLOW_DBG("hikflow_demo_os_mem_proc free p-%p v-%p,size %d\n",mem->paddr,mem->vaddr,mem->size);
    }
    return HIKFLOW_DEMO_OK;
}

/*!< initialize the memory statistics,so we can get the recorde online,it must be initialized after bsc */
static int hikflow_demo_init_mem(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_FAILED);

    /*!< register os and cmm memory callback function */
    STACK_MNG_MEM_FUNC_REG reg = {0};
    reg.os_fxn = hikflow_demo_os_mem_proc;
    reg.cmm_fxn = hikflow_demo_cmm_mem_proc;
    
    /*!< initialize memory record moduler */
    int ret = stack_mng_mem_init(&reg);
    HIKFLOW_RET(ret != ISFW_STAT_OK,HIKFLOW_DEMO_ERR_FAILED);
	stack_mng_push(NULL,(void *)stack_mng_mem_deinit);

    HIKFLOW_DBG("hikflow_demo_init_mem ok\n");
    return HIKFLOW_DEMO_OK;
}

/*!< initialize the yuv stream:mscale,vin ,mem module,only in camera mode */
static int hikflow_demo_init_yuv_stream(HIKFLOW_DEMO_CTRL* pCtrl)
{
    int ret = 0;
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);

    if (HIKFLOW_DEMO_PROC_FROM_CAM == pCtrl->proc_type)
    {                
        /*!< initialize the scale module */
        _stack_mng_proc_time_(ret = hikflow_demo_init_mscale(pCtrl),HIKFLOW_DEMO_WAIT_TIME_OUT);
        HIKFLOW_KEY_RET((ret != 0),ret,"hikflow_demo_init_mscale err");

        /*!< start the scale module */
        ret = opdevsdk_mscale_start(pCtrl->mscale_hdl);
        HIKFLOW_KEY_RET((ret != OPDEVSDK_S_OK),ret,"opdevsdk_mscale_start err");
        stack_mng_push((void *)pCtrl->mscale_hdl,opdevsdk_mscale_stop);
        HIKFLOW_LOG("opdevsdk_mscale_start \n");
    }

    HIKFLOW_LOG("hikflow_demo_init_yuv_stream ok\n");
    return HIKFLOW_DEMO_OK;
}

/*!< creat pthreads to run yuv getting and alorithm proccessing */
static int hikflow_demo_creat_pthread(HIKFLOW_DEMO_CTRL* pCtrl)
{
    int ret = 0;
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);
    pthread_t alg_tid;
    pthread_t vin_tid;
    pthread_t destroy_tid;

    /*!< get data from file and send them to algrithm process in FILE mode */
    if (HIKFLOW_DEMO_PROC_FROM_FILE == pCtrl->proc_type)
    {
        /*!< only need one pthread for processing */
        pthread_create(&alg_tid, NULL, (void *)hikflow_demo_alg_thread_from_file, pCtrl);  
        pthread_join(alg_tid, NULL);
    }
    /*!< get yuv images from camera and send them to algrithm process,which is used for offical running on camera in CAMERA mode */
    else if (HIKFLOW_DEMO_PROC_FROM_CAM == pCtrl->proc_type)
    {                
        /*!< get yuv from vin module and send to mscale module */
        ret = pthread_create(&vin_tid, NULL, (void *)hikflow_demo_vin_get_thread, pCtrl);
        if(ret != 0)
        {
            HIKFLOW_LOG("creat hikflow_demo_vin_get_thread error ret = 0x%x\n",ret);
            return HIKFLOW_DEMO_ERR_FAILED;
        }
        
        /*!< algorithm processing and upload alarm */
        ret = pthread_create(&alg_tid, NULL, (void *)hikflow_demo_alg_thread_from_cam, pCtrl);
        if(ret != 0)
        {
            HIKFLOW_LOG("creat hikflow_demo_alg_proc_thread_ex error ret = 0x%x\n",ret);
            return HIKFLOW_DEMO_ERR_FAILED;
        }
        /*!< the last push,the first enter after exit */
        stack_mng_push((void *)pCtrl,hikflow_demo_quit_thread);
        
        pCtrl->b_init = 1;

        /*!<  creat process destroy pthread for exit */  
        ret = pthread_create(&destroy_tid, NULL, (void *)TSK_hikflow_destroy, pCtrl);
    }

    HIKFLOW_LOG("hikflow_demo_creat_pthread ok\n");
    return HIKFLOW_DEMO_OK;
}

/** 
* @brief            hikflow demo check input param function
*
* @param[in] 		argc    number of input parameters     
* @param[in] 		argv[]  eEach parameter pointer set    
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_demo_param_check(int argc, char *argv[])
{
    if (argc != HIKFLOW_DEMO_INPUT_PARAM_NUM)
    {
		if((argc == HIKFLOW_DEMO_INPUT_PARAM_NUM+1) && (argv[3]!= NULL))
		{
	        printf("the third param is chan %s\n",argv[3]);
		}
		else
		{
	        printf("please check the input param num(%d),must be %d\n",argc,HIKFLOW_DEMO_INPUT_PARAM_NUM);
	        printf("the first param is path where we can get hikflow_config.json\n");
	        printf("the second param is running mode : CAMERA or FILE\n");
	        printf("eg: ./hikflow_demo ./ CAMERA or ./hikflow_demo ./ FILE\n");
	        printf("eg: ./hikflow_demo  /heop/package/hikflow_demo/ CAMERA or ./hikflow_demo  /heop/package/hikflow_demo/  FILE\n");
	        return HIKFLOW_DEMO_ERR_INV_PARAM;
		}
    } 
    
    /*!< FILE:read data from file for test pricision,such as image_list,YUV: get data from camara */
    if((0 != strcmp("CAMERA", argv[2])) && (0 != strcmp("FILE", argv[2])))
    {
        printf("please input argv[2] 'FILE' or 'CAMERA'\n");
        return HIKFLOW_DEMO_ERR_INV_PARAM;
    }

    for(int i = 0; i < argc-1; i++)
    {
        printf("argv[%d] %s\n", i, argv[i]);
    }

    return HIKFLOW_DEMO_OK;
}

/** 
* @brief            hikflow demo set parameters function
*
* @param[in] 		param   input parameter pointer    
* @param[in] 		type    type of parameter,see HIKFLOW_DEMO_PARAM_SET_TYPE    
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_demo_param_set(void* param, int type)
{
	int i = 0;
    HIKFLOW_RET(NULL == param,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_DEMO_CTRL* pCtrl = &hikflow_ctrl;
    HIKFLOW_RET(1 != pCtrl->b_init,HIKFLOW_DEMO_ERR_FAILED);

	switch(type)
	{
		case HIKFLOW_DEMO_PARAM_SET_TYPE_EN:
		{
			char enable = *(char*) param;
			pCtrl->b_start = enable;
            HIKFLOW_LOG("hikflow_demo_param_set b_start %d input %d\n",pCtrl->b_start,enable);
			break;
		}
		case HIKFLOW_DEMO_PARAM_SET_TYPE_RULE:
		{
			HIKFLOW_DEMO_POLYGON* region = (HIKFLOW_DEMO_POLYGON*) param;
			pthread_mutex_lock(&pCtrl->mutex);
			pCtrl->rule.point_num = region->point_num;
			for(i = 0; i < region->point_num && i < HIKFLOW_DEMO_MAX_POINT_NUM; i++)
			{
				pCtrl->rule.point[i].x = (float)(region->point[i].x)/1000;
				pCtrl->rule.point[i].y = (float)(region->point[i].y)/1000;
                HIKFLOW_LOG("hikflow_demo_param_set rule %d x %f  y %f\n",i,pCtrl->rule.point[i].x,pCtrl->rule.point[i].y);
			}
			
			pthread_mutex_unlock(&pCtrl->mutex);
			break;
		}
		case HIKFLOW_DEMO_PARAM_SET_TYPE_ALARM_IMAG:
		{
			HIKFLOW_DEMO_ALARM_IMAGE_INFO * image = (HIKFLOW_DEMO_ALARM_IMAGE_INFO*) param;
			pthread_mutex_lock(&pCtrl->mutex);
		    memset(&pCtrl->image, 0x0, sizeof(HIKFLOW_DEMO_ALARM_IMAGE_INFO));
			pCtrl->image.width = image->width;
			pCtrl->image.height = image->height;
            if(image->quality > 99)
            {
                pCtrl->image.quality = 99;
                HIKFLOW_ERR("hikflow_demo_param_set  invalid  quality %d\n",image->quality);
            }
            else
            {
                pCtrl->image.quality = image->quality;
            }
            
			pthread_mutex_unlock(&pCtrl->mutex);
			break;
		}
		default:
        {
            HIKFLOW_ERR("hikflow_demo_param_set invalid  type %d\n",type);
        }
	}

	return HIKFLOW_DEMO_OK;
}

/** 
* @brief            hikflow demo initialization
*
* @param[in] 		argc    number of input parameters     
* @param[in] 		argv[]  eEach parameter pointer set    
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_demo_init(int argc, char *argv[])
{    
    int i, ret = 0;
    
    /*!< initialize the syslog,it will save the logs in the path /var/log/conaintrt name/dsp.log */
    hikflow_demo_init_syslog();

    HIKFLOW_DEMO_CTRL* pCtrl = &hikflow_ctrl;
    
    /*!< it is required that the hikflow demo must release the hardware resource proactively befor container exit */
    /*!< so the process will receive signal 15 or other abnormal signal (11 or 7...)to do the realeasing job befor container exit */
    hikflow_demo_init_monitor();

    /*!< set process name and version */
    hikflow_demo_init_proc(pCtrl,argv);

    /*!< get config data from hikflow_config.json */
    ret = json_proc_parse_config_json(pCtrl);
    HIKFLOW_KEY_RET(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,"json_proc_parse_json err");
    hikflow_demo_apply_runtime_options(pCtrl);

    /*!< get model attribute names from hikflow_attr.json ,not necessary */
    ret = json_proc_parse_net_attr_json(pCtrl);
    HIKFLOW_ASSER(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED);

    /*!< initialize the status,register set_prt_lvl hf_stat hf_test hf_mem and initialize the online debug server */
    hikflow_demo_init_stat(pCtrl);

    /*!< key step 1: initialize the bsc library */
    _stack_mng_proc_time_(ret = hikflow_demo_init_bsc(pCtrl),HIKFLOW_DEMO_WAIT_TIME_OUT);
    HIKFLOW_KEY_RET(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,"hikflow_demo_init_bsc err");

    /*!< initialize the memory statistics,so we can get the recorde online,it must be initialized after bsc */
    ret = hikflow_demo_init_mem(pCtrl);
    HIKFLOW_KEY_RET(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,"hikflow_demo_init_mem err");

    /*!< key step 2: initialize the scheduler lib,which is the basic lib of hikflow */
    _stack_mng_proc_time_(ret = hikflow_demo_init_sche(pCtrl),HIKFLOW_DEMO_WAIT_TIME_OUT);
    HIKFLOW_KEY_EXIT(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,err0,"hikflow_demo_init_sche err");

    /*!< key step 3: initialization for alarm */
    _stack_mng_proc_time_(ret = hikflow_demo_init_alarm(pCtrl),HIKFLOW_DEMO_WAIT_TIME_OUT); 
    HIKFLOW_KEY_EXIT(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,err0,"hikflow_demo_init_alarm err");
	stack_mng_push(pCtrl,(void *)hikflow_demo_deinit_alarm);

    /*!< key step 4: initialize the hikflow lib */
    _stack_mng_proc_time_(ret = hikflow_demo_init_alg(pCtrl),HIKFLOW_DEMO_WAIT_TIME_OUT);
    HIKFLOW_KEY_EXIT(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,err0,"hikflow_demo_init_alg err");
    
    /*!< key step 5: initialize the yuv stream */
    _stack_mng_proc_time_(ret = hikflow_demo_init_yuv_stream(pCtrl),HIKFLOW_DEMO_WAIT_TIME_OUT);
    HIKFLOW_KEY_EXIT(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,err0,"hikflow_demo_init_yuv_stream err");

    /*!< key step 6: creat pthreads to run yuv getting and alorithm proccessing */
    ret = hikflow_demo_creat_pthread(pCtrl);
    HIKFLOW_KEY_EXIT(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,err0,"hikflow_demo_creat_pthread err");

    HIKFLOW_LOG("hikflow_demo_init ok\n");
    return 0;
err0:
    /*!< de-initialize demo */
    hikflow_demo_deinit(pCtrl);
    return HIKFLOW_DEMO_ERR_FAILED;
}

