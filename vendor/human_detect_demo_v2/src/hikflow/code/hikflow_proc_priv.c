/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         hikflow_proc_priv.c
* @brief        net processing main code
*
* @author       heoper
*
* @date         2023/8/30
* @version      2.0.0
* @note         add fuctions for hikflow
*****************************************************************************/
#include <syslog.h>
#include "hikflow_demo_priv.h"
#include "opdevsdk_hka_types.h"
#include "custom_callback.h"
#include "opdevsdk_hikflow_custom.h"

#define HIKFLOW_PROC_DBG(arg...)                 isfw_log_print("[HF_proc]",ISFW_LOG_LEVEL_DEBUG, __FILE__,__LINE__,##arg)

static int hikflow_proc_is_abnormal_class(HIKFLOW_DEMO_CTRL *pCtrl, int class_type)
{
    int i;
    if (pCtrl->hikflow_config.abnormal_class_count > 0)
    {
        for (i = 0; i < pCtrl->hikflow_config.abnormal_class_count; i++)
        {
            if (pCtrl->hikflow_config.abnormal_classes[i] == class_type)
            {
                return 1;
            }
        }
        return 0;
    }

    return (-1 == pCtrl->hikflow_config.sel_class || class_type == pCtrl->hikflow_config.sel_class);
}

/*!< allocate memory */
static int hikflow_proc_alloc_memory(OPDEVSDK_HKA_MEM_TAB_ST *mem_tab,char *name)
{
    int ret   = 0;
    void    *  u64PhyAddr = NULL;
    void    *ppVirAddr  = NULL;
    int align_size = 0;    
    size_t  align_off = 0;        

    /*!< it is necessary to align */
    HIKFLOW_KEY_RET(mem_tab->alignment <= 1,HIKFLOW_DEMO_ERR_FAILED,"mem_tab->alignment must be bigger than 1");   
    align_size = (int)mem_tab->alignment + mem_tab->size + 2 * sizeof(void *);

    switch (mem_tab->space)
    {
        /*!< cmm(memory with continuos physical address),no-cache*/
        case OPDEVSDK_HKA_MEM_SPACE_NOCACHED:
        case OPDEVSDK_HKA_MEM_SPACE_NOCACHED_PRIOPITY:
        case OPDEVSDK_HKA_MEM_SPACE_CACHED: 
        case OPDEVSDK_HKA_MEM_SPACE_CACHED_PRIOPITY: 
        case OPDEVSDK_HKA_MEM_SPACE_CPU:
        {
            STACK_MNG_MEM_INFO mem_info = {NULL,NULL,0,STACK_MNG_MEM_TYPE_CMM,0,STACK_MNG_MEM_PROC_TYPE_ALLOC,""};
            snprintf(mem_info.name,sizeof(mem_info.name),"%s",name);
            mem_info.size = align_size;
            int ret = stack_mng_mem_get(&mem_info);
            HIKFLOW_KEY_RET(ret != STACK_MNG_OK,HIKFLOW_DEMO_ERR_FAILED,"stack_mng_mem_get err");   
            
            ppVirAddr = mem_info.vaddr;
            u64PhyAddr = mem_info.paddr;   
       
            /*!< clear allocated memory */
            memset((void *)ppVirAddr, 0, align_size);

            if (mem_tab->alignment > 1)               
            {
                /*!< available virtual address */
                mem_tab->base = (void*)(((size_t)ppVirAddr + (size_t)mem_tab->alignment + 2 * sizeof(void *))
                    & (~((size_t)mem_tab->alignment - 1)));

                /*!< record virtual address  */
                ((void **)mem_tab->base)[-1] = (void *)ppVirAddr;

                /*!< record physical address */
                ((void **)mem_tab->base)[-2] = (void *)(size_t)u64PhyAddr;

                /*!< calculate offset of aligned physical address */
                align_off = (size_t)mem_tab->base - (size_t)ppVirAddr;
                mem_tab->phy_base = (void*)(size_t)(u64PhyAddr + align_off);
            }

            return HIKFLOW_DEMO_OK;
        }
        default:
        {
            return HIKFLOW_DEMO_ERR_FAILED;
        }
    }
    return HIKFLOW_DEMO_ERR_FAILED;
}

/*!< allocate memory */
static int hikflow_proc_alloc_memtab(OPDEVSDK_HKA_MEM_TAB_ST *mem_tab, int num,int *mem,char *name)
{
    int ret = HIKFLOW_DEMO_OK, i;
    HIKFLOW_KEY_RET(num <= 0,HIKFLOW_DEMO_ERR_FAILED,"mem_tab->alignment must be bigger than 1");   
    HIKFLOW_KEY_RET(mem_tab == NULL,HIKFLOW_DEMO_ERR_NULL_PTR,"mem_tab is NULL err");   

    int mem_used =0;
    
    /*!< allocate one mem_tab everytime */
    for (i = 0; i < num; i++)
    {
        /*!< allocate memory if size is not zero */
        if (mem_tab[i].size > 0)          
        {
            char tab_name[12]= {0};
            snprintf(tab_name,sizeof(tab_name),"%s_%d",name,i);
            ret = hikflow_proc_alloc_memory(&mem_tab[i],tab_name);
            mem_used+=mem_tab[i].size;
        }
        else                                             
        {
            mem_tab[i].phy_base = NULL;
            mem_tab[i].base = NULL;
        }
        HIKFLOW_DBG("tab: %d, memsize: %f , space %d.\n", i, mem_tab[i].size / 1024.0 / 1024.0, mem_tab[i].space);
    }

    if(mem)
    {
        *mem = mem_used;
    }
    return ret;
}

