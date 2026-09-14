#ifndef __YOLOV8_CUSTOM_0_SUB_1_H__
#define __YOLOV8_CUSTOM_0_SUB_1_H__

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// forward function
// Attention, all element of validshape should be initialized, 
//            this function will not touch the non-valid dim of validshape
// dllp_iou_thresh_threshed_data_1's dim is 2
int yolov8_custom_0_sub_1_Forward(float* dllp_conf_thresh_threshed_data, int32_t x_100, float* dllp_iou_thresh_threshed_data_1, int32_t * dllp_iou_thresh_threshed_data_2_shape, void* __auxi_mem_addr__);


// reshape function
// Attention, all element of maxshape should be initialized, 
//            this function will not touch the non-valid dim of maxshape
int yolov8_custom_0_sub_1_Reshape(int32_t x_100, int32_t * dllp_iou_thresh_threshed_data_2_maxshape, int32_t* auxi_mem_size);


#ifdef __cplusplus
}
#endif
#endif