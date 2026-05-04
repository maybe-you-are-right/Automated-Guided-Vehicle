#ifndef PID_H_
#define PID_H_

#include <stddef.h>
/**
 * 离散 PID（输入为误差 error = 设定值 − 测量值，或由循迹给出的偏差）。
 * 每步调用 pid_compute()，传入本周期误差与采样周期 dt（秒）。
 */

typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    /** |integral| 上限；<= 0 表示不做积分限幅（慎用，易积分饱和） */
    float integral_limit;
    float out_min;
    float out_max;
} pid_context_t;

void pid_init(pid_context_t *ctx, float kp, float ki, float kd, float integral_limit, float out_min, float out_max);

void pid_reset(pid_context_t *ctx);

/**
 * @param error   当前误差
 * @param dt_sec  控制周期，单位秒；须 > 0（例如 0.01 表示 10 ms）
 * @return        控制量（已按 out_min/out_max 限幅）
 */
float pid_compute(pid_context_t *ctx, float error, float dt_sec);

#endif /* PID_H_ */
