/**@file     custom_yolov8_custom_0_layer.h
 * @note     2012-2019 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 * @brief    Define interface and struct of yolov8_custom_0 layer
 * 
 * @author   
 * @date     
 * @version  V1.5.0
 * 
 * @note     
 * @warning  
 */
#ifndef _CUSTOM_YOLOV8_CUSTOM_0_LAYER_H_
#define _CUSTOM_YOLOV8_CUSTOM_0_LAYER_H_

#include <float.h>
#include <string.h>
#include <stdio.h>
#include "custom_callback.h"

#ifdef __cplusplus
extern "C" {
#endif

// the definition of OPDEVSDK_YOLOV8_CUSTOM_0_MODEL_T
typedef struct _OPDEVSDK_YOLOV8_CUSTOM_0_MODEL_T_
{
    // hyperparams
    // params
    OPDEVSDK_HKA_BLOB_ST    dllp_conf_thresh_2_gathernd_1_init_0;
} OPDEVSDK_YOLOV8_CUSTOM_0_MODEL_T;

// the definition of OPDEVSDK_YOLOV8_CUSTOM_0_LAYER_T
typedef struct _OPDEVSDK_YOLOV8_CUSTOM_0_LAYER_T_
{
    // model handle
    OPDEVSDK_YOLOV8_CUSTOM_0_MODEL_T       *model;
    // scratch memory
    void *scratch_memory;
    int   scratch_memory_size;
} OPDEVSDK_YOLOV8_CUSTOM_0_LAYER_T;

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_reshape
*  @brief      compute the shape of the outblob of yolo layer
*  @param[i/o] void                       *handle   handle of yolo layer
               OPDEVSDK_HIKFLOW_LDATA_ST  *ld       informations of yolo layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_reshape(void                      *handle,
                            OPDEVSDK_HIKFLOW_LDATA_ST *ld);

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_GetModelMemsize
*  @brief      get the memory size of yolov8_custom_0 layer for model creation
*  @param[in]  const char                   *hyperparams hyperparams of yolov8_custom_0 layer
               const char                   *param_blobs param_blobs of yolov8_custom_0 layer
               OPDEVSDK_HIKFLOW_LMODEL_ST   *ld          informations of yolov8_custom_0 layer
*  @param[out] OPDEVSDK_HKA_MEM_TAB_ST       mem_tab     memory size and attribution needed for current layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_GetModelMemsize(const char                 *hyperparams,
                                    const char                 *param_blobs,
                                    OPDEVSDK_HIKFLOW_LMODEL_ST *ld,
                                    OPDEVSDK_HKA_MEM_TAB_ST     mem_tab[OPDEVSDK_HKA_MEM_TAB_NUM]);

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_CreateModel
*  @brief      create model handle of yolov8_custom_0 layer
*  @param[in]  const char                   *hyperparams  hyperparams of yolov8_custom_0 layer
               const char                   *param_blobs  param_blobs of yolov8_custom_0 layer
               OPDEVSDK_HIKFLOW_LMODEL_ST   *ld           informations  of yolov8_custom_0 layer
               OPDEVSDK_HKA_BUF_ST           mem_buf      memory resource
*  @param[out] void                        **handle       memory size and attribution needed for yolov8_custom_0 layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_CreateModel(const char                 *hyperparams,
                                const char                 *param_blobs,
                                OPDEVSDK_HIKFLOW_LMODEL_ST *ld,
                                OPDEVSDK_HKA_BUF_ST         mem_buf[OPDEVSDK_HKA_MEM_TAB_NUM],
                                void                      **handle);

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_GetMemsize
*  @brief      get the memory size of yolov8_custom_0 layer for handle creation
*  @param[in]  OPDEVSDK_HIKFLOW_LDATA_ST  *ld       informations of yolov8_custom_0 layer
*  @param[out] OPDEVSDK_HKA_MEM_TAB_ST     mem_tab  memory size and attribution needed for yolov8_custom_0 layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_GetMemsize(OPDEVSDK_HIKFLOW_LDATA_ST   *ld,
                               OPDEVSDK_HKA_MEM_TAB_ST      mem_tab[OPDEVSDK_HKA_MEM_TAB_NUM]);

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_Create
*  @brief      create layer handle of yolov8_custom_0 layer
*  @param[in]  OPDEVSDK_HIKFLOW_LDATA_ST   *ld       informations of yolov8_custom_0 layer
               OPDEVSDK_HKA_BUF_ST          mem_buf  memory resource
*  @param[out] void                       **handle   handle of yolov8_custom_0 layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_Create(OPDEVSDK_HIKFLOW_LDATA_ST *ld,
                           OPDEVSDK_HKA_BUF_ST        mem_buf[OPDEVSDK_HKA_MEM_TAB_NUM],
                           void                     **handle);

/**@fn         CUSTOM_YOLOV8_CUSTOM_0_Forward
*  @brief      do net forward of yolov8_custom_0 layer
*  @param[i/o] void                       *handle   handle of yolov8_custom_0 layer
               OPDEVSDK_HIKFLOW_LDATA_ST  *ld       informations of yolov8_custom_0 layer
*  @return     HRESULT
*/
int CUSTOM_YOLOV8_CUSTOM_0_Forward(void                      *handle,
                            OPDEVSDK_HIKFLOW_LDATA_ST *ld);

#ifdef __cplusplus
}
#endif

#endif
