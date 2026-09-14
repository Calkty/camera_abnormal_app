/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         hikflow_demo.h
* @brief        hikflow demo interface
*          
* @author       heopers
* @date         2023-8-30
* @version      2.0.0
* @note         1. supports hikflow demo initialization with 2 input parameters: execution path, and camera/file-reader read mode
*               2. thread TSK_hikflow_destroy will calls de-initialization function without opening it to the public
*               3. supports configure rules to the demo, such as enable, rule and alarm image size
*****************************************************************************/

#ifndef _HIKFLOW_DEMO_H
#define _HIKFLOW_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief  hikflow_demo return value
*/
#define HIKFLOW_DEMO_OK                     (0)                 /*!< return ok */
#define HIKFLOW_DEMO_ERR_NULL_PTR           (-1)                /*!< invalid pointer */
#define HIKFLOW_DEMO_ERR_INV_PARAM          (-2)                /*!< invalid param */
#define HIKFLOW_DEMO_ERR_FAILED             (-1)                /*!< other error */

#define HIKFLOW_DEMO_INPUT_PARAM_NUM        (3)                 /*!< number of input parameters for the main function, including the execution path and camera/file-reader mode */
#define HIKFLOW_DEMO_MAX_POINT_NUM          (10)                /*!< The maximum number of vertices that can be set by the rule, see HIKFLOW_DEMO_POLYGON structure */

/**
* @brief  the configurable parameter types supported by hikflow_demo
*/
typedef enum _HIKFLOW_DEMO_PARAM_SET_TYPE_
{
    HIKFLOW_DEMO_PARAM_SET_TYPE_EN       	= 0,                /*!< intelligent enable type, supporting on and off */
    HIKFLOW_DEMO_PARAM_SET_TYPE_RULE        = 1,                /*!< intelligent rule type, supports setting rules */
    HIKFLOW_DEMO_PARAM_SET_TYPE_ALARM_IMAG  = 2,                /*!< intelligent alarm jpeg image parameter type, supporting setting the width, height, and quality of the alarm image */
}HIKFLOW_DEMO_PARAM_SET_TYPE;

/**
* @brief  point coordinates, with the upper left corner as origin
*/
typedef struct _HIKFLOW_DEMO_POINT_
{
    unsigned short      x;                                      /*!< x-axis coordinate, value range:0-1000 */
    unsigned short      y;                                      /*!< y-axis coordinate, value range:0-1000 */
}HIKFLOW_DEMO_POINT;

/**
* @brief  points regular structure
*/
typedef struct _HIKFLOW_DEMO_POLYGON_
{
    unsigned int        point_num;                              /*!< number of regular fixed points,the maximum value is HIKFLOW_DEMO_MAX_POINT_NUM */           		
    HIKFLOW_DEMO_POINT  point[HIKFLOW_DEMO_MAX_POINT_NUM];      /*!< points regular vertex coordinate set */ 
}HIKFLOW_DEMO_POLYGON;

/**
* @brief  alarm jpeg image parameters
*/
typedef struct _HIKFLOW_DEMO_ALARM_IMAGE_INFO_
{
    unsigned int  width;                                        /*!< jpeg image width,unit: pixel */
    unsigned int  height;                                       /*!< jpeg image height,unit: pixels */
    unsigned int  quality;                                      /*!< jpeg image quality, value range: (0,100), the larger the value, the smaller the image compression ratio and the higher the quality */
}HIKFLOW_DEMO_ALARM_IMAGE_INFO;

/** 
* @brief            hikflow demo initialization
*
* @param[in] 		argc    number of input parameters     
* @param[in] 		argv[]  eEach parameter pointer set    
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_demo_init(int argc, char *argv[]);

/** 
* @brief            hikflow demo set parameters function
*
* @param[in] 		param   input parameter pointer    
* @param[in] 		type    type of parameter,see HIKFLOW_DEMO_PARAM_SET_TYPE    
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_demo_param_set(void* param, int type);

/** 
* @brief            hikflow demo check input params
*
* @param[in] 		argc    number of input parameters     
* @param[in] 		argv[]  eEach parameter pointer set    
* 
* @return           0 if successful, otherwise an error number returned
*/
int hikflow_demo_param_check(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
