#ifndef BSP_MOTOR_H_
#define BSP_MOTOR_H_

#include "hal_data.h"

/**
 * 单轮双 PWM 波输入电机驱动（H 桥类驱动板：每轮两路 PWM 占空比输入）：
 * - 左轮：一个 GPT（`g_timer3`，硬件 GPT2），GTIOCA / GTIOCB 各一路 PWM → P103 正转侧、P102 反转侧
 * - 右轮：一个 GPT（`g_timer4`，硬件 GPT4），GTIOCA / GTIOCB 各一路 PWM → P302 正转侧、P301 反转侧
 * 与 `ra_gen/hal_data`、`configuration.xml` 一致；两路 GPT 的 period_counts 须相同。
 * 速度接口：`motor_bsp_set_speed` 正占空比仅正转侧 PWM，负占空比仅反转侧 PWM。
 * 若 PCB 桥臂与 GTIOCA/B 对调，在 `bsp_motor.c` 的 `s_motor_wheel_dual_pwm` 中交换 pin_fwd / pin_rev。
 */

typedef enum {
    MOTOR_LEFT = 0,
    MOTOR_RIGHT,
    MOTOR_MAX
} motor_id_t;

typedef enum {
    MOTOR_DIR_FORWARD = 0,
    MOTOR_DIR_BACKWARD,
    MOTOR_DIR_BRAKE,
    MOTOR_DIR_COAST
} motor_dir_t;

void motor_bsp_init(void);
void motor_bsp_set_speed(motor_id_t motor, float duty_percent);
/**
 * 仅方向、不设百分比时：FORWARD/BACKWARD 为双 PWM 单侧 100% 占空（与 set_speed(±100) 一致）；
 * COAST 为双侧 0（滑行）；BRAKE 为双侧满占短接制动。
 */
void motor_bsp_set_direction(motor_id_t motor, motor_dir_t dir);
void motor_bsp_stop(motor_id_t motor, motor_dir_t stop_mode);

/**
 * 单轮双 PWM 同时设占空比（各 0…100%）。循迹常用 `motor_bsp_set_speed` 即可；
 * 需独立控制两桥臂占空比时可用本接口（注意避免两侧同时大占空造成直通，依驱动板说明）。
 */
void motor_bsp_set_dual_pwm(motor_id_t motor, float duty_forward_percent, float duty_reverse_percent);

#endif /* BSP_MOTOR_H_ */
