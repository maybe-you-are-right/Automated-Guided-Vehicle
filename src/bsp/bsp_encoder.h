#ifndef BSP_ENCODER_H_
#define BSP_ENCODER_H_

#include <stdint.h>

/**
 * 增量编码器 BSP（与 FSP `g_left_encoder_irq` / `g_right_encoder_irq` 一致）：
 * - A 相：ICU 外部 IRQ，上升沿触发；左 P402、右 P104。
 * - B 相：GPIO 输入，在 A 上升沿回调内读取；左 P409、右 P500。
 * - 仅在 A 相上升沿计数（非 4 倍频）；电机轴每圈 13 脉冲，减速比 30:1（电机 30 圈 = 轮子 1 圈），
 *   故轮子每圈 390 个计数；轮周长 0.144513 m 用于距离与线速度。
 * 判向：A 上升时 B 为高记 -1、为低记 +1（与接线有关，反了改 `BSP_ENCODER_*_SIGN`）。
 */

#ifndef BSP_ENCODER_MOTOR_PULSES_PER_REV
#define BSP_ENCODER_MOTOR_PULSES_PER_REV (13U)
#endif

#ifndef BSP_ENCODER_GEAR_RATIO_MOTOR_TO_WHEEL
#define BSP_ENCODER_GEAR_RATIO_MOTOR_TO_WHEEL (30U)
#endif

#ifndef BSP_ENCODER_WHEEL_CIRCUMFERENCE_M
#define BSP_ENCODER_WHEEL_CIRCUMFERENCE_M (0.144513f)
#endif

/** 轮子转一圈在「仅 A 上升沿计数」下的脉冲数 */
#define BSP_ENCODER_PULSES_PER_WHEEL_REV ((uint32_t)BSP_ENCODER_MOTOR_PULSES_PER_REV * (uint32_t)BSP_ENCODER_GEAR_RATIO_MOTOR_TO_WHEEL)

#ifndef BSP_ENCODER_LEFT_SIGN
#define BSP_ENCODER_LEFT_SIGN (1)
#endif
#ifndef BSP_ENCODER_RIGHT_SIGN
#define BSP_ENCODER_RIGHT_SIGN (1)
#endif

typedef enum {
    ENCODER_SIDE_LEFT = 0,
    ENCODER_SIDE_RIGHT,
    ENCODER_SIDE_MAX
} encoder_side_t;

void encoder_bsp_init(void);

/** 累计脉冲（有符号，由 A 上升沿 + B 相判定方向） */
int32_t encoder_bsp_pulse_count_get(encoder_side_t side);

/** 脉冲与速度采样基准清零（需与脉冲一致，避免速度尖峰） */
void encoder_bsp_pulse_count_reset(encoder_side_t side);

/** 自上次复位以来累计线位移 (m)，符号与脉冲一致 */
float encoder_bsp_distance_m_get(encoder_side_t side);

void encoder_bsp_distance_reset(encoder_side_t side);

/**
 * 在固定控制周期内调用（如与循迹相同的 dt），根据脉冲差更新线速度 (m/s)。
 * 应在循环中、在读取 `encoder_bsp_velocity_m_s_get` 之前调用。
 */
void encoder_bsp_velocity_sample(float dt_sec);

float encoder_bsp_velocity_m_s_get(encoder_side_t side);

#endif /* BSP_ENCODER_H_ */
