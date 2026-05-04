#include "bsp_track.h"

#include "common_data.h"
#include "r_ioport.h"

/* P000 / P001 / P002 → AD0 / AD1 / AD2；P013 → OUT */
#define TRACK_PIN_AD0 BSP_IO_PORT_00_PIN_00
#define TRACK_PIN_AD1 BSP_IO_PORT_00_PIN_01
#define TRACK_PIN_AD2 BSP_IO_PORT_00_PIN_02
#define TRACK_PIN_OUT BSP_IO_PORT_00_PIN_13

static void track_set_address(uint8_t addr_0_to_7)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, TRACK_PIN_AD0, (addr_0_to_7 & 1U) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TRACK_PIN_AD1, (addr_0_to_7 & 2U) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TRACK_PIN_AD2, (addr_0_to_7 & 4U) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

void track_bsp_init(void)
{
    /* IOPORT 与引脚模式由 g_common_init() / R_IOPORT_Open 完成；此处可留空或做上电默认地址 */
    track_set_address(0U);
}

uint8_t track_bsp_read_channel(uint8_t ch)
{
    if (ch >= TRACK_SENSOR_NUM) {
        return 0U;
    }

    track_set_address(ch);
    R_BSP_SoftwareDelay(TRACK_MUX_DELAY_US, BSP_DELAY_UNITS_MICROSECONDS);

    bsp_io_level_t level = BSP_IO_LEVEL_LOW;
    (void)R_IOPORT_PinRead(&g_ioport_ctrl, TRACK_PIN_OUT, &level);

    return (level == BSP_IO_LEVEL_HIGH) ? 1U : 0U;
}

void track_bsp_read_all(uint8_t out_levels[TRACK_SENSOR_NUM])
{
    for (uint8_t i = 0U; i < TRACK_SENSOR_NUM; i++) {
        out_levels[i] = track_bsp_read_channel(i);
    }
}

uint8_t track_bsp_read_mask(void)
{
    uint8_t mask = 0U;

    for (uint8_t i = 0U; i < TRACK_SENSOR_NUM; i++) {
        if (track_bsp_read_channel(i) != 0U) {
            mask |= (uint8_t)(1U << i);
        }
    }

    return mask;
}
