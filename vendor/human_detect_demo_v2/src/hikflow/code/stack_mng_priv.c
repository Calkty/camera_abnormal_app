/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         stack_mng_priv.c
* @brief        stack_mng main code
*
* @date         2023-8-24
* @version      2.0.0
* @note         1. supports deinitialization function register for directly releasing resource
*               2. set print level
*****************************************************************************/

#include "stack_mng_priv.h"

static STACK_MNG_PROC stack_proc = {0};    
static STACK_MNG_MEM_CTRL stack_mem = {0};    

/** 
* @brief            push a function to stack
*
* @param[in] 		arg             pointer of param     
* @param[in] 		func            callback function     
* 
* @return           0 if successful, otherwise an error number returned
*/
void stack_mng_push(void *arg,void *func)
{
    STACK_MNG_NORET(NULL == func,STACK_MNG_ERR_FAILED);  
    STACK_MNG_PROC *stack_mng = &stack_proc;

    if(stack_mng->top < STACK_MNG_DEEP)
	{
		stack_mng->mng[stack_mng->top].arg = arg;
        if(NULL == stack_mng->mng[stack_mng->top].arg)
        {
            stack_mng->mng[stack_mng->top].release_call_back_ex = func;
        }
        else
        {
            stack_mng->mng[stack_mng->top].release_call_back = func;
        }
        stack_mng->top++;
	}
	else
	{
		STACK_MNG_LOG("stack_mng->top=%d>=%d\n",stack_mng->top,STACK_MNG_DEEP);
	}

    return;
}

static const STACK_MNG_FUNC *stack_mng_pop(void)
{
    STACK_MNG_PROC *stack_mng = &stack_proc;
	if(stack_mng->top > 0)
	{
		return &stack_mng->mng[--stack_mng->top];
	}
	else
	{
		return NULL;
	}
}

/** 
* @brief            call  all register functions  for release resource 
*
* @param[in] 		none     
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_deinit()
{
	const STACK_MNG_FUNC * mng = NULL;
	while(NULL != (mng = stack_mng_pop()))
	{
        if(mng->arg && mng->release_call_back)
        {
            _stack_mng_proc_time_(mng->release_call_back(mng->arg),500*1000);
        }
        else if(mng->release_call_back_ex)
        {
            _stack_mng_proc_time_(mng->release_call_back_ex(),500*1000);
        }
	}

	STACK_MNG_LOG("stack_mng_deinit ok\n");
	return STACK_MNG_OK;
}

/** 
* @brief            read data from file  
*
* @param[in] 		file            file path     
* @param[out] 		pBuf            data buffer address     
* @param[out] 		len             data length
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_open_file(char *file,char **buf,int *len)
{
    int file_len = 0;
	int ret = 0;
    char *content = NULL;
    STACK_MNG_RET((NULL == file),STACK_MNG_ERR_FAILED);

    FILE *fp = fopen(file,"rb");
    STACK_MNG_RET((NULL == fp),STACK_MNG_ERR_FAILED);
    
    *buf = NULL;
    *len = 0;
    
    ret = fseek(fp,0,SEEK_END);     
    STACK_MNG_EXIT((NULL == fp),ret,err1);
    
    file_len = ftell(fp);              
    ret = fseek(fp,0,SEEK_SET);  
    STACK_MNG_EXIT((0 > ret),ret,err1);
    STACK_MNG_EXIT((file_len < 1),file_len,err0);

    content = (char*)STACK_MNG_ALLOC(file_len+1,STACK_MNG_MEM_ALIGN);
    STACK_MNG_EXIT((NULL == content),STACK_MNG_ERR_FAILED,err0);
    
    int ret_num = fread(content,1,file_len,fp);
    STACK_MNG_EXIT((ret_num < 0),ret_num,err1);
    
	memset(content+file_len,0,1);
    fclose(fp);
    *buf = content;
    *len = file_len+1;

    STACK_MNG_DBG("stack_mng_open_file(%s) ok \n",file);
	return STACK_MNG_OK;

err1:
    if(content)
    {
        STACK_MNG_FREE(content);  
    }    
err0:
    if(fp)
    {
        fclose(fp);  
    }
    return STACK_MNG_ERR_FAILED;  
}

/** 
* @brief            close file-release buffer 
*
* @param[in] 		buf             buffer address getted from stack_mng_open_file  
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_close_file(void *buf)
{
    if(buf)
    {
        STACK_MNG_FREE(buf);
    }

    STACK_MNG_DBG("stack_mng_close_fileok \n");    
	return STACK_MNG_OK;
}

/** 
* @brief            write file 
*
* @param[in] 		file            file path     
* @param[in] 		pBuf            data buffer address     
* @param[in] 		len             data length
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_write_file(const char *file,void *pBuf,int len)
{
    STACK_MNG_RET((NULL == file),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((NULL == pBuf),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((0 >= len),STACK_MNG_ERR_FAILED);

	FILE * fid = NULL;
	fid = fopen(file,"wb");
    STACK_MNG_RET((NULL == fid),STACK_MNG_ERR_FAILED);
    	
	fwrite(pBuf,len,1,fid);
	fclose(fid);
    
    STACK_MNG_DBG("stack_mng_write_file(%s) ok \n",file);
	return STACK_MNG_OK;
}

/** 
* @brief            get ns
*
* @param[in] 		none     
* 
* @return           ns if successful, otherwise an error number returned
*/
unsigned long long stack_mng_get_time(void)
{
	struct timespec tv;	 
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return 1000000LL * tv.tv_sec + tv.tv_nsec/1000;
}

