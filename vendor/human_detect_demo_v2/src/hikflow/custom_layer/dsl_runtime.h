#ifndef __DSL_RUNTIME_H__
#define __DSL_RUNTIME_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include "dsl_nms.h"


typedef struct _FLOAT_INDEX_TUPLE_
{
    float value;
    int   index;
}FLOAT_INDEX_TUPLE;

#define DSL_SORT_VALUE_TYPE FLOAT_INDEX_TUPLE
#define DSL_SORT_COMP_PTR_FUNC(a_ptr, b_ptr) ((a_ptr->value) > (b_ptr->value))
#define DSL_SORT_SUFFIX descend
#include "dsl_sort.h"
// ->_DSL_SORT_FLOAT_INDEX_TUPLE_descend


#define DSL_SORT_VALUE_TYPE FLOAT_INDEX_TUPLE
#define DSL_SORT_COMP_PTR_FUNC(a_ptr, b_ptr) ((a_ptr->value) < (b_ptr->value))
#define DSL_SORT_SUFFIX ascend
#include "dsl_sort.h"
// ->_dsl_sort_FLOAT_INDEX_TUPLE_ascend




// definition
#define DSL_RUNTIME_DEBUG_LEVEL 0
#define DSL_MEMINFER_PROGRAM_STACK_MAX_USAGE 10240

// magic number
#define DSL_RUNTIME_MIN_FLOAT32 -3.402823e+38f
#define DSL_RUNTIME_MAX_FLOAT32 3.402823e+38f
#define DSL_RUNTIME_MIN_INT32   -2147483648
#define DSL_RUNTIME_MAX_INT32   2147483647

#ifndef DSL_DLL
#ifdef _WIN32
#define DSL_DLL 
#else
#define DSL_DLL __attribute__((visibility("default")))
#endif
#endif

typedef enum _DSL_DTYPE_{
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
}DSL_DTYPE;

#define OPC_SIZE_ALIGN(size, align)  ((((unsigned int)(size)) + (((unsigned int)(align)) - 1)) & (~(((unsigned int)(align)) - 1)))
#define OPC_SIZE_ALIGN_64(size)      OPC_SIZE_ALIGN(size, 64)
#define OPC_SIZE_ALIGN_128(size)     OPC_SIZE_ALIGN(size, 128)

#define OPC_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define OPC_MIN(a, b) (((a) < (b)) ? (a) : (b))



#ifndef DSL_DLL
#ifdef _WIN32
#define DSL_DLL 
#else
#define DSL_DLL __attribute__((visibility("default")))
#endif
#endif


typedef struct _DSL_MemBuf_
{
    uintptr_t start_ptr;
    uintptr_t curr_ptr;
    uintptr_t end_ptr; // size = end_ptr - start_ptr
}DSL_MemBuf;


typedef struct _DSL_DummyMemBuf_
{
    uintptr_t curr_size;
    uintptr_t max_size;
}DSL_DummyMemBuf;

typedef uintptr_t DSL_DummyPtr;

static void dsl_membuf_init(DSL_MemBuf *mem_buf, void* auxi_mem, size_t auxi_mem_size)
{
    mem_buf->start_ptr = (uintptr_t)auxi_mem;
    mem_buf->curr_ptr = mem_buf->start_ptr;
    mem_buf->end_ptr = mem_buf->start_ptr + auxi_mem_size;
}

static void* _dsl_membuf_alloc(DSL_MemBuf *mem_buf, size_t size)
{
    assert(mem_buf->curr_ptr + size <= mem_buf->end_ptr);
    void *buffer = (void*)mem_buf->curr_ptr;
    mem_buf->curr_ptr += size;
    return buffer;
}

static void* dsl_membuf_get_ptr(DSL_MemBuf *mem_buf)
{
    void *buffer = (void*)mem_buf->curr_ptr;
    return buffer;
}

static size_t dsl_membuf_get_rest_size(DSL_MemBuf *mem_buf)
{
    return (mem_buf->end_ptr - mem_buf->curr_ptr);
}

