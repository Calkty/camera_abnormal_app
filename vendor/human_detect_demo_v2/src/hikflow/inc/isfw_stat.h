/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         isfw_stat.h
* @brief        isfw stat headfile
*
* @date         2022/10/21
* @version      1.0.0
* @note         added isfw status module 
*****************************************************************************/

#ifndef _ISFW_STAT_H_
#define _ISFW_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "isfw_public_def.h"

#define ISFW_STAT_OK             (ISFW_PUBLIC_SUCCESS)
#define ISFW_STAT_FAILED         (ISFW_PUBLIC_FAILURE)
#define ISFW_STAT_MAX_CNT		 (64)

/**
* @brief  other definitions of ISFW_STAT module
*/
#define ISFW_STAT_MAX_CHAR_LEN	        (256)       /*!< maximum string length */
#define ISFW_STAT_PROC_HIST_POW_NUM	    (24)		/*!< The default number of exponential histograms */
#define ISFW_STAT_PROC_LVL_NUM	        (16)		/*!< The maximum number of stages for the cache */
#define ISFW_STAT_PROC_DEF_STOP_TIME    (1000*1000) /*!< The unit is us, if there are no new statistics beyond this time, the cur_fps will be cleared to zero in the proc_stat_print */
#define ISFW_STAT_PROC_CNT_OVF		    (200000000)
#define ISFW_STAT_MAX_SIZE		        (1*1024)    /*!< The module supports up to 1M memory */

typedef void (* isfw_stat_proc_fxn)(void *arg, int argc, char *argv[]);

/**
* @brief  Print callback functionality is implemented by the external module 
*/
typedef void (* isfw_stat_cb_fxn)(char *modulename,char *buf,int size);

typedef struct _ISFW_STAT_PROC_CACHE_
{
	char        cnt[ISFW_STAT_PROC_LVL_NUM];
	long long   start[ISFW_STAT_PROC_LVL_NUM];
	double      val[ISFW_STAT_PROC_LVL_NUM];
}ISFW_STAT_PROC_CACHE;

typedef struct _ISFW_STAT_PROC_STAT_
{
	unsigned long long      start;				    /*!< Intermediate variable: The start time of a process */
	unsigned long long      end;			        /*!< Intermediate variable: The end time of a process */
	double                  sum;			        /*!< Intermediate variable: The sum of the processing time */
	double                  square_sum;		        /*!< Intermediate variable: The sum of squares of the processing time */
	int                     max;			        /*!< Statistical: The longest processing time */
	int                     min;			        /*!< Statistical value: Minimum processing time plus one */
	int                     cnt;			        /*!< Statistical value: The number of times processed */
	int                     ovf;			        /*!< Statistical value: The number of processing times overflowed */
    ISFW_STAT_PROC_CACHE    cache;
	double                  avg;			        /*!< Statistical value: Average processing time */
	double                  sgm;			        /*!< Statistical value: The standard deviation of the processing time */
	float                   cur_fps;		        /*!< Statistical value: Short-term (last 20) processing frequency */
	float                   cur_val;		        /*!< Statistical value: Short-term (last 20) average */
	float                   lng_fps;		        /*!< Statistical value: Average processing frequency in the medium term (last 10,000) */
	float                   lng_val;		        /*!< Statistical value: medium-term (last 10,000) average */
	int                     hist_min;			    /*!< Configuration parameters: The minimum value of the user histogram */
	int                     hist_grid;			    /*!< Configuration parameter: The interval of the user histogram */
	int hist_inner[ISFW_STAT_PROC_HIST_POW_NUM];	/*!< Statistical value: Internal default exponential histogram */

}ISFW_STAT_PROC_STAT;		

typedef struct _ISFW_STAT_REG_ST_
{
	isfw_stat_proc_fxn fxn;    	    /*!< The state executes the function */
	void               *arg;		/*!< Customize passthrough parameters */
	int                argc;		/*!< Number of parameters */
	char               *desc;		/*!< A description of the function of the command  */
	char               *usage;		/*!< A description of the usage of the command */
	char               *example;	/*!< An example of a command */
}ISFW_STAT_REG_ST;

/** 
* @brief            the entry interface for cost statistics  
*           
* @param[in]        stat            State variables
*
* @param[out]       none
* 
* @return           none
*/
void isfw_stat_time_enter(ISFW_STAT_PROC_STAT *stat);

/** 
* @brief            the exit interface for cost statistics
*           
* @param[in]        stat            State variables
*
* @param[out]       none
* 
* @return           none
*/
void isfw_stat_time_exit(ISFW_STAT_PROC_STAT *stat);

/** 
* @brief            numerical statistics
*           
* @param[in]        stat            State
* @param[in]        val             The updated value
*
* @param[out]       none
* 
* @return           none
*/
void isfw_stat_value_record(ISFW_STAT_PROC_STAT * stat, int val);

/** 
* @brief            statistical printing
*           
* @param[in]        stat            The status that needs to be printed,which can be multiple groups
* @param[in]        modulename      Status name
* @param[in]        cnt             Number of states
*
* @param[out]       none
* 
* @return           none
*/
void isfw_stat_proc_stat_print(ISFW_STAT_PROC_STAT **stat, char **modulename, int cnt);

/** 
* @brief            the registration interface for the status module 
*           
* @param[in]        modulename      Status name
* @param[in]        reg_param       Registration parameters
*
* @param[out]       none
* 
* @return           0 if successful, otherwise an error number returned
*/
int isfw_stat_register(char *modulename, ISFW_STAT_REG_ST *reg_param);

/** 
* @brief            the anti-registration interface for the status module 
*           
* @param[in]        modulename      Status name
*
* @param[out]       none
* 
* @return           0 if successful, otherwise an error number returned
*/
int isfw_stat_unregister(char *modulename);

/** 
* @brief            status module initialization
*           
* @param[in]        fxn          Callback function,which will receive the status information
* @param[in]        size         Greater than 0 indicates that the total memory size is limited, and the maximum value is ISFW_STAT_MAX_SIZE in KB 
*
* @param[out]       none
* 
* @return           0 if successful, otherwise an error number returned
*/
int isfw_stat_init(isfw_stat_cb_fxn fxn,int size);

/** 
* @brief            the STAT module deinitialization
*           
* @param[in]        none
*
* @param[out]       none
* 
* @return           none
*/
void isfw_stat_deinit(void);

/** 
* @brief            the process status command
*           
* @param[in]        argc            Number of parameters:NULL-help,ALL-all
* @param[in]        argv            Specific parameters:NULL-help,ALL-all.These can be set to individual status modules
*
* @param[out]       none
* 
* @return           0 if successful, otherwise an error number returned
*/
int isfw_stat_proc_cmd(int argc, char *argv[]);

/** 
* @brief            get print data 
*           
* @param[in]        argc            Number of parameters:NULL-help,ALL-all
* @param[in]        argv            Specific parameters:NULL-help,ALL-all param.These can be set to individual status modules
*
* @param[out]       rev_buf         Receive buff addresses
* @param[out]       rev_size        Receive buff size
* 
* @return           0 if successful, otherwise an error number returned
*/
int isfw_stat_proc_get_buf(int argc, char *argv[],char **rev_buf,unsigned int *rev_size);

/** 
* @brief            release the cache
*           
* @param[in]        rev_buf         Cache buff addresses
* @param[in]        rev_size        Cache content size
*
* @param[out]       none
* 
* @return           0 if successful, otherwise an error number returned
*/
int isfw_stat_proc_release_buf(char *rev_buf,unsigned int rev_size);

#ifdef __cplusplus
}
#endif

#endif