static int stack_mng_is_num_str(char *str)
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

static int stack_mng_str_2_int(char *str)
{
	int ret = 0;
	while('0' <= *str && *str <= '9')
	{
		ret = ret * 10 + str[0] - '0';
		str++;
	}
	return ret;
}

/** 
* @brief            set print level 
*
* @param[in] 		arg             pointer of handle     
* @param[in] 		argc            number of input parameters     
* @param[in] 		argv[]          eEach parameter pointer set    
* 
* @return           0 if successful, otherwise an error number returned
*/
void stack_mng_set_prt_lvl(void *arg,int argc, char *argv[])
{
    int lvl = 0;
    if(argc < 2)
    {
        return;
    }
    if(stack_mng_is_num_str(argv[1]))
    {
        lvl = stack_mng_str_2_int(argv[1]);
    }
    else if(strcmp(argv[1], "NON") == 0 || strcmp(argv[1], "NONE") == 0 || strcmp(argv[1], "none") == 0)
    {
        lvl = 0;
    }
    else if(strcmp(argv[1], "DBG") == 0 || strcmp(argv[1], "DEBUG") == 0 || strcmp(argv[1], "debug") == 0)
    {
        lvl = 1;
    }
    else if(strcmp(argv[1], "LOG") == 0 || strcmp(argv[1], "log") == 0)
    {
        lvl = 2;
    }
    else if(strcmp(argv[1], "WRN") == 0 || strcmp(argv[1], "WARNNING") == 0 || strcmp(argv[1], "warnning") == 0)
    {
        lvl = 3;
    }
    else if(strcmp(argv[1], "ERR") == 0 || strcmp(argv[1], "ERROR") == 0 || strcmp(argv[1], "error") == 0)
    {
        lvl = 4;
    }
    else
    {
        isfw_log_dprint("%s is not a valid print level.", argv[1]);
        return ;
    }

    isfw_log_dprint("module=%s, lvl=%d\n", argv[0], lvl);
    if(0==strcmp(argv[0], "ALL"))
    {
        isfw_log_set_level_all(lvl);
    }
    else
    {
        isfw_log_set_level((const char *)argv[0], lvl);
    }

    return;
}

/*!< get index whose v_addr is minimum value */
static int stack_mng_mem_get_min(STACK_MNG_MEM_CTRL *mem_ctrl,STACK_MNG_MEM_INFO *mem_info)
{
    void *min_addr = (void *)0xffffffffffffffff;
    int idx = -1,i=0;
    for(i = 0; i <  mem_ctrl->used_cnt;i++)
    {
        if(mem_ctrl->mem_info[i].vaddr != NULL && mem_ctrl->mem_info[i].vaddr < min_addr && mem_info->type == mem_ctrl->mem_info[i].type)
        {
            idx = i;
            min_addr = mem_ctrl->mem_info[i].vaddr;
        }
    }

    if(-1 == idx)
    {
        return STACK_MNG_ERR_FAILED;
    }

    *mem_info = mem_ctrl->mem_info[idx];
    return i;
}

