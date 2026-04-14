#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include "driver/gpio.h"
#include "driver/uart.h"

enum class PumpState : uint8_t {
    STOPPED = 0,
    RUNNING = 1,
};

class Pump {
public:
    Pump() = default;
    ~Pump();

    void init(uart_port_t uartPort,
              gpio_num_t txPin,
              gpio_num_t rxPin,
              int baudRate,
              int rtsPin = UART_PIN_NO_CHANGE);
    void start();
    void stop();

    PumpState state = PumpState::STOPPED;
    double    hours = 0.0;
    uint32_t  cycles = 0;

private:
    static constexpr size_t kMaxFrameSize = 32;

    bool sendFrame(const uint8_t *frame, size_t frameLength, const char *action);

    uart_port_t m_uartPort = UART_NUM_0;
    gpio_num_t  m_txPin = GPIO_NUM_NC;
    gpio_num_t  m_rxPin = GPIO_NUM_NC;
    int         m_rtsPin = UART_PIN_NO_CHANGE;
    int         m_baudRate = 0;
    bool        m_initialized = false;
    int64_t     m_startedAtUs = 0;
    std::array<uint8_t, kMaxFrameSize> m_startFrame = {};
    std::array<uint8_t, kMaxFrameSize> m_stopFrame = {};
    size_t      m_startFrameLength = 0;
    size_t      m_stopFrameLength = 0;
};