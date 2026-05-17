#include "app_uart.h"
#include "bsp/bsp_uart.h"

#if (0U != HAL_TRACK_DEBUG_UART)
#include "bsp/bsp_track.h"
#endif

#include <string.h>

/** STOP 命令置 1，RUN 置 0；由主循环读取，不在 UART ISR 内写电机 */
static volatile uint8_t g_car_stop_request = 0U;

#define CMD_BUF_SIZE 32

#if (0U != HAL_TRACK_DEBUG_UART)
static void car_uart_track_send_levels_line(const uint8_t levels[TRACK_SENSOR_NUM], uint8_t raw_out)
{
    char line[16];
    uint8_t pos = 0U;

    for (uint8_t i = 0U; i < TRACK_SENSOR_NUM; i++) {
        line[pos++] = (levels[i] != 0U) ? '1' : '0';
    }
    line[pos++] = ' ';
    line[pos++] = 'D';
    line[pos++] = '=';
    line[pos++] = (raw_out != 0U) ? '1' : '0';
    line[pos++] = '\r';
    line[pos++] = '\n';
    line[pos]   = '\0';
    uart_bsp_write_str(line);
}

void car_uart_track_debug_boot_msg(void)
{
    uart_bsp_write_str("Track test ready, interval=2000ms\r\n");
    uart_bsp_write_str("Hint: black -> some 0; L-R = ch0..ch7 (left=0)\r\n");
}

void car_uart_track_periodic_poll(uint32_t elapsed_ms)
{
    static uint32_t acc_ms = 0U;

    acc_ms += elapsed_ms;
    if (acc_ms < (uint32_t)HAL_TRACK_REPORT_INTERVAL_MS) {
        return;
    }
    acc_ms = 0U;

    uint8_t levels[TRACK_SENSOR_NUM];

    track_bsp_read_all(levels);
    car_uart_track_send_levels_line(levels, track_bsp_read_out_raw());
}
#endif

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