/*!< free memory */
static int hikflow_proc_free_memory(OPDEVSDK_HKA_MEM_TAB_ST *mem_tab)
{
    int    ret = 0;
    HIKFLOW_KEY_RET(mem_tab == NULL,HIKFLOW_DEMO_ERR_NULL_PTR,"mem_tab is NULL err");   
    HIKFLOW_KEY_RET(mem_tab->alignment == 0,HIKFLOW_DEMO_ERR_FAILED,"mem_tab->alignment must be not zero");   

    /*!< select free operation in terms of memory location */
    switch (mem_tab->space)
    {
        /*!< cmm(memory with continuos physical address),cache*/
        case OPDEVSDK_HKA_MEM_SPACE_CACHED:
        case OPDEVSDK_HKA_MEM_SPACE_NOCACHED:
        case OPDEVSDK_HKA_MEM_SPACE_CACHED_PRIOPITY:
        case OPDEVSDK_HKA_MEM_SPACE_NOCACHED_PRIOPITY:
        case OPDEVSDK_HKA_MEM_SPACE_CPU:
        {
            if (mem_tab->phy_base)
            {
                /*!< free aligned memory blocked */
                if (mem_tab->alignment > 1)                  
                {
                    
                    /*!< free buffer */
                    STACK_MNG_MEM_INFO mem_info = {NULL,NULL,HIKFLOW_DEMO_MAX_JSON_LEN,STACK_MNG_MEM_TYPE_CMM,0,STACK_MNG_MEM_PROC_TYPE_FREE,""};
                    mem_info.size = mem_tab->size;
                    mem_info.paddr = ((void **)mem_tab->base)[-2];
                    mem_info.vaddr = ((void **)mem_tab->base)[-1];
                    ret = stack_mng_mem_release(&mem_info);
                    HIKFLOW_ASSER(ret != STACK_MNG_OK,ret);  
                }
                if (ret != OPDEVSDK_S_OK)
                {
                    HIKFLOW_ERR("free_memory_vca failed!\n");
                }

                mem_tab->phy_base = NULL;
                mem_tab->base     = NULL;
                HIKFLOW_DBG("free_memory_vca OK!\n");
                return HIKFLOW_DEMO_OK;
            }
            return HIKFLOW_DEMO_ERR_FAILED;
        } 
        default:
        {
            return HIKFLOW_DEMO_ERR_FAILED;
        }
    }
}

/*!< free memory */
static void hikflow_proc_free_memtab(OPDEVSDK_HKA_MEM_TAB_ST *mem_tab, int num)
{
    HIKFLOW_NORET(num <= 0,HIKFLOW_DEMO_ERR_FAILED);   
    HIKFLOW_NORET(mem_tab == NULL,HIKFLOW_DEMO_ERR_NULL_PTR);   

    for (int i = 0; i < num; i++)
    {
        if (mem_tab[i].base || mem_tab[i].phy_base)
        {
            hikflow_proc_free_memory(&mem_tab[i]);
        }
    }
    return;
}

/*!< read model and get information */
static int hikflow_proc_update_model_info(HIKFLOW_DEMO_CONFIGURATION_ST *config_data, void *model_buffer, int model_size, int *flag_bin_info)
{
    FILE   *model_file     = NULL;
    char   *start_point    = NULL;
    char   *ptr_real       = NULL;
    char   dev_info[]      = "dev_info:";
    char   plat_type[]     = "plat_type:";
    char   dtype[]         = "dtype:";
    char   dformat[]       = "dformat:";
    char   innum[]         = "innum:";
    int    dev_info_bin    = 0;
    int    plat_type_bin   = 0;
    int    dtype_bin       = 0;
    int    dformat_bin     = 0;
    int    in_blob_num_bin = 0;
    int    batch_bin       = 0;
    int    channel_bin     = 0;
    int    height_bin      = 0;
    int    width_bin       = 0;

    HIKFLOW_KEY_RET(NULL == config_data,HIKFLOW_DEMO_ERR_NULL_PTR, "check config error: the config_data is null.");
    HIKFLOW_KEY_RET(NULL == model_buffer,HIKFLOW_DEMO_ERR_NULL_PTR, "check model_buffer error: the model_buffer is null.");

    start_point = (char *)model_buffer + model_size - HIKFLOW_DEMO_MODEL_INFOLEN;

    if (0 == strncmp(start_point, dev_info, strlen(dev_info)))
    {
        *flag_bin_info = 1;
        ptr_real = start_point + strlen(dev_info);
        dev_info_bin = *(int*)ptr_real;
        HIKFLOW_PROC_DBG("has get bin dev_info: %d\n", dev_info_bin);

        ptr_real = ptr_real + strlen(plat_type) + sizeof(int);
        plat_type_bin = *(int*)ptr_real;
        HIKFLOW_PROC_DBG("has get bin plat_type: %d\n", plat_type_bin);

        ptr_real = ptr_real + strlen(dtype) + sizeof(int);
        dtype_bin = *(int*)ptr_real;
        HIKFLOW_PROC_DBG("has get bin dtype: %d\n", dtype_bin);

        ptr_real = ptr_real + strlen(dformat) + sizeof(int);
        dformat_bin = *(int*)ptr_real;
        HIKFLOW_PROC_DBG("has get bin dformat: %d\n", dformat_bin);

        ptr_real = ptr_real + strlen(innum) + sizeof(int);
        in_blob_num_bin = *(int*)ptr_real;
        HIKFLOW_PROC_DBG("has get bin in_blob_num: %d\n", in_blob_num_bin);

        /*!< almost 8 blobs, 32 ints */
        for (int i = 0; i < in_blob_num_bin; ++i)
        {
            ptr_real = ptr_real + sizeof(int);
            batch_bin = *(int*)ptr_real;
            HIKFLOW_PROC_DBG("has get bin batch: %d\n", batch_bin);

            ptr_real = ptr_real + sizeof(int);
            channel_bin = *(int*)ptr_real;
            HIKFLOW_PROC_DBG("has get bin channel: %d\n", channel_bin);

            ptr_real = ptr_real + sizeof(int);
            height_bin = *(int*)ptr_real;
            HIKFLOW_PROC_DBG("has get bin height: %d\n", height_bin);

            ptr_real = ptr_real + sizeof(int);
            width_bin = *(int*)ptr_real;
            HIKFLOW_PROC_DBG("has get bin width: %d\n", width_bin);
        }
    }

    /*!< if bin carry info */
    if (*flag_bin_info == 1)
    {
        config_data->net_input.core_proc_type = plat_type_bin;
        config_data->net_input.data_type = dformat_bin;
        config_data->net_input.batch_cnt = batch_bin;
        config_data->net_input.channel_cnt = channel_bin;
        config_data->net_input.height = height_bin;
        config_data->net_input.width = width_bin;
    } 
    else
    {
        /*!< BIN must carry info */ 
        HIKFLOW_ERR("err: %s doesn't carry information. please check your BIN.\n", config_data->model_path);
        return HIKFLOW_DEMO_ERR_FAILED;
    }

    return HIKFLOW_DEMO_OK;
}

