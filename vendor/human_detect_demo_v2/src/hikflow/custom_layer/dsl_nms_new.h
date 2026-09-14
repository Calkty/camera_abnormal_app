#ifndef  _NMS_H_
#define  _NMS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CLASS_NUM (91)

typedef struct _BBOX_T
{
    float x;
    float y;
    float w;
    float h;
    float rotate;
}BBOX_T;

typedef struct POST_PROC_DET_STACK
{
    BBOX_T  bbox;
    int     batch_index;
    int     box_index;
    float   prob;
} POST_PROC_DET_STACK_T;

/**@fn         nms_comparator
*  @brief      create the comparator of qsort 
*  @param[in]  const void           *pa first  compare object
*              const void           *pb second compare object
*  @param[out] 
*  @return     status
*  @see
*/
static int nms_comparator(const void *pa, const void *pb)
{
    POST_PROC_DET_STACK_T a = *(POST_PROC_DET_STACK_T *)pa;
    POST_PROC_DET_STACK_T b = *(POST_PROC_DET_STACK_T *)pb;

    float diff = a.prob - b.prob;
    if (diff < 0)
    {
        return 1;
    }
    else if (diff > 0)
    {
        return -1;
    }
    return 0;
}


#define DSL_SORT_VALUE_TYPE POST_PROC_DET_STACK_T
#define DSL_SORT_COMP_PTR_FUNC(a_ptr, b_ptr) nms_comparator(a_ptr, b_ptr)
#define DSL_SORT_SUFFIX descend
#include "dsl_sort.h"
// ->_DSL_SORT_POST_PROC_DET_STACK_T_descend


static int nms_reshape(int in1_n, int in1_c, int in1_h,
                int in2_n, int in2_c, int in2_h, int max_output_boxes_per_class,
                int *assist_mem_size, int outshape[4]);

static int nms_forward(float *boxes, int in1_n, int in1_c, int in1_h,
                float *scores, int in2_n, int in2_c, int in2_h,
                float *output, int *outshape, int max_output_boxes_per_class,
                float iou_threshold, float score_threshold,
                char *__auxi_mem_addr__);
/**@file     YoloRegion.c
 * @note     2012-2021 HangZhou Hikvision Digital Technology Co., Ltd. All Right
 * Reserved.
 * @brief    internal implementation of yolo process module in custom layer
 * standard
 *
 * @author   guixinzhe
 * @date     2021/02/04
 * @version  V01.0.0
 *
 * @note     Initial Draft
 */

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include "nms.h"


/**@fn         box_overlap
*  @brief      compute the overlap length of two box
*  @param[in]  float                *x1 x/y coordinate of first box
*              float                *w1 width/height of first box
*              float                *x2 x/y coordinate of second box
*              float                *w2 width/height of second box
*  @param[out] 
*  @return      float
*  @see
*/
static float box_overlap(float x1,
                         float w1,
                         float x2,
                         float w2)
{
    float l1 = x1;
    float l2 = x2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1;
    float r2 = x2 + w2;
    float right = r1 < r2 ? r1 : r2;

    return right - left;
}

/**@fn         box_intersection   
*  @brief      compute intersection size of two box
*  @param[in]  BBOX_T                 bbox1 first box
*              BBOX_T                 bbox2 second box
*  @param[out]
*  @return      float
*  @see
*/
static float box_inter(BBOX_T bbox1, BBOX_T bbox2)
{
    float w = box_overlap(bbox1.x, bbox1.w, bbox2.x, bbox2.w); ///< x direction overlap length
    float h = box_overlap(bbox1.y, bbox1.h, bbox2.y, bbox2.h); ///< y direction overlap lenght
    if (w < 0 || h < 0)
        return 0;
    float area = w * h;

    return area;
}

/**@fn         box_union
*  @brief      compute union size of two box
*  @param[in]  BBOX_T                 bbox1 first box
*              BBOX_T                 bbox2 second box
*  @param[out]
*  @return     float
*  @see
*/
static float box_union(BBOX_T bbox1, BBOX_T bbox2)
{
    float i = box_inter(bbox1, bbox2);
    float u = bbox1.w * bbox1.h + bbox2.w * bbox2.h - i;

    return u;
}