/*!< get index throug visual address */
static int stack_mng_mem_get_idx(STACK_MNG_MEM_CTRL *mem_ctrl,STACK_MNG_MEM_INFO *mem_info)
{
    STACK_MNG_RET((NULL == mem_ctrl),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((NULL == mem_info),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((STACK_MNG_MEM_INFO_MAX_LEN < mem_ctrl->used_cnt),STACK_MNG_ERR_FAILED);
    
    for(int i = 0; i < mem_ctrl->used_cnt;i++)
    {
        if(mem_ctrl->mem_info[i].vaddr == NULL || mem_ctrl->mem_info[i].vaddr != mem_info->vaddr)
        {
            STACK_MNG_DBG("mem_ctrl->mem_info[i].vaddr %p get %p \n",mem_ctrl->mem_info[i].vaddr,mem_info->vaddr);
            continue;
        }
        STACK_MNG_DBG("find i %d addr %p\n",i,mem_info->vaddr);
        return i;
    }

    return STACK_MNG_ERR_FAILED;
}

/** 
* @brief            initialize the memory recode module  
*
* @param[in] 		reg             memory register callback     
* @param[out] 		none            
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_mem_init(STACK_MNG_MEM_FUNC_REG *reg)
{
    STACK_MNG_RET((0 != stack_mem.init_flg),STACK_MNG_OK);
    STACK_MNG_RET((0 == reg->cmm_fxn),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((0 == reg->os_fxn),STACK_MNG_ERR_FAILED);
    STACK_MNG_MEM_CTRL *mem_ctrl = &stack_mem;

    /*!<only manage os memory and cmm mempry */
    memset(mem_ctrl,0,sizeof(STACK_MNG_MEM_CTRL));
    mem_ctrl->statistics[STACK_MNG_MEM_TYPE_OS].fxn = reg->os_fxn;
    mem_ctrl->statistics[STACK_MNG_MEM_TYPE_CMM].fxn = reg->cmm_fxn;
    mem_ctrl->statistics[STACK_MNG_MEM_TYPE_OS].type = STACK_MNG_MEM_TYPE_OS;
    mem_ctrl->statistics[STACK_MNG_MEM_TYPE_CMM].type = STACK_MNG_MEM_TYPE_CMM;
    
    /*!<initialize for changing statistics */
    pthread_mutex_init(&mem_ctrl->mutex, NULL);
    mem_ctrl->init_flg = 1;
    
    STACK_MNG_DBG("stack_mng_mem_init ok \n");
    return STACK_MNG_OK;
}

/** 
* @brief            get memory,must pay attention on size, type and proc_type
*
* @param[in/out]    mem_info        memory requied
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_mem_get(STACK_MNG_MEM_INFO *mem_info)
{
    STACK_MNG_MEM_CTRL *mem_ctrl = &stack_mem;
    STACK_MNG_RET((1 != mem_ctrl->init_flg),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((NULL == mem_info),STACK_MNG_ERR_FAILED);

    /*!<must alloc */
    STACK_MNG_RET((STACK_MNG_MEM_PROC_TYPE_ALLOC != mem_info->proc_type),STACK_MNG_ERR_FAILED);
    STACK_MNG_DBG("enter v_addr %p p_addr_%p  size %d tag %d proc %d type %d name %s\n",mem_info->vaddr,\
        mem_info->paddr,mem_info->size,mem_info->tag,mem_info->proc_type,mem_info->type,mem_info->name);

    int ret = 0;
    pthread_mutex_lock(&mem_ctrl->mutex);
    for(int i = 0;i < STACK_MNG_MEM_TYPE_MAX_NUM;i++)
    {
        /*!<jurge os or cmm type */
        if(i != mem_info->type)
        {
            continue;
        }
        
        /*!<statistics for os or cmm */
        STACK_MNG_MEM_STATISTICS    *statistics = &mem_ctrl->statistics[i];
        
        /*!<pre_alloc shows will to be allocated,alloc_size means having been allocated,faild_alloc refer to memory not be allocated */
        statistics->pre_alloc += mem_info->size;

        /*!< allocate memory */
        ret = statistics->fxn(mem_info);
        STACK_MNG_DBG("exit v_addr %p p_addr_%p  size %d tag %d proc %d type %d name %s\n",\
            mem_info->vaddr,mem_info->paddr,mem_info->size,mem_info->tag,mem_info->proc_type,mem_info->type,mem_info->name);
        STACK_MNG_DBG("statistics->pre_alloc=%llu \n",statistics->pre_alloc);
        if(ret != STACK_MNG_OK)
        {
            statistics->alloc_faild_cnt++;
            statistics->faild_alloc+=mem_info->size;
            STACK_MNG_DBG("statistics->alloc_faild_cnt=%d failed %llu\n",statistics->alloc_faild_cnt,statistics->faild_alloc);
            pthread_mutex_unlock(&mem_ctrl->mutex);
            return STACK_MNG_ERR_FAILED;
        }
        
        statistics->alloc_size += mem_info->size;
        statistics->alloc_suc_cnt++;

        /*!< save memory information to buffers */
        if(mem_ctrl->used_cnt < STACK_MNG_MEM_INFO_MAX_LEN)
        {
            mem_ctrl->mem_info[mem_ctrl->used_cnt]= *mem_info;
            mem_ctrl->mem_info[mem_ctrl->used_cnt].tag = STACK_MNG_MEM_TAG_USED;
            mem_ctrl->used_cnt++;
        }
        
        /*!< max_size means the maximum memory allocated befor */
        if(statistics->max_size < statistics->alloc_size)
        {
            statistics->max_size = statistics->alloc_size;
        }
        
        STACK_MNG_DBG("statistics->alloc_size=%d suc %d cnt %d max_size %d\n",statistics->alloc_size,statistics->alloc_suc_cnt,mem_ctrl->used_cnt,statistics->max_size);
        break;
    }
    pthread_mutex_unlock(&mem_ctrl->mutex);
    
    return STACK_MNG_OK;
}

