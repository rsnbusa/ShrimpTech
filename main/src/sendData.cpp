#ifndef TYPESsdata_H_
#define TYPESsdata_H_
#define GLOBAL
#include "includes.h" 
#include "globals.h"
// for shrimp solar blower meter
extern void print_blower(char * title,solarSystem_t *msolar);
extern void delay(uint32_t son);
extern void seed_blower();
meshunion_t * sendData(bool forced)
{
    time_t now;
    struct tm timeinfo;
    int thismonth;
    meshunion_t* thisNode;
    uint8_t     thismac[6];

    // esp_base_mac_addr_get((uint8_t*)thismac);
    time(&now);
    // localtime_r(&now, &timeinfo);
    // thismonth=timeinfo.tm_mon;
    thisNode=(meshunion_t*)calloc(1,sizeof(meshunion_t));
    if(!thisNode)
    {
        ESP_LOGE(MESH_TAG,"error sendata no RAM\n");
        return NULL;
    }
    thisNode->cmd=MESH_INT_DATA_BIN;
    thisNode->nodedata.tstamp=now;
    if(!esp_mesh_is_root())
        theBlower.setStatsMsgOut();
    thisNode->nodedata.msgnum=theBlower.getStatsMsgOut();
    thisNode->nodedata.nodeid=theConf.poolid;
    thisNode->nodedata.subnode=theConf.unitid;
    
// ========================================================================

    seed_blower();          // for simulations only  ERASE later

// ========================================================================

    solarSystem_t *msolar;
    msolar=theBlower.getPtrSolarsystem();

    memcpy((void*)&thisNode->nodedata.solarData.solarSystem,msolar,sizeof(solarSystem_t));
    // print_blower("Senddata",msolar);
    return thisNode;        //must be freed by caller
}
#endif