#include "Valve.hpp"

#include <cstring>
#include "includes.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

Valve::~Valve()
{
    if (m_initialized) {
        deinit();
    }
}

void Valve::init(gpio_num_t gpio1, gpio_num_t gpio2, const char *name, uint16_t openDelayMs, uint16_t closeDelayMs)
{
    strncpy(m_name, name, sizeof(m_name) - 1);
    m_name[sizeof(m_name) - 1] = '\0';

    open_gpio       = gpio1;
    close_gpio       = gpio2;
    m_openDelayMs = openDelayMs;
    m_closeDelayMs = closeDelayMs;
    m_state        = ValveState::NONE;
    m_initialized  = true;

    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << gpio1) | (1ULL << gpio2),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,         // the ssr is providing GND so keep the +rail active until GPIO changes it
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    // Both outputs LOW (de-energised) at init
    // start Valve in CLOSED state
    gpio_set_level(open_gpio, 1);
    gpio_set_level(close_gpio, 0);
    // vTaskDelay(pdMS_TO_TICKS(m_closeDelayMs));
    m_state = ValveState::CLOSE;
    MESP_LOGI(TAG, "Valve '%s' initialised - GPIO%d / GPIO%d, openDelay=%u closeDelay=%u...closing please wait",
             m_name, open_gpio, close_gpio, m_openDelayMs, m_closeDelayMs);

}

void Valve::deinit()
{
    if (!m_initialized) return;

    gpio_set_level(open_gpio, 1);
    gpio_set_level(close_gpio, 1);
    gpio_reset_pin(open_gpio);
    gpio_reset_pin(close_gpio);

    m_state       = ValveState::NONE;
    m_initialized = false;

    MESP_LOGI(TAG, "Valve '%s' de-initialised", m_name);
}

void Valve::setCurrSetting(uint16_t setting)
{
    m_currSetting = setting;
    MESP_LOGI(TAG, "Valve '%s' currSetting = %u", m_name, m_currSetting);
}

void Valve::setTimer(uint16_t openDelayMs, uint16_t closeDelayMs)
{
    m_openDelayMs = openDelayMs;
    m_closeDelayMs = closeDelayMs;
    MESP_LOGI(TAG, "Valve '%s' openDelay=%u closeDelay=%u", m_name, m_openDelayMs, m_closeDelayMs);
}

void Valve::open()
{
    if (!m_initialized) {
        MESP_LOGW(TAG, "Valve '%s' not initialised", m_name);
        return;
    }
    gpio_set_level(open_gpio, 1);
    gpio_set_level(close_gpio, 0);
    m_state = ValveState::OPEN;
    MESP_LOGI(TAG, "Valve '%s' OPEN", m_name);
    vTaskDelay(pdMS_TO_TICKS(m_openDelayMs));

}

void Valve::close()
{
    if (!m_initialized) {
        MESP_LOGW(TAG, "Valve '%s' not initialised", m_name);
        return;
    }
    gpio_set_level(open_gpio, 0);
    gpio_set_level(close_gpio, 1);
    m_state = ValveState::CLOSE;
    MESP_LOGI(TAG, "Valve '%s' CLOSED", m_name);
    vTaskDelay(pdMS_TO_TICKS(m_closeDelayMs));
}
