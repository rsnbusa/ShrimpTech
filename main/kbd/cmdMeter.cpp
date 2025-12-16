#ifndef TYPESmeter_H_
#define TYPESmeter_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

int cmdMeter(int argc, char **argv)
{
  // char update[80],initd[0];
  struct tm *timeinfo;
  bool mmes=false, hhora=false;
  char *uupdate=(char*)malloc(100);
  char *iinitd=(char*)malloc(100);
  time_t mio;
  int pulse_count;

  for (int a=0;a<MAXDEVSS;a++)
  {      
    // printf("lastupdate %lu\n",medidor.lastupdate);
    mio=theBlower.getLastUpdate();
    timeinfo=localtime((time_t*)&mio);
    strftime(uupdate, 100, "%c", timeinfo);
    mio=theBlower.getLifeDate();
    timeinfo=localtime((time_t*)&mio);
    strftime(iinitd, 100, "%c", timeinfo);
  //   if(strlen(theBlower.getMID())>0)
  //   {
  
  //     // ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit[a], &pulse_count));
  //     printf("Meter[%d] [id=%s] [BPK %d]  [Beat %d]  [Beatlife %d]  [kwhStart %d]  [KwhLife %d] PayMode [%s] Prebal [%d] OnOff [%s]",a,theBlower.getMID(),theBlower.getBPK(),
  //     theBlower.getBeats(),theBlower.getBeatsLife(),theBlower.getKstart(),theBlower.getLkwh(),theBlower.getPay()==0?"POST":"PREPAID",
  //     theBlower.getPrebal(),theBlower.getOnOff()==0?"ON":"OFF" );
  //     printf(" MaxAmp [%d] MinAmp [%d] [Update %s ] [Initd %s]\n",theBlower.getMaxamp(),theBlower.getMinamp(),uupdate,iinitd);
  //     for(int b=0;b<12;b++)
  //     {
  //       if(theBlower.getMonth(b)>0)
  //       {
  //         uint32_t check=0;
  //         printf("Month[%d]=%d ",b,theBlower.getMonth(b));
  //         mmes=true;
  //         for (int c=0;c<24;c++)
  //         {
  //           if(theBlower.getMonthHour(b,c)>0)
  //           {
  //             printf("Hour[%d]=%d ",c,theBlower.getMonthHour(b,c));
  //             hhora=true;
  //             check+=theBlower.getMonthHour(b,c);
  //           }
  //         }
  //         if (hhora)
  //           printf( "check %d\n",check);
  //           hhora=false;
  //       }
  //       if( mmes)
  //         printf("\n");
  //         mmes=false;
  //     }
  //   }
  }
  free(uupdate);
  free(iinitd);
  return 0;
}
#endif