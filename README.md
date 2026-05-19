# RA4M2 红外循迹小车

基于 **Renesas RA4M2** 的红外循迹机器人固件。底层外设由 **e² studio / FSP** 生成（`ra_gen/`、`ra_cfg/`），业务逻辑集中在 **`src/`**。当前默认以 **10 ms 外环循迹 PID + 差速占空比** 运行；编码器速度内环与 UART 灰度调试可在宏开关下启用。

## 功能特性

| 功能 | 说明 |
| --- | --- |
| 8 路循迹 | 3 线地址选通 + 单线 OUT，逻辑对齐 STM32 参考 `track_test` |
| 外环循迹 | 加权重心 → 归一化偏差约 ±1 → PID → 左右轮差速（%） |
| 速度内环 | 代码保留（编码器 + 低通 + 速度 PID），**默认关闭**，可按需恢复 |
| 电机驱动 | 左右各 1 个 GPT，**双 PWM 互补** 输入 H 桥驱动板 |
| UART | `STOP` / `RUN` 启停；可选周期打印 8 路灰度（默认关） |
| 控制周期 | 默认 **10 ms**（`HAL_LINE_FOLLOW_PERIOD_MS`） |

## 硬件概览

| 模块 | 说明 |
| --- | --- |
| 主控 | Renesas RA4M2 |
| 开发环境 | e² studio + Renesas FSP |
| 循迹 | 8 路模块：**P002/P001/P000** → AD0/AD1/AD2，**P013** → OUT |
| 电机 PWM | 左 GPT2（P102/P103），右 GPT4（P301/P302） |
| 编码器 | 左右增量式，A 相 IRQ + B 相 GPIO |
| 串口 | SCI9：**P109** TX，**P110** RX |

引脚与 GPT 绑定详见 **[docs/引脚配置说明.md](docs/%E5%BC%95%E8%84%9A%E9%85%8D%E7%BD%AE%E8%AF%B4%E6%98%8E.md)**。

## 软件结构

```text
ra_gen/main.c          → g_hal_init() → hal_entry()
src/
├── hal_entry.c        # 应用入口、10 ms 主循环、UART 启停
├── app/
│   ├── APP_MOTOR.c/.h # 循迹外环 PID、差速、可选速度内环
│   └── app_uart.c/.h  # STOP/RUN、可选灰度调试输出
├── algorithm/
│   ├── pid.c/.h       # 离散 PID
│   └── filter.c/.h    # 一阶低通（速度内环用）
└── bsp/
    ├── bsp_track.c/.h   # 8 路灰度选通读取
    ├── bsp_motor.c/.h   # 双 PWM 电机
    ├── bsp_encoder.c/.h # 编码器计数与速度
    └── bsp_uart.c/.h    # SCI9 环形缓冲
```

**生成代码（慎改）**：`ra_gen/`、`ra_cfg/`、`ra/`、`configuration.xml`。改引脚或定时器请在 FSP 中配置后重新生成，并核对 `src/bsp/` 中的引脚宏。

## 控制流程（当前默认）

```text
hal_entry 每 10 ms:
  1. car_uart_command_poll()     // STOP / RUN
  2. [可选] car_uart_track_periodic_poll()  // HAL_TRACK_DEBUG_UART=1
  3. 若 STOP → app_motor_stop()，延时后继续
  4. 若 RUN  → app_motor_line_follow_update(base=15%, dt=0.01s)
               // 内部: track_bsp_read_all → 外环 PID → motor_bsp_set_speed
  5. R_BSP_SoftwareDelay(10 ms)
```

**说明：**

- **外环**：`APP_MOTOR.c` 中 `line_follow_update` 读 8 路 → `line_normalized_error` → `pid_compute` → `left = base ± corr`。
- **内环**：`line_follow_update` 内速度 PID 段当前为注释状态；`hal_entry` RUN 路径未调用 `encoder_bsp_velocity_sample`。启用内环需同时恢复两处逻辑。
- **循迹电平**：黑线 / 检测到 → OUT 高 → `levels[i]=1`（可用 `app_motor_set_sensor_invert(1)` 取反）。
- **通道顺序**：`ch0` 为最左探头，`ch7` 为最右。

## 默认参数（与源码一致）

主循环（`src/hal_entry.c`）：

```c
#define HAL_LINE_FOLLOW_BASE_SPEED (15.0f)   /* 基准占空比 % */
#define HAL_LINE_FOLLOW_PERIOD_MS  (10U)
```

循迹外环 PID（`src/app/APP_MOTOR.h`）：

```c
#define APP_MOTOR_PID_KP_DEFAULT           (8.0f)
#define APP_MOTOR_PID_KI_DEFAULT           (0.0f)
#define APP_MOTOR_PID_KD_DEFAULT           (0.0f)
#define APP_MOTOR_CORRECTION_LIMIT_DEFAULT (15.0f)
```

循迹 BSP（`src/bsp/bsp_track.h`）：

```c
#define TRACK_MUX_DELAY_US (500U)   /* 地址切换后稳定时间，勿随意改小 */
```

UART（`src/app/app_uart.h`）：

```c
#define HAL_TRACK_DEBUG_UART (0U)              /* 1=周期打印 8 路灰度 */
#define HAL_TRACK_REPORT_INTERVAL_MS (2000U)
```

## 串口使用

| 命令 | 作用 |
| --- | --- |
| `STOP` | 刹车并停止循迹更新 |
| `RUN` | 恢复循迹（从 STOP 恢复时会 `app_motor_pid_reset()`） |

连接 **P109（MCU TX）→ USB 转串口 RX**，**P110（MCU RX）→ USB 转串口 TX**，共地。波特率见 `ra_gen/hal_data.c` 中 `g_uart9_cfg`。

开启灰度调试：将 `HAL_TRACK_DEBUG_UART` 设为 `1U` 后重新编译，串口将周期性输出类似 `00110000 D=0`（8 位从左到右，黑线为 1）。

## 构建方法

```bash
cd Debug
make
```

清理并完整构建：

```bash
cd Debug
make clean
make all
```

输出：`Debug/game_project1.elf`、`.hex`、`.srec`、`.map`。

## 参数与调试入口

| 目的 | 位置 |
| --- | --- |
| 循迹 PID / 修正限幅 | `src/app/APP_MOTOR.h` |
| 传感器权重、丢线逻辑 | `src/app/APP_MOTOR.c`（`s_weights`） |
| 基准速度、周期 | `src/hal_entry.c` |
| 灰度引脚、稳定时间 | `src/bsp/bsp_track.c`、`.h` |
| 电机 GTIOCA/B、互补 PWM | `src/bsp/bsp_motor.c`、`.h` |
| 速度内环 PID / 低通 | `APP_MOTOR.h` + `APP_MOTOR.c` 内环代码块 |
| 运行时误差 / PID 输出 | `app_motor_last_line_error_get()`、`app_motor_last_pid_out_get()` |

速度内环相关（内环启用后有效）：`app_motor_last_speed_err_*_get()`、`app_motor_speed_meas_*_m_s_get()`。

## 开发说明

- 语言标准：**C99**；优化 `-O2`，浮点 **hard** ABI。
- 命名：`layer_module_action()`，类型 `*_t`；注释多为中文。
- 修改引脚：FSP 重新生成 → 更新 `bsp_*.c` 宏 → 更新 `docs/引脚配置说明.md`。
- 实车标定：PID、基准速度、轮径/编码器比例、传感器 invert 需上车调整。
- AI 协作文档：仓库根目录 `AGENTS.md`、`CLAUDE.md`。

## 许可证

仓库尚未添加开源许可证文件；公开维护时建议补充 `LICENSE`。
