#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t lvgl_port_get_required_frame_buffer_count(void);
bool lvgl_port_init(void *panel_handle, void *touch_handle, uint16_t width, uint16_t height,
                    uint8_t x_align, uint8_t y_align);
bool lvgl_port_deinit(void);
bool lvgl_port_lock(int timeout_ms);
bool lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif
