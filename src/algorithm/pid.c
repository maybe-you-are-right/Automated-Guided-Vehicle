#include "pid.h"

void pid_init(pid_context_t *ctx, float kp, float ki, float kd, float integral_limit, float out_min, float out_max)
{
    if (ctx == NULL) {
        return;
    }

    ctx->kp              = kp;
    ctx->ki              = ki;
    ctx->kd              = kd;
    ctx->integral_limit  = integral_limit;
    ctx->out_min         = out_min;
    ctx->out_max         = out_max;
    ctx->integral        = 0.0f;
    ctx->prev_error      = 0.0f;
}

void pid_reset(pid_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    ctx->integral   = 0.0f;
    ctx->prev_error = 0.0f;
}

float pid_compute(pid_context_t *ctx, float error, float dt_sec)
{
    if (ctx == NULL) {
        return 0.0f;
    }

    if (dt_sec <= 0.0f) {
        dt_sec = 1.0f;
    }

    const float p = ctx->kp * error;

    ctx->integral += error * dt_sec;
    if (ctx->integral_limit > 0.0f) {
        if (ctx->integral > ctx->integral_limit) {
            ctx->integral = ctx->integral_limit;
        } else if (ctx->integral < -ctx->integral_limit) {
            ctx->integral = -ctx->integral_limit;
        }
    }

    const float i = ctx->ki * ctx->integral;

    const float de_dt = (error - ctx->prev_error) / dt_sec;
    ctx->prev_error     = error;
    const float d       = ctx->kd * de_dt;

    float out = p + i + d;
    if (out > ctx->out_max) {
        out = ctx->out_max;
    } else if (out < ctx->out_min) {
        out = ctx->out_min;
    }

    return out;
}
