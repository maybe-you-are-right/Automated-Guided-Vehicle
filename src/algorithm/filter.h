#ifndef FILTER_H_
#define FILTER_H_

#include <stdint.h>

/**
 * 滑动平均（float）：调用方提供长度 window_len 的缓冲区，无动态分配。
 * 未满窗时输出为当前已有样本的算术平均；满窗后为环形缓冲上的固定窗长平均。
 */

typedef struct {
    float   *p_buf;       /**< 样本缓冲，长度 >= window_len */
    uint32_t window_len;  /**< 窗长，>= 1 */
    uint32_t idx;         /**< 满窗后指向下一个将被覆盖的位置（即最旧样本下标） */
    uint32_t fill;        /**< 已写入样本数，0 … window_len */
    float    sum;         /**< 窗内样本和 */
} filter_ma_f32_t;

void  filter_ma_f32_init(filter_ma_f32_t *ctx, float *p_storage, uint32_t window_len);
void  filter_ma_f32_reset(filter_ma_f32_t *ctx);
float filter_ma_f32_run(filter_ma_f32_t *ctx, float x);

/**
 * 一阶低通 IIR：y = alpha * x + (1 - alpha) * y_prev，0 < alpha <= 1。
 * alpha 越小输出越平滑（更信历史）；alpha=1 等价于直通。
 * 亦可用时间常数初始化：alpha = dt / (tau + dt)（一阶连续系统离散近似）。
 */

typedef struct {
    float alpha; /**< 当前系数 */
    float y;     /**< 上一拍输出 */
} filter_lpf_1st_f32_t;

void filter_lpf_1st_f32_init_alpha(filter_lpf_1st_f32_t *ctx, float alpha, float y0);
void filter_lpf_1st_f32_init_tau(filter_lpf_1st_f32_t *ctx, float sample_dt_sec, float time_constant_sec, float y0);
void filter_lpf_1st_f32_reset(filter_lpf_1st_f32_t *ctx, float y0);
void filter_lpf_1st_f32_alpha_set(filter_lpf_1st_f32_t *ctx, float alpha);

/** 推入新测量 x，返回滤波后 y */
float filter_lpf_1st_f32_run(filter_lpf_1st_f32_t *ctx, float x);

#endif /* FILTER_H_ */
