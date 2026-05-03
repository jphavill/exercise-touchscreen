/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_timer.h"
#include <assert.h>
#include <limits.h>
#undef ESP_UTILS_LOG_TAG
#define ESP_UTILS_LOG_TAG "LvPort"
#include "esp_lib_utils.h"
#include "lvgl_v8_port.h"

using namespace esp_panel::drivers;

#define LVGL_PORT_BUFFER_NUM_MAX                (2)

static SemaphoreHandle_t lvgl_mux = nullptr;
static TaskHandle_t lvgl_task_handle = nullptr;
static esp_timer_handle_t lvgl_tick_timer = NULL;
static void *lvgl_buf[LVGL_PORT_BUFFER_NUM_MAX] = {};

#if LVGL_PORT_AVOID_TEAR
#if LVGL_PORT_DIRECT_MODE
static void flush_callback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    LCD *lcd = (LCD *)drv->user_data;

    if (lv_disp_flush_is_last(drv)) {
        lcd->switchFrameBufferTo(color_map);

        ulTaskNotifyValueClear(NULL, ULONG_MAX);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    lv_disp_flush_ready(drv);
}

#elif LVGL_PORT_FULL_REFRESH && LVGL_PORT_DISP_BUFFER_NUM == 2

static void flush_callback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    LCD *lcd = (LCD *)drv->user_data;

    lcd->switchFrameBufferTo(color_map);

    ulTaskNotifyValueClear(NULL, ULONG_MAX);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    lv_disp_flush_ready(drv);
}

#elif LVGL_PORT_FULL_REFRESH && LVGL_PORT_DISP_BUFFER_NUM == 3

static void *lvgl_port_lcd_last_buf = NULL;
static void *lvgl_port_lcd_next_buf = NULL;
static void *lvgl_port_flush_next_buf = NULL;

void flush_callback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    LCD *lcd = (LCD *)drv->user_data;

    drv->draw_buf->buf1 = color_map;
    drv->draw_buf->buf2 = lvgl_port_flush_next_buf;
    lvgl_port_flush_next_buf = color_map;

    lcd->switchFrameBufferTo(color_map);

    lvgl_port_lcd_next_buf = color_map;

    lv_disp_flush_ready(drv);
}
#endif

IRAM_ATTR bool onLcdVsyncCallback(void *user_data)
{
    BaseType_t need_yield = pdFALSE;
#if LVGL_PORT_FULL_REFRESH && (LVGL_PORT_DISP_BUFFER_NUM == 3)
    if (lvgl_port_lcd_next_buf != lvgl_port_lcd_last_buf) {
        lvgl_port_flush_next_buf = lvgl_port_lcd_last_buf;
        lvgl_port_lcd_last_buf = lvgl_port_lcd_next_buf;
    }
#else
    TaskHandle_t task_handle = (TaskHandle_t)user_data;
    xTaskNotifyFromISR(task_handle, ULONG_MAX, eNoAction, &need_yield);
#endif
    return (need_yield == pdTRUE);
}

#else

void flush_callback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    LCD *lcd = (LCD *)drv->user_data;
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

    lcd->drawBitmap(offsetx1, offsety1, offsetx2 - offsetx1 + 1, offsety2 - offsety1 + 1, (const uint8_t *)color_map);
    if (lcd->getBus()->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        lv_disp_flush_ready(drv);
    }
}