/*!< release model memory */
static int hikflow_proc_deinit_net_info(HIKFLOW_DEMO_NET_INFO_ST * net_info)
{
    HIKFLOW_RET((NULL == net_info),HIKFLOW_DEMO_ERR_NULL_PTR);
    int ret = 0;

    /*!< release model buffer */
    if(net_info->model_buffer != NULL)
    {
        /*!< free model buffer */
        STACK_MNG_MEM_INFO mem_info = {NULL,NULL,0,STACK_MNG_MEM_TYPE_CMM,0,STACK_MNG_MEM_PROC_TYPE_FREE,""};
        mem_info.size = STACK_MNG_SIZE_ALIGN(net_info->model_size, STACK_MNG_PAGE_ALIGN);;
        mem_info.paddr = (void *)(size_t)net_info->model_phy_base;
        mem_info.vaddr = net_info->model_buffer;
        ret = stack_mng_mem_release(&mem_info);
        HIKFLOW_ASSER(ret != STACK_MNG_OK,ret);  

        net_info->model_buffer = NULL;
        net_info->model_phy_base = 0;
        net_info->model_size = 0;
        net_info->model_file_used = 0;
    }

    HIKFLOW_LOG("hikflow_proc_deinit_net_info ok\n");
    return HIKFLOW_DEMO_OK;
}

/*!< load hikflow model to buffer and update net param */
static int hikflow_proc_init_net_info(HIKFLOW_DEMO_NET_INFO_ST * net_info, HIKFLOW_DEMO_CONFIGURATION_ST *config_data)
{
    FILE   *model_file   = NULL;
    int    ret           = 0;
    int    flag_bin_info = 0;
    
    HIKFLOW_KEY_RET((NULL == config_data),HIKFLOW_DEMO_ERR_NULL_PTR,"config_data is null");
    HIKFLOW_KEY_RET((NULL == net_info),HIKFLOW_DEMO_ERR_NULL_PTR, "net_info is null");

    /*!< first step: read model file to buffer */
    model_file = fopen(config_data->model_path, "rb");
    HIKFLOW_LOG("open model_file(%s)\n",config_data->model_path);
    HIKFLOW_KEY_RET((NULL == model_file),HIKFLOW_DEMO_ERR_FAILED, "model_file open err");

    fseek(model_file, 0, SEEK_END);
    net_info->model_size = (unsigned int)ftell(model_file);
    rewind(model_file);
    HIKFLOW_KEY_EXIT((net_info->model_size <= 0),HIKFLOW_DEMO_ERR_FAILED,err0, "model_size<=0");

    int buf_size = STACK_MNG_SIZE_ALIGN(net_info->model_size, STACK_MNG_PAGE_ALIGN);

    STACK_MNG_MEM_INFO mem_info = {NULL,NULL,0,STACK_MNG_MEM_TYPE_CMM,0,STACK_MNG_MEM_PROC_TYPE_ALLOC,"model_cache"};
    mem_info.size = buf_size;
    ret = stack_mng_mem_get(&mem_info);
    HIKFLOW_KEY_EXIT((ret != STACK_MNG_OK),ret,err0, "stack_mng_mem_get err");

    net_info->model_buffer = mem_info.vaddr;
    net_info->model_phy_base = (size_t)mem_info.paddr;       
    net_info->model_file_used += buf_size;

    fread(net_info->model_buffer, 1, net_info->model_size, model_file);

    /*!< second step: read model and get information,such as :net width,net height */
    ret = hikflow_proc_update_model_info(config_data,net_info->model_buffer,net_info->model_size, &flag_bin_info);
    HIKFLOW_KEY_EXIT((HIKFLOW_DEMO_OK != ret ),ret,err1, "hikflow_proc_check_plat_config err");

    if (NULL != model_file)
    {
        fclose(model_file);
        model_file = NULL;
    }

    /*!< update to net_info */
    memcpy(&net_info->net_input,&config_data->net_input,sizeof(HIKFLOW_DEMO_NET_IN_ST));
    HIKFLOW_LOG("hikflow_proc_init_net_info ok\n");
    return HIKFLOW_DEMO_OK;
    
err1:
    mem_info.proc_type = STACK_MNG_MEM_PROC_TYPE_FREE;
    stack_mng_mem_release(&mem_info);
    net_info->model_file_used = 0;
err0:
    if (NULL != model_file)
    {
        fclose(model_file);
        model_file = NULL;
    }
    
    HIKFLOW_ERR("hikflow_proc_init_net_info failed\n");
    return HIKFLOW_DEMO_ERR_FAILED;
}

/*!< destroy model handle */
static int hikflow_proc_destroy_model_handle(HIKFLOW_DEMO_NET_INFO_ST * net_info)
{
    HIKFLOW_RET((NULL == net_info),HIKFLOW_DEMO_ERR_NULL_PTR);
    int ret = 0;
     
    /*!< first step :release model handle */
    if(NULL != net_info->model_handle)
    {
        ret =opdevsdk_hikflow_ReleaseModel(net_info->model_handle);
        net_info->model_handle = NULL;
    }

    /*!< second step :release mem_tab alloced by hikflow_proc_alloc_memtab */
    hikflow_proc_free_memtab(net_info->model_tab, OPDEVSDK_HKA_MEM_TAB_NUM);
    net_info->model_mem_used  = 0;
    HIKFLOW_LOG("hikflow_proc_destroy_model ok\n");
    return HIKFLOW_DEMO_OK;
}

