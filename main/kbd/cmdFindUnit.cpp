#ifndef TYPESfunit_H_
#define TYPESfunit_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
#include "defines.h"

extern void blinkRoot(void *parg);

int cmdFindUnit(int argc, char **argv)
{
  if(blinkHandle)
    vTaskDelete(blinkHandle);

  for (int a=0;a<10;a++)
  {
        gpio_set_level((gpio_num_t)WIFILED,1);
        delay(100);
        gpio_set_level((gpio_num_t)WIFILED,0);
        delay(100);
  }  
  if (esp_mesh_is_root())
    xTaskCreate(&blinkRoot,"root",1024,NULL, 5, &blinkHandle);
 else
    gpio_set_level((gpio_num_t)WIFILED,1);

  return 0;
}
#endif