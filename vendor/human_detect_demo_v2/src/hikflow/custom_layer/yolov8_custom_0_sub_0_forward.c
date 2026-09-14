
#include "dsl_runtime.h"
#include "yolov8_custom_0_sub_0.h"
#ifdef __cplusplus
extern "C" {
#endif

void yolov8_custom_0_sub_0(void * dllp_conf_thresh_1_obj_data, void * num_454_3_transpose, void * num_457_1_transpose, void * dllp_conf_thresh_2_gathernd_1_init_0, void * dllp_conf_thresh_threshed_data_1, void * dllp_conf_thresh_threshed_data_1_shape0, void* __auxi_mem_addr__, size_t __auxi_mem_size__)
{
    DSL_MemBuf mem_buf;
    dsl_membuf_init(&mem_buf, __auxi_mem_addr__,__auxi_mem_size__);
    int32_t __tmp_27;
    int32_t __tmp_36_1;
    int32_t* __tmp_53 = dsl_membuf_alloc(&mem_buf, "__tmp_53", sizeof(int32_t) * (1));
    ((int32_t*)__tmp_53)[0] = 0;
    ((int32_t*)__tmp_53)[0] = 0;
    int32_t* __tmp_54 = dsl_membuf_alloc(&mem_buf, "__tmp_54", sizeof(int32_t) * (1));
    ((int32_t*)__tmp_54)[0] = 0;
    ((int32_t*)__tmp_54)[0] = 1;
    float* __tmp_55 = dsl_membuf_alloc(&mem_buf, "__tmp_55", sizeof(float) * (1));
    ((float*)__tmp_55)[0] = 0.0f;
    ((float*)__tmp_55)[0] = 0.5f;
    int32_t* __tmp_56 = dsl_membuf_alloc(&mem_buf, "__tmp_56", sizeof(int32_t) * (1));
    ((int32_t*)__tmp_56)[0] = 0;
    ((int32_t*)__tmp_56)[0] = 1000;
    int32_t* __tmp_57 = dsl_membuf_alloc(&mem_buf, "__tmp_57", sizeof(int32_t) * (2));
    for (int32_t i = 0; i < 2; i += 1)
    {
        ((int32_t*)__tmp_57)[i] = 0;
    }
    ((int32_t*)__tmp_57)[0] = 1;
    ((int32_t*)__tmp_57)[1] = 1;
    int8_t* dllp_conf_thresh_1_scored_obj_data = dsl_membuf_alloc(&mem_buf, "dllp_conf_thresh_1_scored_obj_data", sizeof(int8_t) * (8400));
    for (int32_t j = 0; j < 8400; j += 1)
    {
        ((int8_t*)dllp_conf_thresh_1_scored_obj_data)[j] = (int8_t)((((float*)__tmp_55)[0]) < (((float*)dllp_conf_thresh_1_obj_data)[j]));
    }
    int32_t __tmp_58;
    int32_t* __tmp_26 = dsl_membuf_alloc(&mem_buf, "__tmp_26", sizeof(int32_t) * (16800));
    __tmp_58 = 0;
    for (int32_t j_1 = 0; j_1 < 8400; j_1 += 1)
    {
        if ((bool)(((int8_t*)dllp_conf_thresh_1_scored_obj_data)[j_1]))
        {
            ((int32_t*)__tmp_26)[(__tmp_58) * (2)] = 0;
            ((int32_t*)__tmp_26)[((__tmp_58) * (2)) + (1)] = j_1;
            __tmp_58 = (__tmp_58) + (1);
        }
    }
    __tmp_27 = __tmp_58;
    int32_t* dllp_conf_thresh_1_nonzero_shape_unslice = dsl_membuf_alloc(&mem_buf, "dllp_conf_thresh_1_nonzero_shape_unslice", sizeof(int32_t) * (2));
    ((int32_t*)dllp_conf_thresh_1_nonzero_shape_unslice)[0] = 2;
    ((int32_t*)dllp_conf_thresh_1_nonzero_shape_unslice)[1] = __tmp_27;
    ((int32_t*)__tmp_56)[0] = OPC_MIN(((int32_t*)dllp_conf_thresh_1_nonzero_shape_unslice)[1], ((int32_t*)__tmp_56)[0]);
    int32_t __tmp_35_1;
    __tmp_35_1 = ((int32_t*)__tmp_53)[0];
    __tmp_36_1 = ((int32_t*)__tmp_56)[0];
    float* dllp_conf_thresh_1_batch_float = dsl_membuf_alloc(&mem_buf, "dllp_conf_thresh_1_batch_float", sizeof(float) * (1000));
    for (int32_t i_1 = 0; i_1 < __tmp_36_1; i_1 += 1)
    {
        ((float*)dllp_conf_thresh_1_batch_float)[i_1] = (float)(((int32_t*)__tmp_26)[(i_1) * (2)]);
    }
    float* dllp_conf_thresh_1_data_no_batch = dsl_membuf_alloc(&mem_buf, "dllp_conf_thresh_1_data_no_batch", sizeof(float) * (80000));
    for (int32_t i_2 = 0; i_2 < __tmp_36_1; i_2 += 1)
    {
        int32_t __licm_ = (i_2) * (2);
        int32_t __licm__1 = (i_2) * (80);
        for (int32_t j_2 = 0; j_2 < 80; j_2 += 1)
        {
            int32_t cse_var_1 = __licm_;
            ((float*)dllp_conf_thresh_1_data_no_batch)[(__licm__1) + (j_2)] = ((float*)num_457_1_transpose)[(((((int32_t*)__tmp_26)[cse_var_1]) * (672000)) + ((((int32_t*)__tmp_26)[(cse_var_1) + (1)]) * (80))) + (j_2)];
        }
    }
    float* num_456_constant = dsl_membuf_alloc(&mem_buf, "num_456_constant", sizeof(float) * (4000));
    for (int32_t i_3 = 0; i_3 < __tmp_36_1; i_3 += 1)
    {
        int32_t __licm__2 = (i_3) * (2);
        int32_t __licm__3 = (i_3) * (4);
        for (int32_t j_3 = 0; j_3 < 4; j_3 += 1)
        {
            int32_t cse_var_2 = __licm__2;
            ((float*)num_456_constant)[(__licm__3) + (j_3)] = ((float*)dllp_conf_thresh_2_gathernd_1_init_0)[(((((int32_t*)__tmp_26)[cse_var_2]) * (33600)) + ((((int32_t*)__tmp_26)[(cse_var_2) + (1)]) * (4))) + (j_3)];
        }
    }
    float* num_456 = dsl_membuf_alloc(&mem_buf, "num_456", sizeof(float) * (4000));
    for (int32_t i_4 = 0; i_4 < __tmp_36_1; i_4 += 1)
    {
        int32_t __licm__4 = (i_4) * (2);
        int32_t __licm__5 = (i_4) * (4);
        for (int32_t j_4 = 0; j_4 < 4; j_4 += 1)
        {
            int32_t cse_var_3 = __licm__4;
            ((float*)num_456)[(__licm__5) + (j_4)] = ((float*)num_454_3_transpose)[(((((int32_t*)__tmp_26)[cse_var_3]) * (33600)) + ((((int32_t*)__tmp_26)[(cse_var_3) + (1)]) * (4))) + (j_4)];
        }
    }
    for (int32_t i_5 = 0; i_5 < __tmp_36_1; i_5 += 1)
    {
        int32_t __licm__6 = (i_5) * (4);
        for (int32_t j_5 = 0; j_5 < 4; j_5 += 1)
        {
            int32_t cse_var_4 = (__licm__6) + (j_5);
            ((float*)num_456)[cse_var_4] = (((float*)num_456)[cse_var_4]) * (((float*)num_456_constant)[cse_var_4]);
        }
    }
    for (int32_t i_6 = 0; i_6 < __tmp_36_1; i_6 += 1)
    {
        int32_t cse_var_5 = (i_6) * (85);
        int32_t __licm__7 = (i_6) * (4);
        int32_t __licm__8 = cse_var_5;
        for (int32_t i_7 = 0; i_7 < 4; i_7 += 1)
        {
            ((float*)dllp_conf_thresh_threshed_data_1)[(__licm__8) + (i_7)] = ((float*)num_456)[(__licm__7) + (i_7)];
        }
        int32_t __licm__9 = (i_6) * (80);
        int32_t __licm__10 = cse_var_5;
        for (int32_t i_8 = 0; i_8 < 80; i_8 += 1)
        {
            ((float*)dllp_conf_thresh_threshed_data_1)[((__licm__10) + (i_8)) + (4)] = ((float*)dllp_conf_thresh_1_data_no_batch)[(__licm__9) + (i_8)];
        }
        ((float*)dllp_conf_thresh_threshed_data_1)[(cse_var_5) + (84)] = ((float*)dllp_conf_thresh_1_batch_float)[i_6];
    }
    ((int32_t*)dllp_conf_thresh_threshed_data_1_shape0)[0] = __tmp_36_1;
    dsl_membuf_free(&mem_buf, num_456, sizeof(float) * (4000));
    dsl_membuf_free(&mem_buf, num_456_constant, sizeof(float) * (4000));
    dsl_membuf_free(&mem_buf, dllp_conf_thresh_1_data_no_batch, sizeof(float) * (80000));
    dsl_membuf_free(&mem_buf, dllp_conf_thresh_1_batch_float, sizeof(float) * (1000));
    dsl_membuf_free(&mem_buf, dllp_conf_thresh_1_nonzero_shape_unslice, sizeof(int32_t) * (2));
    dsl_membuf_free(&mem_buf, __tmp_26, sizeof(int32_t) * (16800));
    dsl_membuf_free(&mem_buf, dllp_conf_thresh_1_scored_obj_data, sizeof(int8_t) * (8400));
    dsl_membuf_free(&mem_buf, __tmp_57, sizeof(int32_t) * (2));
    dsl_membuf_free(&mem_buf, __tmp_56, sizeof(int32_t) * (1));
    dsl_membuf_free(&mem_buf, __tmp_55, sizeof(float) * (1));
    dsl_membuf_free(&mem_buf, __tmp_54, sizeof(int32_t) * (1));
    dsl_membuf_free(&mem_buf, __tmp_53, sizeof(int32_t) * (1));
}

DSL_DLL int yolov8_custom_0_sub_0_Forward(float* dllp_conf_thresh_1_obj_data, float* num_454_3_transpose, float* num_457_1_transpose, float* dllp_conf_thresh_2_gathernd_1_init_0, float* dllp_conf_thresh_threshed_data_1, int32_t * dllp_conf_thresh_threshed_data_2_shape, void* __auxi_mem_addr__)
{
    
    // define internal function return symbols
    int32_t dllp_conf_thresh_threshed_data_1_shape0;
    
    int32_t auxi_memsize = 0;
    int32_t dllp_conf_thresh_threshed_data_2_maxshape[5];  // max 5 dim
    
    yolov8_custom_0_sub_0_Reshape(dllp_conf_thresh_threshed_data_2_maxshape, &auxi_memsize);

    
    // Invoke Entry Function
    yolov8_custom_0_sub_0(dllp_conf_thresh_1_obj_data, num_454_3_transpose, num_457_1_transpose, dllp_conf_thresh_2_gathernd_1_init_0, dllp_conf_thresh_threshed_data_1, &dllp_conf_thresh_threshed_data_1_shape0, __auxi_mem_addr__, auxi_memsize);
    
    
    // Set Output Tensor Shapes
    // dllp_conf_thresh_threshed_data_1's dim is 2
    dllp_conf_thresh_threshed_data_2_shape[0] = dllp_conf_thresh_threshed_data_1_shape0;
    dllp_conf_thresh_threshed_data_2_shape[1] = 85;
    
    
    return 0;
}

#ifdef __cplusplus
}
#endif