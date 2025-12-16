#ifndef TYPESnode_H_
#define TYPESnode_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

int cmdNode(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&appNode);
    if (nerrors != 0) {
        arg_print_errors(stderr, appNode.end, argv[0]);
        return 0;
    }
  if (appNode.password->count) 
  {   
    if(strcmp(appNode.password->sval[0],theConf.kpass )==0)
    {
      if (appNode.newpass->count)
       {
          int nn=atoi(appNode.newpass->sval[0]);
          printf("New Node ID [%d]\n",nn);
          theConf.poolid=nn;
          write_to_flash();
          // esp_restart();
       }
    }
    else
      printf("Wrong password\n");
  }

  return 0;
}
#endif