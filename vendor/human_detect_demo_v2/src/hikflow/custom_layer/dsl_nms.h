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
    float   prob[MAX_CLASS_NUM];
    int     sort_class;
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
    float diff = 0;

    ///< compare class prob if clasee is sorted
    if (b.sort_class >= 0)
    {
        diff = a.prob[b.sort_class] - b.prob[b.sort_class];
    }
    ///< compare with object prob
    else
    {
        diff = 0;
    }
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


#define SMALLEST_SCORE 0.001


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


static void draw_detections_output(POST_PROC_DET_STACK_T *dets, int num, int max_output_boxes_per_class, 
    float thresh, int classes, void *output, int *output_num, int max_output_num)
{
    float* opt = (float*)output;
    int boxes_count[MAX_CLASS_NUM] = {0};

    int opt_index = 0;
    int i = 0;
    int j = 0;

    *output_num = 0;

    for (i = 0; i < num; ++i)
    {
        for (j = 0; j < classes; j++)
        {
            if ((boxes_count[j] < max_output_boxes_per_class) && (dets[i].prob[j] > thresh))
            {
                opt[opt_index * 3] = dets[i].batch_index;
                opt[opt_index * 3 + 1] = j;
                opt[opt_index * 3 + 2] = dets[i].box_index;
                boxes_count[j]++;
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


static void do_nms_sort(POST_PROC_DET_STACK_T *dets, int total, int classes, float iou_thresh, float score_thresh,
    int *class_activate)
{
    int i, j, k;

    for (k = 0; k < classes; ++k)
    {
        if (*(class_activate + k) == 1)
        {
            ///< init sort class
            for (i = 0; i < total; ++i)
            {
                dets[i].sort_class = k;
            }
            ///< sort detection with nms_comparator
            qsort(dets, total, sizeof(POST_PROC_DET_STACK_T), nms_comparator);
            //_dsl_partial_sort_POST_PROC_DET_STACK_T_descend((POST_PROC_DET_STACK_T*)dets, (POST_PROC_DET_STACK_T*)dets + total, (POST_PROC_DET_STACK_T*)dets + total);

            for (i = 0; i < total; ++i)
            {
                if (dets[i].prob[k] <= score_thresh)
                {
                    continue;
                }
                BBOX_T bbox1 = dets[i].bbox;
                for (j = i + 1; j < total; ++j)
                {
                    BBOX_T bbox2 = dets[j].bbox;
                    if (box_iou(bbox1, bbox2) > iou_thresh)
                    {
                        dets[j].prob[k] = score_thresh - 1;
                    }
                }
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
                                 POST_PROC_DET_STACK_T *dets,
                                 int *class_activate)
{
    int det_num = 0;
    int class_flag = 0;
    int batch_mask = -1;

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
            dets[det_num].prob[class_index] = scores[(batch_index * class_num + class_index) * box_num + box_index];
            if (dets[det_num].prob[class_index] > score_threshold)
            {
                class_flag = 1;
                *(class_activate + class_index) = 1;
            }
        }

        if (class_flag)
        {
            dets[det_num].bbox.x = boxes[(batch_index * box_num + box_index) * box_len];
            dets[det_num].bbox.y = boxes[(batch_index * box_num + box_index) * box_len + 1];
            dets[det_num].bbox.w = boxes[(batch_index * box_num + box_index) * box_len + 2] - dets[det_num].bbox.x;
            dets[det_num].bbox.h = boxes[(batch_index * box_num + box_index) * box_len + 3] - dets[det_num].bbox.y;
            if (box_len > 5)
            {
                dets[det_num].bbox.rotate = boxes[(batch_index * box_num + box_index) * box_len + 4];
            }
            dets[det_num].batch_index = batch_index;
            dets[det_num].box_index = box_index;

            det_num++;
        }
    }
    return det_num;
}


static int get_region_detections_parallel(float *boxes,
                                          float *scores,
                                          int batch_index,
                                          int box_num,
                                          int box_len,
                                          int class_num,
                                          float score_threshold,
                                          POST_PROC_DET_STACK_T *dets)
{
    int det_num = 0;
    int class_flag = 0;

    for (int box_index = 0; box_index < box_num; box_index++)
    {
        class_flag = 0;

        dets[det_num].prob[batch_index] = scores[(0 * class_num + batch_index) * box_num + box_index];
        if (dets[det_num].prob[batch_index] > score_threshold)
        {
            class_flag = 1;
        }

        if (class_flag)
        {
            dets[det_num].bbox.x = boxes[(batch_index * box_num + box_index) * box_len];
            dets[det_num].bbox.y = boxes[(batch_index * box_num + box_index) * box_len + 1];
            dets[det_num].bbox.w = boxes[(batch_index * box_num + box_index) * box_len + 2] - dets[det_num].bbox.x;
            dets[det_num].bbox.h = boxes[(batch_index * box_num + box_index) * box_len + 3] - dets[det_num].bbox.y;
            dets[det_num].batch_index = batch_index;
            dets[det_num].box_index = box_index;

            det_num++;
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
    *assist_mem_size = sizeof(POST_PROC_DET_STACK_T) * in1_n * in1_c;

    return 0;
}


static int nms_forward(float *boxes, int in1_n, int in1_c, int in1_h,
                float *scores, int in2_n, int in2_c, int in2_h,
                float *output, int *outshape, int max_output_boxes_per_class,
                float iou_threshold, float score_threshold,
                char *__auxi_mem_addr__)
{
    int batch_num = in1_n;
    int output_num = 0;

    POST_PROC_DET_STACK_T *detections_data = NULL;

    int box_num = in1_c;
    int box_len = in1_h;
    int class_num = in2_c;
    int bbx_num = 0;
    int total_bbx_num = 0;
    int max_output_num = in1_n * in1_c;

    if (max_output_num < 1000)
    {
        max_output_num = max_output_num * 2;
    }
    else
    {
        max_output_num = max_output_num + 1000;
    }
    

    detections_data = (POST_PROC_DET_STACK_T *)__auxi_mem_addr__;
    int mem_size = sizeof(POST_PROC_DET_STACK_T) * box_num;
    memset(detections_data, 0, mem_size); ///< clear last result

    int class_activate[MAX_CLASS_NUM];
    memset(class_activate, 0, MAX_CLASS_NUM * sizeof(int));

    /*if (score_threshold < SMALLEST_SCORE) // risk
    {
        score_threshold = SMALLEST_SCORE;
    }*/

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
            bbx_num = get_region_detections(boxes, scores, batch_index, box_num, box_len, class_num,
                score_threshold, detections_data, class_activate);

            ///< nms sort for all the detection
            do_nms_sort((POST_PROC_DET_STACK_T *)detections_data, bbx_num, class_num,
                iou_threshold, score_threshold, class_activate);
        }
        else // dllp parallel nms layer
        {
            ///< get all detection result
            bbx_num = get_region_detections_parallel(boxes, scores, batch_index, box_num, box_len, class_num,
                score_threshold, detections_data);

            ///< nms sort for all the detection
            for (int m = 0; m < MAX_CLASS_NUM; m++)
            {
                *(class_activate + m) = 1;
            }
            do_nms_sort((POST_PROC_DET_STACK_T *)detections_data, bbx_num, class_num,
                iou_threshold, score_threshold, class_activate);
        }
        total_bbx_num = total_bbx_num + bbx_num;
        detections_data = detections_data + bbx_num;
    }

    // set to origin place
    detections_data = (POST_PROC_DET_STACK_T *)__auxi_mem_addr__;
    draw_detections_output((POST_PROC_DET_STACK_T *)detections_data, total_bbx_num, max_output_boxes_per_class,
        score_threshold, class_num, output, &output_num, max_output_num);

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