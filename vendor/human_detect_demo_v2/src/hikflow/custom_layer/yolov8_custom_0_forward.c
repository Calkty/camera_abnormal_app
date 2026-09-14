#include "dsl_runtime.h"
#include "yolov8_custom_0.h"
#include "yolov8_custom_0_sub_0.h"
#include "yolov8_custom_0_sub_1.h"

#define SUB_LEN 2
#define SHAPE_LEN 4

#ifdef __cplusplus
extern "C" {
#endif

DSL_DLL int yolov8_custom_0_Forward(float * dllp_conf_thresh_1_obj_data, float * num_454_3_transpose, float * num_457_1_transpose, float * dllp_conf_thresh_2_gathernd_1_init_0, float * dllp_iou_thresh_threshed_data, int32_t * dllp_iou_thresh_threshed_data_2_shape, void * __auxi_mem_addr__)
{
    int32_t sub_output_shape[SUB_LEN * SHAPE_LEN] = {0};
    int32_t sub_output_size[SUB_LEN] = {0};
    int32_t sub_auxi_mem_size[SUB_LEN] = {0};
    int32_t tmp = 1;

    yolov8_custom_0_sub_0_Reshape(&sub_output_shape[0 * SHAPE_LEN], &sub_auxi_mem_size[0]);
    yolov8_custom_0_sub_1_Reshape(sub_output_shape[0 * SHAPE_LEN + 0], &sub_output_shape[1 * SHAPE_LEN], &sub_auxi_mem_size[1]);

    for (int m = 0; m < SUB_LEN; m++)
    {
        tmp = 1;
        for (int n = 0; n < SHAPE_LEN; n++)
        {
            if (sub_output_shape[m * SHAPE_LEN + n] != 0)
            {
                tmp = tmp * sub_output_shape[m * SHAPE_LEN + n];
                sub_output_shape[m * SHAPE_LEN + n] = 0;
            }
        }
        sub_output_size[m] = tmp;
    }

    float *dllp_conf_thresh_threshed_data = __auxi_mem_addr__;
    memset(dllp_conf_thresh_threshed_data, 0, sub_output_size[0] * sizeof(float));
    void *__real_auxi_mem_addr__ = dllp_conf_thresh_threshed_data + sub_output_size[0];

    memset(__real_auxi_mem_addr__, 0, sub_auxi_mem_size[0]);
    yolov8_custom_0_sub_0_Forward(dllp_conf_thresh_1_obj_data, num_454_3_transpose, num_457_1_transpose, dllp_conf_thresh_2_gathernd_1_init_0, dllp_conf_thresh_threshed_data, &sub_output_shape[0 * SHAPE_LEN], __real_auxi_mem_addr__);
    memset(__real_auxi_mem_addr__, 0, sub_auxi_mem_size[1]);
    yolov8_custom_0_sub_1_Forward(dllp_conf_thresh_threshed_data, sub_output_shape[0 * SHAPE_LEN + 0], dllp_iou_thresh_threshed_data, &sub_output_shape[1 * SHAPE_LEN], __real_auxi_mem_addr__);

    for (int m = 0; m < SHAPE_LEN; m++)
    {
        dllp_iou_thresh_threshed_data_2_shape[m] = sub_output_shape[(SUB_LEN - 1) * SHAPE_LEN + m];
    }

    return 0;
}

#ifdef __cplusplus
}
#endif