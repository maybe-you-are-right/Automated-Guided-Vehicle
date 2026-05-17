#include "bsp_track.h"

#include "common_data.h"
#include "r_ioport.h"

/* P002 / P001 / P000 → AD0 / AD1 / AD2；P013 → OUT */
#define TRACK_PIN_AD0 BSP_IO_PORT_00_PIN_02
#define TRACK_PIN_AD1 BSP_IO_PORT_00_PIN_01
#define TRACK_PIN_AD2 BSP_IO_PORT_00_PIN_00
#define TRACK_PIN_OUT BSP_IO_PORT_00_PIN_13

static void track_set_address(uint8_t channel)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, TRACK_PIN_AD0,
                      (channel & 0x01U) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TRACK_PIN_AD1,
                      (channel & 0x02U) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, TRACK_PIN_AD2,
                      (channel & 0x04U) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

static void track_settle_delay(void)
{
    R_BSP_SoftwareDelay(TRACK_MUX_DELAY_US, BSP_DELAY_UNITS_MICROSECONDS);
}

void track_bsp_init(void)
{
    /* 与 TrackSensor_Init 一致：地址线默认拉低 */
    track_set_address(0U);
}

uint8_t track_bsp_read_out_raw(void)
{
    bsp_io_level_t level = BSP_IO_LEVEL_LOW;

    (void)R_IOPORT_PinRead(&g_ioport_ctrl, TRACK_PIN_OUT, &level);
    return (level == BSP_IO_LEVEL_HIGH) ? 1U : 0U;
}

uint8_t track_bsp_read_channel(uint8_t ch)
{
    if (ch >= TRACK_SENSOR_NUM) {
        return 0U;
    }

    track_set_address(ch);
    track_settle_delay();
    return track_bsp_read_out_raw();
}

void track_bsp_read_all(uint8_t out_levels[TRACK_SENSOR_NUM])
{
    for (uint8_t channel = 0U; channel < TRACK_SENSOR_NUM; channel++) {
        track_set_address(channel);
        track_settle_delay();
        out_levels[channel] = track_bsp_read_out_raw();
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
