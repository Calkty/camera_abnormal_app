/**@file     custom_yolov8_custom_0_sub_1_layer.c
 * @note     2012-2019 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 * @brief    Implementation of yolov8_custom_0_sub_1 layer
 * 
 * @author   
 * @date     
 * @version  V1.5.0
 * 
 * @note     
 * @warning  
 */
#include <float.h>
#include <string.h>
#include <stdio.h>
#include "custom_yolov8_custom_0_sub_1_layer.h"
#include "yolov8_custom_0_sub_1.h"

#define CUSTOM_YOLOV8_CUSTOM_0_SUB_1_MEM_ALIGN_SIZE          128
#define CUSTOM_YOLOV8_CUSTOM_0_SUB_1_ALIGN(size)             (((size) + ((128) - 1)) & (~((128) - 1)))
#define OPC_SIZE_ALIGN(size, align)             ((((int)size) + ((align)-1)) & (~((align)-1)))

//static int dim1[4] = {3,3,3,3};
static int dim2[4] = {0,1,2,3};
//static int dim3[4] = {1,2,3,3};

static int dim1[4] = {0,0,0,0};
//static int dim2[4] = {0,1,1,1};
static int dim3[4] = {0,1,2,2};
static int dim4[4] = {0,1,2,3};

void*   OPC_RUNTIME_set_mem_addr(void* addr);

