/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         stack_mng.h
* @brief        stack_mng interface
*
* @date         2023-8-24
* @version      2.0.0
* @note         1. supports deinitialization function register for directly releasing resource
*               2. set print level
*****************************************************************************/

#ifndef _STACK_MNG_H_
#define _STACK_MNG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief  return value
*/
#define STACK_MNG_OK                        (0)         /*!< return ok */
#define STACK_MNG_ERR_FAILED                (-1)        /*!< return ok failed */

#define STACK_MNG_DEEP                      (200)       /*!< deep in STACK_MNG_PROC */

#define STACK_MNG_MEM_INFO_MAX_LEN          (200)       /*!< memory store deep */
#define STACK_MNG_MEM_CHAR_MAX_LEN          (12)        /*!< maximum char length */

/**
* @brief  malloc memory
*/
#define STACK_MNG_SIZE_ALIGN(size, align)   (((size) + ((align) - 1)) & (~((align) - 1)))
#define STACK_MNG_SIZE_ALIGN_16(size)       STACK_MNG_SIZE_ALIGN(size, 16)
#define STACK_MNG_PAGE_ALIGN                (0x1000)

/**
* @brief  deintialization callback funtion
*/
typedef struct _STACK_MNG_FUNC_
{
	void *arg;                                          /*!< user param ,this paran can't be in stack */
	int (*release_call_back)        (void *arg);        /*!< callback with pointer param */
	int (*release_call_back_ex)     (void);             /*!< callback without param */
    
}STACK_MNG_FUNC;

/**
* @brief  stack records the callback funtions registered
*/
typedef struct _STACK_MNG_PROC_
{	
	int                 top;                            /*!< put counts */
	STACK_MNG_FUNC      mng[STACK_MNG_DEEP];            /*!< every callback */
}STACK_MNG_PROC;

