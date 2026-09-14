#include "dsl_runtime.h"
#include "yolov8_custom_0_sub_0.h"

#ifdef __cplusplus
extern "C" {
#endif

void yolov8_custom_0_sub_0_get_memsize(int32_t* auxi_mem_size)
{
    DSL_DummyMemBuf total_buf = {0};
    DSL_DummyPtr __tmp_53 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (1));
    DSL_DummyPtr __tmp_54 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (1));
    DSL_DummyPtr __tmp_55 = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (1));
    DSL_DummyPtr __tmp_56 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (1));
    DSL_DummyPtr __tmp_57 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (2));
    DSL_DummyPtr dllp_conf_thresh_1_scored_obj_data = dsl_membuf_dummy_alloc(&total_buf, sizeof(int8_t) * (8400));
    DSL_DummyPtr __tmp_26 = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (16800));
    DSL_DummyPtr dllp_conf_thresh_1_nonzero_shape_unslice = dsl_membuf_dummy_alloc(&total_buf, sizeof(int32_t) * (2));
    DSL_DummyPtr dllp_conf_thresh_1_batch_float = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (1000));
    DSL_DummyPtr dllp_conf_thresh_1_data_no_batch = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (80000));
    DSL_DummyPtr num_456_constant = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (4000));
    DSL_DummyPtr num_456 = dsl_membuf_dummy_alloc(&total_buf, sizeof(float) * (4000));
    dsl_membuf_dummy_free(&total_buf, num_456, sizeof(float) * (4000));
    dsl_membuf_dummy_free(&total_buf, num_456_constant, sizeof(float) * (4000));
    dsl_membuf_dummy_free(&total_buf, dllp_conf_thresh_1_data_no_batch, sizeof(float) * (80000));
    dsl_membuf_dummy_free(&total_buf, dllp_conf_thresh_1_batch_float, sizeof(float) * (1000));
    dsl_membuf_dummy_free(&total_buf, dllp_conf_thresh_1_nonzero_shape_unslice, sizeof(int32_t) * (2));
    dsl_membuf_dummy_free(&total_buf, __tmp_26, sizeof(int32_t) * (16800));
    dsl_membuf_dummy_free(&total_buf, dllp_conf_thresh_1_scored_obj_data, sizeof(int8_t) * (8400));
    dsl_membuf_dummy_free(&total_buf, __tmp_57, sizeof(int32_t) * (2));
    dsl_membuf_dummy_free(&total_buf, __tmp_56, sizeof(int32_t) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_55, sizeof(float) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_54, sizeof(int32_t) * (1));
    dsl_membuf_dummy_free(&total_buf, __tmp_53, sizeof(int32_t) * (1));
    *auxi_mem_size = total_buf.max_size;
}






DSL_DLL int yolov8_custom_0_sub_0_Reshape(int32_t * dllp_conf_thresh_threshed_data_2_maxshape, int32_t *auxi_mem_size)
{
    
    dllp_conf_thresh_threshed_data_2_maxshape[0] = 1000;
    dllp_conf_thresh_threshed_data_2_maxshape[1] = 85;
    
    
    yolov8_custom_0_sub_0_get_memsize(auxi_mem_size);

    return 0;
}

#ifdef __cplusplus
}
#endif