/*!< creat model handle */
static int hikflow_proc_create_model_handle(HIKFLOW_DEMO_NET_INFO_ST * net_info)
{
    int ret = 0,mem_used = 0;
    OPDEVSDK_HKA_MODEL_INFO_ST   params_info;
    OPDEVSDK_HKA_MEM_TAB_ST      hikflow_mem_tab_model[OPDEVSDK_HKA_MEM_TAB_NUM];
    OPDEVSDK_HKA_MEM_TAB_ST      hikflow_mem_tab[OPDEVSDK_HKA_MEM_TAB_NUM];
    
    HIKFLOW_KEY_RET((NULL == net_info),HIKFLOW_DEMO_ERR_NULL_PTR, "net_info is null");

    /*!< first step: get memory: must give the model buff and private layer to hikflow */
    params_info.model_buf   = net_info->model_buffer;
    params_info.model_size  = net_info->model_size;
    params_info.proc_type   = net_info->net_input.core_proc_type;
    
    /*!< register function of private layer */
    params_info.custom_cb.get_model_memsize = (void*)Custom_Layer_GetModelMemsize;
    params_info.custom_cb.create_model      = (void*)Custom_Layer_CreateModel;
    params_info.custom_cb.get_memsize       = (void*)Custom_Layer_GetMemsize;
    params_info.custom_cb.create            = (void*)Custom_Layer_Create;
    params_info.custom_cb.reshape           = (void*)Custom_Layer_Reshape;
    params_info.custom_cb.forward           = (void*)Custom_Layer_Forward;

    /*!< get memory size */
    ret = opdevsdk_hikflow_GetModelMemSize(&params_info, net_info->model_tab);
    HIKFLOW_KEY_RET((0 != ret ),ret, "opdevsdk_hikflow_GetModelMemSize err");

    char name[8]="model";
    /*!< second step: allocate memory for model */
    ret = hikflow_proc_alloc_memtab(net_info->model_tab, OPDEVSDK_HKA_MEM_TAB_NUM,&mem_used,name);
    HIKFLOW_KEY_RET((0 != ret ),ret, "opdevsdk_hikflow_Device_Alloc_Memtab err");
    net_info->model_mem_used += mem_used;

    /*!< third step: creat model handle */
    ret = opdevsdk_hikflow_CreateModel(&params_info, net_info->model_tab, &net_info->model_handle);    
    HIKFLOW_KEY_EXIT((0 != ret || NULL == net_info->model_handle),ret,err0, "opdevsdk_hikflow_CreateModel err");

    HIKFLOW_LOG("hikflow_proc_create_model (%p) ok\n",net_info->model_handle);
    return HIKFLOW_DEMO_OK;
err0:
    /*!< free memory */
    hikflow_proc_free_memtab(net_info->model_tab, OPDEVSDK_HKA_MEM_TAB_NUM);
    net_info->model_mem_used  = 0;
    HIKFLOW_ERR("hikflow_proc_create_model failed\n");
    return HIKFLOW_DEMO_ERR_FAILED;
}

/*!< update blob type */
static int hikflow_proc_update_net_info(HIKFLOW_DEMO_NET_INFO_ST  *net_info)
{
    HIKFLOW_RET((NULL == net_info),HIKFLOW_DEMO_ERR_NULL_PTR);
    OPDEVSDK_HIKFLOW_PARAM_ST *param_info_net = &net_info->hf_info;

    param_info_net->in_blob_num   = 1;
    if (1 == net_info->net_input.data_type)
    {
        param_info_net->in_blob_param[0].src_format     = OPDEVSDK_HKA_BGR;  /*!< BGR */
    }
    else if(2 == net_info->net_input.data_type)
    {
        param_info_net->in_blob_param[0].src_format     = OPDEVSDK_HKA_YVU420; /*!< NV21 */
    }
	else if(4 == net_info->net_input.data_type)   
	{        
		param_info_net->in_blob_param[0].src_format     = OPDEVSDK_HKA_YUV420; /*!< NV12 */    
	}
    else
    {
        HIKFLOW_ERR(" 'data_type' just support test_data_type(BGR) or test_data_type(YVU420). Please check.\n");
        param_info_net->in_blob_param[0].src_format     = OPDEVSDK_HKA_YUV420; 
        return HIKFLOW_DEMO_ERR_FAILED;
    }
    
    HIKFLOW_LOG("hikflow_proc_update_net_info  in_blob_param[0].src_format %d OK\n",param_info_net->in_blob_param[0].src_format);
    return HIKFLOW_DEMO_OK;
}

/*!< destroy hikflow net handle */
static int hikflow_proc_destroy_net_handle(HIKFLOW_DEMO_NET_INFO_ST * net_info)
{
    HIKFLOW_RET((NULL == net_info),HIKFLOW_DEMO_ERR_NULL_PTR);
    int ret = 0;
     
    /*!< first step :release net handle */
    if(NULL != net_info->net_handle)
    {
        ret =opdevsdk_hikflow_Release(net_info->net_handle);
        net_info->net_handle = NULL;
    }

    /*!< second step :release mem_tab alloced by hikflow_proc_alloc_memtab */
    hikflow_proc_free_memtab(net_info->mem_tab, OPDEVSDK_HKA_MEM_TAB_NUM);
    net_info->net_mem_used  = 0;
    
    HIKFLOW_LOG("hikflow_proc_destroy_net_handle ok\n");
    return OPDEVSDK_S_OK;
}