static DSL_DummyPtr dsl_membuf_dummy_alloc(DSL_DummyMemBuf *mem_buf, size_t size)
{
    DSL_DummyPtr curr_offset = mem_buf->curr_size;
    mem_buf->curr_size += size;
    mem_buf->max_size = OPC_MAX(mem_buf->max_size, mem_buf->curr_size);
    return curr_offset;
}

static void dsl_membuf_dummy_free(DSL_DummyMemBuf *mem_buf, DSL_DummyPtr ptr, size_t size)
{
    assert(mem_buf->curr_size == ptr + size);
    mem_buf->curr_size -= size;
}


static void dsl_membuf_dummy_merge_ifthenelse(DSL_DummyMemBuf *mem_buf, 
                                                      DSL_DummyMemBuf *then_mem_buf, 
                                                      DSL_DummyMemBuf *else_mem_buf)
{
    assert(then_mem_buf->curr_size == 0);
    assert(else_mem_buf->curr_size == 0);
    mem_buf->max_size = OPC_MAX(mem_buf->max_size,
                        mem_buf->curr_size + OPC_MAX(then_mem_buf->max_size, else_mem_buf->max_size));
}

#define dsl_membuf_alloc(mem_buf, name, sym_size)\
    _dsl_membuf_alloc(mem_buf, sym_size)

static void _dsl_membuf_free(DSL_MemBuf *mem_buf, void* ptr, size_t size)
{
    assert(mem_buf->curr_ptr == (uintptr_t)ptr + size);
    mem_buf->curr_ptr = (uintptr_t)ptr;
}

