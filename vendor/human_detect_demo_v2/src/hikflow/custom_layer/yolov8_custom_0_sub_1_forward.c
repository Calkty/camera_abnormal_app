
#include "dsl_runtime.h"
#include "yolov8_custom_0_sub_1.h"
#ifdef __cplusplus
extern "C" {
#endif

void yolov8_custom_0_sub_1(void * dllp_conf_thresh_threshed_data, int32_t x_100, void * dllp_iou_thresh_threshed_data_1, void * dllp_iou_thresh_threshed_data_1_shape0, void* __auxi_mem_addr__, size_t __auxi_mem_size__)
{
    DSL_MemBuf mem_buf;
    dsl_membuf_init(&mem_buf, __auxi_mem_addr__,__auxi_mem_size__);
    int32_t cse_var_6 = (x_100) * (80);
    int32_t cse_var_5 = (x_100) * (512);
    int32_t cse_var_4 = (x_100) * (4);
    int32_t cse_var_3 = (x_100) * (2);
    int32_t cse_var_2 = OPC_MIN(cse_var_3, (x_100) + (1000));
    int32_t cse_var_1 = (cse_var_2) * (3);
    int32_t __tmp_57;
    int32_t* __tmp_75 = dsl_membuf_alloc(&mem_buf, "__tmp_75", sizeof(int32_t) * (1));
    ((int32_t*)__tmp_75)[0] = 0;
    ((int32_t*)__tmp_75)[0] = 0;
    int32_t* __tmp_76 = dsl_membuf_alloc(&mem_buf, "__tmp_76", sizeof(int32_t) * (1));
    ((int32_t*)__tmp_76)[0] = 0;
    ((int32_t*)__tmp_76)[0] = 1;
    float* __tmp_77 = dsl_membuf_alloc(&mem_buf, "__tmp_77", sizeof(float) * (1));
    ((float*)__tmp_77)[0] = 0.0f;
    ((float*)__tmp_77)[0] = 0.5f;
    int32_t* __tmp_78 = dsl_membuf_alloc(&mem_buf, "__tmp_78", sizeof(int32_t) * (1));
    ((int32_t*)__tmp_78)[0] = 0;
    ((int32_t*)__tmp_78)[0] = 1000;
    int32_t* __tmp_79 = dsl_membuf_alloc(&mem_buf, "__tmp_79", sizeof(int32_t) * (4));
    for (int32_t i = 0; i < 4; i += 1)
    {
        ((int32_t*)__tmp_79)[i] = 0;
    }
    ((int32_t*)__tmp_79)[0] = 2;
    ((int32_t*)__tmp_79)[1] = 2;
    ((int32_t*)__tmp_79)[2] = 80;
    ((int32_t*)__tmp_79)[3] = 1;
    float* __tmp_80 = dsl_membuf_alloc(&mem_buf, "__tmp_80", sizeof(float) * (1));
    ((float*)__tmp_80)[0] = 0.0f;
    ((float*)__tmp_80)[0] = 2.0f;
    int32_t* __tmp_81 = dsl_membuf_alloc(&mem_buf, "__tmp_81", sizeof(int32_t) * (1));
    ((int32_t*)__tmp_81)[0] = 0;
    ((int32_t*)__tmp_81)[0] = 2;
    int32_t* __tmp_82 = dsl_membuf_alloc(&mem_buf, "__tmp_82", sizeof(int32_t) * (1));
    ((int32_t*)__tmp_82)[0] = 0;
    ((int32_t*)__tmp_82)[0] = 3;
    int32_t* __tmp_83 = dsl_membuf_alloc(&mem_buf, "__tmp_83", sizeof(int32_t) * (1));
    ((int32_t*)__tmp_83)[0] = 0;
    ((int32_t*)__tmp_83)[0] = 1;
    float* dllp_iou_thresh_half_wh = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_half_wh", sizeof(float) * (cse_var_3));
    for (int32_t i_1 = 0; i_1 < x_100; i_1 += 1)
    {
        int32_t __licm_ = (i_1) * (85);
        int32_t __licm__1 = (i_1) * (2);
        for (int32_t j = 0; j < 2; j += 1)
        {
            ((float*)dllp_iou_thresh_half_wh)[(__licm__1) + (j)] = (((float*)dllp_conf_thresh_threshed_data)[((__licm_) + (j)) + (2)]) / (((float*)__tmp_80)[0]);
        }
    }
    float* dllp_iou_thresh_xy2 = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_xy2", sizeof(float) * (cse_var_3));
    for (int32_t i_2 = 0; i_2 < x_100; i_2 += 1)
    {
        int32_t __licm__2 = (i_2) * (2);
        int32_t __licm__3 = (i_2) * (85);
        for (int32_t j_1 = 0; j_1 < 2; j_1 += 1)
        {
            int32_t cse_var_7 = (__licm__2) + (j_1);
            ((float*)dllp_iou_thresh_xy2)[cse_var_7] = (((float*)dllp_conf_thresh_threshed_data)[(__licm__3) + (j_1)]) + (((float*)dllp_iou_thresh_half_wh)[cse_var_7]);
        }
    }
    for (int32_t i_3 = 0; i_3 < x_100; i_3 += 1)
    {
        int32_t __licm__4 = (i_3) * (2);
        int32_t __licm__5 = (i_3) * (85);
        for (int32_t j_2 = 0; j_2 < 2; j_2 += 1)
        {
            int32_t cse_var_8 = (__licm__4) + (j_2);
            ((float*)dllp_iou_thresh_half_wh)[cse_var_8] = (((float*)dllp_conf_thresh_threshed_data)[(__licm__5) + (j_2)]) - (((float*)dllp_iou_thresh_half_wh)[cse_var_8]);
        }
    }
    float* dllp_iou_thresh_xyxy = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_xyxy", sizeof(float) * (cse_var_4));
    for (int32_t i_4 = 0; i_4 < x_100; i_4 += 1)
    {
        int32_t cse_var_13 = (i_4) * (4);
        int32_t cse_var_12 = (i_4) * (2);
        int32_t __licm__6 = cse_var_12;
        int32_t __licm__7 = cse_var_13;
        for (int32_t i_5 = 0; i_5 < 2; i_5 += 1)
        {
            ((float*)dllp_iou_thresh_xyxy)[(__licm__7) + (i_5)] = ((float*)dllp_iou_thresh_half_wh)[(__licm__6) + (i_5)];
        }
        int32_t __licm__8 = cse_var_12;
        int32_t __licm__9 = cse_var_13;
        for (int32_t i_6 = 0; i_6 < 2; i_6 += 1)
        {
            ((float*)dllp_iou_thresh_xyxy)[((__licm__9) + (i_6)) + (2)] = ((float*)dllp_iou_thresh_xy2)[(__licm__8) + (i_6)];
        }
    }
    float* dllp_iou_thresh_box = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_box", sizeof(float) * ((x_100) * (5)));
    for (int32_t i_7 = 0; i_7 < x_100; i_7 += 1)
    {
        int32_t cse_var_15 = (i_7) * (85);
        int32_t cse_var_14 = (i_7) * (5);
        int32_t __licm__10 = (i_7) * (2);
        int32_t __licm__11 = cse_var_14;
        for (int32_t i_8 = 0; i_8 < 2; i_8 += 1)
        {
            ((float*)dllp_iou_thresh_box)[(__licm__11) + (i_8)] = ((float*)dllp_iou_thresh_half_wh)[(__licm__10) + (i_8)];
        }
        int32_t __licm__12 = cse_var_15;
        int32_t __licm__13 = cse_var_14;
        for (int32_t i_9 = 0; i_9 < 2; i_9 += 1)
        {
            ((float*)dllp_iou_thresh_box)[((__licm__13) + (i_9)) + (2)] = ((float*)dllp_conf_thresh_threshed_data)[((__licm__12) + (i_9)) + (2)];
        }
        ((float*)dllp_iou_thresh_box)[(cse_var_14) + (4)] = ((float*)dllp_conf_thresh_threshed_data)[(cse_var_15) + (84)];
    }
    float* dllp_iou_thresh_obj_cls_dim3 = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_obj_cls_dim3", sizeof(float) * (cse_var_6));
    for (int32_t j_3 = 0; j_3 < 80; j_3 += 1)
    {
        int32_t __licm__14 = (j_3) * (x_100);
        for (int32_t k = 0; k < x_100; k += 1)
        {
            ((float*)dllp_iou_thresh_obj_cls_dim3)[(k) + (__licm__14)] = ((float*)dllp_conf_thresh_threshed_data)[(((k) * (85)) + (j_3)) + (4)];
        }
    }
    float* __tmp_54 = dsl_membuf_alloc(&mem_buf, "__tmp_54", sizeof(float) * (cse_var_1));
    int32_t* dllp_iou_thresh_index_shape = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_index_shape", sizeof(int32_t) * (2));
    int8_t* _tmp_buf = dsl_membuf_alloc(&mem_buf, "_tmp_buf", sizeof(int8_t) * (cse_var_5));
    nms_forward((float*)dllp_iou_thresh_xyxy, 1, x_100, 4, (float*)dllp_iou_thresh_obj_cls_dim3, 1, 80, x_100, (void*)__tmp_54, (void*)dllp_iou_thresh_index_shape, 1000, 0.5f, 0.5f, (void*)_tmp_buf);
    dsl_membuf_free(&mem_buf, _tmp_buf, sizeof(int8_t) * (cse_var_5));
    __tmp_57 = ((int32_t*)dllp_iou_thresh_index_shape)[0];
    int32_t* dllp_iou_thresh_index_1 = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_index_1", sizeof(int32_t) * (cse_var_1));
    for (int32_t i_10 = 0; i_10 < __tmp_57; i_10 += 1)
    {
        int32_t __licm__15 = (i_10) * (3);
        for (int32_t j_4 = 0; j_4 < 3; j_4 += 1)
        {
            int32_t cse_var_9 = (__licm__15) + (j_4);
            ((int32_t*)dllp_iou_thresh_index_1)[cse_var_9] = (int32_t)(((float*)__tmp_54)[cse_var_9]);
        }
    }
    int32_t* dllp_iou_thresh_index_class = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_index_class", sizeof(int32_t) * (cse_var_2));
    for (int32_t i_11 = 0; i_11 < __tmp_57; i_11 += 1)
    {
        ((int32_t*)dllp_iou_thresh_index_class)[i_11] = ((int32_t*)dllp_iou_thresh_index_1)[((i_11) * (3)) + (((int32_t*)__tmp_76)[0])];
    }
    float* dllp_iou_thresh_index_class_float = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_index_class_float", sizeof(float) * (cse_var_2));
    for (int32_t i_12 = 0; i_12 < __tmp_57; i_12 += 1)
    {
        ((float*)dllp_iou_thresh_index_class_float)[i_12] = (float)(((int32_t*)dllp_iou_thresh_index_class)[i_12]);
    }
    int32_t __tmp_62_1;
    __tmp_62_1 = ((int32_t*)__tmp_81)[0];
    int32_t __tmp_63_1;
    __tmp_63_1 = ((int32_t*)__tmp_82)[0];
    float* dllp_iou_thresh_box_result = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_box_result", sizeof(float) * ((cse_var_2) * (5)));
    for (int32_t i_13 = 0; i_13 < __tmp_57; i_13 += 1)
    {
        int32_t __licm__16 = (((int32_t*)dllp_iou_thresh_index_1)[((i_13) * (3)) + (2)]) * (5);
        int32_t __licm__17 = (i_13) * (5);
        for (int32_t i_14 = 0; i_14 < 5; i_14 += 1)
        {
            ((float*)dllp_iou_thresh_box_result)[(__licm__17) + (i_14)] = ((float*)dllp_iou_thresh_box)[(__licm__16) + (i_14)];
        }
    }
    float* dllp_iou_thresh_score = dsl_membuf_alloc(&mem_buf, "dllp_iou_thresh_score", sizeof(float) * (cse_var_2));
    for (int32_t i_15 = 0; i_15 < __tmp_57; i_15 += 1)
    {
        int32_t cse_var_10 = (i_15) * (3);
        ((float*)dllp_iou_thresh_score)[i_15] = ((float*)dllp_conf_thresh_threshed_data)[((((((int32_t*)dllp_iou_thresh_index_1)[(cse_var_10) + (2)]) * (85)) + (((int32_t*)dllp_iou_thresh_index_1)[(cse_var_10) + (1)])) + (((((int32_t*)dllp_iou_thresh_index_1)[cse_var_10]) * (80)) * (x_100))) + (4)];
    }
    for (int32_t i_16 = 0; i_16 < __tmp_57; i_16 += 1)
    {
        int32_t cse_var_11 = (i_16) * (7);
        ((float*)dllp_iou_thresh_threshed_data_1)[cse_var_11] = ((float*)dllp_iou_thresh_index_class_float)[i_16];
        ((float*)dllp_iou_thresh_threshed_data_1)[(cse_var_11) + (1)] = ((float*)dllp_iou_thresh_score)[i_16];
        int32_t __licm__18 = (i_16) * (5);
        for (int32_t i_17 = 0; i_17 < 5; i_17 += 1)
        {
            ((float*)dllp_iou_thresh_threshed_data_1)[((cse_var_11) + (i_17)) + (2)] = ((float*)dllp_iou_thresh_box_result)[(__licm__18) + (i_17)];
        }
    }
    ((int32_t*)dllp_iou_thresh_threshed_data_1_shape0)[0] = __tmp_57;
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_score, sizeof(float) * (cse_var_2));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_box_result, sizeof(float) * ((cse_var_2) * (5)));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_index_class_float, sizeof(float) * (cse_var_2));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_index_class, sizeof(int32_t) * (cse_var_2));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_index_1, sizeof(int32_t) * (cse_var_1));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_index_shape, sizeof(int32_t) * (2));
    dsl_membuf_free(&mem_buf, __tmp_54, sizeof(float) * (cse_var_1));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_obj_cls_dim3, sizeof(float) * (cse_var_6));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_box, sizeof(float) * ((x_100) * (5)));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_xyxy, sizeof(float) * (cse_var_4));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_xy2, sizeof(float) * (cse_var_3));
    dsl_membuf_free(&mem_buf, dllp_iou_thresh_half_wh, sizeof(float) * (cse_var_3));
    dsl_membuf_free(&mem_buf, __tmp_83, sizeof(int32_t) * (1));
    dsl_membuf_free(&mem_buf, __tmp_82, sizeof(int32_t) * (1));
    dsl_membuf_free(&mem_buf, __tmp_81, sizeof(int32_t) * (1));
    dsl_membuf_free(&mem_buf, __tmp_80, sizeof(float) * (1));
    dsl_membuf_free(&mem_buf, __tmp_79, sizeof(int32_t) * (4));
    dsl_membuf_free(&mem_buf, __tmp_78, sizeof(int32_t) * (1));
    dsl_membuf_free(&mem_buf, __tmp_77, sizeof(float) * (1));
    dsl_membuf_free(&mem_buf, __tmp_76, sizeof(int32_t) * (1));
    dsl_membuf_free(&mem_buf, __tmp_75, sizeof(int32_t) * (1));
}

