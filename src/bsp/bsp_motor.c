#include "bsp_motor.h"
#include <assert.h>

#include "r_gpt.h"

/** 单轮双 PWM：一个 GPT 上 GTIOCA + GTIOCB 两路独立占空比输出至电机驱动板 */
typedef struct {
    gpt_instance_ctrl_t *p_ctrl;
    const timer_cfg_t   *p_cfg;
    gpt_io_pin_t         pin_fwd; /**< 正转侧 PWM 引脚（GTIOCA） */
    gpt_io_pin_t         pin_rev; /**< 反转侧 PWM 引脚（GTIOCB） */
} motor_wheel_dual_pwm_t;

static const motor_wheel_dual_pwm_t s_motor_wheel_dual_pwm[MOTOR_MAX] = {
    [MOTOR_LEFT] = {
        .p_ctrl  = &g_timer3_ctrl,
        .p_cfg   = &g_timer3_cfg,
        .pin_fwd = GPT_IO_PIN_GTIOCA,
        .pin_rev = GPT_IO_PIN_GTIOCB,
    },
    [MOTOR_RIGHT] = {
        .p_ctrl  = &g_timer4_ctrl,
        .p_cfg   = &g_timer4_cfg,
        .pin_fwd = GPT_IO_PIN_GTIOCA,
        .pin_rev = GPT_IO_PIN_GTIOCB,
    },
};

static const motor_wheel_dual_pwm_t *motor_wheel_hw_get(motor_id_t motor)
{
    assert((unsigned int)motor < (unsigned int)MOTOR_MAX);
    return &s_motor_wheel_dual_pwm[motor];
}

static uint32_t motor_pwm_period_counts(void)
{
    return s_motor_wheel_dual_pwm[MOTOR_LEFT].p_cfg->period_counts;
}

static uint32_t percent_to_duty_counts(float percent)
{
    const uint32_t period = motor_pwm_period_counts();
    float           abs_percent = (percent >= 0.0f) ? percent : -percent;

    if (abs_percent > 100.0f) {
        abs_percent = 100.0f;
    }
    if (abs_percent < 0.0f) {
        abs_percent = 0.0f;
    }

    return (uint32_t)((abs_percent * (float)period) / 100.0f);
}

static void motor_wheel_dual_pwm_counts_set(const motor_wheel_dual_pwm_t *hw, uint32_t fwd_counts, uint32_t rev_counts)
{
    R_GPT_DutyCycleSet(hw->p_ctrl, fwd_counts, hw->pin_fwd);
    R_GPT_DutyCycleSet(hw->p_ctrl, rev_counts, hw->pin_rev);
}

static void motor_wheel_dual_pwm_idle(const motor_wheel_dual_pwm_t *hw)
{
    motor_wheel_dual_pwm_counts_set(hw, 0U, 0U);
}

static void motor_wheel_dual_pwm_brake(const motor_wheel_dual_pwm_t *hw)
{
    const uint32_t period = motor_pwm_period_counts();
    motor_wheel_dual_pwm_counts_set(hw, period, period);
}

void motor_bsp_init(void)
{
    fsp_err_t err;
    const uint32_t period = motor_pwm_period_counts();

    assert(s_motor_wheel_dual_pwm[MOTOR_LEFT].p_cfg->period_counts
           == s_motor_wheel_dual_pwm[MOTOR_RIGHT].p_cfg->period_counts);

    for (motor_id_t m = MOTOR_LEFT; m < MOTOR_MAX; m++) {
        const motor_wheel_dual_pwm_t *hw = motor_wheel_hw_get(m);
        err = R_GPT_Open(hw->p_ctrl, hw->p_cfg);
        assert(FSP_SUCCESS == err);
        err = R_GPT_PeriodSet(hw->p_ctrl, period);
        assert(FSP_SUCCESS == err);
        err = R_GPT_Start(hw->p_ctrl);
        assert(FSP_SUCCESS == err);
        motor_wheel_dual_pwm_idle(hw);
    }
}

void motor_bsp_set_dual_pwm(motor_id_t motor, float duty_forward_percent, float duty_reverse_percent)
{
    const motor_wheel_dual_pwm_t *hw = motor_wheel_hw_get(motor);
    const uint32_t                 fwd = percent_to_duty_counts(duty_forward_percent);
    const uint32_t                 rev = percent_to_duty_counts(duty_reverse_percent);
    motor_wheel_dual_pwm_counts_set(hw, fwd, rev);
}

void motor_bsp_set_direction(motor_id_t motor, motor_dir_t dir)
{
    const motor_wheel_dual_pwm_t *hw = motor_wheel_hw_get(motor);
    const uint32_t                 period = motor_pwm_period_counts();

    switch (dir) {
        case MOTOR_DIR_FORWARD:
            motor_wheel_dual_pwm_counts_set(hw, period, 0U);
            break;
        case MOTOR_DIR_BACKWARD:
            motor_wheel_dual_pwm_counts_set(hw, 0U, period);
            break;
        case MOTOR_DIR_BRAKE:
            motor_wheel_dual_pwm_brake(hw);
            break;
        case MOTOR_DIR_COAST:
        default:
            motor_wheel_dual_pwm_idle(hw);
            break;
    }
}

void motor_bsp_set_speed(motor_id_t motor, float duty_percent)
{
    if (duty_percent >= 0.0f) {
        motor_bsp_set_dual_pwm(motor, duty_percent, 0.0f);
    } else {
        motor_bsp_set_dual_pwm(motor, 0.0f, -duty_percent);
    }
}

void motor_bsp_stop(motor_id_t motor, motor_dir_t stop_mode)
{
    const motor_wheel_dual_pwm_t *hw = motor_wheel_hw_get(motor);

    if (stop_mode == MOTOR_DIR_BRAKE) {
        motor_wheel_dual_pwm_brake(hw);
    } else {
        motor_wheel_dual_pwm_idle(hw);
    }
}