/*!< creat hikflow net handle */
static int hikflow_proc_create_net_handle(HIKFLOW_DEMO_NET_INFO_ST  *net_info)
{      
    HIKFLOW_RET((NULL == net_info),HIKFLOW_DEMO_ERR_NULL_PTR);

    int    ret = 0,mem_used = 0;;
    OPDEVSDK_HIKFLOW_PARAM_ST *param_info_net = &net_info->hf_info;

    /*!< update blob type */
    ret = hikflow_proc_update_net_info(net_info);
    HIKFLOW_ASSER((HIKFLOW_DEMO_OK != ret),ret);

    /*!< net handle memory size will be calcaluted with image info,model info  */
   	param_info_net->in_blob_param[0].src_blob.space     = OPDEVSDK_HKA_MEM_SPACE_CACHED;
    param_info_net->in_blob_param[0].src_blob.dim       = 4;
    param_info_net->in_blob_param[0].src_blob.type      = OPDEVSDK_HKA_DATA_U08;
    param_info_net->in_blob_param[0].src_blob.shape[0]  = net_info->net_input.batch_cnt;
    param_info_net->in_blob_param[0].src_blob.shape[1]  = net_info->net_input.channel_cnt;
    param_info_net->in_blob_param[0].src_blob.shape[2]  = net_info->net_input.height;
    param_info_net->in_blob_param[0].src_blob.shape[3]  = net_info->net_input.width;
	param_info_net->in_blob_param[0].src_blob.stride[3] = net_info->net_input.width * sizeof(unsigned char);
    param_info_net->in_blob_param[0].src_blob.stride[2] = net_info->net_input.height;

    param_info_net->handle_num        = 1;
    param_info_net->handle[0].type    = OPDEVSDK_HIKFLOW_MODEL_HANDLE;
    param_info_net->handle[0].handle  = net_info->model_handle;

    /*!< all the blobs in last layer are output */
    param_info_net->out_blob_num                  = 1;
    param_info_net->out_blob_info[0].layer_idx    = -1;
    param_info_net->out_blob_info[0].blob_idx     = 0;

    /*!< first step: get memory */
    ret = opdevsdk_hikflow_GetMemSize(param_info_net, net_info->mem_tab);
    HIKFLOW_KEY_RET((0 != ret ),ret, "opdevsdk_hikflow_GetModelMemSize err");

    /*!< second step: allocate memory */
    char name[8]="net";
    ret = hikflow_proc_alloc_memtab(net_info->mem_tab, OPDEVSDK_HKA_MEM_TAB_NUM,&mem_used,name);
    HIKFLOW_KEY_RET((0 != ret),ret, "opdevsdk_hikflow_Device_Alloc_Memtab err");
    net_info->net_mem_used += mem_used;

    /*!< third step: create network handle */
    ret = opdevsdk_hikflow_Create(param_info_net, net_info->mem_tab, &net_info->net_handle);
    HIKFLOW_KEY_EXIT((0 != ret || NULL == net_info->net_handle),ret,err0, "opdevsdk_hikflow_Create err");

    HIKFLOW_LOG("hikflow_proc_createHandle (%p) ok\n",net_info->net_handle);
    return OPDEVSDK_S_OK;    
err0:
    hikflow_proc_free_memtab(net_info->mem_tab, OPDEVSDK_HKA_MEM_TAB_NUM);
    net_info->net_mem_used  = 0;
    HIKFLOW_ERR("hikflow_proc_create_net_handle err\n");
    return HIKFLOW_DEMO_ERR_FAILED;
}

/*!< algorithm process */
static int hikflow_proc_net(HIKFLOW_DEMO_NET_INFO_ST  *net_info,
                            OPDEVSDK_HIKFLOW_FORWARD_OUT_INFO_ST    *hkann_out,
                            void                                    *pfrm,
                            unsigned int                             num)
{
    int    ret            = 0;
    float  im_info[3]     = {0};  
    OPDEVSDK_HIKFLOW_FORWARD_IN_INFO_ST     hkann_in = {0};

    /*!< check the input point */
    HIKFLOW_KEY_RET((NULL == net_info),HIKFLOW_DEMO_ERR_NULL_PTR, "hikflow_proc_Process: config_data is null.");
    HIKFLOW_KEY_RET((NULL == net_info->net_handle),HIKFLOW_DEMO_ERR_NULL_PTR, "hikflow_proc_Process: net_handle is null.");
    HIKFLOW_KEY_RET((NULL == pfrm),HIKFLOW_DEMO_ERR_NULL_PTR, "hikflow_proc_Process: pfrm is null.");
    HIKFLOW_KEY_RET((NULL == hkann_out),HIKFLOW_DEMO_ERR_NULL_PTR, "hikflow_proc_Process: hkann_out is null.");
    HIKFLOW_KEY_RET((0 == num),HIKFLOW_DEMO_ERR_NULL_PTR, "hikflow_proc_Process: num = 0.");

    /*!< forward module do not support image scale, so set im_info[2] = 1.0 */
    im_info[0] = net_info->net_input.height * 1.0f;
    im_info[1] = net_info->net_input.width * 1.0f;
    im_info[2] = 1.0;
    
    /*!< configuration of input params */
    hkann_in.in_blob_num                = 1;
    hkann_in.in_blob[0].src_format      = net_info->hf_info.in_blob_param[0].src_format;
    hkann_in.in_blob[0].src_blob.space  = net_info->hf_info.in_blob_param[0].src_blob.space;
    hkann_in.in_blob[0].src_blob.format = OPDEVSDK_HKA_FORMAT_NCHW;
    hkann_in.in_blob[0].src_blob.type   = OPDEVSDK_HKA_DATA_U08;

    hkann_in.in_blob[0].src_blob.dim       = net_info->hf_info.in_blob_param[0].src_blob.dim;
    hkann_in.in_blob[0].src_blob.data      = pfrm;
    hkann_in.in_blob[0].src_blob.shape[0]  = num;
    hkann_in.in_blob[0].src_blob.shape[1]  = net_info->net_input.channel_cnt;
    hkann_in.in_blob[0].src_blob.shape[2]  = net_info->net_input.height;
    hkann_in.in_blob[0].src_blob.shape[3]  = net_info->net_input.width;
	hkann_in.in_blob[0].src_blob.stride[3] = net_info->net_input.width * sizeof(unsigned char);
    hkann_in.in_blob[0].src_blob.stride[2] = net_info->net_input.height;
    
    ret = opdevsdk_hikflow_Process(net_info->net_handle, 0,&hkann_in, sizeof(hkann_in), hkann_out, sizeof(*hkann_out));
    HIKFLOW_KEY_RET((0 != ret),ret, "opdevsdk_hikflow_Process err");
    return HIKFLOW_DEMO_OK;
}

