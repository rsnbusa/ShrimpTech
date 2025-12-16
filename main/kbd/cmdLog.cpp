#ifndef TYPESllog_H_
#define TYPESllog_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

int cmdLog(int argc, char **argv)
{
    char linea[200];

    int nerrors = arg_parse(argc, argv, (void **)&logArgs);
    if (nerrors != 0) {
        arg_print_errors(stderr, logArgs.end, argv[0]);
        return 0;
    }

 if (logArgs.show->count) {
    int cuanto=logArgs.show->ival[0];
    int ccuanto=cuanto;
    ESP_LOGI(MESH_TAG,"Show %d log lines",cuanto);
    fclose(myFile);
    myFile= fopen("/spiffs/log.txt", "r");
    if (myFile == NULL) 
    {
        ESP_LOGE(TAG,"Could not open Log file\n");
        return 0;
    }
    while(cuanto>0)
    {
        if( fgets (linea, sizeof(linea), myFile)!=NULL ) 
        {
            printf("[%3d]%s",ccuanto-cuanto+1,linea);
            cuanto--;
        }
        else
        {
            fclose(myFile);
            myFile= fopen("/spiffs/log.txt", "a");
            if (myFile == NULL) 
            {
                ESP_LOGE(MESH_TAG,"Failed to open file for append");
            }
            return 0;
        }
     }
 }

 if (logArgs.erase->count) 
 {
    int cuanto=logArgs.erase->ival[0];
   ESP_LOGI(MESH_TAG,"Erase Log file confimed %d",cuanto);

    fclose(myFile);
    remove("/spiffs/log.txt");
    myFile= fopen("/spiffs/log.txt", "a");
    if (myFile == NULL) 
    {
        ESP_LOGE(MESH_TAG,"Failed to open file for append erase");
    }
 }

    return 0;
}
#endif