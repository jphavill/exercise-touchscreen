#include "lvgl_port.h"

#include <assert.h>

#include "esp_log.h"
#include "esp_lv_adapter.h"

namespace {
constexpr const char *TAG = "lvgl_port";
constexpr esp_lv_adapter_rotation_t kDisplayRotation = ESP_LV_ADAPTER_ROTATE_90;
constexpr esp_lv_adapter_tear_avoid_mode_t kTearMode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;

lv_display_t *s_display = nullptr;
lv_indev_t *s_touch = nullptr;

struct DisplayRounderConfig {
    uint8_t x_align;
    uint8_t y_align;
};

DisplayRounderConfig s_rounder = {};

void round_area_cb(lv_area_t *area, void *user_data)
{
    auto *rounder = static_cast<DisplayRounderConfig *>(user_data);
    if (!rounder) {
        return;
    }

    if (rounder->x_align > 1) {
        area->x1 &= ~(rounder->x_align - 1);
        area->x2 = (area->x2 & ~(rounder->x_align - 1)) + rounder->x_align - 1;
    }

    if (rounder->y_align > 1) {
        area->y1 &= ~(rounder->y_align - 1);
        area->y2 = (area->y2 & ~(rounder->y_align - 1)) + rounder->y_align - 1;
    }
}

bool validate_logical_resolution(lv_display_t *display)
{
#if LVGL_VERSION_MAJOR < 9
    const int hor = lv_disp_get_hor_res(display);
    const int ver = lv_disp_get_ver_res(display);
#else
    const int hor = lv_display_get_horizontal_resolution(display);
    const int ver = lv_display_get_vertical_resolution(display);
#endif

    ESP_LOGI(TAG, "LVGL logical resolution: %dx%d", hor, ver);
    assert((hor == 480) && (ver == 800));
    return (hor == 480) && (ver == 800);
}
} // namespace

uint8_t lvgl_port_get_required_frame_buffer_count(void)
{
    return esp_lv_adapter_get_required_frame_buffer_count(kTearMode, kDisplayRotation);
}

bool lvgl_port_init(void *panel_handle_raw, void *touch_handle_raw, uint16_t width, uint16_t height,
                    uint8_t x_align, uint8_t y_align)
{
    if (!panel_handle_raw) {
        ESP_LOGE(TAG, "Invalid LCD panel handle");
        return false;
    }

    auto panel_handle = reinterpret_cast<esp_lcd_panel_handle_t>(panel_handle_raw);
    auto touch_handle = reinterpret_cast<esp_lcd_touch_handle_t>(touch_handle_raw);

    const esp_lv_adapter_config_t adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    esp_err_t err = esp_lv_adapter_init(&adapter_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lv_adapter_init failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_lv_adapter_display_config_t disp_cfg = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(
        panel_handle, nullptr, width, height, kDisplayRotation
    );
    disp_cfg.tear_avoid_mode = kTearMode;
    disp_cfg.profile.use_psram = true;
    disp_cfg.profile.buffer_height = 20;

    s_display = esp_lv_adapter_register_display(&disp_cfg);
    if (!s_display) {
        ESP_LOGE(TAG, "Display registration failed");
        return false;
    }

    s_rounder.x_align = x_align;
    s_rounder.y_align = y_align;
    if ((s_rounder.x_align > 1) || (s_rounder.y_align > 1)) {
        err = esp_lv_adapter_set_area_rounder_cb(s_display, round_area_cb, &s_rounder);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set area rounder callback: %s", esp_err_to_name(err));
            return false;
        }
    }

    if (touch_handle) {
        const esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(s_display, touch_handle);
        s_touch = esp_lv_adapter_register_touch(&touch_cfg);
        if (!s_touch) {
            ESP_LOGE(TAG, "Touch registration failed");
            return false;
        }
    } else {
        ESP_LOGW(TAG, "Touch handle is null, skipping touch registration");
    }

    err = esp_lv_adapter_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lv_adapter_start failed: %s", esp_err_to_name(err));
        return false;
    }

    return validate_logical_resolution(s_display);
}

bool lvgl_port_deinit(void)
{
    if (s_touch) {
        esp_lv_adapter_unregister_touch(s_touch);
        s_touch = nullptr;
    }

    if (s_display) {
        esp_lv_adapter_unregister_display(s_display);
        s_display = nullptr;
    }

    return (esp_lv_adapter_deinit() == ESP_OK);
}

bool lvgl_port_lock(int timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms) == ESP_OK;
}

bool lvgl_port_unlock(void)
{
    esp_lv_adapter_unlock();
    return true;
}
