# RA4M2 红外循迹小车

这是一个基于 Renesas RA4M2 的红外循迹机器人固件工程，使用 e² studio / FSP 生成底层初始化代码，手写业务代码集中在 `src/` 目录。项目实现了 8 路红外传感器循迹、差速电机控制、编码器速度反馈、PID 调节和 UART 启停控制，适合作为 RA 系列 MCU 小车控制、循迹算法调试和嵌入式分层设计的参考工程。

## 功能特性

- 8 路红外循迹传感器读取，使用加权重心法计算归一化线路偏差
- 外环循迹 PID，根据线路偏差输出左右轮差速修正量
- 左右轮速度内环 PID，结合编码器反馈提升速度一致性
- 一阶低通滤波处理编码器测量速度，降低速度反馈抖动
- 双 PWM H 桥电机驱动抽象，支持前进、后退、刹车和滑行
- UART 命令轮询，支持运行 / 停止控制
- 主要控制周期默认 10 ms，基础速度和 PID 参数可通过宏配置

## 硬件概览

| 模块 | 说明 |
| --- | --- |
| 主控 | Renesas RA4M2 |
| 开发环境 | e² studio + Renesas FSP |
| 循迹传感器 | 8 路红外循迹模块，3 位地址线 + 单路 OUT |
| 电机驱动 | 左右轮各一路双 PWM H 桥控制 |
| 编码器 | 左右轮增量编码器，外部 IRQ 计数 |
| 通信 | SCI9 UART |

更详细的引脚说明见 [docs/引脚配置说明.md](docs/%E5%BC%95%E8%84%9A%E9%85%8D%E7%BD%AE%E8%AF%B4%E6%98%8E.md)。

## 软件结构

```text
src/
├── hal_entry.c              # 主入口与 10 ms 周期控制循环
├── app/
│   ├── APP_MOTOR.c/.h       # 循迹、差速控制、速度内环 PID
│   └── app_uart.c/.h        # 应用层 UART 命令处理
├── algorithm/
│   ├── pid.c/.h             # 离散 PID 控制器
│   └── filter.c/.h          # 一阶低通滤波
└── bsp/
    ├── bsp_motor.c/.h       # 电机 PWM 与方向控制
    ├── bsp_track.c/.h       # 8 路红外传感器读取
    ├── bsp_encoder.c/.h     # 编码器计数、距离与速度估算
    └── bsp_uart.c/.h        # UART BSP 封装
```

生成代码和 FSP 配置主要位于：

- `ra_gen/`
- `ra_cfg/`
- `ra/`
- `configuration.xml`

通常应优先修改 `src/` 下的手写代码，只有在调整外设、引脚或 FSP 配置时才更新生成相关目录。

## 控制流程

`hal_entry()` 完成 HAL、UART、编码器、电机与循迹模块初始化后进入周期循环：

1. 轮询 UART 命令，处理小车启停状态。
2. 采样编码器速度。
3. 读取 8 路红外传感器状态。
4. 计算线路偏差并执行循迹 PID。
5. 根据差速结果生成左右轮目标速度。
6. 执行左右轮速度 PID 内环修正。
7. 输出最终 PWM 占空比到电机驱动。

默认配置：

```c
#define HAL_LINE_FOLLOW_BASE_SPEED (38.0f)
#define HAL_LINE_FOLLOW_PERIOD_MS  (10U)
```

## 构建方法

本工程使用 e² studio 生成的 Makefile。命令行构建可进入 `Debug` 目录：

```bash
cd Debug
make
```

清理并重新构建：

```bash
cd Debug
make clean
make all
```

常见输出文件：

- `Debug/game_project1.elf`
- `Debug/game_project1.hex`
- `Debug/game_project1.srec`
- `Debug/game_project1.map`

## 参数调试入口

循迹 PID 默认参数位于 `src/app/APP_MOTOR.h`：

```c
#define APP_MOTOR_PID_KP_DEFAULT (50.0f)
#define APP_MOTOR_PID_KI_DEFAULT (0.0f)
#define APP_MOTOR_PID_KD_DEFAULT (8.0f)
```

速度内环 PID 默认参数：

```c
#define APP_MOTOR_SPEED_PID_KP_DEFAULT (35.0f)
#define APP_MOTOR_SPEED_PID_KI_DEFAULT (12.0f)
#define APP_MOTOR_SPEED_PID_KD_DEFAULT (0.08f)
```

运行时可通过以下接口读取调试量：

- `app_motor_last_line_error_get()`
- `app_motor_last_pid_out_get()`
- `app_motor_last_speed_err_left_get()`
- `app_motor_last_speed_err_right_get()`
- `app_motor_speed_meas_raw_m_s_get()`
- `app_motor_speed_meas_filtered_m_s_get()`

## 开发说明

- 代码标准为 C99。
- 手写代码主要在 `src/` 目录。
- `ra_gen/`、`ra_cfg/`、`ra/` 和 `configuration.xml` 由 FSP / e² studio 管理。
- 调整引脚、GPT、SCI、IRQ 等外设配置后，需要重新生成 FSP 代码，并同步检查 BSP 映射。
- 固件参数需要结合实际底盘、电机、轮径、编码器线数和赛道材质进行标定。

## 许可证

当前仓库尚未添加开源许可证。如需作为正式公开项目长期维护，建议补充 `LICENSE` 文件。