static void update_callback(lv_disp_drv_t *drv)
{
    LCD *lcd = (LCD *)drv->user_data;
    auto transformation = lcd->getTransformation();
    static bool disp_init_mirror_x = transformation.mirror_x;
    static bool disp_init_mirror_y = transformation.mirror_y;
    static bool disp_init_swap_xy = transformation.swap_xy;

    switch (drv->rotated) {
    case LV_DISP_ROT_NONE:
        lcd->swapXY(disp_init_swap_xy);
        lcd->mirrorX(disp_init_mirror_x);
        lcd->mirrorY(disp_init_mirror_y);
        break;
    case LV_DISP_ROT_90:
        lcd->swapXY(!disp_init_swap_xy);
        lcd->mirrorX(disp_init_mirror_x);
        lcd->mirrorY(!disp_init_mirror_y);
        break;
    case LV_DISP_ROT_180:
        lcd->swapXY(disp_init_swap_xy);
        lcd->mirrorX(!disp_init_mirror_x);
        lcd->mirrorY(!disp_init_mirror_y);
        break;
    case LV_DISP_ROT_270:
        lcd->swapXY(!disp_init_swap_xy);
        lcd->mirrorX(!disp_init_mirror_x);
        lcd->mirrorY(disp_init_mirror_y);
        break;
    }

    ESP_UTILS_LOGD("Update display rotation to %d", drv->rotated);
}

#endif

void rounder_callback(lv_disp_drv_t *drv, lv_area_t *area)
{
    LCD *lcd = (LCD *)drv->user_data;
    uint8_t x_align = lcd->getBasicAttributes().basic_bus_spec.x_coord_align;
    uint8_t y_align = lcd->getBasicAttributes().basic_bus_spec.y_coord_align;

    if (x_align > 1) {
        area->x1 &= ~(x_align - 1);
        area->x2 = (area->x2 & ~(x_align - 1)) + x_align - 1;
    }

    if (y_align > 1) {
        area->y1 &= ~(y_align - 1);
        area->y2 = (area->y2 & ~(y_align - 1)) + y_align - 1;
    }
}

