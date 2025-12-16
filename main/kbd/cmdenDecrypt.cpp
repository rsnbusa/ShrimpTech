#ifndef TYPESdecry_H_
#define TYPESdecry_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern int aes_encrypt(const char* src, size_t son, char *dst,const char *cualKey);

int cmdEnDecrypt(int argc, char **argv)
{
  int dkey,err;
  const char *mode;
  char kkey[17],laclave[33],ikey[10];

    int nerrors = arg_parse(argc, argv, (void **)&endec);
    if (nerrors != 0) {
        arg_print_errors(stderr, endec.end, argv[0]);
        return 0;
    }

    if (endec.key->count) 
    {
      dkey=endec.key->ival[0];
      if(dkey<=0)
        return 0;
      sprintf(kkey,"%016d",dkey);
      // printf("num [%s]\n",kkey);
      sprintf(laclave,"%s%s",kkey,kkey);
      // printf("clave [%s] %d\n",laclave,strlen(laclave));
      char *aca=(char*)calloc (1000,1);
      err=aes_encrypt(SUPERSECRET,sizeof(SUPERSECRET),aca,laclave);
      printf("%02x%02x%02x%02x\n",aca[0],aca[1],aca[2],aca[3]);
      free(aca);
      // ESP_LOG_BUFFER_HEX(MESH_TAG,aca,err);
    }
      
  return 0;
}
#endif