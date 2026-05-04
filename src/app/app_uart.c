#include "app_uart.h"
#include "bsp_uart.h"

#include <string.h>

/** STOP 命令置 1，RUN 置 0；由主循环读取，不在 UART ISR 内写电机 */
static volatile uint8_t g_car_stop_request = 0U;

#define CMD_BUF_SIZE 32

void car_uart_command_poll(void)
{
    static char cmd_buf[CMD_BUF_SIZE];
    static uint32_t cmd_len = 0;

    uint8_t ch;

    while (uart_bsp_read_byte(&ch))
    {
        if ((ch == '\n') || (ch == '\r'))
        {
            if (cmd_len > 0)
            {
                cmd_buf[cmd_len] = '\0';

                if (strcmp(cmd_buf, "STOP") == 0)
                {
                    g_car_stop_request = 1U;
                    uart_bsp_write_str("OK STOP\r\n");
                }
                else if (strcmp(cmd_buf, "RUN") == 0)
                {
                    g_car_stop_request = 0U;
                    uart_bsp_write_str("OK RUN\r\n");
                }
                else
                {
                    uart_bsp_write_str("ERR UNKNOWN CMD\r\n");
                }

                cmd_len = 0;
            }
        }
        else
        {
            if (cmd_len < (CMD_BUF_SIZE - 1U))
            {
                cmd_buf[cmd_len++] = (char)ch;
            }
            else
            {
                cmd_len = 0;
                uart_bsp_write_str("ERR CMD TOO LONG\r\n");
            }
        }
    }
}

uint8_t car_uart_halted_get(void)
{
    return g_car_stop_request;
}