/** 
* @brief            hikflow net initialization funtion
*
* @param[in] 		pCtrl   handel     
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_proc_init(HIKFLOW_DEMO_CTRL* pCtrl)
{
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);
    
    /*!< load hikflow model to buffer and update net param */
    int ret = hikflow_proc_init_net_info(&pCtrl->net_info,&pCtrl->hikflow_config);
    HIKFLOW_KEY_RET((HIKFLOW_DEMO_OK != ret),ret, "hikflow_proc_init_net_info err");
    stack_mng_push(&pCtrl->net_info,hikflow_proc_deinit_net_info);
    
    /*!< creat model handle and will give the handle to net */
    ret = hikflow_proc_create_model_handle(&pCtrl->net_info);
    HIKFLOW_KEY_RET((HIKFLOW_DEMO_OK != ret),ret, "hikflow_proc_create_model err");
    stack_mng_push(&pCtrl->net_info,hikflow_proc_destroy_model_handle);

    /*!< free model buffer afer used */
    ret = hikflow_proc_deinit_net_info(&pCtrl->net_info);
    HIKFLOW_ASSER((HIKFLOW_DEMO_OK != ret),ret);

    /*!< creat hikflow net handle */
    ret = hikflow_proc_create_net_handle(&pCtrl->net_info);
    HIKFLOW_KEY_RET((HIKFLOW_DEMO_OK != ret),ret,  "hikflow_proc_create_net_handle err");
    stack_mng_push(&pCtrl->net_info,hikflow_proc_destroy_net_handle);
    return HIKFLOW_DEMO_OK;
}

/** 
* @brief            net processing function in camera input mode  
*
* @param[in] 		pCtrl       handel     
* @param[in] 		pfrm        image frame information     
* @param[out] 		ptarget     returned target information   
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_proc_alg_from_cam(HIKFLOW_DEMO_CTRL* pCtrl,OPDEVSDK_VIDEO_FRAME_INFO_ST *pfrm,OPDEVSDK_POS_TARGET_LIST_INFO_ST *ptarget)
{
    HIKFLOW_RET(NULL == pfrm,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == ptarget,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(NULL == pCtrl,HIKFLOW_DEMO_ERR_NULL_PTR);

    int ret = 0;
    int detectnum = 0;
    HIKFLOW_DEMO_RULE rule = {0};
	int in_rule = 0,j = 0;
	
    HIKFLOW_DEMO_BOX_INFO_ST *box_info  = NULL;
    OPDEVSDK_HIKFLOW_FORWARD_OUT_INFO_ST     hkann_out = {0};
    memset(&hkann_out, 0, sizeof(OPDEVSDK_HIKFLOW_FORWARD_OUT_INFO_ST));

    ret = hikflow_proc_net(&pCtrl->net_info, &hkann_out, (void*)(size_t)pfrm->yuvFrame.pVirAddr[0],1);
    HIKFLOW_KEY_RET((HIKFLOW_DEMO_OK != ret),ret, "hikflow_proc_net error");
    
    /*!< detection output */
    //HIKFLOW_DBG("----------------------------------------------------------------\n");
    //HIKFLOW_DBG("this is detection output \n    the out format is class,score,x,y,width,height,batch_index\n");
    //HIKFLOW_DBG("if the model is not detection,or the out format is different,\n    please change it \n");
    //HIKFLOW_DBG("----------------------------------------------------------------\n");
    HIKFLOW_LOG("----------------------------------------------------------------\n");
    HIKFLOW_LOG("this is detection output \n    the out format is class,score,x,y,width,height,batch_index\n");
    HIKFLOW_LOG("if the model is not detection,or the out format is different,\n    please change it \n");
    HIKFLOW_LOG("----------------------------------------------------------------\n");
    detectnum = hkann_out.output_blob[0].shape[0];
    // HIKFLOW_ERR("detectnum = %d\n", detectnum);
    syslog(LOG_LOCAL2 | LOG_NOTICE,
    "detectnum = %d",
    detectnum);
    int idx = 0;
    for(int n = 0; n < detectnum; n++)
    {
		in_rule = 0;
        box_info = (HIKFLOW_DEMO_BOX_INFO_ST*)hkann_out.output_blob[0].data;
        box_info = box_info + n;
        HIKFLOW_DBG("current bounding box: %d\n", n);
        HIKFLOW_DBG("classs = %f\n", box_info->class_type);
        HIKFLOW_DBG("score  = %f\n", box_info->score);
        HIKFLOW_DBG("x      = %f\n", box_info->bbox.x);
        HIKFLOW_DBG("y      = %f\n", box_info->bbox.y);
        HIKFLOW_DBG("w      = %f\n", box_info->bbox.width);
        HIKFLOW_DBG("h      = %f\n", box_info->bbox.height);
        HIKFLOW_DBG("batch  = %f\n", box_info->batch_idx);

        float x = 0, y = 0;
        /*!< if abnormal_classes is configured, only those classes are treated as abnormal; otherwise keep sel_class behavior */
        if(idx < HIKFLOW_DEMO_MAX_OUTPUT_BOX_NUM &&
            hikflow_proc_is_abnormal_class(pCtrl, (int)box_info->class_type))
        {
			OPDEVSDK_POS_REGION_ST tmp_ptgt = {0};
			tmp_ptgt.pointNum = 4;

			pthread_mutex_lock(&pCtrl->mutex);
			memcpy(&rule,&pCtrl->rule,sizeof(HIKFLOW_DEMO_RULE));
			pthread_mutex_unlock(&pCtrl->mutex);

			/*!< normalization algorithm outputs the coordinates of the target */
            x = box_info->bbox.x / pCtrl->net_info.net_input.width;
            y = box_info->bbox.y / pCtrl->net_info.net_input.height;
            tmp_ptgt.point[0].x = x > 1 ? 1 : x;
            tmp_ptgt.point[0].y = y > 1 ? 1 : y;
			
            x = (box_info->bbox.x + box_info->bbox.width) / pCtrl->net_info.net_input.width;
            y = box_info->bbox.y / pCtrl->net_info.net_input.height;
            tmp_ptgt.point[1].x = x > 1 ? 1 : x;
            tmp_ptgt.point[1].y = y > 1 ? 1 : y;

            x = (box_info->bbox.x + box_info->bbox.width) / pCtrl->net_info.net_input.width;
            y = (box_info->bbox.y + box_info->bbox.height) / pCtrl->net_info.net_input.height;
            tmp_ptgt.point[2].x = x > 1 ? 1 : x;
            tmp_ptgt.point[2].y = y > 1 ? 1 : y;

            x = box_info->bbox.x / pCtrl->net_info.net_input.width;
            y = (box_info->bbox.y + box_info->bbox.height) / pCtrl->net_info.net_input.height;
            tmp_ptgt.point[3].x = x > 1 ? 1 : x;
            tmp_ptgt.point[3].y = y > 1 ? 1 : y;

			x = (tmp_ptgt.point[0].x + tmp_ptgt.point[1].x)/2;
			y = (tmp_ptgt.point[0].y + tmp_ptgt.point[3].y)/2;

			/*!< check whether the coordinates are within the currently set rule area, if they are within the area, in_rule = 1 */
			int k = rule.point_num - 1; 
			for(j = 0; j < rule.point_num; j++)
			{
				if(((rule.point[j].y < y && rule.point[k].y>=y) || (rule.point[k].y<y && rule.point[j].y>=y))
						&&(rule.point[j].x<=x || rule.point[k].x<=x)) 
					   {
						   in_rule^=(rule.point[j].x +
									 (y-rule.point[j].y)/(rule.point[k].y-rule.point[j].y)*
									 (rule.point[k].x-rule.point[j].x) < x);
					   }
					   k = j;
 			}

			/*!< save the information of targets within the current rule area */
 			if(in_rule == 1)
			{
	            ptarget->tgtList.pTgt[idx].region.pointNum = 4;
	            ptarget->tgtList.pTgt[idx].id = idx+1;
	            ptarget->tgtList.pTgt[idx].trace_time = 0;
	            ptarget->tgtList.pTgt[idx].color = 0;
	            ptarget->tgtList.pTgt[idx].res[0] = (int)box_info->class_type;
 				memcpy(&ptarget->tgtList.pTgt[idx].region, &tmp_ptgt, sizeof(OPDEVSDK_POS_REGION_ST));				
	            HIKFLOW_DBG("n %d alg_w %d alg_h %d x %f y %f w %f h %f\n", 
	                    idx, pCtrl->net_info.net_input.width, pCtrl->net_info.net_input.height, ptarget->tgtList.pTgt[idx].region.point[0].x,\
	                    ptarget->tgtList.pTgt[idx].region.point[1].x, ptarget->tgtList.pTgt[idx].region.point[1].y, \
	                    ptarget->tgtList.pTgt[idx].region.point[3].y);
				idx++;
			}
        }
    }
    ptarget->tgtList.tgtNum = idx;
    HIKFLOW_DBG("hikflow detectnum %d filter %d \n",detectnum,ptarget->tgtList.tgtNum);

    ptarget->timeStamp = pfrm->timeStamp;
    return HIKFLOW_DEMO_OK;
}

