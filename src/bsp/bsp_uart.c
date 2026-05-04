#include "bsp_uart.h"

#include <assert.h>
#include <string.h>

#include "hal_data.h"

/** SCI9 实例（P109 TX / P110 RX） */
#define UART_BSP_INST g_uart9

#define UART_RX_RING_SIZE (256U)

static uint8_t s_rx_ring[UART_RX_RING_SIZE];
static volatile uint32_t s_rx_head;
static volatile uint32_t s_rx_tail;

static volatile uint8_t s_tx_busy;

static void ring_push_byte(uint8_t b)
{
    const uint32_t head = s_rx_head;
    const uint32_t next = (head + 1U) % UART_RX_RING_SIZE;
    const uint32_t tail = s_rx_tail;

    if (next == tail) {
        /* 缓冲满：丢弃新字节 */
        return;
    }

    s_rx_ring[head] = b;
    s_rx_head       = next;
}

static uint32_t ring_count_get(void)
{
    return (s_rx_head - s_rx_tail + UART_RX_RING_SIZE) % UART_RX_RING_SIZE;
}

/**
 * FSP 配置的回调名（configuration.xml module.driver.uart.callback）。
 * 在 UART_EVENT_RX_CHAR 时收字节；TX_COMPLETE 时清除发送忙标志。
 */
void user_usart_callback(uart_callback_args_t *p_args)
{
    if (NULL == p_args) {
        return;
    }

    switch (p_args->event) {
        case UART_EVENT_RX_CHAR:
            ring_push_byte((uint8_t)(p_args->data & 0xFFU));
            break;
        case UART_EVENT_TX_COMPLETE:
            s_tx_busy = 0U;
            break;
        default:
            break;
    }
}

void uart_bsp_init(void)
{
    fsp_err_t err;

    s_rx_head = 0U;
    s_rx_tail = 0U;
    s_tx_busy = 0U;

    err = UART_BSP_INST.p_api->open(UART_BSP_INST.p_ctrl, UART_BSP_INST.p_cfg);
    assert(FSP_SUCCESS == err);
}

fsp_err_t uart_bsp_deinit(void)
{
    return UART_BSP_INST.p_api->close(UART_BSP_INST.p_ctrl);
}

fsp_err_t uart_bsp_write(const uint8_t *data, uint32_t len)
{
    fsp_err_t err;

    if ((NULL == data) && (len > 0U)) {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if (0U == len) {
        return FSP_SUCCESS;
    }

    s_tx_busy = 1U;
    err       = UART_BSP_INST.p_api->write(UART_BSP_INST.p_ctrl, data, len);
    if (FSP_SUCCESS != err) {
        s_tx_busy = 0U;
        return err;
    }

    while (0U != s_tx_busy) {
        /* 等待 TEI 回调清除 busy */
    }

    return FSP_SUCCESS;
}

fsp_err_t uart_bsp_write_str(const char *s)
{
    if (NULL == s) {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    return uart_bsp_write((const uint8_t *)s, (uint32_t)strlen(s));
}

uint32_t uart_bsp_rx_count(void)
{
    return ring_count_get();
}

_Bool uart_bsp_read_byte(uint8_t *out)
{
    if (NULL == out) {
        return 0;
    }

    if (s_rx_head == s_rx_tail) {
        return 0;
    }

    *out      = s_rx_ring[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1U) % UART_RX_RING_SIZE;
    return 1;
}

uint32_t uart_bsp_read(uint8_t *dst, uint32_t maxlen)
{
    uint32_t n = 0U;

    if ((NULL == dst) || (0U == maxlen)) {
        return 0U;
    }

    while ((n < maxlen) && (uart_bsp_read_byte(&dst[n]) != 0)) {
        n++;
    }

    return n;
}

void uart_bsp_rx_flush(void)
{
    uint8_t tmp;

    while (uart_bsp_read_byte(&tmp) != 0) {
        /* drain */
    }
}
