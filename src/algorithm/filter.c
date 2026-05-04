#include "filter.h"

#include <assert.h>

void filter_ma_f32_init(filter_ma_f32_t *ctx, float *p_storage, uint32_t window_len)
{
    assert(ctx != NULL);
    assert(p_storage != NULL);
    assert(window_len >= 1U);

    ctx->p_buf       = p_storage;
    ctx->window_len  = window_len;
    ctx->idx         = 0U;
    ctx->fill        = 0U;
    ctx->sum         = 0.0f;
}

void filter_ma_f32_reset(filter_ma_f32_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->idx  = 0U;
    ctx->fill = 0U;
    ctx->sum  = 0.0f;
}

float filter_ma_f32_run(filter_ma_f32_t *ctx, float x)
{
    assert(ctx != NULL);
    assert(ctx->p_buf != NULL);
    assert(ctx->window_len >= 1U);

    if (ctx->fill < ctx->window_len) {
        ctx->p_buf[ctx->fill] = x;
        ctx->sum += x;
        ctx->fill++;
        return ctx->sum / (float)ctx->fill;
    }

    {
        const float oldest = ctx->p_buf[ctx->idx];
        ctx->sum += x - oldest;
        ctx->p_buf[ctx->idx] = x;
        ctx->idx             = (ctx->idx + 1U) % ctx->window_len;
        return ctx->sum / (float)ctx->window_len;
    }
}

static float filter_lpf_clamp_alpha(float alpha)
{
    if (alpha > 1.0f) {
        return 1.0f;
    }
    if (alpha < 1.0e-9f) {
        return 1.0e-9f;
    }
    return alpha;
}

void filter_lpf_1st_f32_init_alpha(filter_lpf_1st_f32_t *ctx, float alpha, float y0)
{
    assert(ctx != NULL);
    ctx->alpha = filter_lpf_clamp_alpha(alpha);
    ctx->y     = y0;
}

void filter_lpf_1st_f32_init_tau(filter_lpf_1st_f32_t *ctx, float sample_dt_sec, float time_constant_sec, float y0)
{
    float alpha = 1.0f;

    assert(ctx != NULL);

    if ((sample_dt_sec > 0.0f) && (time_constant_sec > 0.0f)) {
        alpha = sample_dt_sec / (time_constant_sec + sample_dt_sec);
    } else {
        /* dt 或 tau 无效时退化为略平滑 */
        alpha = 1.0f;
    }

    filter_lpf_1st_f32_init_alpha(ctx, alpha, y0);
}

void filter_lpf_1st_f32_reset(filter_lpf_1st_f32_t *ctx, float y0)
{
    if (ctx == NULL) {
        return;
    }
    ctx->y = y0;
}

void filter_lpf_1st_f32_alpha_set(filter_lpf_1st_f32_t *ctx, float alpha)
{
    if (ctx == NULL) {
        return;
    }
    ctx->alpha = filter_lpf_clamp_alpha(alpha);
}

float filter_lpf_1st_f32_run(filter_lpf_1st_f32_t *ctx, float x)
{
    assert(ctx != NULL);
    ctx->y = ctx->alpha * x + (1.0f - ctx->alpha) * ctx->y;
    return ctx->y;
}
