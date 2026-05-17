#ifndef BSP_TRACK_H_
#define BSP_TRACK_H_

#include <stdint.h>

/**
 * 8 路灰度循迹模块（3 线地址选通 + 单线 OUT），逻辑与 track_test/track_sensor.c 一致。
 * 引脚（与 ra_gen/pin_data.c 一致）：
 *   P002 → AD0，P001 → AD1，P000 → AD2（GPIO 输出）
 *   P013 → OUT（数字输入，上拉）
 * 通道：ch 0…7 从左到右对应传感器 1…8；地址 = AD2:AD1:AD0 二进制。
 * 电平：黑线 / 检测到 → OUT 高 → 读数为 1。
 */

#define TRACK_SENSOR_NUM 8U

#ifndef TRACK_MUX_DELAY_US
/** 地址切换后稳定时间（µs），与 STM32 track_test 中 TRACK_SETTLE_US 一致 */
#define TRACK_MUX_DELAY_US (500U)
#endif

void track_bsp_init(void);

/**
 * 顺序读取 8 路数字量。
 * @param out_levels 长度 8，out_levels[i] 为 ch i 的 0/1（1 = OUT 高）。
 */
void track_bsp_read_all(uint8_t out_levels[TRACK_SENSOR_NUM]);

/**
 * @return 位掩码 bit0…bit7 对应 ch0…ch7，1 表示 OUT 为高。
 */
uint8_t track_bsp_read_mask(void);

/**
 * @param ch 0…7，从左到右
 * @return 0 或 1
 */
uint8_t track_bsp_read_channel(uint8_t ch);

/** 读 OUT 当前电平（不改变地址；宜在 read_all 之后调用） */
uint8_t track_bsp_read_out_raw(void);

#endif /* BSP_TRACK_H_ */