/** 
* @brief            net processing function in file-reading mode
*
* @param[in] 		pCtrl       handel     
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_proc_alg_from_file(HIKFLOW_DEMO_CTRL* pCtrl)
{
    int ret     = 0;
    unsigned long long  total   = 0;
    FILE  *file       = NULL;
    void  *img_data   = NULL;
    char  file_name[256]   = { '\0' };
    int   total_pic_num    = 0;  
    int   data_size        = 0;
    FILE  *pic_file        = NULL;

    long    one_time = 0;
    int     i = 0, j = 0, k = 0;
    struct timeval time_begin, time_end;
        
    unsigned long long phy_base   = 0; 
    unsigned char      *data_temp = NULL;   

    char res_file_name[256] = { 0 };

    HIKFLOW_DEMO_BOX_INFO_ST *box_info  = NULL;
    OPDEVSDK_HIKFLOW_FORWARD_OUT_INFO_ST     hkann_out;
    memset(&hkann_out, 0, sizeof(hkann_out));

    HIKFLOW_LOG("fopen %s start\n", pCtrl->hikflow_config.image_list);

    /*!< open test images */
    file = fopen(pCtrl->hikflow_config.image_list, "rb");
    HIKFLOW_RET(NULL == file,HIKFLOW_DEMO_ERR_NULL_PTR);

    while (fgets(file_name, 256, file) != NULL)
    {
        total_pic_num++;
    }
    
    fseek(file, 0, SEEK_SET);
    HIKFLOW_LOG("total_pic_num:%d\n", total_pic_num);

    /*!< allocate input images memory for bgr */
    if (pCtrl->net_info.hf_info.in_blob_param[0].src_format == OPDEVSDK_HKA_BGR)
    {
        data_size = pCtrl->net_info.net_input.batch_cnt * pCtrl->net_info.net_input.width * pCtrl->net_info.net_input.height*  pCtrl->net_info.net_input.channel_cnt;
    }
    else /*!< allocate input images memory for yuv */ 
    {
        data_size = pCtrl->net_info.net_input.batch_cnt * pCtrl->net_info.net_input.width * pCtrl->net_info.net_input.height  * 3 / 2;
    }

    STACK_MNG_MEM_INFO mem_info = {NULL,NULL,data_size,STACK_MNG_MEM_TYPE_CMM,0,STACK_MNG_MEM_PROC_TYPE_ALLOC,"file_input"};
    mem_info.size = data_size;
    ret = stack_mng_mem_get(&mem_info);
    HIKFLOW_KEY_EXIT(ret != STACK_MNG_OK,ret,err0,"stack_mng_mem_get err");

    img_data = mem_info.vaddr;
    phy_base = (size_t)mem_info.paddr;   

    for (j = 0; j < total_pic_num; j += pCtrl->net_info.net_input.batch_cnt)
    {
        data_temp = (unsigned char *)img_data;

        for (k = 0; k < pCtrl->net_info.net_input.batch_cnt; k++)
        {
            if ((j + k) >= total_pic_num)
            {
                HIKFLOW_LOG("all pic has process.\n");
                break;
            }
            
            fscanf(file, "%s", file_name);
            file_name[strlen(file_name) + 1] = '\0';
            
            HIKFLOW_LOG("process pic : %s\n",file_name);
                
            pic_file = fopen(file_name, "rb");
            HIKFLOW_KEY_EXIT(NULL == pic_file,ret,err1,"fopen pic_file err");
            fseek(pic_file, 0, SEEK_SET);

            /*!< read all bgr images */
            if (pCtrl->net_info.hf_info.in_blob_param[0].src_format == OPDEVSDK_HKA_BGR)
            {
                data_size = pCtrl->net_info.net_input.width * pCtrl->net_info.net_input.height*  pCtrl->net_info.net_input.channel_cnt;
            }
            else/*!< read nv21 images */
            {
                data_size = pCtrl->net_info.net_input.width * pCtrl->net_info.net_input.height  * 3 / 2;
            }
            
            fread(data_temp, data_size, 1, pic_file);
            data_temp += data_size;

            if (NULL != pic_file)
            {
                fclose(pic_file);
                pic_file = NULL;
            }
        }

        /*!< send data to hikflow */
        gettimeofday(&time_begin, NULL);
        ret = hikflow_proc_net(&pCtrl->net_info,&hkann_out, img_data,k);
        gettimeofday(&time_end, NULL);
        one_time = 1000000 * (time_end.tv_sec - time_begin.tv_sec) + (time_end.tv_usec - time_begin.tv_usec);
        total += one_time;
        HIKFLOW_KEY_EXIT((ret != 0),ret,err1,"hikflow_proc_Process err");
        
        /*!< save reslt */
        float score = 0;
        FILE *result_fp      = NULL;
        for(int m = 0; m < hkann_out.blob_num; m++)
        {
            ret = snprintf(res_file_name,256,"./user_data/picture_%d_blob_%d_out.txt",j,m);	
            HIKFLOW_LOG("open res_file_name %s  \n",res_file_name);
            result_fp = fopen(res_file_name, "w+");
            HIKFLOW_KEY_EXIT((result_fp == NULL),ret,err1,"fopen rslt file err");
            
            HIKFLOW_LOG("----------------------------------------------------------------\n");
            HIKFLOW_LOG("this is general output \n");
            HIKFLOW_LOG("you can get result in %s\n",res_file_name);
            HIKFLOW_LOG("----------------------------------------------------------------\n");

            int n = hkann_out.output_blob[m].shape[0] * 
                    hkann_out.output_blob[m].shape[1] * 
                    hkann_out.output_blob[m].shape[2] * 
                    hkann_out.output_blob[m].shape[3];

            HIKFLOW_LOG("blob_num = %d,shape[0] = %d,shape[1] =%d,shape[2] =%d,shape[3] =%d\n",hkann_out.blob_num,
                                        hkann_out.output_blob[m].shape[0],
                                        hkann_out.output_blob[m].shape[1],
                                        hkann_out.output_blob[m].shape[2],
                                        hkann_out.output_blob[m].shape[3]);
            int max_score_index = 0;
            float max_score = 0;
            for (int c = 0; c < n; c++)
            {
                score = ((float *)(hkann_out.output_blob[m].data))[c]; 
                fprintf(result_fp, "%f\n", score);
                if (max_score < score){
                    max_score = score;
                    max_score_index = c;
                }
            }
            
            HIKFLOW_LOG("max score: %f, max score index: %d\r\n", max_score, max_score_index);
        
            /*!< detection output */
            HIKFLOW_LOG("----------------------------------------------------------------\n");
            HIKFLOW_LOG("this is detection output \n    the out format is class,score,x,y,width,height,batch_index\n");
            HIKFLOW_LOG("if the model is not detection,or the out format is different,\n    please change it \n");
            HIKFLOW_LOG("----------------------------------------------------------------\n");

            int detectnum = hkann_out.output_blob[m].shape[0];
            for(i = 0; i < detectnum; i++)
            {
                box_info = (HIKFLOW_DEMO_BOX_INFO_ST *)hkann_out.output_blob[m].data;
                box_info = box_info + i;
                
                HIKFLOW_LOG("current bounding box: %d\n", i);
                HIKFLOW_LOG("classs  = %f\n", box_info->class_type);
                HIKFLOW_LOG("score   = %f\n", box_info->score);
                HIKFLOW_LOG("x       = %f\n", box_info->bbox.x);
                HIKFLOW_LOG("y       = %f\n", box_info->bbox.y);
                HIKFLOW_LOG("w       = %f\n", box_info->bbox.width);
                HIKFLOW_LOG("h       = %f\n", box_info->bbox.height);
                HIKFLOW_LOG("batch   = %f\n", box_info->batch_idx);
            }
            HIKFLOW_LOG("----------------------------------------------------------------\n");	 
        }
        
        if (NULL != result_fp)
        {
            fclose(result_fp);
            result_fp = NULL;
        }	
    }
    
    HIKFLOW_LOG("Process avg time:%llu us\n", total / total_pic_num);

    if (NULL != img_data)
    {
        mem_info.proc_type = STACK_MNG_MEM_PROC_TYPE_FREE;
        ret = stack_mng_mem_release(&mem_info);
        img_data = NULL;
    }
    
    HIKFLOW_LOG("hikflow_proc_alg_from_file done...\n");
    return HIKFLOW_DEMO_OK;
    
err1:
    if (NULL != img_data)
    {
        mem_info.proc_type = STACK_MNG_MEM_PROC_TYPE_FREE;
        ret = stack_mng_mem_release(&mem_info);
        img_data = NULL;
    }
err0:
    if (NULL != file)
    {
        fclose(file);
        file = NULL;
    }    
    HIKFLOW_ERR("hikflow_proc_alg_from_file err...\n");
    return HIKFLOW_DEMO_ERR_FAILED;
}

