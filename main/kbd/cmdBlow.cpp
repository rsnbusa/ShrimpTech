#ifndef TYPESrconf_H_
#define TYPESrconf_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"


  uint16_t randomInt(int min, int max) 
  {
    return min + rand() % (max - min + 1);
  }

  float randomFloat(float min,float max) 
  {
    float scale = rand() / (float)RAND_MAX;  // [0, 1.0]
    return min + scale * (max - min);
  }

  void seed_blower()
  {
    theBlower.setPVPanel(randomInt(0,1),randomFloat(340,390),randomFloat(340,390),randomFloat(14,15),randomFloat(14,15));
    theBlower.setBattery(randomInt(20,80),randomInt(20,100),randomInt(0,5000),randomFloat(10,40));
    theBlower.setEnergy(randomInt(780,820),randomInt(720,820),randomInt(720,850),randomInt(720,820),randomFloat(40,42),randomFloat(40,42),randomFloat(40,42),randomFloat(40,42),randomFloat(40,42),randomFloat(40,42));
    theBlower.setSensors(randomFloat(4,8),randomFloat(4,8),randomFloat(18,30),randomFloat(22,32),randomFloat(40,99)  );
  }
  
int cmdBlow(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void **)&blowArgs);
  if (nerrors != 0) {
      arg_print_errors(stderr, blowArgs.end, argv[0]);
      return 0;
  }
  if (blowArgs.seed->count) 
  {
    srand(time(NULL));
    seed_blower();
    return 0;
  }
  if (blowArgs.init->count) 
  {
    theBlower.setPVPanel(0,0,0,0,0);
    theBlower.setBattery(0,0,0,0);
    theBlower.setEnergy(0,0,0,0,0,0,0,0,0,0);
    theBlower.setSensors(0,0,0,0,0);
    return 0;
  }

}

#endif