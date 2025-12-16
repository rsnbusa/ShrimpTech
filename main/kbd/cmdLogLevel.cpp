#ifndef TYPESllevl_H_
#define TYPESllevl_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

char levels[6][10]={"None","Error","Warn","Info","Debug","Verbose"};

int findLevel(char * cual)
{
  for (int a=0;a<6;a++)
    if(strcmp(cual,levels[a])==0)
      return a;
  
  return -1;
}

int cmdLogLevel(int argc, char **argv)
{
  char  aca[20];
  int lev=-1;

  int nerrors = arg_parse(argc, argv, (void **)&loglevel);
  if (nerrors != 0) {
      arg_print_errors(stderr, loglevel.end, argv[0]);
      return 0;
  }
  if (loglevel.level->count) 
  {
    strcpy(aca,loglevel.level->sval[0]);
    for (int x=0; x<strlen(aca); x++)
        aca[x]=toupper(aca[x]);
    lev=findLevel(aca);
    if(lev>=0)
    {
      printf("Level set to %s\n",aca);
      theConf.loglevel=lev;
      write_to_flash();
      esp_log_level_set("*",(esp_log_level_t)theConf.loglevel);
    }

  }
  else
    printf("Invalid Level\n");

  return 0;
}
#endif