DSL_DLL int yolov8_custom_0_sub_1_Forward(float* dllp_conf_thresh_threshed_data, int32_t x_100, float* dllp_iou_thresh_threshed_data_1, int32_t * dllp_iou_thresh_threshed_data_2_shape, void* __auxi_mem_addr__)
{
    
    // define internal function return symbols
    int32_t dllp_iou_thresh_threshed_data_1_shape0;
    
    int32_t auxi_memsize = 0;
    int32_t dllp_iou_thresh_threshed_data_2_maxshape[5];  // max 5 dim
    
    yolov8_custom_0_sub_1_Reshape(x_100, dllp_iou_thresh_threshed_data_2_maxshape, &auxi_memsize);

    
    // Invoke Entry Function
    yolov8_custom_0_sub_1(dllp_conf_thresh_threshed_data, x_100, dllp_iou_thresh_threshed_data_1, &dllp_iou_thresh_threshed_data_1_shape0, __auxi_mem_addr__, auxi_memsize);
    
    
    // Set Output Tensor Shapes
    // dllp_iou_thresh_threshed_data_1's dim is 2
    dllp_iou_thresh_threshed_data_2_shape[0] = dllp_iou_thresh_threshed_data_1_shape0;
    dllp_iou_thresh_threshed_data_2_shape[1] = 7;
    
    
    return 0;
}

#ifdef __cplusplus
}
#endif