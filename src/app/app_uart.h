#ifndef APP_UART_H_
#define APP_UART_H_

#include <stdint.h>

/**
 * 串口命令解析（与 bsp_uart 配合）：
 * - 接收：SCI RXI 中断在 `user_usart_callback` 中把字节写入环形缓冲（见 bsp_uart.c）。
 * - 解析：主循环调用 `car_uart_command_poll()`，从环缓取字节拼行，遇 \\r/\\n 处理命令。
 * - STOP：置内部停机请求；RUN：清除。主循环用 `car_uart_halted_get()` 判断是否停循迹/刹车。
 */
void car_uart_command_poll(void);

/** 非 0：主机已发 STOP，应停车；0：RUN 或上电默认，可正常循迹 */
uint8_t car_uart_halted_get(void);

#endif /* APP_UART_H_ */