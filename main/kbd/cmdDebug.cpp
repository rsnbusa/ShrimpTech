#ifndef TYPESdbg_H_
#define TYPESdbg_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"


int cmdDebug(int argc, char **argv)
{
   char aca[10];

    int nerrors = arg_parse(argc, argv, (void **)&dbgArg);
    if (nerrors != 0) {
        arg_print_errors(stderr, dbgArg.end, argv[0]);
        return 0;
    }

   if (dbgArg.schedule->count) 
   {
         strcpy(aca,dbgArg.schedule->sval[0]);
         for (int x=0; x<strlen(aca); x++)
            aca[x]=toupper(aca[x]);
         if(strcmp(aca,"ON")==0)
            theConf.debug_flags |= (1U << dSCH);
         if(strcmp(aca,"OFF")==0)
            theConf.debug_flags &= ~(1U << dSCH);
   }

   if (dbgArg.mesh->count) 
   {
         strcpy(aca,dbgArg.mesh->sval[0]);
         for (int x=0; x<strlen(aca); x++)
            aca[x]=toupper(aca[x]);
         if(strcmp(aca,"ON")==0)
            theConf.debug_flags |= (1U << dMESH);
         if(strcmp(aca,"OFF")==0)
            theConf.debug_flags &= ~(1U << dMESH);
   }

   if (dbgArg.ble->count) 
   {
         strcpy(aca,dbgArg.ble->sval[0]);
         for (int x=0; x<strlen(aca); x++)
            aca[x]=toupper(aca[x]);
         if(strcmp(aca,"ON")==0)
            theConf.debug_flags |= (1U << dBLE);
         if(strcmp(aca,"OFF")==0)
            theConf.debug_flags &= ~(1U << dBLE);
   }

   if (dbgArg.mqtt->count) 
   {
         strcpy(aca,dbgArg.mqtt->sval[0]);
         for (int x=0; x<strlen(aca); x++)
            aca[x]=toupper(aca[x]);
         if(strcmp(aca,"ON")==0)
            theConf.debug_flags |= (1U << dMQTT);
         if(strcmp(aca,"OFF")==0)
            theConf.debug_flags &= ~(1U << dMQTT);
   }

   if (dbgArg.xcmds->count) 
   {
         strcpy(aca,dbgArg.xcmds->sval[0]);
         for (int x=0; x<strlen(aca); x++)
            aca[x]=toupper(aca[x]);
         if(strcmp(aca,"ON")==0)
            theConf.debug_flags |= (1U << dXCMDS);
         if(strcmp(aca,"OFF")==0)
            theConf.debug_flags &= ~(1U << dXCMDS);
   }

   if (dbgArg.blow->count) 
   {
         strcpy(aca,dbgArg.blow->sval[0]);
         for (int x=0; x<strlen(aca); x++)
            aca[x]=toupper(aca[x]);
         if(strcmp(aca,"ON")==0)
            theConf.debug_flags |= (1U << dBLOW);
         if(strcmp(aca,"OFF")==0)
            theConf.debug_flags &= ~(1U << dBLOW);
   }

   if (dbgArg.all->count) 
   {
         strcpy(aca,dbgArg.all->sval[0]);
         for (int x=0; x<strlen(aca); x++)
            aca[x]=toupper(aca[x]);
         if(strcmp(aca,"ON")==0)
            theConf.debug_flags =0xFFFFFFFF;
         if(strcmp(aca,"OFF")==0)
            theConf.debug_flags =0;
   }
   printf("Debug %x\n",theConf.debug_flags);
   write_to_flash();

  return 0;
}
#endif