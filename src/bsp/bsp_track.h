#ifndef BSP_TRACK_H_
#define BSP_TRACK_H_

#include <stdint.h>

/**
 * YB-MVX05-V1.0 类 8 路灰度循迹模块（地址选通 + 单线 OUT）。
 * 引脚（与 ra_gen/pin_data.c 一致）：
 *   P000 → AD0，P001 → AD1，P002 → AD2（输出）
 *   P013 → OUT（数字输入，已配置上拉）
 * 通道：地址二进制 AD2:AD1:AD0 = 0…7 对应传感器 1…8。
 */

#define TRACK_SENSOR_NUM 8U

#ifndef TRACK_MUX_DELAY_US
/** 地址切换后等待多路开关稳定的时间（微秒），可按模块说明微调 */
#define TRACK_MUX_DELAY_US (5U)
#endif

void track_bsp_init(void);

/**
 * 读取 8 路数字量。
 * @param out_levels 长度 8，out_levels[i] 为通道 (i+1) 的 0/1（与 OUT 高/低一致）。
 */
void track_bsp_read_all(uint8_t out_levels[TRACK_SENSOR_NUM]);

/**
 * @return 位掩码：bit0…bit7 对应通道 1…8，1 表示 OUT 为高。
 */
uint8_t track_bsp_read_mask(void);

/**
 * 读取单路。
 * @param ch 0…7 对应传感器 1…8
 * @return 0 或 1
 */
uint8_t track_bsp_read_channel(uint8_t ch);

#endif /* BSP_TRACK_H_ */
