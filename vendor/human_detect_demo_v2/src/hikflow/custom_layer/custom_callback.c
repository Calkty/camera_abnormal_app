/**@file     custom_callback.c
 * @note     2012-2019 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 * @brief    Defines and interfaces of custom layer
 * 
 * @author   
 * @date     
 * @version  V1.5.0
 * 
 * @note     
 * @warning  
 */
#include <string.h>
#include <stdio.h>
#include "custom_callback.h"

/**@fn         Custom_Layer_GetModelMemsize
*  @brief      get the memory size of custom layers for model creation
*  @param[in]  const char                   *hyperparams  hyperparams of current layer
               const char                   *param_blobs  param_blobs of current layer
               OPDEVSDK_HIKFLOW_LMODEL_ST   *ld           informations of current layer
*  @param[out] OPDEVSDK_HKA_MEM_TAB_ST       mem_tab      memory size and attribution needed for current layer
*  @return     HRESULT
*/
int Custom_Layer_GetModelMemsize(const char                 *hyperparams,
                                 const char                 *param_blobs,
                                 OPDEVSDK_HIKFLOW_LMODEL_ST *ld,
                                 OPDEVSDK_HKA_MEM_TAB_ST     mem_tab[OPDEVSDK_HKA_MEM_TAB_NUM])
{
    int hr = 0;
    ALG_LOG_INFO("Custom_Layer_GetModelMemsize %s\n", ld->type);
    
    if (0 == strcmp(ld->type, "yolov8_custom_0_sub_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_0_GetModelMemsize(hyperparams, param_blobs, ld, mem_tab);
    }
    else if (0 == strcmp(ld->type, "yolov8_custom_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_GetModelMemsize(hyperparams, param_blobs, ld, mem_tab);
    }
    else if (0 == strcmp(ld->type, "yolov8_custom_0_sub_1"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetModelMemsize(hyperparams, param_blobs, ld, mem_tab);
    }
    else
    {
        hr = -1;
		ALG_LOG_INFO("Custom_Layer_GetModelMemsize error!\n");
    }
    return hr;
}

/**@fn         Custom_Layer_CreateModel
*  @brief      create model handle for custom layers
*  @param[in]  const char                   *hyperparams  hyperparams of current layer
               const char                   *param_blobs  param_blobs of current layer
               OPDEVSDK_HIKFLOW_LMODEL_ST   *ld           informations of current layer
               OPDEVSDK_HKA_BUF_ST           mem_buf      memory resource
*  @param[out] void                        **handle       memory size and attribution needed for current layer
*  @return     HRESULT
*/
int Custom_Layer_CreateModel(const char                  *hyperparams,
                             const char                  *param_blobs,
                             OPDEVSDK_HIKFLOW_LMODEL_ST  *ld,
                             OPDEVSDK_HKA_BUF_ST          mem_buf[OPDEVSDK_HKA_MEM_TAB_NUM],
                             void                       **handle)
{
    int hr = 0;
    ALG_LOG_INFO("Custom_Layer_CreateModel %s\n", ld->type);

    if (0 == strcmp(ld->type, "yolov8_custom_0_sub_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_0_CreateModel(hyperparams, param_blobs, ld, mem_buf, handle);
    }
    else if (0 == strcmp(ld->type, "yolov8_custom_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_CreateModel(hyperparams, param_blobs, ld, mem_buf, handle);
    }
    else if (0 == strcmp(ld->type, "yolov8_custom_0_sub_1"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_CreateModel(hyperparams, param_blobs, ld, mem_buf, handle);
    }
    else
    {
        hr = -1;
		ALG_LOG_INFO("Custom_Layer_CreateModel error!\n");
    }
    return hr;
}

/**@fn         Custom_Layer_GetMemsize
*  @brief      get the memory size of current layer for handle creation
*  @param[in]  OPDEVSDK_HIKFLOW_LDATA_ST   *ld       informations of current layer
*  @param[out] OPDEVSDK_HKA_MEM_TAB_ST      mem_tab  memory size and attribution needed for current layer
*  @return     HRESULT
*/
int Custom_Layer_GetMemsize(OPDEVSDK_HIKFLOW_LDATA_ST  *ld,
                            OPDEVSDK_HKA_MEM_TAB_ST     mem_tab[OPDEVSDK_HKA_MEM_TAB_NUM])
{
    int hr = 0;
    ALG_LOG_INFO("Custom_Layer_GetMemsize %s\n", ld->layer_model->type);
    
    if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0_sub_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_0_GetMemsize(ld, mem_tab);
    }
    else if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_GetMemsize(ld, mem_tab);
    }
    else if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0_sub_1"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_GetMemsize(ld, mem_tab);
    }
    else
    {
        hr = -1;
		ALG_LOG_INFO("Custom_Layer_GetMemsize error!\n");
    }
    return hr;
}

/**@fn         Custom_Layer_Create
*  @brief      create layer handle of current layer
*  @param[in]  OPDEVSDK_HIKFLOW_LDATA_ST   *ld       informations of current layer
               OPDEVSDK_HKA_BUF_ST          mem_buf  memory resource
*  @param[out] void                       **handle   handle of current layer
*  @return     HRESULT
*/
int Custom_Layer_Create(OPDEVSDK_HIKFLOW_LDATA_ST  *ld,
                        OPDEVSDK_HKA_BUF_ST         mem_buf[OPDEVSDK_HKA_MEM_TAB_NUM],
                        void                      **handle)
{
    int hr = 0;
    ALG_LOG_INFO("Custom_Layer_Create %s\n", ld->layer_model->type);

    if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0_sub_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_0_Create(ld, mem_buf, handle);
    }
    else if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_Create(ld, mem_buf, handle);
    }
    else if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0_sub_1"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Create(ld, mem_buf, handle);
    }
    else
    {
        hr = -1;
		ALG_LOG_INFO("Custom_Layer_Create error!\n");
    }
    return hr;
}