/**
* @brief  calcalate the cost time,and printf the value if it has beyonded the _threshold_us_
*/
#define _stack_mng_proc_time_(_func_, _threshold_us_)	\
	{	\
		unsigned long long _time_start_ = stack_mng_get_time();	\
		_func_;	\
		unsigned long long _time_end_ = stack_mng_get_time();	\
		if(_threshold_us_ <= _time_end_-_time_start_) \
			printf("<%lld>%s:%d: %s time cost: %lldus\n",_time_end_, __FILE__, __LINE__, #_func_, _time_end_-_time_start_); \
	}

/**
* @brief memory information allocated
*/
typedef struct _STACK_MNG_MEM_INFO_
{
    void    *vaddr;     /*!< visual address */
    void    *paddr;     /*!< physical address */
    int     size;       /*!< memory used */
    int     type;       /*!< os memory or cmm memory */
    int     tag;        /*!< memory status,such as used,free,err */
    int     proc_type;  /*!< process type ,alloc or free */
    char    name[12];   /*!< memory name */
}STACK_MNG_MEM_INFO;

/**
* @brief  memory register function
*/
typedef int (*stack_mng_mem_fxn)(STACK_MNG_MEM_INFO *mem);

/**
 * @brief  Enumerations of memory type.
 */
typedef enum _STACK_MNG_MEM_TYPE_
{
    STACK_MNG_MEM_TYPE_OS      = 0,     /*!< os memory */
    STACK_MNG_MEM_TYPE_CMM     = 1,     /*!< cmm memory */
    STACK_MNG_MEM_TYPE_MAX_NUM = 2
} STACK_MNG_MEM_TYPE;

/**
 * @brief  Enumerations of memory process type.
 */
typedef enum _STACK_MNG_MEM_PROC_TYPE_
{
    STACK_MNG_MEM_PROC_TYPE_ALLOC   = 0,    /*!< to allocate */
    STACK_MNG_MEM_PROC_TYPE_FREE    = 1,    /*!< to free */
    STACK_MNG_MEM_PROC_TYPE_MAX_IDX = 2
} STACK_MNG_MEM_PROC_TYPE;

/**
 * @brief  Enumerations of memory type.
 */
typedef enum _STACK_MNG_MEM_TAG_
{
    STACK_MNG_MEM_TAG_FREE      = 0,    /*!< memory has been free,err status */
    STACK_MNG_MEM_TAG_USED      = 1,    /*!< memory has been used,success status */
    STACK_MNG_MEM_TAG_ERR       = 2,    /*!< to free memory failed,err status */
    STACK_MNG_MEM_TAG_MAX_NUM   = 3
} STACK_MNG_MEM_TAG;

/**
* @brief memory user alloc or free function  for every memory type 
*/
typedef struct _STACK_MNG_MEM_FUNC_REG_
{
    stack_mng_mem_fxn     os_fxn;       /*!< memory user alloc or free function  for os memory */   
    stack_mng_mem_fxn     cmm_fxn;      /*!< memory user alloc or free function  for cmm memory */   
}STACK_MNG_MEM_FUNC_REG;

/**
* @brief memory statistics for every memory type
*/
typedef struct _STACK_MNG_MEM_STATISTICS_
{
    void                    *start_vaddr;       /*!< start visual address for every memory type */ 
    void                    *start_paddr;       /*!< start physical address for every memory type */   
    int                     type;               /*!< see STACK_MNG_MEM_TYPE */ 
    unsigned long long      max_size;           /*!< memory maximum size allocated */ 
    unsigned long long      pre_alloc;          /*!< memory size will to allocate */ 
    unsigned long long      alloc_size;         /*!< memory size  allocated now */ 
    unsigned long long      faild_alloc;        /*!< memory size allocated failed,now */ 
    unsigned int            alloc_suc_cnt;      /*!< allocate success times */ 
    unsigned int            alloc_faild_cnt;    /*!< allocate failed times */ 
    unsigned int            release_suc_cnt;    /*!< release success times */ 
    unsigned int            release_failed_cnt; /*!< allocate failed times */ 
    stack_mng_mem_fxn       fxn;                /*!< callback funtion */

}STACK_MNG_MEM_STATISTICS;

/**
* @brief  memory ctrl
*/
typedef struct _STACK_MNG_MEM_CTRL_
{
    STACK_MNG_MEM_INFO          mem_info[STACK_MNG_MEM_INFO_MAX_LEN];       /*!< memory information allocated in every time */
    int                         used_cnt;                                   /*!< used idx */
    STACK_MNG_MEM_STATISTICS    statistics[STACK_MNG_MEM_TYPE_MAX_NUM];     /*!< statistics for every memory type */
    pthread_mutex_t             mutex;                                      /*!< lock for statistics */
    int                         init_flg;                                   /*!< module initialzation flag */
}STACK_MNG_MEM_CTRL;


/** 
* @brief            call  all register functions  for release resource 
*
* @param[in] 		none     
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_deinit();

/** 
* @brief            push a function to stack
*
* @param[in] 		arg             pointer of param     
* @param[in] 		func            callback function     
* 
* @return           0 if successful, otherwise an error number returned
*/
void stack_mng_push(void *arg,void *func);

/** 
* @brief            write file 
*
* @param[in] 		file            file path     
* @param[in] 		pBuf            data buffer address     
* @param[in] 		len             data length
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_write_file(const char *file,void *pBuf,int len);

/** 
* @brief            close file-release buffer 
*
* @param[in] 		buf             buffer address getted from stack_mng_open_file  
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_close_file(void *buf);

/** 
* @brief            read data from file  
*
* @param[in] 		file            file path     
* @param[out] 		pBuf            data buffer address     
* @param[out] 		len             data length
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_open_file(char *file,char **buf,int *len);

/** 
* @brief            get ns
*
* @param[in] 		none     
* 
* @return           ns if successful, otherwise an error number returned
*/
unsigned long long stack_mng_get_time(void);

/** 
* @brief            set print level 
*
* @param[in] 		arg             pointer of handle     
* @param[in] 		argc            number of input parameters     
* @param[in] 		argv[]          eEach parameter pointer set    
* 
* @return           0 if successful, otherwise an error number returned
*/
void stack_mng_set_prt_lvl(void *arg,int argc, char *argv[]);

/** 
* @brief            initialize the memory recode module  
*
* @param[in] 		reg             memory register callback     
* @param[out] 		none            
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_mem_init(STACK_MNG_MEM_FUNC_REG *reg);

/** 
* @brief            get memory,must pay attention on size, type and proc_type
*
* @param[in/out]    mem_info        memory requied
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_mem_get(STACK_MNG_MEM_INFO *mem_info);

/** 
* @brief            release memory,must pay attention on vaddr,paddr, type and proc_type
*
* @param[in/out]    mem_info        memory will be free
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_mem_release(STACK_MNG_MEM_INFO *mem_info);

/** 
* @brief            de-initialize the memory recode module  
*
* @param[in] 		none     
* @param[out] 		none            
* 
* @return           0 if successful, otherwise an error number returned
*/
int stack_mng_mem_deinit();

/** 
* @brief            print the memory recode module  
*
* @param[in] 		none     
* @param[out] 		none            
* 
* @return           0 if successful, otherwise an error number returned
*/
void stack_mng_mem_stat_prt();

#ifdef __cplusplus
}
#endif

#endif

