#ifndef TYPESerase_H_
#define TYPESerase_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern void erase_config();

int cmdErase(int argc, char **argv)
{
  printf("Erase Controller...");
  erase_config();
  printf("done\n");
  return 0;
}
#endif