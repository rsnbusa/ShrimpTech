#ifndef TYPESapp_H_
#define TYPESapp_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

int cmdApp(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&appSSID);
    if (nerrors != 0) {
        arg_print_errors(stderr, appSSID.end, argv[0]);
        return 0;
    }
  if (appSSID.password->count) 
  {   
    if(strcmp(appSSID.password->sval[0],theConf.kpass )==0)
    {
      if (appSSID.newpass->count)
       {
          strcpy(theConf.thessid,appSSID.newpass->sval[0]);
          printf("SSID change to [%s]\n",theConf.thessid);
          write_to_flash();
          esp_restart();
       }
    }
    else
      printf("Wrong password\n");
  }

  return 0;
}
#endif