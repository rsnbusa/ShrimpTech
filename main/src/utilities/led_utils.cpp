#define GLOBAL
#include "includes.h"
#include "defines.h"
#include "led_utils.h"
#include "time_utils.h"

extern "C" {

void blinkRoot(void *pArg)
{
    uint32_t cuanto = (uint32_t)pArg;
    while(1)
    {
        gpio_set_level((gpio_num_t)WIFILED, 1);
        delay(cuanto);
        gpio_set_level((gpio_num_t)WIFILED, 0);
        delay(cuanto);
    }
} 

void blinkConf(void *pArg)
{
    uint32_t cuanto = (uint32_t)pArg;
    while(1)
    {
        gpio_set_level((gpio_num_t)WIFILED, 1);
        // gpio_set_level((gpio_num_t)BEATPIN, 0);
        delay(cuanto);
        gpio_set_level((gpio_num_t)WIFILED, 0);
        // gpio_set_level((gpio_num_t)BEATPIN, 1);
        delay(cuanto);
    }
}

} // extern "C"
