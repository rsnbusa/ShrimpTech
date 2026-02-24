#define GLOBAL
#include "time_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {

void delay(uint32_t cuanto)
{
    vTaskDelay(cuanto/portTICK_PERIOD_MS);
}

uint32_t xmillis(void)
{
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

} // extern "C"