/**@fn         get_covariance_matrix
*/
static int get_covariance_matrix(BBOX_T bbox, float* res)
{
    float a = bbox.w * bbox.w / 12;
    float b = bbox.h * bbox.h / 12;
    float cos2 = cos(bbox.rotate) * cos(bbox.rotate);
    float sin2 = sin(bbox.rotate) * sin(bbox.rotate);

    res[0] = a * cos2 + b * sin2;
    res[1] = a * sin2 + b * cos2;
    res[2] = (a - b) * cos(bbox.rotate) * sin(bbox.rotate);

    return 0;
}

/**@fn         box_iou
*  @brief      compute iou size of two box
*  @param[in]  BBOX_T                 bbox1 first box
*              BBOX_T                 bbox2 second box
*  @param[out]
*  @return     float                    iou
*  @see
*/
static float box_iou(BBOX_T bbox1, BBOX_T bbox2)
{
    if ((bbox1.rotate == 0) && (bbox2.rotate == 0))
    {
        return box_inter(bbox1, bbox2) / box_union(bbox1, bbox2);
    }
    else
    {
        float res1[3] = {0};
        float res2[3] = {0};
        float y1 = bbox1.y + bbox1.h / 2;
        float y2 = bbox2.y + bbox2.h / 2;
        float x1 = bbox1.x + bbox1.w / 2;
        float x2 = bbox2.x + bbox2.w / 2;
        
        get_covariance_matrix(bbox1, res1);
        get_covariance_matrix(bbox2, res2);

        float tmp1 = res1[0] * res1[1] - res1[2] * res1[2];
        float tmp2 = res2[0] * res2[1] - res2[2] * res2[2];

        if (tmp1 < 0)
        {
            tmp1 = 0;
        }
        if (tmp2 < 0)
        {
            tmp2 = 0;
        }

        float t1 = (((res1[0] + res2[0]) * (y1 - y2) * (y1 - y2) + (res1[1] + res2[1]) * (x1 - x2) * (x1 - x2)) /
            ((res1[0] + res2[0]) * (res1[1] + res2[1]) - (res1[2] + res2[2]) * (res1[2] + res2[2]) + DBL_EPSILON)) * 0.25;
        float t2 = (((res1[2] + res2[2]) * (x2 - x1) * (y1 - y2)) / 
            ((res1[0] + res2[0]) * (res1[1] + res2[1]) - (res1[2] + res2[2]) * (res1[2] + res2[2]) + DBL_EPSILON)) * 0.5;
        float t3 = log(((res1[0] + res2[0]) * (res1[1] + res2[1]) - (res1[2] + res2[2]) * (res1[2] + res2[2])) / 
            (4 * sqrt(tmp1 * tmp2) + DBL_EPSILON) + DBL_EPSILON) * 0.5;
        float bd = t1 + t2 + t3;
        if (bd < DBL_EPSILON)
        {
            bd = DBL_EPSILON;
        }
        if (bd > 100)
        {
            bd = 100;
        }
        return (1 - sqrt(1.0 - exp(-bd) + DBL_EPSILON));
    }
}


static void draw_detections_output(POST_PROC_DET_STACK_T **dets, int *real_box_num, int class_num, int max_output_num,
    float thresh, void *output, int *output_num)
{
    int opt_index = 0;
    float* opt = (float*)output;

    for (int class_index = 0; class_index < class_num; class_index++)
    {
        for (int i = 0; i < real_box_num[class_index]; ++i)
        {
            if (dets[class_index][i].prob > thresh)
            {
                opt[opt_index * 3] = dets[class_index][i].batch_index;
                opt[opt_index * 3 + 1] = class_index;
                opt[opt_index * 3 + 2] = dets[class_index][i].box_index;
                opt_index++;
            }
            
            if (opt_index == max_output_num)
            {
                break;
            }
        }
        if (opt_index == max_output_num)
        {
            break;
        }
    }

    *output_num = opt_index;
}