/** 
* @brief            release memory,must pay attention on vaddr,paddr, type and proc_type
*
* @param[in/out]    mem_info        memory will be free
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_mem_release(STACK_MNG_MEM_INFO *mem_info)
{
    STACK_MNG_MEM_CTRL *mem_ctrl = &stack_mem;
    STACK_MNG_RET((1 != mem_ctrl->init_flg),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((NULL == mem_info),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((STACK_MNG_MEM_PROC_TYPE_FREE != mem_info->proc_type),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((STACK_MNG_MEM_TYPE_OS == mem_info->type && NULL == mem_info->vaddr),STACK_MNG_ERR_FAILED);
    STACK_MNG_RET((STACK_MNG_MEM_TYPE_CMM == mem_info->type && NULL == mem_info->vaddr && NULL == mem_info->paddr),STACK_MNG_ERR_FAILED);

    int ret = 0;
    STACK_MNG_DBG("enter v_addr %p p_addr_%p  size %d tag %d proc %d type %d name %s\n",\
        mem_info->vaddr,mem_info->paddr,mem_info->size,mem_info->tag,mem_info->proc_type,mem_info->type,mem_info->name);
    pthread_mutex_lock(&mem_ctrl->mutex);
    for(int i = 0;i < STACK_MNG_MEM_TYPE_MAX_NUM;i++)
    {
        /*!<jurge os or cmm type */
        if(i != mem_info->type)
        {
            continue;
        }
        
        int page_size = 0;
        STACK_MNG_MEM_STATISTICS    *statistics = &mem_ctrl->statistics[i];

        /*!< free memory */
        ret = statistics->fxn(mem_info);
        int idx = stack_mng_mem_get_idx(mem_ctrl,mem_info);

        /*!< free success */
        if(ret == STACK_MNG_OK)
        {
            STACK_MNG_DBG("mem_ctrl->used_cnt=%d \n",mem_ctrl->used_cnt);
            /*!< delete the last one ,and save it to the buffer,where will to be free */
            if(idx >= 0 && mem_ctrl->used_cnt != 0 && mem_ctrl->used_cnt <= STACK_MNG_MEM_INFO_MAX_LEN)
            {
                page_size = mem_ctrl->mem_info[idx].size;
                mem_ctrl->mem_info[idx] = mem_ctrl->mem_info[mem_ctrl->used_cnt-1];
                memset(&mem_ctrl->mem_info[mem_ctrl->used_cnt-1],0,sizeof(STACK_MNG_MEM_INFO));
                mem_ctrl->used_cnt--;
            }
            else
            {
                page_size = mem_info->size;
            }
            /*!< update statistics */
            statistics->alloc_size -= page_size;
            statistics->pre_alloc -= page_size;
            statistics->release_suc_cnt++;            
            STACK_MNG_DBG("page_size %d alloc_size=%llu pre %llu cnt %d  \n",\
                page_size,statistics->alloc_size,statistics->pre_alloc,statistics->release_suc_cnt);
        }
        else
        {
            /*!< free failed,make a tag */
            if(idx >= 0)
            {
                mem_ctrl->mem_info[idx].tag =STACK_MNG_MEM_TAG_ERR;
            }
            statistics->release_failed_cnt++;
            STACK_MNG_DBG("statistics->release_failed_cnt=%d \n",statistics->release_failed_cnt);
        }
        
        break;
    }
    
    pthread_mutex_unlock(&mem_ctrl->mutex);
    return STACK_MNG_OK;
}

