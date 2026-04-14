#include "Pump.hpp"

#include <algorithm>
#include <cinttypes>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "Pump";

Pump::~Pump()
{
    if (m_initialized) {
        stop();
        uart_driver_delete(m_uartPort);
        m_initialized = false;
    }
}

void Pump::init(uart_port_t uartPort,
                gpio_num_t txPin,
                gpio_num_t rxPin,
                int baudRate,
                int rtsPin)
{

    if (m_initialized) {
        stop();
        uart_driver_delete(m_uartPort);
        m_initialized = false;
    }

    m_uartPort = uartPort;
    m_txPin = txPin;
    m_rxPin = rxPin;
    m_rtsPin = rtsPin;
    m_baudRate = baudRate;
    m_startedAtUs = 0;
    state = PumpState::STOPPED;
    hours = 0.0;
    cycles = 0;


    uart_config_t config;
    std::memset(&config, 0, sizeof(config));
    config.baud_rate = m_baudRate;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.rx_flow_ctrl_thresh = 0;
    config.source_clk = UART_SCLK_DEFAULT;
    config.flags.allow_pd = 0;
    config.flags.backup_before_sleep = 0;

    esp_err_t err = uart_driver_install(m_uartPort, 256, 0, 0, nullptr, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return;
    }

    err = uart_param_config(m_uartPort, &config);
    if (err == ESP_OK) {
        err = uart_set_pin(m_uartPort, m_txPin, m_rxPin, m_rtsPin, UART_PIN_NO_CHANGE);
    }
    if (err == ESP_OK) {
        err = uart_set_mode(m_uartPort, UART_MODE_RS485_HALF_DUPLEX);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "RS485 init failed: %s", esp_err_to_name(err));
        uart_driver_delete(m_uartPort);
        return;
    }

    m_initialized = true;

    ESP_LOGI(TAG, "Pump initialised on UART%d, TX=GPIO%d RX=GPIO%d RTS=GPIO%d baud=%d",
             m_uartPort, m_txPin, m_rxPin, m_rtsPin, m_baudRate);
}

void Pump::start()
{
    if (!m_initialized) {
        ESP_LOGW(TAG, "Pump not initialised");
        return;
    }

    if (state == PumpState::RUNNING) {
        return;
    }

    if (!sendFrame(m_startFrame.data(), m_startFrameLength, "start")) {
        return;
    }

    m_startedAtUs = esp_timer_get_time();
    state = PumpState::RUNNING;
    ++cycles;

    ESP_LOGI(TAG, "Pump started, cycles=%" PRIu32, cycles);
}

void Pump::stop()
{
    if (!m_initialized) {
        ESP_LOGW(TAG, "Pump not initialised");
        return;
    }

    if (state != PumpState::RUNNING) {
        return;
    }

    if (!sendFrame(m_stopFrame.data(), m_stopFrameLength, "stop")) {
        return;
    }

    const int64_t elapsedUs = esp_timer_get_time() - m_startedAtUs;
    if (elapsedUs > 0) {
        hours += static_cast<double>(elapsedUs) / 3600000000.0;
    }

    m_startedAtUs = 0;
    state = PumpState::STOPPED;

    ESP_LOGI(TAG, "Pump stopped, hours=%.4f, cycles=%" PRIu32, hours, cycles);
}

bool Pump::sendFrame(const uint8_t *frame, size_t frameLength, const char *action)
{
    const int written = uart_write_bytes(m_uartPort, frame, frameLength);
    if (written < 0 || static_cast<size_t>(written) != frameLength) {
        ESP_LOGE(TAG, "Failed to queue %s frame on UART%d", action, m_uartPort);
        return false;
    }

    const esp_err_t err = uart_wait_tx_done(m_uartPort, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Timed out sending %s frame: %s", action, esp_err_to_name(err));
        return false;
    }

    return true;
}