static void do_nms_sort(POST_PROC_DET_STACK_T *dets, int real_box_num, float iou_thresh, float score_thresh)
{
    int i, j, k;
  
    ///< sort detection with nms_comparator
    qsort(dets, real_box_num, sizeof(POST_PROC_DET_STACK_T), nms_comparator);

    for (i = 0; i < real_box_num; ++i)
    {
        BBOX_T bbox1 = dets[i].bbox;
        for (j = i + 1; j < real_box_num; ++j)
        {
            BBOX_T bbox2 = dets[j].bbox;
            if (box_iou(bbox1, bbox2) > iou_thresh)
            {
                dets[j].prob = score_thresh - 1;
            }
        }
    }
}


static int get_region_detections(float *boxes,
                                 float *scores,
                                 int batch_index,
                                 int box_num,
                                 int box_len,
                                 int class_num,
                                 float score_threshold,
                                 int max_output_boxes_per_class,
                                 POST_PROC_DET_STACK_T **dets,
                                 int *real_box_num)
{
    int class_flag = 0;
    int batch_mask = -1;

    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;
    float rotate = 0;

    if (box_len > 4) // batch_mask
    {
        batch_mask = batch_index;
        batch_index = 0;
    }

    for (int box_index = 0; box_index < box_num; box_index++)
    {
        if ((batch_mask >= 0) && (boxes[(batch_index * box_num + box_index) * box_len + box_len - 1] != batch_mask))
        {
            continue;
        }

        class_flag = 0;

        for (int class_index = 0; class_index < class_num; class_index++)
        {
            int real_box_num_this_class = *(real_box_num + class_index);
            if (real_box_num_this_class >= max_output_boxes_per_class)
            {
                continue;
            }
            dets[class_index][real_box_num_this_class].prob = 
                scores[(batch_index * class_num + class_index) * box_num + box_index];
            if (dets[class_index][real_box_num_this_class].prob > score_threshold)
            {
                if (class_flag == 0)
                {
                    class_flag = 1;
                    x = boxes[(batch_index * box_num + box_index) * box_len];
                    y = boxes[(batch_index * box_num + box_index) * box_len + 1];
                    w = boxes[(batch_index * box_num + box_index) * box_len + 2] - x;
                    h = boxes[(batch_index * box_num + box_index) * box_len + 3] - y;
                    if (box_len > 5)
                    {
                        rotate = boxes[(batch_index * box_num + box_index) * box_len + 4];
                    }
                }
                dets[class_index][real_box_num_this_class].bbox.x = x;
                dets[class_index][real_box_num_this_class].bbox.y = y;
                dets[class_index][real_box_num_this_class].bbox.w = w;
                dets[class_index][real_box_num_this_class].bbox.h = h;
                if (box_len > 5)
                {
                    dets[class_index][real_box_num_this_class].bbox.rotate = rotate;
                }
                dets[class_index][real_box_num_this_class].batch_index = batch_index;
                dets[class_index][real_box_num_this_class].box_index = box_index;
                *(real_box_num + class_index) = real_box_num_this_class + 1;
            }
        }
    }
}


static int get_region_detections_parallel(float *boxes,
                                          float *scores,
                                          int batch_index,
                                          int box_num,
                                          int box_len,
                                          int class_num,
                                          float score_threshold,
                                          POST_PROC_DET_STACK_T **dets,
                                          int *real_box_num)
{
    int det_num = 0;
    int class_flag = 0;

    int class_index = batch_index;

    for (int box_index = 0; box_index < box_num; box_index++)
    {
        class_flag = 0;

        int real_box_num_this_class = *(real_box_num + class_index);
        dets[class_index][real_box_num_this_class].prob = 
            scores[batch_index * box_num + box_index];
        if (dets[class_index][real_box_num_this_class].prob > score_threshold)
        {
            dets[class_index][real_box_num_this_class].bbox.x = boxes[(batch_index * box_num + box_index) * box_len];
            dets[class_index][real_box_num_this_class].bbox.y = boxes[(batch_index * box_num + box_index) * box_len + 1];
            dets[class_index][real_box_num_this_class].bbox.w = boxes[(batch_index * box_num + box_index) * box_len + 2] -
                dets[class_index][real_box_num_this_class].bbox.x;
            dets[class_index][real_box_num_this_class].bbox.h = boxes[(batch_index * box_num + box_index) * box_len + 3] -
                dets[class_index][real_box_num_this_class].bbox.y;
            dets[class_index][real_box_num_this_class].batch_index = batch_index;
            dets[class_index][real_box_num_this_class].box_index = box_index;
            *(real_box_num + class_index) = real_box_num_this_class + 1;
        }
    }
    return det_num;
}