/** 
* @brief            de-initialize the memory recode module  
*
* @param[in] 		none     
* @param[out] 		none            
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_mem_deinit()
{
    STACK_MNG_MEM_CTRL *mem_ctrl = &stack_mem;
    STACK_MNG_RET((1 != mem_ctrl->init_flg),STACK_MNG_ERR_FAILED);
    STACK_MNG_LOG("stack_mng_mem_deinit enter \n");

    stack_mng_mem_stat_prt();
    pthread_mutex_lock(&mem_ctrl->mutex);
    for(int i = 0;i < STACK_MNG_MEM_TYPE_MAX_NUM;i++)
    {
        STACK_MNG_MEM_STATISTICS    *statistics = &mem_ctrl->statistics[i];

        STACK_MNG_LOG("i %d statistics->alloc_size %d \n",i,statistics->alloc_size);
        /*!< if alloc_size is not zero ,it means that the system has not destroy memory itself */
        if(0 == statistics->alloc_size)
        {
            continue;
        }

        /*!<when alloc_size !=0,check which one has not been free,and then free them */
        for(int j = 0;j < mem_ctrl->used_cnt;j++)
        {
            STACK_MNG_MEM_INFO *mem_info = &mem_ctrl->mem_info[j];
            if(NULL == mem_info->vaddr || i != mem_info->type)
            {
                continue;
            }
            
            mem_info->proc_type = STACK_MNG_MEM_PROC_TYPE_FREE;
            int ret = statistics->fxn(mem_info);
            if(ret == STACK_MNG_OK)
            {
                statistics->alloc_size -= mem_info->size;
                statistics->pre_alloc -= mem_info->size;
                statistics->release_suc_cnt++;            
            }
            else
            {
                statistics->release_failed_cnt++;
            }
        }
        
        /*!< the print means that the allocate memory times at the same time is larger than STACK_MNG_MEM_INFO_MAX_LEN */
        if(statistics->alloc_size != 0)
        {
            char name[12] = "cmm";
            if(i == 0)
            {
                snprintf(name,sizeof(name),"%s","s");
            }
            STACK_MNG_ERR("stack_mng_mem_deinit already have (%llu) memory not free(%s) \n",statistics->alloc_size,name);
        }
    }   
    pthread_mutex_unlock(&mem_ctrl->mutex);

    pthread_mutex_destroy(&mem_ctrl->mutex);
    memset(mem_ctrl,0,sizeof(STACK_MNG_MEM_CTRL));

    STACK_MNG_DBG("stack_mng_mem_deinit ok \n");
    return STACK_MNG_OK;
}

