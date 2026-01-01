#define GLOBAL
#include "time_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {

void delay(uint32_t cuanto)
{
    vTaskDelay(pdMS_TO_TICKS(cuanto));
}

uint32_t xmillis(void)
{
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

} // extern "C"
