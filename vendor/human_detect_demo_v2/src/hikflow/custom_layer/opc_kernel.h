/***************************************************************************************************
* 
* copyright information: copy right (c) 2010-2018, Hangzhou Hikvision Co., Ltd, all rights reserved
*
* file name: opc_kernel.h
* abstract: opc framework kernel function declaration file
* direction: All functions are optional implementations. Function templates are provided in the form of comments,
* and some examples of data types are listed. Configuration function fusion is not supported temporarily
* 
***************************************************************************************************/

#ifndef _OPC_KERNEL_H_
#define _OPC_KERNEL_H_
#include <stdbool.h>
// #define char signed char  // It shall be defined in c file to ensure that char is signed

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DSL_INT8,
    DSL_INT16,
    DSL_INT32,
    DSL_INT64,
    DSL_UINT8,
    DSL_UINT16,
    DSL_UINT32,
    DSL_UINT64,
    DSL_FLOAT,
    DSL_DOUBLE
};
#define DSL_TENSOR_MAX_DIM 5
typedef struct __DSL_TENSOR__{
    void* buffer;
    int dtype;
    int shape[DSL_TENSOR_MAX_DIM];
    int dim;
    int size;
    //int external_buf;
} DSL_TENSOR;
typedef DSL_TENSOR *DSL_TENSOR_PTR;

DSL_TENSOR_PTR _dsl_raw_buffer_to_tensor(void *tensor_ptr, const void* buffer, int dtype, int dim, ...);