static int nms_reshape(int in1_n, int in1_c, int in1_h,
                int in2_n, int in2_c, int in2_h, int max_output_boxes_per_class,
                int *assist_mem_size, int outshape[2])
{
    int tmp = 0;
    if (in1_c > in2_c * max_output_boxes_per_class)
    {
        outshape[0] = in1_n * in2_c * max_output_boxes_per_class;
    }
    else
    {
        outshape[0] = in1_n * in1_c;
    }
    outshape[1] = 3;
    //outshape[2] = 1;
    //outshape[3] = 1;
    *assist_mem_size = sizeof(POST_PROC_DET_STACK_T) * in1_n * in1_c * in2_c;

    return 0;
}


static int nms_forward(float *boxes, int in1_n, int in1_c, int in1_h,
                float *scores, int in2_n, int in2_c, int in2_h,
                float *output, int *outshape, int max_output_boxes_per_class,
                float iou_threshold, float score_threshold,
                char *__auxi_mem_addr__)
{
    int batch_num = in1_n;
    int box_num = in1_c;
    int box_len = in1_h;
    int class_num = in2_c;
    int max_output_num = in1_n * in1_c;
    int real_box_num[MAX_CLASS_NUM] = {0};
    POST_PROC_DET_STACK_T *detections_data[MAX_CLASS_NUM] = {NULL};
    int output_num = 0;

    if (max_output_num < 1000)
    {
        max_output_num = max_output_num * 2;
    }
    else
    {
        max_output_num = max_output_num + 1000;
    }

    for (int class_index = 0; class_index < class_num; class_index++)
    {
        detections_data[class_index] = (POST_PROC_DET_STACK_T *)__auxi_mem_addr__ + class_index * batch_num * box_num;
        int mem_size = sizeof(POST_PROC_DET_STACK_T) * batch_num * box_num;
        memset(detections_data[class_index], 0, mem_size);
    }

    if (box_len > 4)
    {
        for (int box_index = 0; box_index < box_num; box_index++)
        {
            if (batch_num < boxes[box_index * box_len + box_len - 1] + 1)
            {
                batch_num = boxes[box_index * box_len + box_len - 1] + 1;
            }
        }
    }

    for (int batch_index = 0; batch_index < batch_num; batch_index++)
    {
        if (in1_n == in2_n)
        {
            ///< get all detection result
            get_region_detections(boxes, scores, batch_index, box_num, box_len, class_num,
                score_threshold, max_output_boxes_per_class, detections_data, real_box_num);
        }
        else // dllp parallel nms layer
        {
            ///< get all detection result
            get_region_detections_parallel(boxes, scores, batch_index, box_num, box_len, class_num,
                score_threshold, detections_data, real_box_num);
        }
        ///< nms sort for all the detection
        for (int class_index = 0; class_index < class_num; class_index++)
        {
            if (real_box_num[class_index] != 0)
            {
                do_nms_sort(detections_data[class_index], real_box_num[class_index],
                    iou_threshold, score_threshold);
            }
        }
    }

    draw_detections_output(detections_data, real_box_num, class_num, max_output_num, score_threshold,
        output, &output_num);

    outshape[0] = output_num;///< final output detection box num
    outshape[1] = 3;
    //outshape[2] = 1;
    //outshape[3] = 1;

    return 0;
}
                
                
#ifdef __cplusplus
}
#endif

#endif