/**@fn         CUSTOM_alloc_buffer
*  @brief      alloc memory buffer
*  @param[in]  OPDEVSDK_HKA_BUF_ST           *ivs_buf    memory buffer to be allocated
               unsigned int                   size
               OPDEVSDK_HKA_MEM_ALIGNMENT_EN  alignment  informations  of current layer
               unsigned int                   clean      memory size and attribution needed for current layer
*  @return     HRESULT
*/
static void  *CUSTOM_alloc_buffer(OPDEVSDK_HKA_BUF_ST          *ivs_buf,
                                  unsigned int                  size,
                                  OPDEVSDK_HKA_MEM_ALIGNMENT_EN alignment,
                                  unsigned int                  clean)
{
    int              align = (int)alignment;
    void            *buf   = NULL;
    unsigned int     free_size = 0;
    buf = (void *)(((size_t)ivs_buf->cur_pos + (align - 1)) & (~(align - 1)));
    ///< Calculate the starting address
    if ((size_t)ivs_buf->end < (size_t)buf)
    {
        buf = NULL;
        return buf;
    }
    ///< Calculate the remaining memory buffer size
    free_size = (char *)ivs_buf->end - (char *)buf;
    ///< Insufficient memory space, return null pointer
    if (free_size < size)
    {
        buf = NULL;
    }
    else
    {
        ///< Clean up allocated memory
        if (clean)
        {
            memset(buf, 0, size);
        }

        ///< Update the free memory address
        ivs_buf->cur_pos = (void *)((char *)buf + size);
    }
    return buf;
}
/**@fn         CUSTOM_YOLOV8_CUSTOM_0_SUB_1_reshape
*  @brief      compute the shape of the outblob of YOLOV8_CUSTOM_0_SUB_1 layer
*  @param[i/o] void                       *handle   handle of YOLOV8_CUSTOM_0_SUB_1 layer
               OPDEVSDK_HIKFLOW_LDATA_ST  *ld       informations of YOLOV8_CUSTOM_0_SUB_1 layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_SUB_1_reshape(void                      *handle,
                            OPDEVSDK_HIKFLOW_LDATA_ST *ld)
{
    int sts         = HIKFLOW_STS_OK;
    OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_LAYER_T  *yolov8_custom_0_sub_1_layer = (OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_LAYER_T *)handle;

    ///< define the output of reshape function
    int dllp_iou_thresh_threshed_data_2_maxshape[4] = {0};
    ///< define the memory of reshape function
    int auxi_mem_size = 0;
    ///< define the shape of reshape function
    int x_100 = ld->input_blobs[0]->shape[dim1[0]];
    int C1 = ld->input_blobs[0]->shape[dim1[1]];
    int H1 = ld->input_blobs[0]->shape[dim1[2]];
    int W1 = ld->input_blobs[0]->shape[dim1[3]];
    ///< call reshape function
    sts = yolov8_custom_0_sub_1_Reshape( x_100, dllp_iou_thresh_threshed_data_2_maxshape, &auxi_mem_size );
    CHECK_ERROR(HIKFLOW_STS_OK != sts, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_reshape yolov8_custom_0_sub_1_Reshape failed!", HIKFLOW_STS_ERR_CUS_LAYER_RESHAPE_FAILED);
    ///< set the output blob
    int j = 0;

    for (j = 0;j < 4;j++)
    {
        if (dllp_iou_thresh_threshed_data_2_maxshape[j] == 0)
        {
           dllp_iou_thresh_threshed_data_2_maxshape[j] = 1;
        }
    }
    ld->output_blobs[0].dim      = 4;
    ld->output_blobs[0].type     = OPDEVSDK_HKA_DATA_F32; ///< custom layer type is float32
    ld->output_blobs[0].format   = OPDEVSDK_HKA_FORMAT_NCHW;
    ld->output_blobs[0].shape[3] = dllp_iou_thresh_threshed_data_2_maxshape[dim4[3]];
    ld->output_blobs[0].shape[2] = dllp_iou_thresh_threshed_data_2_maxshape[dim4[2]];
    ld->output_blobs[0].shape[1] = dllp_iou_thresh_threshed_data_2_maxshape[dim4[1]];
    ld->output_blobs[0].shape[0] = dllp_iou_thresh_threshed_data_2_maxshape[dim4[0]];
    ld->output_blobs[0].stride[0]= ld->output_blobs[0].shape[0]; 
    ld->output_blobs[0].stride[1]= ld->output_blobs[0].shape[1]; 
    ld->output_blobs[0].stride[2]= ld->output_blobs[0].shape[2]; 
    ld->output_blobs[0].stride[3]= ld->output_blobs[0].shape[3]; 

    ALG_LOG_INFO("ld->output_blobs\ndim:%d type:%d format:%d\nshpae[0]:%d shpae[1]:%d shpae[2]:%d shpae[3]:%d stride:%d\n",
             ld->output_blobs[0].dim, ld->output_blobs[0].type, ld->output_blobs[0].format,
             ld->output_blobs[0].shape[0], ld->output_blobs[0].shape[1],
             ld->output_blobs[0].shape[2], ld->output_blobs[0].shape[3],
             ld->output_blobs[0].stride[3]);
    ///< set the memory size
    yolov8_custom_0_sub_1_layer->scratch_memory_size = auxi_mem_size;
    return HIKFLOW_STS_OK;
}

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_SUB_1_init_model
*  @brief      get initialization of YOLOV8_CUSTOM_0_SUB_1 model params
*  @param[in]  const char                   *hyperparams    hyperparams of YOLOV8_CUSTOM_0_SUB_1 layer
               const char                   *param_blobs    param_blobs of YOLOV8_CUSTOM_0_SUB_1 layer
               OPDEVSDK_HIKFLOW_LMODEL_ST   *ld             informations of YOLOV8_CUSTOM_0_SUB_1 layer
               int                          init_blob       initial of blob
               OPDEVSDK_HKA_BUF_ST          *malloc_buf     malloc buf

*  @param[out] OPDEVSDK_YLOUT_MODEL_T       *yolov8_custom_0_sub_1_model yolov8_custom_0_sub_1_model model inforamtion
*  @return     HRESULT
*/
static int CUSTOM_YOLOV8_CUSTOM_0_SUB_1_init_model(const char                         *hyperparams,
                                      const char                         *param_blobs,
                                      OPDEVSDK_HIKFLOW_LMODEL_ST         *ld,
                                      OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_MODEL_T          *yolov8_custom_0_sub_1_model,
                                      int                                 init_blob,
                                      OPDEVSDK_HKA_BUF_ST                *malloc_buf)
{
    int sts         = HIKFLOW_STS_OK;
    int r           = 0;
    int blob_size   = 0;
    int malloc_size = 0;
    int i           = 0;
    return HIKFLOW_STS_OK;
}

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetModelMemsize
*  @brief      get the memory size of yolov8_custom_0_sub_1 layer for model creation
*  @param[in]  const char                   *hyperparams hyperparams of yolov8_custom_0_sub_1 layer
               const char                   *param_blobs param_blobs of yolov8_custom_0_sub_1 layer
               OPDEVSDK_HIKFLOW_LMODEL_ST   *ld          informations of yolov8_custom_0_sub_1 layer
