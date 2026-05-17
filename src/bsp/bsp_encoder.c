#include "bsp_encoder.h"

#include <assert.h>

#include "common_data.h"
#include "r_ioport.h"

/* B 相输入（A 相由 ICU 引脚占用，不在此列） */
#define ENCODER_LEFT_B_PIN  BSP_IO_PORT_04_PIN_09
#define ENCODER_RIGHT_B_PIN BSP_IO_PORT_05_PIN_00

static volatile int32_t s_pulse_left;
static volatile int32_t s_pulse_right;

static int32_t s_vel_prev_left;
static int32_t s_vel_prev_right;
static float   s_vel_left_m_s;
static float   s_vel_right_m_s;

static float encoder_meters_per_pulse(void)
{
    return BSP_ENCODER_WHEEL_CIRCUMFERENCE_M / (float)BSP_ENCODER_PULSES_PER_WHEEL_REV;
}

static void encoder_pulse_add_left(int32_t delta)
{
    s_pulse_left += delta;
}

static void encoder_pulse_add_right(int32_t delta)
{
    s_pulse_right += delta;
}

/**
 * A 相上升沿时刻读 B 相：B 为高记 -1，为低记 +1（与编码器接线/左右安装有关时可改 SIGN 宏）。
 */
static int32_t encoder_quadrature_delta_from_b(bsp_io_level_t b_level)
{
    return (b_level == BSP_IO_LEVEL_HIGH) ? -1 : 1;
}

void left_encoder_irq_callback(external_irq_callback_args_t *p_args)
{
    (void)p_args;
    bsp_io_level_t b_level = BSP_IO_LEVEL_LOW;
    (void)R_IOPORT_PinRead(&g_ioport_ctrl, ENCODER_LEFT_B_PIN, &b_level);
    encoder_pulse_add_left(encoder_quadrature_delta_from_b(b_level) * (int32_t)BSP_ENCODER_LEFT_SIGN);
}

void right_encoder_irq_callback(external_irq_callback_args_t *p_args)
{
    (void)p_args;
    bsp_io_level_t b_level = BSP_IO_LEVEL_LOW;
    (void)R_IOPORT_PinRead(&g_ioport_ctrl, ENCODER_RIGHT_B_PIN, &b_level);
    encoder_pulse_add_right(encoder_quadrature_delta_from_b(b_level) * (int32_t)BSP_ENCODER_RIGHT_SIGN);
}

void encoder_bsp_init(void)
{
    fsp_err_t err;

    s_pulse_left        = 0;
    s_pulse_right       = 0;
    s_vel_prev_left     = 0;
    s_vel_prev_right    = 0;
    s_vel_left_m_s      = 0.0f;
    s_vel_right_m_s     = 0.0f;

    err = g_left_encoder_irq.p_api->open(g_left_encoder_irq.p_ctrl, g_left_encoder_irq.p_cfg);
    assert(FSP_SUCCESS == err);
    err = g_left_encoder_irq.p_api->enable(g_left_encoder_irq.p_ctrl);
    assert(FSP_SUCCESS == err);

    err = g_right_encoder_irq.p_api->open(g_right_encoder_irq.p_ctrl, g_right_encoder_irq.p_cfg);
    assert(FSP_SUCCESS == err);
    err = g_right_encoder_irq.p_api->enable(g_right_encoder_irq.p_ctrl);
    assert(FSP_SUCCESS == err);

    /* 避免上电后首次 velocity_sample 把已有脉冲当作一个周期内的增量 */
    s_vel_prev_left  = s_pulse_left;
    s_vel_prev_right = s_pulse_right;
}

int32_t encoder_bsp_pulse_count_get(encoder_side_t side)
{
    if (side == ENCODER_SIDE_LEFT) {
        return s_pulse_left;
    }
    if (side == ENCODER_SIDE_RIGHT) {
        return s_pulse_right;
    }
    return 0;
}

void encoder_bsp_pulse_count_reset(encoder_side_t side)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (side == ENCODER_SIDE_LEFT) {
        s_pulse_left     = 0;
        s_vel_prev_left  = 0;
        s_vel_left_m_s   = 0.0f;
    } else if (side == ENCODER_SIDE_RIGHT) {
        s_pulse_right     = 0;
        s_vel_prev_right  = 0;
        s_vel_right_m_s   = 0.0f;
    }
    __set_PRIMASK(primask);
}

float encoder_bsp_distance_m_get(encoder_side_t side)
{
    const float k = encoder_meters_per_pulse();
    return (float)encoder_bsp_pulse_count_get(side) * k;
}

void encoder_bsp_distance_reset(encoder_side_t side)
{
    encoder_bsp_pulse_count_reset(side);
}

void encoder_bsp_velocity_sample(float dt_sec)
{
    int32_t l;
    int32_t r;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    l = s_pulse_left;
    r = s_pulse_right;
    __set_PRIMASK(primask);

    if (dt_sec <= 1.0e-9f) {
        s_vel_left_m_s  = 0.0f;
        s_vel_right_m_s = 0.0f;
        return;
    }

    {
        const int32_t dl = l - s_vel_prev_left;
        const int32_t dr = r - s_vel_prev_right;
        const float   k  = encoder_meters_per_pulse();
        s_vel_prev_left  = l;
        s_vel_prev_right = r;
        s_vel_left_m_s   = ((float)dl * k) / dt_sec;
        s_vel_right_m_s  = ((float)dr * k) / dt_sec;
    }
}

float encoder_bsp_velocity_m_s_get(encoder_side_t side)
{
    if (side == ENCODER_SIDE_LEFT) {
        return s_vel_left_m_s;
    }
    if (side == ENCODER_SIDE_RIGHT) {
        return s_vel_right_m_s;
    }
    return 0.0f;
}
