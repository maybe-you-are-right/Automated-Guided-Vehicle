#ifndef APP_UART_H_
#define APP_UART_H_

#include <stdint.h>

/** 1=周期打印 8 路灰度（与 STM32 track_test 同款）；0=关闭 */
#ifndef HAL_TRACK_DEBUG_UART
#define HAL_TRACK_DEBUG_UART (1U)
#endif

/** 灰度周期上报间隔 (ms)，与 STM32 TRACK_REPORT_INTERVAL_MS 同类 */
#ifndef HAL_TRACK_REPORT_INTERVAL_MS
#define HAL_TRACK_REPORT_INTERVAL_MS (2000U)
#endif

/**
 * 串口命令解析（与 bsp_uart 配合）：
 * - STOP / RUN：主循环用 `car_uart_halted_get()` 判断是否停车。
 * - HAL_TRACK_DEBUG_UART=1：上电提示 + 每 HAL_TRACK_REPORT_INTERVAL_MS 发一行
 *   `00110000 D=0`（8 位从左到右，levels[0]=最左；D 为 read_all 后 OUT 脚电平）。
 */
void car_uart_command_poll(void);

uint8_t car_uart_halted_get(void);

#if (0U != HAL_TRACK_DEBUG_UART)
/** 上电发一次 ready / hint（在 app_motor_init 之后调用） */
void car_uart_track_debug_boot_msg(void);

/** 主循环每周期传入 elapsed_ms（如 10），到间隔则读传感器并 UART 输出一行 */
void car_uart_track_periodic_poll(uint32_t elapsed_ms);
#endif

#endif /* APP_UART_H_ */