*  @param[out] OPDEVSDK_HKA_MEM_TAB_ST       mem_tab     memory size and attribution needed for current layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetModelMemsize(const char                 *hyperparams,
                                    const char                 *param_blobs,
                                    OPDEVSDK_HIKFLOW_LMODEL_ST *ld,
                                    OPDEVSDK_HKA_MEM_TAB_ST     mem_tab[OPDEVSDK_HKA_MEM_TAB_NUM])
{
    int sts                 = HIKFLOW_STS_OK;
    int malloc_size         = 0;
    OPDEVSDK_HKA_MEM_TAB_ST *malloc_handle_tab = mem_tab;
    OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_MODEL_T yolov8_custom_0_sub_1_model = {0};
    CHECK_ERROR(NULL == ld,          "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetModelMemsize error:ld is null\n",
                HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    CHECK_ERROR(NULL == mem_tab,     "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetModelMemsize error: NULL = mem_tab\n",
                HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    memset(mem_tab, 0, sizeof(OPDEVSDK_HKA_MEM_TAB_ST) * OPDEVSDK_HKA_MEM_TAB_NUM);
    ///< calculate the memory size of model handle
    malloc_size = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_ALIGN(sizeof(OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_MODEL_T));
    ///< call init_model function，get the information of shape
    sts = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_init_model(hyperparams, param_blobs, ld, &yolov8_custom_0_sub_1_model, 0, NULL);
    CHECK_ERROR(HIKFLOW_STS_OK != sts, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetModelMemsize error: Init Model Failed!\n", sts);
    ///< set the memory tab
    malloc_handle_tab->size = malloc_size;
    malloc_handle_tab->alignment = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_MEM_ALIGN_SIZE;
    malloc_handle_tab->attrs = OPDEVSDK_HKA_MEM_PERSIST;
    malloc_handle_tab->space = OPDEVSDK_HKA_MEM_SPACE_CPU;

    return HIKFLOW_STS_OK;
}

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_SUB_1_CreateModel
*  @brief      create model handle of yolov8_custom_0_sub_1 layer
*  @param[in]  const char                   *hyperparams  hyperparams of yolov8_custom_0_sub_1 layer
               const char                   *param_blobs  param_blobs of yolov8_custom_0_sub_1 layer
               OPDEVSDK_HIKFLOW_LMODEL_ST   *ld           informations  of yolov8_custom_0_sub_1 layer
               OPDEVSDK_HKA_BUF_ST           mem_buf      memory resource
*  @param[out] void                        **handle       memory size and attribution needed for yolov8_custom_0_sub_1 layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_SUB_1_CreateModel(const char                  *hyperparams,
                                const char                  *param_blobs,
                                OPDEVSDK_HIKFLOW_LMODEL_ST  *ld,
                                OPDEVSDK_HKA_BUF_ST          mem_buf[OPDEVSDK_HKA_MEM_TAB_NUM],
                                void                       **handle)
{
    int sts         = HIKFLOW_STS_OK;
    int malloc_size = 0;
    OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_MODEL_T *yolov8_custom_0_sub_1_model = NULL;
    OPDEVSDK_HKA_BUF_ST    *malloc_buf = NULL;
    CHECK_ERROR(NULL == handle, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_CreateModel error: NULL = handle\n",
                HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    CHECK_ERROR(NULL == mem_buf, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_CreateModel error: NULL = mem_buf\n",
                HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    malloc_buf = &mem_buf[0];
    ///< set npu information
    ld->npu_enum_val = OPDEVSDK_HIKFLOW_NPU_TYPE_0;
    ///< set the memory
    malloc_size = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_ALIGN(sizeof(OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_MODEL_T));
    yolov8_custom_0_sub_1_model = (OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_MODEL_T *)CUSTOM_alloc_buffer(malloc_buf,
                                                                      malloc_size,
                                                                      CUSTOM_YOLOV8_CUSTOM_0_SUB_1_MEM_ALIGN_SIZE,
                                                                      1);
    ///< call init_model function to initial the params and hyperparams
    sts = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_init_model(hyperparams, param_blobs, ld, yolov8_custom_0_sub_1_model, 1, malloc_buf);
    CHECK_ERROR(HIKFLOW_STS_OK != sts, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetModelMemsize error: Init Model Failed!\n", sts);
    *handle = yolov8_custom_0_sub_1_model;
    return 0;
}

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetMemsize
*  @brief      get the memory size of yolov8_custom_0_sub_1 layer for handle creation
*  @param[in]  OPDEVSDK_HIKFLOW_LDATA_ST  *ld       informations of yolov8_custom_0_sub_1 layer
*  @param[out] OPDEVSDK_HKA_MEM_TAB_ST     mem_tab  memory size and attribution needed for yolov8_custom_0_sub_1 layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetMemsize(OPDEVSDK_HIKFLOW_LDATA_ST   *ld,
                               OPDEVSDK_HKA_MEM_TAB_ST      mem_tab[OPDEVSDK_HKA_MEM_TAB_NUM])
{
    int sts              = HIKFLOW_STS_OK;
    int temp_malloc_size = 0;
    int malloc_size      = 0;
    OPDEVSDK_HKA_MEM_TAB_ST         *malloc_tab             = NULL;
    ///< assist mem tab
    OPDEVSDK_HKA_MEM_TAB_ST         *aux_tab             = NULL;
    OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_LAYER_T   yolov8_custom_0_sub_1_layer    = { 0 };
    ///< import param check
    CHECK_ERROR(NULL == ld, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetMemsize error: NULL = ld\n",
                HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    CHECK_ERROR(NULL == mem_tab, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetMemsize error: NULL = mem_tab\n",
                HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    memset(mem_tab, 0, sizeof(mem_tab[0]) * OPDEVSDK_HKA_MEM_TAB_NUM);
    malloc_tab   = &mem_tab[0];
    aux_tab      = &mem_tab[1];
    yolov8_custom_0_sub_1_layer.model = ld->layer_model->model_handle;
    ///< call reshape function to calculate the size of params blob
    sts = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_reshape(&yolov8_custom_0_sub_1_layer, ld);
    CHECK_ERROR(0 != sts,"CUSTOM_YOLOV8_CUSTOM_0_SUB_1_reshape error!", sts);
    ///< calculate the memory size which layer handle need
    temp_malloc_size = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_ALIGN(sizeof(OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_LAYER_T));
    malloc_size     += temp_malloc_size;
    ///<  calculate the memory size which output blob need
    temp_malloc_size = ld->output_blobs[0].shape[0] *
                       ld->output_blobs[0].shape[1] *
                       ld->output_blobs[0].shape[2] *
                       ld->output_blobs[0].shape[3] *
                       sizeof(float);
    temp_malloc_size = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_ALIGN(temp_malloc_size);
    malloc_size     += temp_malloc_size;
    ///< calculate the scratch memory size
    temp_malloc_size = yolov8_custom_0_sub_1_layer.scratch_memory_size;
    temp_malloc_size = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_ALIGN(temp_malloc_size);
    ///< set aux memory table
    aux_tab->size      = temp_malloc_size;
    aux_tab->alignment = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_MEM_ALIGN_SIZE;
    aux_tab->attrs     = OPDEVSDK_HKA_MEM_SCRATCH;
    aux_tab->space     = OPDEVSDK_HKA_MEM_SPACE_CPU;
    ///< set memory table
    malloc_tab->size      = malloc_size;
    malloc_tab->alignment = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_MEM_ALIGN_SIZE;
    malloc_tab->attrs     = OPDEVSDK_HKA_MEM_PERSIST;
    malloc_tab->space     = OPDEVSDK_HKA_MEM_SPACE_CPU;

    return HIKFLOW_STS_OK;
}

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Create
*  @brief      create layer handle of yolov8_custom_0_sub_1 layer
*  @param[in]  OPDEVSDK_HIKFLOW_LDATA_ST   *ld       informations of yolov8_custom_0_sub_1 layer
               OPDEVSDK_HKA_BUF_ST          mem_buf  memory resource
*  @param[out] void                       **handle   handle of yolov8_custom_0_sub_1 layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Create(OPDEVSDK_HIKFLOW_LDATA_ST *ld,
                           OPDEVSDK_HKA_BUF_ST        mem_buf[OPDEVSDK_HKA_MEM_TAB_NUM],
                           void                     **handle)
{
    int sts              = HIKFLOW_STS_OK;
    int temp_malloc_size = 0;
    int malloc_size      = 0;

    OPDEVSDK_HKA_BUF_ST        *malloc_buf     = NULL;
    OPDEVSDK_HKA_BUF_ST        *aux_buf     = NULL;
    OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_LAYER_T  *yolov8_custom_0_sub_1_layer = NULL;
    CHECK_ERROR(NULL == ld, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Create error: NULL = ld\n",
                HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    CHECK_ERROR(NULL == mem_buf, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Create error: NULL = mem_buf\n",
                HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    CHECK_ERROR(NULL == handle, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Create error: NULL = handle\n",
                HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    malloc_buf   = &mem_buf[0];
    aux_buf      = &mem_buf[1];
    ///< allocate mem for layer handle
    malloc_size = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_ALIGN(sizeof(OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_LAYER_T));
    yolov8_custom_0_sub_1_layer = (OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_LAYER_T*) CUSTOM_alloc_buffer(malloc_buf,
                                                                      malloc_size,
                                                                      CUSTOM_YOLOV8_CUSTOM_0_SUB_1_MEM_ALIGN_SIZE,
                                                                      1);
    CHECK_ERROR(NULL == yolov8_custom_0_sub_1_layer, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Create handle CUSTOM_alloc_buffer failed!", HIKFLOW_STS_ERR_CUS_LAYER_ALLOC_ERROR);
    yolov8_custom_0_sub_1_layer->model = ld->layer_model->model_handle;
    ///< call reshape function to calculate the size of params blob
    sts = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_reshape(yolov8_custom_0_sub_1_layer, ld);
    CHECK_ERROR(0 != sts, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_reshape error!", sts);
    ///< the output blob memory

    temp_malloc_size = ld->output_blobs[0].shape[0] *
                       ld->output_blobs[0].shape[1] *
                       ld->output_blobs[0].shape[2] *
                       ld->output_blobs[0].shape[3] *
                       sizeof(float);
    malloc_size = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_ALIGN(temp_malloc_size);
    ld->output_blobs[0].data = CUSTOM_alloc_buffer(malloc_buf,
                                                              malloc_size,
                                                              CUSTOM_YOLOV8_CUSTOM_0_SUB_1_MEM_ALIGN_SIZE,
                                                              1);
    CHECK_ERROR(NULL == ld->output_blobs[0].data, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Create output_blobs CUSTOM_alloc_buffer failed!", HIKFLOW_STS_ERR_CUS_LAYER_ALLOC_ERROR);
    ALG_LOG_INFO("CUSTOM_MYLAYER_Create:ld->output_blobs\ndim:%d type:%d format:%d\nshpae[0]:%d shpae[1]:%d shpae[2]:%d shpae[3]:%d stride:%d\n",
             ld->output_blobs[0].dim, ld->output_blobs[0].type, ld->output_blobs[0].format,
             ld->output_blobs[0].shape[0], ld->output_blobs[0].shape[1],
             ld->output_blobs[0].shape[2], ld->output_blobs[0].shape[3],
             ld->output_blobs[0].stride[3]);
    yolov8_custom_0_sub_1_layer->scratch_memory = aux_buf->cur_pos;
    *handle = yolov8_custom_0_sub_1_layer;

    return HIKFLOW_STS_OK;
}

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Forward
*  @brief      do net forward of yolov8_custom_0_sub_1 layer
*  @param[i/o] void                       *handle   handle of yolov8_custom_0_sub_1 layer
               OPDEVSDK_HIKFLOW_LDATA_ST  *ld       informations of yolov8_custom_0_sub_1 layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Forward(void                      *handle,
                            OPDEVSDK_HIKFLOW_LDATA_ST *ld)
{
    int sts = HIKFLOW_STS_OK;
    void *__auxi_mem_addr__ = NULL;
 
    OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_LAYER_T  *yolov8_custom_0_sub_1_layer = (OPDEVSDK_YOLOV8_CUSTOM_0_SUB_1_LAYER_T *)handle;
    __auxi_mem_addr__ = yolov8_custom_0_sub_1_layer->scratch_memory;
    ///< define the input of forward function
    float *dllp_conf_thresh_threshed_data = (float*)ld->input_blobs[0]->data;
    ///< define the output of forward function
    float *dllp_iou_thresh_threshed_data = (float*)ld->output_blobs[0].data;

    ///< define the real shape of forward function
    int x_100 = ld->input_blobs[0]->shape[dim1[0]];
    int C1 = ld->input_blobs[0]->shape[dim1[1]];
    int H1 = ld->input_blobs[0]->shape[dim1[2]];
    int W1 = ld->input_blobs[0]->shape[dim1[3]];

    ///< define the out shape for forward function
    int dllp_iou_thresh_threshed_data_2_shape[4] = {0};
    CHECK_ERROR(NULL == handle, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Forward handle = null error!\n", HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    CHECK_ERROR(NULL == ld, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Forward ld = null error!\n", HIKFLOW_STS_ERR_CUS_LAYER_NULL_PTR);
    ///< call forward function
    sts = yolov8_custom_0_sub_1_Forward( dllp_conf_thresh_threshed_data, x_100, dllp_iou_thresh_threshed_data, dllp_iou_thresh_threshed_data_2_shape, __auxi_mem_addr__ );
    CHECK_ERROR(HIKFLOW_STS_OK != sts, "CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Forward yolov8_custom_0_sub_1_Forward failed!", HIKFLOW_STS_ERR_CUS_LAYER_FORWARD_FAILED);

  

    ///< set the output blob
    ld->output_blobs[0].shape[3] = dllp_iou_thresh_threshed_data_2_shape[dim4[3]];
    ld->output_blobs[0].shape[2] = dllp_iou_thresh_threshed_data_2_shape[dim4[2]];
    ld->output_blobs[0].shape[1] = dllp_iou_thresh_threshed_data_2_shape[dim4[1]];
    if (ld->output_blobs[0].shape[3] == 0)
        ld->output_blobs[0].shape[3] = 1;
    if (ld->output_blobs[0].shape[2] == 0)
        ld->output_blobs[0].shape[2] = 1;
    if (ld->output_blobs[0].shape[1] == 0)
        ld->output_blobs[0].shape[1] = 1;
    ld->output_blobs[0].shape[0] = dllp_iou_thresh_threshed_data_2_shape[dim4[0]];

    ld->output_blobs[0].stride[0]= (ld->output_blobs[0].shape[0]);
    ld->output_blobs[0].stride[1]= (ld->output_blobs[0].shape[1]);
    ld->output_blobs[0].stride[2]= (ld->output_blobs[0].shape[2]);
    ld->output_blobs[0].stride[3]= (ld->output_blobs[0].shape[3]);
    
    return 0;
}

