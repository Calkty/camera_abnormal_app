/****************************************************************************** 
 *  @file audioplay.h
 *  @note HangZhou Hikvision Digital Technology Co., Ltd. All Rights Reserved.
 *  @brief 
 *
 *  @author   zhouwenjie7@hikvision.com.cn
 *  @date     2022-1-12
 *
 *  @note History:
 *  @note
 ******************************************************************************/
#ifndef __AUDIOPLAY_H_
#define __AUDIOPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

/* @fn int audio_play_init(void)
 * @brief 初始化语音播报接口
 * @param[in] 无
 * @param[in] 无
 * @return 0 正常/-1 异常
 */
int audio_play_init(void);

/* @fn int audio_play(void)
 * @brief 自定义语音播报接口
 * @param[in] 无
 * @param[out] 无
 * @return 0 正常/-1 异常
 */
int audio_play(void);

#ifdef __cplusplus
}
#endif

#endif //__AUDIOPLAY_H_

