#ifndef TYPESfram_H_
#define TYPESfram_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
#include <random>

int randomval(int min, int max)
{
  srand(time(NULL));
    return (esp_random() % (max - min + 1)) + min;
}

int frandomval(float min, float max)
{
  srand(time(NULL));
  int mmin=min*10.0;
  int mmax=max*10.0;
  int val=esp_random() % (mmax - mmin + 1) + mmin;
  return (float)val/10.0;
}

int cmdFram(int argc, char **argv)
{

  time_t now;
  struct tm timeinfo;
  char  strftime_buf[100];

  uint32_t aca;

    int nerrors = arg_parse(argc, argv, (void **)&framArg);
    if (nerrors != 0) {
        arg_print_errors(stderr, framArg.end, argv[0]);
        return 0;
    }

 if (framArg.format->count) {
  int cuanto=0;
    theBlower.deinit();
    theBlower.format();    
    time(&now);
    theBlower.writeCreationDate(now);

    printf("Fram Formatted\n");
    return 0;
 }

 if (framArg.stats->count) {
    const char *que=framArg.stats->sval[0];

    if(strcmp(que,"read")==0)
    {
      time_t ahora=theBlower.getStatsLastCountTS();
      printf("====== Node stats ======\n");
      printf("Msgs In: %d Bytes In: %d\n",theBlower.getStatsBytesIn());
      printf("Msgs Out: %d Bytes Out: %d\n",theBlower.getStatsMsgOut(),theBlower.getStatsBytesOut());
      printf("Nodes %d Blowers %d lastDazte %s",theBlower.getStatsLastNodeCount(),theBlower.getStatsLastBlowerCount(),ctime(&ahora));
    }
 }

 if (framArg.seed->count) {

    theBlower.setPVPanel(randomval(0,1),randomval(345,360),randomval(345,360),frandomval(13.5,14.7),frandomval(13.5,14.7));      //load the PV panel data from FRAM
    theBlower.setBattery(randomval(20,85),randomval(0,10),1,frandomval(22.3,32.3));      //load the Battery data from FRAM
    theBlower.setEnergy(randomval(50,58),randomval(50,58),randomval(350,658),randomval(350,658),frandomval(39000.0,41200.0),
      frandomval(39000.0,41200.0),frandomval(39000.0,41200.0),frandomval(39000.0,41200.0),frandomval(39.0,42.3),frandomval(39.0,42.3));       //load the Energy data from FRAM
    theBlower.setSensors(frandomval(3.5,7.0), frandomval(4.3,6.6),frandomval(18.4,29.2),frandomval(24.4,32.2),frandomval(20.40,88.40));       //load the Solar System data from FRAM
    print_blower("Seed",theBlower.getPtrSolarsystem(),true);
    return 0;
 }

  return 0;
}
#endif