/** 
* @brief            print the memory recode module  
*
* @param[in] 		none     
* @param[out] 		none            
* 
* @return           0 if successful, otherwise an error number returned
*/
void stack_mng_mem_stat_prt()
{
    STACK_MNG_MEM_CTRL *mem_ctrl = &stack_mem;
    STACK_MNG_NORET((1 != mem_ctrl->init_flg),STACK_MNG_ERR_FAILED);
    int type_idx = 0,i=0;
    
    isfw_log_dprint("\n==========================stack_mng_mem_stat_prt start==========================\n\n");
    pthread_mutex_lock(&mem_ctrl->mutex);
    for(int i = 0;i < STACK_MNG_MEM_TYPE_MAX_NUM;i++)
    {
        type_idx = 0;
        STACK_MNG_MEM_STATISTICS    *statistics = &mem_ctrl->statistics[i];
        char type_name[16];
        if(STACK_MNG_MEM_TYPE_OS == i)
        {
            snprintf(type_name,16,"%s","os");
        }
        else if(STACK_MNG_MEM_TYPE_CMM == i)
        {
            snprintf(type_name,16,"%s","cmm");
        }         
        isfw_log_dprint("-------------%s memory-------------\n",type_name);

        /*!<alloc_size =0 means all memory has been free or not used  */
        if(0 == statistics->alloc_size)
        {
            isfw_log_dprint("    to require %llu    alloc %llu    fail %llu     max_size    %llu\n",\
                statistics->pre_alloc,statistics->alloc_size,statistics->faild_alloc,statistics->max_size);
            isfw_log_dprint("    alloc_cnt faild    %d  succ    %d\n",statistics->alloc_faild_cnt,statistics->alloc_suc_cnt);
            isfw_log_dprint("    relea_cnt faild    %d  succ   %d\n",statistics->release_failed_cnt,statistics->release_suc_cnt);
            continue;
        }

        STACK_MNG_MEM_INFO mem_info_min = {0};
        mem_info_min.type = i;

        /*!< get start address */
        int idx = stack_mng_mem_get_min(mem_ctrl,&mem_info_min); 
        
        if(idx != -1)
        {
            statistics->start_paddr = mem_info_min.paddr;        
            statistics->start_vaddr = mem_info_min.vaddr;
        }
        else
        {
            statistics->start_paddr = NULL;        
            statistics->start_vaddr = NULL;
        }
        
        isfw_log_dprint("    start_vaddr %p     p_addr   %p  \n",statistics->start_vaddr,statistics->start_paddr);
        isfw_log_dprint("    to require %llu    alloc %llu    fail %llu     max_szie    %llu\n",\
            statistics->pre_alloc,statistics->alloc_size,statistics->faild_alloc,statistics->max_size);
        isfw_log_dprint("    alloc_cnt faild    %d  succ    %d\n",statistics->alloc_faild_cnt,statistics->alloc_suc_cnt);
        isfw_log_dprint("    relea_cnt faild    %d  succ   %d\n",statistics->release_failed_cnt,statistics->release_suc_cnt);
        
        isfw_log_dprint("  idx  v_addr        p_addr     size  tag   name\n");
        /*!<print every memory allcated */
        for(int j = 0;j < STACK_MNG_MEM_INFO_MAX_LEN;j++)
        {
            STACK_MNG_MEM_INFO *mem_info = &mem_ctrl->mem_info[j];
            if(NULL == mem_info->vaddr || mem_info->type != i)
            {
                continue;
            }
            char name[16];
            if(STACK_MNG_MEM_TAG_ERR == mem_info->tag)
            {
                snprintf(name,8,"%s","err");
            }
            else if(STACK_MNG_MEM_TAG_FREE == mem_info->tag)
            {
                snprintf(name,8,"%s","free");
            }            
             else if(STACK_MNG_MEM_TAG_USED == mem_info->tag)
            {
                snprintf(name,8,"%s","use");
            }       
             
            isfw_log_dprint("%4d  %p  %p %d %s  %s\n",type_idx,mem_info->vaddr,mem_info->paddr,mem_info->size,name,mem_info->name);
            type_idx++;
        }
    }   
    pthread_mutex_unlock(&mem_ctrl->mutex);
    isfw_log_dprint("==========================stack_mng_mem_stat_prt end==========================\n");
    return;
}

