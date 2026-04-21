#pragma once

#include <cstdint>
#include "driver/gpio.h"

enum class ValveState : uint8_t {
    NONE  = 0,
    OPEN  = 1,
    CLOSE = 2,
};

class Valve {
public:
    Valve() = default;
    ~Valve();

    void init(gpio_num_t gpio1, gpio_num_t gpio2, const char *name, uint16_t openDelayMs, uint16_t closeDelayMs);
    void deinit();

    void setCurrSetting(uint16_t setting);
    void setTimer(uint16_t openDelayMs, uint16_t closeDelayMs);

    void open();
    void close();

    // Accessors
    const char    *getName()        const { return m_name; }
    ValveState     getState()       const { return m_state; }
    ValveState     getStatus()      const { return m_state; }
    uint16_t       getCurrSetting() const { return m_currSetting; }
    uint16_t       getTimerSetting()const { return m_openDelayMs; }
    uint16_t       getOpenDelay()   const { return m_openDelayMs; }
    uint16_t       getCloseDelay()  const { return m_closeDelayMs; }

private:
    char        m_name[40]    = {};
    gpio_num_t  open_gpio       = GPIO_NUM_NC;
    gpio_num_t  close_gpio       = GPIO_NUM_NC;
    ValveState  m_state       = ValveState::NONE;
    uint16_t    m_currSetting = 0;
    uint16_t    m_openDelayMs = 0;
    uint16_t    m_closeDelayMs = 0;
    bool        m_initialized = false;
};
