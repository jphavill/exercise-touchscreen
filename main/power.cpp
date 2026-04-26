#include "power.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#include "config.h"

enum class BacklightState : uint8_t {
  Unknown,
  Off,
  Dim,
  On
};

static BacklightState g_state = BacklightState::Unknown;
static bool g_pwmReady = false;

static void set_backlight(uint8_t level, BacklightState state) {
  if (g_state == state) {
    return;
  }
  if (!g_pwmReady) {
    gpio_set_level(static_cast<gpio_num_t>(BACKLIGHT_PIN), level > 0 ? 1 : 0);
    g_state = state;
    return;
  }
  ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(BACKLIGHT_PWM_CHANNEL), level);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(BACKLIGHT_PWM_CHANNEL));
  g_state = state;
}

void power_init() {
  ledc_timer_config_t timer_cfg = {};
  timer_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
  timer_cfg.duty_resolution = static_cast<ledc_timer_bit_t>(BACKLIGHT_PWM_RES_BITS);
  timer_cfg.timer_num = LEDC_TIMER_0;
  timer_cfg.freq_hz = BACKLIGHT_PWM_FREQ;
  timer_cfg.clk_cfg = LEDC_AUTO_CLK;

  ledc_channel_config_t channel_cfg = {};
  channel_cfg.gpio_num = BACKLIGHT_PIN;
  channel_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
  channel_cfg.channel = static_cast<ledc_channel_t>(BACKLIGHT_PWM_CHANNEL);
  channel_cfg.intr_type = LEDC_INTR_DISABLE;
  channel_cfg.timer_sel = LEDC_TIMER_0;
  channel_cfg.duty = 0;
  channel_cfg.hpoint = 0;

  g_pwmReady = (ledc_timer_config(&timer_cfg) == ESP_OK) &&
               (ledc_channel_config(&channel_cfg) == ESP_OK);

  if (!g_pwmReady) {
    gpio_reset_pin(static_cast<gpio_num_t>(BACKLIGHT_PIN));
    gpio_set_direction(static_cast<gpio_num_t>(BACKLIGHT_PIN), GPIO_MODE_OUTPUT);
  }

  g_state = BacklightState::Unknown;
  set_backlight(BACKLIGHT_LEVEL_OFF, BacklightState::Off);
}

void backlight_on() {
  set_backlight(BACKLIGHT_LEVEL_ON, BacklightState::On);
}

void backlight_dim() {
  set_backlight(BACKLIGHT_LEVEL_DIM, BacklightState::Dim);
}

void backlight_off() {
  set_backlight(BACKLIGHT_LEVEL_OFF, BacklightState::Off);
}
