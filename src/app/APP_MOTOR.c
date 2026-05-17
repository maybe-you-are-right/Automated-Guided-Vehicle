#include "APP_MOTOR.h"

#include "algorithm/filter.h"
#include "algorithm/pid.h"
#include "bsp/bsp_encoder.h"
#include "bsp/bsp_motor.h"
#include "bsp/bsp_track.h"

#ifndef APP_MOTOR_LOST_LINE_ERROR
/** 全丢线时使用的归一化误差方向幅值（±1 表示全力找线） */
#define APP_MOTOR_LOST_LINE_ERROR (1.0f)
#endif

static pid_context_t s_line_pid;
static pid_context_t s_speed_pid_left;
static pid_context_t s_speed_pid_right;
static uint8_t       s_sensor_invert = 0U;

static float s_last_norm_error = 0.0f;
static float s_last_pid_error  = 0.0f;
static float s_last_pid_out    = 0.0f;
static float s_last_speed_err_left  = 0.0f;
static float s_last_speed_err_right = 0.0f;

static filter_lpf_1st_f32_t s_meas_lpf_left;
static filter_lpf_1st_f32_t s_meas_lpf_right;
static uint8_t              s_meas_lpf_inited = 0U;
static float               s_last_meas_filt_l = 0.0f;
static float               s_last_meas_filt_r = 0.0f;
//可能需要辨别正负号
/* 传感器几何位置权重（通道 1…8 对应下标 0…7），与物理左→右一致 */
static const float s_weights[TRACK_SENSOR_NUM] = {
    -3.5f, -2.5f, -1.5f, -1.0f, 1.0f, 1.5f, 2.5f, 3.5f,
};