#if DSL_RUNTIME_DEBUG_LEVEL > 1
#define dsl_membuf_free(mem_buf, ptr, sym_size)\
    printf("free %s\n", #ptr);\
    _dsl_membuf_free(mem_buf, ptr, sym_size)
#else
#define dsl_membuf_free(mem_buf, ptr, sym_size)\
    _dsl_membuf_free(mem_buf, ptr, sym_size)
#endif

#define dsl_swap_ptr(ptr_a, ptr_b)\
    {\
        void* __ptr_swap_tmp__ = ptr_b;\
        ptr_b = ptr_a;\
        ptr_a = __ptr_swap_tmp__;\
    }


// 
static void topk_float(void *input, void *output_value, void *output_index, void *mem_buf, int k, int axis, int is_ascend, int ndim, ...)
{

    int tensor_max_shape[10];
    va_list arguments;
    int i = 0;
    va_start(arguments, ndim); // ndim 
    for(i = 0; i < ndim; i++)  //  while ((temp = va_arg(arguments, int)) != END)
    {
        tensor_max_shape[i] = va_arg(arguments, int32_t);
    }
    va_end(arguments);

    if (axis < 0) {
        axis = ndim + axis;
    }
    // assert(axis < ndim);
    int num = tensor_max_shape[axis];
    // 
    void *sorter = (void*)mem_buf; //   (sizeof(input.dtype) + sizeof(int))*num
    //  float
    float *data_ptr = (float*)input;
    int *indices_ptr = (int*)output_index;
    float *values_ptr = (float*)output_value;

    // axis >=0 && axis < input.ndim
    if (k < 1) {
        k = num;
    }
    bool use_partial_sort = k * 64 <= num;

    // 
    int axis_mul_before = 1;
    int axis_mul_after = 1;
    for (int i = 0; i < ndim; ++i) {
        if (i < axis) {
            axis_mul_before *= tensor_max_shape[i];
        } else if (i > axis) {
            axis_mul_after *= tensor_max_shape[i];
        }
    }

    for (int i = 0; i < axis_mul_before; ++i) {
        for (int j = 0; j < axis_mul_after; ++j) {
            // 
            //memset((void*)sorter, 0, num*2*sizeof(float));
            int src_base_idx = i * num * axis_mul_after + j;
            int dst_base_idx = i * k * axis_mul_after + j;
            for (int kk = 0; kk < num; ++kk) {
                int full_idx = src_base_idx + kk * axis_mul_after;
                ((FLOAT_INDEX_TUPLE*)sorter + kk)->value = data_ptr[full_idx];
                ((FLOAT_INDEX_TUPLE*)sorter + kk)->index = kk;
            }

            int cnt = k > 0 ? k : num;
            if (is_ascend)
            {
                if(use_partial_sort)
                {
                    _dsl_partial_sort_FLOAT_INDEX_TUPLE_ascend((FLOAT_INDEX_TUPLE*)sorter, (FLOAT_INDEX_TUPLE*)sorter + cnt, (FLOAT_INDEX_TUPLE*)sorter + num);
                }
                else
                {
                    _dsl_nth_element_FLOAT_INDEX_TUPLE_ascend((FLOAT_INDEX_TUPLE*)sorter, (FLOAT_INDEX_TUPLE*)sorter + cnt, (FLOAT_INDEX_TUPLE*)sorter + num);
                    _dsl_sort_FLOAT_INDEX_TUPLE_ascend((FLOAT_INDEX_TUPLE*)sorter, (FLOAT_INDEX_TUPLE*)sorter + cnt);
                }

            }
            else
            {
                if(use_partial_sort)
                {
                    _dsl_partial_sort_FLOAT_INDEX_TUPLE_descend((FLOAT_INDEX_TUPLE*)sorter, (FLOAT_INDEX_TUPLE*)sorter + cnt, (FLOAT_INDEX_TUPLE*)sorter + num);
                }
                else
                {
                    _dsl_nth_element_FLOAT_INDEX_TUPLE_descend((FLOAT_INDEX_TUPLE*)sorter, (FLOAT_INDEX_TUPLE*)sorter + cnt, (FLOAT_INDEX_TUPLE*)sorter + num);
                    _dsl_sort_FLOAT_INDEX_TUPLE_descend((FLOAT_INDEX_TUPLE*)sorter, (FLOAT_INDEX_TUPLE*)sorter + cnt);
                }
            }
            // 
            for (int kk = 0; kk < cnt; ++kk) {
                if (indices_ptr != NULL) {
                    indices_ptr[dst_base_idx + kk * axis_mul_after] = ((FLOAT_INDEX_TUPLE*)sorter + kk)->index;
                }
                if (values_ptr != NULL) {
                    values_ptr[dst_base_idx + kk * axis_mul_after] = ((FLOAT_INDEX_TUPLE*)sorter + kk)->value;
                }
            }
            //
        }
    }
}

static inline float
fastpow2 (float p)
{
  union { float f; uint32_t i; } vp = { p };
  int sign = (vp.i >> 31);
  int w = p;
  float z = p - w + sign;
  union { uint32_t i; float f; } v = { (1 << 23) * (p + 121.2740838f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z) };
  return v.f; // error 1.58868e-05
}
 
static inline float
fastexp (float p)
{
  return fastpow2 (1.442695040f * p); // 1.60712e-05   allclose  1e-4
}

static inline float fastlog2 (float x)
{
    union { float f; uint32_t i; } vx = { x };
    union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | (0x7e << 23) };
    float y = vx.i;
    y *= 1.0 / (1 << 23);
    return y - 124.22544637f - 1.498030302f * mx.f - 1.72587999f / (0.3520887068f + mx.f); // 2.09352e-05
    //return y - 126.94269504f; // even faster and rougher
}

static inline float
fastlog (float x)
{
    return 0.69314718f * fastlog2 (x); // error 2.09348e-05
}
static inline float
fastpow (float x,
         float p)
{
    return fastpow2 (p * fastlog2 (x));
}

static inline float
fastsinh (float p)
{
  return 0.5f * (fastexp (p) - fastexp (-p));
}

static inline float
fastcosh (float p)
{
  return 0.5f * (fastexp (p) + fastexp (-p));
}

static inline float
fasttanh (float p)
{
  return -1.0f + 2.0f / (1.0f + fastexp (-2.0f * p));
}





#ifdef __cplusplus
}
#endif 
#endif
