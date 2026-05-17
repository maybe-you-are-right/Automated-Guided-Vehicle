#ifndef APP_MOTOR_H_
#define APP_MOTOR_H_

#include <stdint.h>

#include "bsp/bsp_encoder.h"

/**
 * 应用层电机 + 红外循迹差速控制 + 编码器速度内环（可选）。
 * 依赖：bsp_motor、bsp_track、bsp_encoder、algorithm/pid。
 *
 * 外环（循迹）：8 路数字量做加权重心，归一化约 [-1,1] → PID → 左右目标占空比
 *   左 = base + corr，右 = base - corr。
 * 内环（速度）：将目标占空比线性映射为轮目标线速度 (m/s)，与编码器实测速度比较，
 *   左右各一 PID，输出占空比修正量，抑制负载/摩擦变化，提高抗扰与速度一致性。
 * 编码器测量速度经一阶低通（`APP_MOTOR_SPEED_MEAS_LPF_TAU_SEC` + 本周期 `dt_sec`）后参与速度 PID；
 * 原始瞬时速度仍可从 `app_motor_speed_meas_raw_m_s_get` 读取便于调试。
 * 主循环须先 `encoder_bsp_velocity_sample(dt)` 再调用本模块更新（与 `hal_entry` 一致）。
 */

#ifndef APP_MOTOR_PID_KP_DEFAULT
#define APP_MOTOR_PID_KP_DEFAULT (8.0f)
#endif
#ifndef APP_MOTOR_PID_KI_DEFAULT
#define APP_MOTOR_PID_KI_DEFAULT (0.0f)
#endif
#ifndef APP_MOTOR_PID_KD_DEFAULT
#define APP_MOTOR_PID_KD_DEFAULT (0.0f)
#endif
#ifndef APP_MOTOR_PID_INTEGRAL_LIMIT_DEFAULT
#define APP_MOTOR_PID_INTEGRAL_LIMIT_DEFAULT (5.0f)
#endif
/** PID 输出（差速修正量）绝对值上限，单位与 motor 占空比相同（百分比量级） */
#ifndef APP_MOTOR_CORRECTION_LIMIT_DEFAULT
#define APP_MOTOR_CORRECTION_LIMIT_DEFAULT (15.0f)
#endif

/** 占空比 ±100% 时对应的轮目标线速度幅值 (m/s)，用于外环占空比→内环速度给定，需实车标定 */
#ifndef APP_MOTOR_WHEEL_V_AT_FULL_DUTY_M_S
#define APP_MOTOR_WHEEL_V_AT_FULL_DUTY_M_S (1.45f)
#endif
/** 速度环 PID：误差 = 目标速度 − 编码器速度 (m/s)，输出为占空比修正 (%)，左右共用增益 */
#ifndef APP_MOTOR_SPEED_PID_KP_DEFAULT
#define APP_MOTOR_SPEED_PID_KP_DEFAULT (25.0f)
#endif
#ifndef APP_MOTOR_SPEED_PID_KI_DEFAULT
#define APP_MOTOR_SPEED_PID_KI_DEFAULT (0.0f)
#endif
#ifndef APP_MOTOR_SPEED_PID_KD_DEFAULT
#define APP_MOTOR_SPEED_PID_KD_DEFAULT (0.0f)
#endif
#ifndef APP_MOTOR_SPEED_PID_INTEGRAL_LIMIT_DEFAULT
#define APP_MOTOR_SPEED_PID_INTEGRAL_LIMIT_DEFAULT (0.12f)
#endif
/** 单轮速度环输出修正量限幅 (%)，避免与循迹环打架 */
#ifndef APP_MOTOR_SPEED_PID_OUT_LIMIT_DEFAULT
#define APP_MOTOR_SPEED_PID_OUT_LIMIT_DEFAULT (22.0f)
#endif

/** 测量轮速一阶低通时间常数 τ (s)；α = dt/(τ+dt)，须与控制周期 dt_sec 一致使用 */
#ifndef APP_MOTOR_SPEED_MEAS_LPF_TAU_SEC
#define APP_MOTOR_SPEED_MEAS_LPF_TAU_SEC (0.05f)
#endif

void app_motor_init(void);

/**
 * 传感器极性：模块若“压黑为 0、白为 1”，可置 invert=1 再调 PID。
 * @param invert 0 表示直接使用 track 读数；非 0 表示每路 0/1 取反后再算误差。
 */
void app_motor_set_sensor_invert(uint8_t invert);

/**
 * 按当前红外状态做差速循迹并写电机 PWM。
 * @param base_speed_percent 基准前进速度 -100…100（与 motor_bsp_set_speed 一致）
 * @param dt_sec             与主循环周期一致，秒（如 0.01 = 10 ms）
 */
void app_motor_line_follow_update(float base_speed_percent, float dt_sec);

/** 两轮刹车停止（可按需在 .c 内改为 COAST） */
void app_motor_stop(void);

/** 直接设定左右轮占空比，绕开循迹（调试或手动模式） */
void app_motor_set_differential_manual(float left_percent, float right_percent);

/** 重置循迹 PID 与左右速度环 PID（丢线恢复、急停后再启动等） */
void app_motor_pid_reset(void);

/** 运行时改循迹 PID */
void app_motor_set_pid_gains(float kp, float ki, float kd, float integral_limit);

/** 运行时改速度环 PID（左右轮相同参数） */
void app_motor_set_speed_pid_gains(float kp, float ki, float kd, float integral_limit);

float app_motor_last_line_error_get(void);
float app_motor_last_pid_out_get(void);
/** 最近一次速度环误差 (m/s)，目标−滤波后测量；调试用 */
float app_motor_last_speed_err_left_get(void);
float app_motor_last_speed_err_right_get(void);

/** 编码器瞬时测量速度 (m/s)，未经 APP_MOTOR 低通，供调试/分析 */
float app_motor_speed_meas_raw_m_s_get(encoder_side_t side);

/** 经测量低通后的轮速 (m/s)，与速度 PID 使用一致；未跑过循迹更新前为 0 */
float app_motor_speed_meas_filtered_m_s_get(encoder_side_t side);

#endif /* APP_MOTOR_H_ */
