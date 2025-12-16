#define GLOBAL
#include "globals.h"

char * convertBinaryToJson(meshunion_t* elNodo)
{

    char tit[20];
    int thismonth;
    struct tm timeinfo;
    char *lmessage;
    time_t now;

    now=elNodo->nodedata.tstamp;
    localtime_r(&now, &timeinfo);
    thismonth=timeinfo.tm_mon;

    cJSON *local_root=cJSON_CreateObject();
    if(local_root==NULL)
    {
        printf("cannot create root convertbinarytojson\n");
        return NULL;
    }

    cJSON_AddStringToObject(local_root,"cmd",elNodo->cmd);
    cJSON_AddNumberToObject(local_root,"ControlId",elNodo->nodedata.nodeid);
    cJSON_AddNumberToObject(local_root,"Subnode",elNodo->nodedata.subnode);

    cJSON *medArray=cJSON_CreateArray();
    if(medArray)
    {
        for (int a=0;a<MAXDEVSS;a++)
        {
            if(strlen(elNodo->nodedata.metersData[a].meter_id)>0)
            {

                    cJSON *datos=cJSON_CreateObject();
                    if(datos)
                    {
                        cJSON_AddStringToObject(datos,"m",elNodo->nodedata.metersData[a].meter_id);
                        cJSON_AddNumberToObject(datos,"k",elNodo->nodedata.metersData[a].kwhlife);
                        cJSON *monthArray=cJSON_CreateArray();

                            cJSON *datosmes=cJSON_CreateObject();
                            sprintf(tit,"M%d",thismonth);
                            cJSON_AddNumberToObject(datosmes,tit,elNodo->nodedata.metersData[a].month);
                            cJSON *hourArray=cJSON_CreateArray();
                            for (int h=0;h<24;h++)
                            {
                                cJSON *datoshora=cJSON_CreateObject();
                                sprintf(tit,"H%d",h);
                                cJSON_AddNumberToObject(datoshora,tit,elNodo->nodedata.metersData[a].horas[h]);
                                cJSON_AddItemToArray(hourArray, datoshora);
                            }
                            cJSON_AddItemToObject(datosmes, "Hours",hourArray);
                            cJSON_AddItemToArray(monthArray, datosmes);
                        // }
                        cJSON_AddItemToObject(datos, "Months",monthArray);
                        cJSON_AddItemToArray(medArray, datos);
                    }
                    else
                    {
                        // manage this error
                    }
            }
        }
        char *buf=(char*)calloc(80,1);
        if(buf)
        {
            strftime(buf, 50, "%c", &timeinfo);
            cJSON_AddStringToObject(local_root,"TS",buf);
            free(buf);
        }

        if(medArray)
            cJSON_AddItemToObject(local_root, "Medidores",medArray);
            
           lmessage=cJSON_PrintUnformatted(local_root);
        //    printf("bintoJson [%s]\n",lmessage);
    }
    else
        {
            cJSON_Delete(local_root);
            return NULL;
        }

    cJSON_Delete(local_root);
    return lmessage; // MUST BE FREED by calling routine or another routine
}

