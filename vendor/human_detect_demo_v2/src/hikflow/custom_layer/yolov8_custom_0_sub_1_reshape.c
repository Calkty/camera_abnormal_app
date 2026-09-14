#include "dsl_runtime.h"
#include "yolov8_custom_0_sub_1.h"

#ifdef __cplusplus
extern "C" {
#endif

void yolov8_custom_0_sub_1_get_memsize(int32_t x_100, int32_t* auxi_mem_size)
{
    DSL_DummyMemBuf total_buf = {0};
    int32_t cse_var_6 = (x_100) * (80);
    int32_t cse_var_5 = (x_100) * (512);
    int32_t cse_var_4 = (x_100) * (4);
    int32_t cse_var_3 = (x_100) * (2);
    int32_t cse_var_2 = OPC_MIN(cse_var_3, (x_100) + (1000));
    int32_t cse_var_1 = (cse_var_2) * (3);
    DSL_DummyPtr __tmp_75 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (1));
    DSL_DummyPtr __tmp_76 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (1));
    DSL_DummyPtr __tmp_77 = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (1));
    DSL_DummyPtr __tmp_78 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (1));
    DSL_DummyPtr __tmp_79 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (4));
    DSL_DummyPtr __tmp_80 = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (1));
    DSL_DummyPtr __tmp_81 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (1));
    DSL_DummyPtr __tmp_82 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (1));
    DSL_DummyPtr __tmp_83 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (1));
    DSL_DummyPtr dllp_iou_thresh_half_wh = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (cse_var_3));
    DSL_DummyPtr dllp_iou_thresh_xy2 = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (cse_var_3));
    DSL_DummyPtr dllp_iou_thresh_xyxy = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (cse_var_4));
    DSL_DummyPtr dllp_iou_thresh_box = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * ((x_100) * (5)));
    DSL_DummyPtr dllp_iou_thresh_obj_cls_dim3 = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (cse_var_6));
    DSL_DummyPtr __tmp_54 = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (cse_var_1));
    DSL_DummyPtr dllp_iou_thresh_index_shape = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (2));
    DSL_DummyPtr _tmp_buf = dsl_membuf_dummy_alloc(&total_buf, sizeof(int8_t) * (cse_var_5));
    dsl_membuf_dummy_free(&total_buf, _tmp_buf, sizeof(int8_t) * (cse_var_5));
    DSL_DummyPtr dllp_iou_thresh_index_1 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (cse_var_1));
    DSL_DummyPtr dllp_iou_thresh_index_class = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (cse_var_2));
    DSL_DummyPtr dllp_iou_thresh_index_class_float = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (cse_var_2));
    DSL_DummyPtr dllp_iou_thresh_box_result = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * ((cse_var_2) * (5)));
    DSL_DummyPtr dllp_iou_thresh_score = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (cse_var_2));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_score, sizeof(float) * (cse_var_2));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_box_result, sizeof(float) * ((cse_var_2) * (5)));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_index_class_float, sizeof(float) * (cse_var_2));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_index_class, sizeof(int32_t) * (cse_var_2));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_index_1, sizeof(int32_t) * (cse_var_1));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_index_shape, sizeof(int32_t) * (2));
    dsl_membuf_dummy_free(&total_buf, __tmp_54, sizeof(float) * (cse_var_1));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_obj_cls_dim3, sizeof(float) * (cse_var_6));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_box, sizeof(float) * ((x_100) * (5)));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_xyxy, sizeof(float) * (cse_var_4));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_xy2, sizeof(float) * (cse_var_3));
    dsl_membuf_dummy_free(&total_buf, dllp_iou_thresh_half_wh, sizeof(float) * (cse_var_3));
    dsl_membuf_dummy_free(&total_buf, __tmp_83, sizeof(int32_t) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_82, sizeof(int32_t) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_81, sizeof(int32_t) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_80, sizeof(float) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_79, sizeof(int32_t) * (4));
    dsl_membuf_dummy_free(&total_buf, __tmp_78, sizeof(int32_t) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_77, sizeof(float) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_76, sizeof(int32_t) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_75, sizeof(int32_t) * (1));
    *auxi_mem_size = total_buf.max_size;
}






DSL_DLL int yolov8_custom_0_sub_1_Reshape(int32_t x_100, int32_t * dllp_iou_thresh_threshed_data_2_maxshape, int32_t *auxi_mem_size)
{
    
    dllp_iou_thresh_threshed_data_2_maxshape[0] = OPC_MIN((x_100) * (2), (x_100) + (1000));
    dllp_iou_thresh_threshed_data_2_maxshape[1] = 7;
    
    
    yolov8_custom_0_sub_1_get_memsize(x_100, auxi_mem_size);

    return 0;
}

#ifdef __cplusplus
}
#endif