static lv_disp_t *display_init(LCD *lcd)
{
    ESP_UTILS_CHECK_FALSE_RETURN(lcd != nullptr, nullptr, "Invalid LCD device");
    ESP_UTILS_CHECK_FALSE_RETURN(lcd->getRefreshPanelHandle() != nullptr, nullptr, "LCD device is not initialized");

    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;

    auto lcd_width = lcd->getFrameWidth();
    auto lcd_height = lcd->getFrameHeight();
    int buffer_size = 0;

    ESP_UTILS_LOGD("Malloc memory for LVGL buffer");
#if !LVGL_PORT_AVOID_TEAR
    buffer_size = lcd_width * LVGL_PORT_BUFFER_SIZE_HEIGHT;
    for (int i = 0; (i < LVGL_PORT_BUFFER_NUM) && (i < LVGL_PORT_BUFFER_NUM_MAX); i++) {
        lvgl_buf[i] = heap_caps_malloc(buffer_size * sizeof(lv_color_t), LVGL_PORT_BUFFER_MALLOC_CAPS);
        assert(lvgl_buf[i]);
        ESP_UTILS_LOGD("Buffer[%d] address: %p, size: %d", i, lvgl_buf[i], buffer_size * sizeof(lv_color_t));
    }
#else
    buffer_size = lcd_width * lcd_height;
#if (LVGL_PORT_DISP_BUFFER_NUM >= 3) && LVGL_PORT_FULL_REFRESH
    lvgl_port_lcd_last_buf = lcd->getFrameBufferByIndex(0);
    lvgl_buf[0] = lcd->getFrameBufferByIndex(1);
    lvgl_buf[1] = lcd->getFrameBufferByIndex(2);
    lvgl_port_lcd_next_buf = lvgl_port_lcd_last_buf;
    lvgl_port_flush_next_buf = lvgl_buf[1];
#elif LVGL_PORT_DISP_BUFFER_NUM >= 2
    for (int i = 0; (i < LVGL_PORT_DISP_BUFFER_NUM) && (i < LVGL_PORT_BUFFER_NUM_MAX); i++) {
        lvgl_buf[i] = lcd->getFrameBufferByIndex(i);
    }
#endif
#endif

    lv_disp_draw_buf_init(&disp_buf, lvgl_buf[0], lvgl_buf[1], buffer_size);

    ESP_UTILS_LOGD("Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = flush_callback;
    disp_drv.hor_res = lcd_width;
    disp_drv.ver_res = lcd_height;
#if LVGL_PORT_AVOID_TEAR
#if LVGL_PORT_FULL_REFRESH
    disp_drv.full_refresh = 1;
#elif LVGL_PORT_DIRECT_MODE
    disp_drv.direct_mode = 1;
#endif
#else
    if (lcd->getBasicAttributes().basic_bus_spec.isFunctionValid(LCD::BasicBusSpecification::FUNC_SWAP_XY) &&
            lcd->getBasicAttributes().basic_bus_spec.isFunctionValid(LCD::BasicBusSpecification::FUNC_MIRROR_X) &&
            lcd->getBasicAttributes().basic_bus_spec.isFunctionValid(LCD::BasicBusSpecification::FUNC_MIRROR_Y)) {
        disp_drv.drv_update_cb = update_callback;
    } else {
        disp_drv.sw_rotate = 1;
    }
#endif
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = (void *)lcd;
    if ((lcd->getBasicAttributes().basic_bus_spec.x_coord_align > 1) ||
            (lcd->getBasicAttributes().basic_bus_spec.y_coord_align > 1)) {
        disp_drv.rounder_cb = rounder_callback;
    }

    return lv_disp_drv_register(&disp_drv);
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    Touch *tp = (Touch *)indev_drv->user_data;
    TouchPoint point;

    int read_touch_result = tp->readPoints(&point, 1, 0);
    if (read_touch_result > 0) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static lv_indev_t *indev_init(Touch *tp)
{
    ESP_UTILS_CHECK_FALSE_RETURN(tp != nullptr, nullptr, "Invalid touch device");
    ESP_UTILS_CHECK_FALSE_RETURN(tp->getPanelHandle() != nullptr, nullptr, "Touch device is not initialized");

    static lv_indev_drv_t indev_drv_tp;

    ESP_UTILS_LOGD("Register input driver to LVGL");
    lv_indev_drv_init(&indev_drv_tp);
    indev_drv_tp.type = LV_INDEV_TYPE_POINTER;
    indev_drv_tp.read_cb = touchpad_read;
    indev_drv_tp.user_data = (void *)tp;

    return lv_indev_drv_register(&indev_drv_tp);
}

#if !LV_TICK_CUSTOM
static void tick_increment(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_PORT_TICK_PERIOD_MS);
}

static bool tick_init(void)
{
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &tick_increment,
        .name = "LVGL tick"
    };
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer), false, "Create LVGL tick timer failed"
    );
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_timer_start_periodic(lvgl_tick_timer, LVGL_PORT_TICK_PERIOD_MS * 1000), false,
        "Start LVGL tick timer failed"
    );

    return true;
}

static bool tick_deinit(void)
{
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_timer_stop(lvgl_tick_timer), false, "Stop LVGL tick timer failed"
    );
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_timer_delete(lvgl_tick_timer), false, "Delete LVGL tick timer failed"
    );
    return true;
}
#endif

