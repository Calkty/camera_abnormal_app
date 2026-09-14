/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         isfw_log.h
* @brief        isfw log interface
*
* @date         2022/10/21
* @version      1.0.0
* @note         added the isfw log module 
*****************************************************************************/

#ifndef _ISFW_LOG_H_
#define _ISFW_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "isfw_public_def.h"
#define ISFW_LOG_OK             (ISFW_PUBLIC_SUCCESS)
#define ISFW_LOG_FAILED         (ISFW_PUBLIC_FAILURE)
#define ISFW_LOG_MAX_MDL_NUM    (2000)                  /*!< Maximum number of modules */
#define ISFW_LOG_MAX_THRD_NUM   (4)                     /*!< Getting thread idx values 0, 1, 2, 3 is managed by customer */

/**
* @brief   the print levels supported 
*/
typedef enum _ISFW_LOG_LEVEL_ENUM_
{
    ISFW_LOG_LEVEL_NONE         = 0,    	/*!< Don't output log printing */
	ISFW_LOG_LEVEL_DEBUG        = 1,		/*!< Debugging information */
    ISFW_LOG_LEVEL_INFO         = 2,		/*!< Information of interest */
    ISFW_LOG_LEVEL_WARN         = 3,		/*!< Indicate situations where potential errors occurred. Some information is not error messages, but it is also necessary to give the programmer some hints */
    ISFW_LOG_LEVEL_ERROR        = 4,		/*!< It is pointed out that an error event has occurred, and only ERR level printing is output */
	ISFW_LOG_LEVEL_FATAL        = 5,		/*!< Serious error */
    ISFW_LOG_LEVEL_DPT          = 6,        /*!< Print directly */
    ISFW_LOG_LEVEL_MAX          = 8,        /*!< Maximum value */
}ISFW_LOG_LEVEL_ENUM;

/**
* @brief  dprint file buffer information
*/
typedef struct _ISFW_LOG_DPRINT_FILE_
{
	int                 buflen;             /*!< The length of buffer */
	int                 widx;               /*!< The widx of buffer */
	char                *name;              /*!< The name of module */
	char                *buf;               /*!< The pointer to buffer */
}ISFW_LOG_DPRINT_FILE;

/**
* @brief  directly print file description information
*/
typedef struct _ISFW_LOG_DPRINT_PARAM_
{
    ISFW_LOG_DPRINT_FILE    *file;          /*!< the pointer to file name */
    int                     tid;            /*!< the value of tid.Temporarily replace it with a thread number */
    int                     max_size;       /*!< file max size */
}ISFW_LOG_DPRINT_PARAM;

/**
* @brief  The print callback is implemented by the external module,where can receives the log, the function provides the name, level, file name and line number of logs. 
*/
typedef void (*isfw_log_cb_fxn)(const char *modulename, ISFW_LOG_LEVEL_ENUM level, const char *filename, const int line, const char *str);

/** 
* @brief            initialize the dprint file
*           
* @param[in]        file            file descriptor
*
* @param[out]       none
* 
* @return           none
*/
void isfw_log_dprint_file_init(ISFW_LOG_DPRINT_FILE *file);

/** 
* @brief            deinitialize the dprint file
*           
* @param[in]        file            file descriptor
*
* @param[out]       none
* 
* @return           none
*/
void isfw_log_dprint_file_deinit(ISFW_LOG_DPRINT_FILE *file);

/** 
* @brief            directly print without log level
*           
* @param[in]        thread_idx      thread numbers 0, 1, 2, external management
* @param[in]        file            the file specified
* @param[in]        size            maximum memory value
*
* @param[out]       none
* 
* @return           none
*/
void isfw_log_dprint_redirction(unsigned int  thread_idx, ISFW_LOG_DPRINT_FILE *file,int size);

/** 
* @brief            Handling functions without print level are generally used for status printing, debugging printing that is not controlled by the level.
*           
* @param[in]        format          number of variadic parameters 
* @param[in]        ...             variadic parameters
*           
* @param[out]       none
* 
* @return           none
*/
void isfw_log_dprint(char *format, ...);

/** 
* @brief            the initialization of log module 
*           
* @param[in]        none
*           
* @param[out]       none
* 
* @return           0 if successful, otherwise an error number returned
*/
int isfw_log_init();

/** 
* @brief            the deinitialization of log module 
*           
* @param[in]        none
*           
* @param[out]       none
* 
* @return           none
*/
void isfw_log_deinit(void);

/** 
* @brief            print processing functions with hierarchies
*           
* @param[in]        modulename      module name
* @param[in]        level           the print level 
* @param[in]        filename        file name 
* @param[in]        line            line number
* @param[in]        format          number of variadic parameters
* @param[in]        ...             variadic parameters
*           
* @param[out]       none
* 
* @return           none
*/
void isfw_log_print(const char *modulename,ISFW_LOG_LEVEL_ENUM level,  const char *filename, const int line,const char *format, ...);

/** 
* @brief            set the module print level
*           
* @param[in]        modulename      module name
* @param[in]        level           level
*           
* @param[out]       none
* 
* @return           none
*/
void isfw_log_set_level(const char *modulename, ISFW_LOG_LEVEL_ENUM level);

/** 
* @brief            get the module print level
*           
* @param[in]        modulename      module name
*           
* @param[out]       level           level
* 
* @return           none
*/
void isfw_log_get_level(const char *modulename, ISFW_LOG_LEVEL_ENUM *level);

/** 
* @brief            set the print level for all modules
*           
* @param[in]        level           level
*           
* @param[out]       none
* 
* @return           none
*/
void isfw_log_set_level_all(ISFW_LOG_LEVEL_ENUM level);

/** 
* @brief            Set the print callback function,through which the customer can receives the print log and send then to the actual terminals, for example, print out to syslog, or write it to file .
*           
* @param[in]        fxn        Print the callback and return to the higher-level parameters: module name, level, log file name, line number
*           
* @param[out]       none
* 
* @return           none
*/
void isfw_log_set_cb_fxn(isfw_log_cb_fxn fxn);

/** 
* @brief            get the print callback function
*           
* @param[in]        none
*           
* @param[out]       none
* 
* @return           function pointer
*/
isfw_log_cb_fxn isfw_log_get_cb_fxn(void);

#ifdef __cplusplus
}
#endif

#endif