static float clampf(float x, float lo, float hi)
{
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

/** 仅更新 Kp/Ki/Kd 与积分限幅，不改变输出限幅（仍用 algorithm/pid） */
static void motor_pid_ctx_gains_write(pid_context_t *ctx, float kp, float ki, float kd, float integral_limit)
{
    ctx->kp             = kp;
    ctx->ki             = ki;
    ctx->kd             = kd;
    ctx->integral_limit = integral_limit;
}

/**
 * 单轮速度环 pid_init；当前左右共用默认宏。
 * 若将来左右增益不同，可拆成两个函数或在此处分别传参。
 */
static void motor_speed_pid_ctx_init_default(pid_context_t *ctx)
{
    pid_init(ctx,
             APP_MOTOR_SPEED_PID_KP_DEFAULT,
             APP_MOTOR_SPEED_PID_KI_DEFAULT,
             APP_MOTOR_SPEED_PID_KD_DEFAULT,
             APP_MOTOR_SPEED_PID_INTEGRAL_LIMIT_DEFAULT,
             -APP_MOTOR_SPEED_PID_OUT_LIMIT_DEFAULT,
             APP_MOTOR_SPEED_PID_OUT_LIMIT_DEFAULT);
}

static void motor_speed_pid_pair_init_defaults(void)
{
    motor_speed_pid_ctx_init_default(&s_speed_pid_left);
    motor_speed_pid_ctx_init_default(&s_speed_pid_right);
}

static void motor_speed_pid_pair_reset(void)
{
    pid_reset(&s_speed_pid_left);
    pid_reset(&s_speed_pid_right);
}

/** 将目标占空比线性映射为轮目标线速度 (m/s)，与编码器速度量纲一致 */
static float app_motor_duty_to_target_vel_m_s(float duty_percent)
{
    return (duty_percent / 100.0f) * APP_MOTOR_WHEEL_V_AT_FULL_DUTY_M_S;
}

static void app_motor_meas_lpf_clear(void)
{
    filter_lpf_1st_f32_reset(&s_meas_lpf_left, 0.0f);
    filter_lpf_1st_f32_reset(&s_meas_lpf_right, 0.0f);
    s_meas_lpf_inited      = 0U;
    s_last_meas_filt_l     = 0.0f;
    s_last_meas_filt_r    = 0.0f;
}

/**
 * @return 归一化线偏差，约 [-1, 1]：正值表示线重心在车体右侧，应增大左轮转速。
 */
static float line_normalized_error(const uint8_t levels[TRACK_SENSOR_NUM])
{
    float wsum = 0.0f;
    float vsum = 0.0f;

    for (uint8_t i = 0U; i < TRACK_SENSOR_NUM; i++) {
        float v = (levels[i] != 0U) ? 1.0f : 0.0f;
        if (s_sensor_invert != 0U) {
            v = 1.0f - v;
        }
        wsum += s_weights[i] * v;
        vsum += v;
    }

    if (vsum < 0.5f) {
        if (s_last_norm_error > 0.01f) {
            return APP_MOTOR_LOST_LINE_ERROR;
        }
        if (s_last_norm_error < -0.01f) {
            return -APP_MOTOR_LOST_LINE_ERROR;
        }
        return 0.0f;
    }

    const float pos = wsum / vsum;
    const float norm = pos / 3.5f;
    s_last_norm_error = norm;
    return norm;
}

void app_motor_init(void)
{
    motor_bsp_init();
    track_bsp_init();

    pid_init(&s_line_pid,
             APP_MOTOR_PID_KP_DEFAULT,
             APP_MOTOR_PID_KI_DEFAULT,
             APP_MOTOR_PID_KD_DEFAULT,
             APP_MOTOR_PID_INTEGRAL_LIMIT_DEFAULT,
             -APP_MOTOR_CORRECTION_LIMIT_DEFAULT,
             APP_MOTOR_CORRECTION_LIMIT_DEFAULT);

    //motor_speed_pid_pair_init_defaults();

    s_sensor_invert   = 0U;
    s_last_norm_error = 0.0f;
    s_last_pid_out    = 0.0f;
    s_last_speed_err_left  = 0.0f;
    s_last_speed_err_right = 0.0f;

    app_motor_meas_lpf_clear();
}

void app_motor_set_sensor_invert(uint8_t invert)
{
    s_sensor_invert = invert;
}

void app_motor_set_pid_gains(float kp, float ki, float kd, float integral_limit)
{
    motor_pid_ctx_gains_write(&s_line_pid, kp, ki, kd, integral_limit);
}

void app_motor_set_speed_pid_gains(float kp, float ki, float kd, float integral_limit)
{
    motor_pid_ctx_gains_write(&s_speed_pid_left, kp, ki, kd, integral_limit);
    motor_pid_ctx_gains_write(&s_speed_pid_right, kp, ki, kd, integral_limit);
}

void app_motor_pid_reset(void)
{
    pid_reset(&s_line_pid);
    motor_speed_pid_pair_reset();
    app_motor_meas_lpf_clear();
}

float app_motor_last_line_error_get(void)
{
    return s_last_pid_error;
}

float app_motor_last_pid_out_get(void)
{
    return s_last_pid_out;
}

float app_motor_last_speed_err_left_get(void)
{
    return s_last_speed_err_left;
}

float app_motor_last_speed_err_right_get(void)
{
    return s_last_speed_err_right;
}

float app_motor_speed_meas_raw_m_s_get(encoder_side_t side)
{
    return encoder_bsp_velocity_m_s_get(side);
}

float app_motor_speed_meas_filtered_m_s_get(encoder_side_t side)
{
    if (side == ENCODER_SIDE_LEFT) {
        return s_last_meas_filt_l;
    }
    if (side == ENCODER_SIDE_RIGHT) {
        return s_last_meas_filt_r;
    }
    return 0.0f;
}

void app_motor_line_follow_update(float base_speed_percent, float dt_sec)
{
    uint8_t levels[TRACK_SENSOR_NUM];
    track_bsp_read_all(levels);

    const float err = line_normalized_error(levels);
    s_last_pid_error = err;
    const float corr = pid_compute(&s_line_pid, err, dt_sec);
    s_last_pid_out = corr;

    float left  = base_speed_percent + corr;
    float right = base_speed_percent - corr;

    left  = clampf(left, -100.0f, 100.0f);
    right = clampf(right, -100.0f, 100.0f);


//    /* 速度内环：测量速度经 LPF 后与目标比较（原始速度仍可从 bsp / raw_get 读取） */
//   {
//       const float v_raw_l = encoder_bsp_velocity_m_s_get(ENCODER_SIDE_LEFT);
//         const float v_raw_r = encoder_bsp_velocity_m_s_get(ENCODER_SIDE_RIGHT);

//         if ((0U == s_meas_lpf_inited) && (dt_sec > 1.0e-9f)) {
//             filter_lpf_1st_f32_init_tau(&s_meas_lpf_left,
//                                         dt_sec,
//                                         APP_MOTOR_SPEED_MEAS_LPF_TAU_SEC,
//                                         0.0f);
//             filter_lpf_1st_f32_init_tau(&s_meas_lpf_right,
//                                         dt_sec,
//                                         APP_MOTOR_SPEED_MEAS_LPF_TAU_SEC,
//                                         0.0f);
//             s_meas_lpf_inited = 1U;
//         }

//         const float v_meas_l = (0U != s_meas_lpf_inited) ? filter_lpf_1st_f32_run(&s_meas_lpf_left, v_raw_l) : v_raw_l;
//         const float v_meas_r = (0U != s_meas_lpf_inited) ? filter_lpf_1st_f32_run(&s_meas_lpf_right, v_raw_r) : v_raw_r;

//         s_last_meas_filt_l = v_meas_l;
//         s_last_meas_filt_r = v_meas_r;

//         const float v_tgt_l = app_motor_duty_to_target_vel_m_s(left);
//         const float v_tgt_r = app_motor_duty_to_target_vel_m_s(right);

//         s_last_speed_err_left  = v_tgt_l - v_meas_l;
//         s_last_speed_err_right = v_tgt_r - v_meas_r;

//         const float u_l = pid_compute(&s_speed_pid_left, s_last_speed_err_left, dt_sec);
//         const float u_r = pid_compute(&s_speed_pid_right, s_last_speed_err_right, dt_sec);

//         left  = clampf(left + u_l, -100.0f, 100.0f);
//         right = clampf(right + u_r, -100.0f, 100.0f);
//     }


    motor_bsp_set_speed(MOTOR_LEFT, left);
    motor_bsp_set_speed(MOTOR_RIGHT, right);
}

void app_motor_stop(void)
{
    motor_speed_pid_pair_reset();
    s_last_speed_err_left  = 0.0f;
    s_last_speed_err_right = 0.0f;
    app_motor_meas_lpf_clear();
    motor_bsp_stop(MOTOR_LEFT, MOTOR_DIR_BRAKE);
    motor_bsp_stop(MOTOR_RIGHT, MOTOR_DIR_BRAKE);
}

void app_motor_set_differential_manual(float left_percent, float right_percent)
{
    motor_bsp_set_speed(MOTOR_LEFT, clampf(left_percent, -100.0f, 100.0f));
    motor_bsp_set_speed(MOTOR_RIGHT, clampf(right_percent, -100.0f, 100.0f));
}