static void lvgl_port_task(void *arg)
{
    (void)arg;
    ESP_UTILS_LOGD("Starting LVGL task");

    uint32_t task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
    while (1) {
        if (lvgl_port_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            lvgl_port_unlock();
        }
        if (task_delay_ms > LVGL_PORT_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_PORT_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_PORT_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

IRAM_ATTR bool onDrawBitmapFinishCallback(void *user_data)
{
    lv_disp_drv_t *drv = (lv_disp_drv_t *)user_data;

    lv_disp_flush_ready(drv);

    return false;
}

bool lvgl_port_init(LCD *lcd, Touch *tp)
{
    ESP_UTILS_CHECK_FALSE_RETURN(lcd != nullptr, false, "Invalid LCD device");

    auto bus_type = lcd->getBus()->getBasicAttributes().type;
#if LVGL_PORT_AVOID_TEAR
    ESP_UTILS_CHECK_FALSE_RETURN(
        (bus_type == ESP_PANEL_BUS_TYPE_RGB) || (bus_type == ESP_PANEL_BUS_TYPE_MIPI_DSI), false,
        "Avoid tearing function only works with RGB/MIPI-DSI LCD now"
    );
    ESP_UTILS_LOGI(
        "Avoid tearing is enabled, mode: %d, rotation: %d", LVGL_PORT_AVOID_TEARING_MODE, LVGL_PORT_ROTATION_DEGREE
    );
#endif

    lv_disp_t *disp = nullptr;
    lv_indev_t *indev = nullptr;

    lv_init();
#if !LV_TICK_CUSTOM
    ESP_UTILS_CHECK_FALSE_RETURN(tick_init(), false, "Initialize LVGL tick failed");
#endif

    ESP_UTILS_LOGI("Initializing LVGL display driver");
    disp = display_init(lcd);
    ESP_UTILS_CHECK_NULL_RETURN(disp, false, "Initialize LVGL display driver failed");
    lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);

    if (bus_type != ESP_PANEL_BUS_TYPE_RGB) {
        ESP_UTILS_LOGD("Attach refresh finish callback to LCD");
        lcd->attachDrawBitmapFinishCallback(onDrawBitmapFinishCallback, (void *)disp->driver);
    }

    if (tp != nullptr) {
        ESP_UTILS_LOGD("Initialize LVGL input driver");
        indev = indev_init(tp);
        ESP_UTILS_CHECK_NULL_RETURN(indev, false, "Initialize LVGL input driver failed");
    }

    ESP_UTILS_LOGD("Create mutex for LVGL");
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_mux, false, "Create LVGL mutex failed");

    ESP_UTILS_LOGD("Create LVGL task");
    BaseType_t core_id = (LVGL_PORT_TASK_CORE < 0) ? tskNO_AFFINITY : LVGL_PORT_TASK_CORE;
    BaseType_t ret = xTaskCreatePinnedToCore(lvgl_port_task, "lvgl", LVGL_PORT_TASK_STACK_SIZE, NULL,
                     LVGL_PORT_TASK_PRIORITY, &lvgl_task_handle, core_id);
    ESP_UTILS_CHECK_FALSE_RETURN(ret == pdPASS, false, "Create LVGL task failed");

#if LVGL_PORT_AVOID_TEAR
    lcd->attachRefreshFinishCallback(onLcdVsyncCallback, (void *)lvgl_task_handle);
#endif

    return true;
}

bool lvgl_port_lock(int timeout_ms)
{
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_mux, false, "LVGL mutex is not initialized");

    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE);
}

bool lvgl_port_unlock(void)
{
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_mux, false, "LVGL mutex is not initialized");

    xSemaphoreGiveRecursive(lvgl_mux);

    return true;
}

bool lvgl_port_deinit(void)
{
#if !LV_TICK_CUSTOM
    ESP_UTILS_CHECK_FALSE_RETURN(tick_deinit(), false, "Deinitialize LVGL tick failed");
#endif

    ESP_UTILS_CHECK_FALSE_RETURN(lvgl_port_lock(-1), false, "Lock LVGL failed");
    if (lvgl_task_handle != nullptr) {
        vTaskDelete(lvgl_task_handle);
        lvgl_task_handle = nullptr;
    }
    ESP_UTILS_CHECK_FALSE_RETURN(lvgl_port_unlock(), false, "Unlock LVGL failed");

#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#else
    ESP_UTILS_LOGW("LVGL memory is custom, `lv_deinit()` will not work");
#endif
#if !LVGL_PORT_AVOID_TEAR
    for (int i = 0; i < LVGL_PORT_BUFFER_NUM; i++) {
        if (lvgl_buf[i] != nullptr) {
            free(lvgl_buf[i]);
            lvgl_buf[i] = nullptr;
        }
    }
#endif
    if (lvgl_mux != nullptr) {
        vSemaphoreDelete(lvgl_mux);
        lvgl_mux = nullptr;
    }

    return true;
}
