#ifndef BSP_UART_H_
#define BSP_UART_H_

#include <stdint.h>

#include "bsp_api.h"

/**
 * SCI9 串口 BSP（与 docs/引脚配置说明.md 及 ra_gen 一致）：
 * - P109：SCI9 TXD
 * - P110：SCI9 RXD
 * 硬件实例与波特率等参数由 FSP 生成的 g_uart9 / g_uart9_cfg 决定。
 *
 * 使用前在应用入口（如 hal_entry）中调用 uart_bsp_init()。
 * 接收：中断里将字节写入内部环形缓冲，应用通过 uart_bsp_read* 取出。
 * 发送：uart_bsp_write / uart_bsp_write_str 为阻塞方式，发完再返回。
 */

void uart_bsp_init(void);
fsp_err_t uart_bsp_deinit(void);

/** 阻塞发送 len 字节；len 为 0 时立即返回 FSP_SUCCESS */
fsp_err_t uart_bsp_write(const uint8_t *data, uint32_t len);

/** 阻塞发送以 '\\0' 结尾的字符串 */
fsp_err_t uart_bsp_write_str(const char *s);

/** 环形缓冲内可读字节数 */
uint32_t uart_bsp_rx_count(void);

/** 读出一字节；无数据返回 false */
_Bool uart_bsp_read_byte(uint8_t *out);

/** 读出最多 maxlen 字节，返回实际读出个数 */
uint32_t uart_bsp_read(uint8_t *dst, uint32_t maxlen);

/** 清空接收环形缓冲 */
void uart_bsp_rx_flush(void);

#endif /* BSP_UART_H_ */