/**@fn         Custom_Layer_reshape
*  @brief      compute the shape of the tops of current layer
*  @param[i/o] void                       *handle    handle of current layer
               OPDEVSDK_HIKFLOW_LDATA_ST  *ld        informations of current layer
*  @return     HRESULT
*/
int Custom_Layer_Reshape(void                       *handle,
                         OPDEVSDK_HIKFLOW_LDATA_ST  *ld)
{
    int hr = 0;
    ALG_LOG_INFO("Custom_Layer_Reshape %s\n", ld->layer_model->type);

    if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0_sub_0"))
    {
        // compute the shape of the out blobs.
		hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_0_reshape(handle, ld);
    }
    else if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0"))
    {
        // compute the shape of the out blobs.
		hr = CUSTOM_YOLOV8_CUSTOM_0_reshape(handle, ld);
    }
    else if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0_sub_1"))
    {
        // compute the shape of the out blobs.
		hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_reshape(handle, ld);
    }
    else
    {
        hr = -1;
		ALG_LOG_INFO("Custom_Layer_Reshape error!\n");
    }
    return hr;
}

/**@fn         Custom_Layer_Forward
*  @brief      do net forward of current layer
*  @param[i/o] void                       *handle    handle of current layer
               OPDEVSDK_HIKFLOW_LDATA_ST  *ld        informations of current layer
*  @return     HRESULT
*/
int Custom_Layer_Forward(void                       *handle,
                         OPDEVSDK_HIKFLOW_LDATA_ST  *ld)
{
    int hr = 0;
    ALG_LOG_INFO("Custom_Layer_Forward %s\n", ld->layer_model->type);
    
    if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0_sub_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_0_Forward(handle, ld);
    }
    else if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_Forward(handle, ld);
    }
    else if (0 == strcmp(ld->layer_model->type, "yolov8_custom_0_sub_1"))
    {
        hr = CUSTOM_YOLOV8_CUSTOM_0_SUB_1_Forward(handle, ld);
    }
    else
    {
        hr = -1;
		ALG_LOG_INFO("Custom_Layer_Forward error!\n");
    }
    return hr;
}