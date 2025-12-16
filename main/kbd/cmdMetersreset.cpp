#ifndef TYPESmreset_H_
#define TYPESmreset_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

int cmdMetersreset(int argc, char **argv)
{
  mesh_data_t         data;
  esp_err_t           err;

      if(esp_mesh_is_root())
      {
        char *broadcast=(char*)calloc(50,1);
        if(!broadcast)
        {
            ESP_LOGE(MESH_TAG,"Could not malloc collect meter FATAL");
            return 0;
        }

        strcpy(broadcast,"{\"cmd\":\"reinstall\"}");      //cJSON is long /elaborate for this simple message
        data.data   =(uint8_t*)broadcast;
        data.size   = strlen(broadcast);
        data.proto  = MESH_PROTO_BIN;
        data.tos    = MESH_TOS_P2P;
        //send a Broadcast Message to all nodes to send their data
        err= esp_mesh_send( &GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);         //broadcast msg to mesh
        if(err)
            ESP_LOGE(MESH_TAG,"Broadcast failed. Now message in this slots %x",err);
        free(broadcast);    
      }
        return 0;
}
#endif