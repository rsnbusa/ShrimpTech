
#ifndef TYPESbt_H_
#define TYPESbt_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"


int cmdBasetimer(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void **)&basetimer);
  if (nerrors != 0) {
      arg_print_errors(stderr, basetimer.end, argv[0]);
      return 0;
  }
  if (basetimer.first->count) 
  {
      int lev=basetimer.first->ival[0];
      if(lev<1)
        lev=10;
      printf("First timer %d set to %d\n",theConf.baset,lev);
      theConf.baset=BASETIMER=lev;    //will take effect in NEXT timer repeat
      // if(repeatTimer)
      //   if( xTimerChangePeriod( repeatTimer,pdMS_TO_TICKS(BASETIMER), 100 )== pdFAIL )
      // // if( xTimerChangePeriod( repeatTimer,pdMS_TO_TICKS(HOUR*BASETIMER), 100 )== pdFAIL )
      //     ESP_LOGE(MESH_TAG,"Error setting basetime %d",BASETIMER);
  }
  if (basetimer.repeat->count) 
  {
    int lev=basetimer.repeat->ival[0];
    printf("Repeat timer %d set to %d\n",theConf.repeat,lev);
        theConf.repeat=lev;
  }

  write_to_flash();
  
  return 0;
}
#endif