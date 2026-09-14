#include "dsl_runtime.h"
#include "yolov8_custom_0.h"
#include "yolov8_custom_0_sub_0.h"
#include "yolov8_custom_0_sub_1.h"

#define SUB_LEN 2
#define SHAPE_LEN 4

#ifdef __cplusplus
extern "C" {
#endif

DSL_DLL int yolov8_custom_0_Reshape(int32_t * dllp_iou_thresh_threshed_data_2_maxshape, int32_t * auxi_mem_size)
{
    int32_t sub_output_shape[SUB_LEN * SHAPE_LEN] = {0};
    int32_t sub_output_size[SUB_LEN] = {0};
    int32_t sub_auxi_mem_size[SUB_LEN] = {0};
    int32_t tmp = 1;

    yolov8_custom_0_sub_0_Reshape(&sub_output_shape[0 * SHAPE_LEN], &sub_auxi_mem_size[0]);
    yolov8_custom_0_sub_1_Reshape(sub_output_shape[0 * SHAPE_LEN + 0], &sub_output_shape[1 * SHAPE_LEN], &sub_auxi_mem_size[1]);

    *auxi_mem_size = 0;
    for (int m = 0; m < SUB_LEN; m++)
    {
        if (sub_auxi_mem_size[m] > *auxi_mem_size)
        {
            *auxi_mem_size = sub_auxi_mem_size[m];
        }

        tmp = 1;
        for (int n = 0; n < SHAPE_LEN; n++)
        {
            if (sub_output_shape[m * SHAPE_LEN + n] != 0)
            {
                tmp = tmp * sub_output_shape[m * SHAPE_LEN + n];
            }
        }
        sub_output_size[m] = tmp;
    }

    for (int m = 0; m < SUB_LEN - 1; m++)
    {
        *auxi_mem_size = *auxi_mem_size + sub_output_size[m] * sizeof(float);
    }

    for (int m = 0; m < SHAPE_LEN; m++)
    {
        dllp_iou_thresh_threshed_data_2_maxshape[m] = sub_output_shape[(SUB_LEN - 1) * SHAPE_LEN + m];
    }

    return 0;
}

#ifdef __cplusplus
}
#endif