#define dsl_raw_buffer_to_tensor(tr, tr_raw_buffer, dtype, tr_dim, ...)\
    DSL_TENSOR t_##tr;\
    DSL_TENSOR_PTR tr = _dsl_raw_buffer_to_tensor(&t_##tr, tr_raw_buffer, dtype, tr_dim,  ##__VA_ARGS__);
    //DSL_TENSOR_PTR tr = _dsl_raw_buffer_to_tensor(buf_top_pptr, tr_raw_buffer, dtype, tr_dim,  ##__VA_ARGS__);
//DSL_TENSOR_PTR dsl_raw_buffer_to_tensor(DSL_TENSOR_PTR tr, void* buffer, int dtype, int dim, ...);
void topk(DSL_TENSOR *input, DSL_TENSOR *output_value, DSL_TENSOR *output_index, DSL_TENSOR *mem_buf, int k, int axis, bool is_ascend);

// ----------------------------no function fusion----------------------------

// -----ELTWISE

// int OPC_KERNEL_eltwise_add_PxPxPxi(x *p_out, x *p_a, x *p_b, int len); 

// int OPC_KERNEL_eltwise_sub_PxPxPxi(x *p_out, x *p_a, x *p_b, int len); 

// int OPC_KERNEL_eltwise_mul_PxPxPxi(x *p_out, x *p_a, x *p_b, int len); 

// int OPC_KERNEL_eltwise_div_PxPxPxi(x *p_out, x *p_a, x *p_b, int len); 

// int OPC_KERNEL_eltwise_min_PxPxPxi(x *p_out, x *p_a, x *p_b, int len); 

// int OPC_KERNEL_eltwise_max_PxPxPxi(x *p_out, x *p_a, x *p_b, int len); 

// int OPC_KERNEL_eltwise_or_PxPxPxi(x *p_out, x *p_a, x *p_b, int len); 

// int OPC_KERNEL_eltwise_and_PxPxPxi(x *p_out, x *p_a, x *p_b, int len); 

// Single operand
// int OPC_KERNEL_eltwise_cast_PxPxi(x *p_out, x *p_a, int len);

// int OPC_KERNEL_eltwise_exp_PxPxi(x *p_out, x *p_a, int len);

// int OPC_KERNEL_eltwise_log_PxPxi(x *p_out, x *p_a, int len);

// int OPC_KERNEL_eltwise_abs_PxPxi(x *p_out, x *p_a, int len);

// int OPC_KERNEL_eltwise_rec_PxPxi(x *p_out, x *p_a, int len); // reciprocal

// int OPC_KERNEL_eltwise_ceil_PxPxi(x *p_out, x *p_a, int len);

// int OPC_KERNEL_eltwise_floor_PxPxi(x *p_out, x *p_a, int len);

// int OPC_KERNEL_eltwise_not_PxPxi(x *p_out, x *p_a, int len);

// -----BROADCAST

// broadcast_add
//int OPC_KERNEL_broadcast_add_PxPxxi(x *p_out, x *p_a, ixnt b, int len);

// broadcast_sub
//int OPC_KERNEL_broadcast_sub_PxPxxi(x *p_out, x *p_a, x b, int len);

// broadcast_mul
//int OPC_KERNEL_broadcast_mul_PxPxxi(x *p_out, x *p_a, x b, int len);

// broadcast_div
//int OPC_KERNEL_broadcast_div_PxPxxi(x *p_out, x *p_a, x b, int len);

// broadcast_max
//int OPC_KERNEL_broadcast_max_PxPxxi(x *p_out, x *p_a, x b, int len);

// broadcast_min
//int OPC_KERNEL_broadcast_min_PxPxxi(x *p_out, x *p_a, x b, int len);

// broadcast_shift
//int OPC_KERNEL_broadcast_shift_PxPxxi(x *p_out, x *p_a, x b, int len);

// -----REDUCE

//Used to configure the initial value of sum, which must be provided together with sum
// int OPC_KERNEL_reduce_sum_reset_Pxi(x *p_out, int lane);  
// int OPC_KERNEL_reduce_sum_PxPxi(x *p_out, x *p_a, int lane);
// int OPC_KERNEL_reduce_slide_sum_PxPxii(x *p_out, x *p_a, int lane, int times);

// int OPC_KERNEL_reduce_max_reset_Pxi(x *p_out, int lane);  
// int OPC_KERNEL_reduce_max_PxPxi(x *p_out, x *p_a, int lane,);
// int OPC_KERNEL_reduce_slide_max_PxPxii(x *p_out, x *p_a, int lane, int times);

// int OPC_KERNEL_reduce_mul_reset_Pxi(x *p_out, int lane);  
// int OPC_KERNEL_reduce_mul_PxPxi(x *p_out, x *p_a, int lane);
// int OPC_KERNEL_reduce_slide_mul_PxPxii(x *p_out, x *p_a, int lane, int times);

// int OPC_KERNEL_reduce_mac_reset_Pxi(x *p_out, int lane);  
// int OPC_KERNEL_reduce_mac_PxPxiPx(x *p_out, x *p_a, int lane, x *p_b);
// int OPC_KERNEL_reduce_slide_mac_PxPxiPxi(x *p_out, x *p_a, int lane, x *p_b, int times);



// ----------------------------function fusion----------------------------
// Only basic quantization operators are fused into separate interfaces

// quantization operators   (fused py cast cast shift xxx cast)
// int OPC_KERNEL_eltwise_add_quantize_PciPciPcii(char *p_out, int out_fl, char *p_a, int a_fl, char *p_b, int b_fl, int len);

// int OPC_KERNEL_eltwise_sub_quantize_PciPciPcii(char *p_out, int out_fl, char *p_a, int a_fl, char *p_b, int b_fl, int len);

// int OPC_KERNEL_eltwise_mul_quantize_PciPciPcii(char *p_out, int out_fl, char *p_a, int a_fl, char *p_b, int b_fl, int len);

// int OPC_KERNEL_eltwise_div_quantize_PciPciPcii(char *p_out, int out_fl, char *p_a, int a_fl, char *p_b, int b_fl, int len);

// ----------------------------single funciton--------------(Alternative, providing fixed interface, not supporting configuration)

// int OPC_KERNEL_conv_nchw_oihw(float *out, 
//                               float *data, int n, int c, int h, int w,
//                               float *kernel, int k_o, int k_i, int k_h, int k_w,
//                               int stride_h, int stride_w,
//                               int pad);

// int OPC_KERNEL_matmul(float *out, 
//                       float *a, int a_h, int a_w,
//                       float *b, int b_h, int b_w);



#ifdef __cplusplus
}
#endif 

#endif //_OPC_KERNEL_H_
