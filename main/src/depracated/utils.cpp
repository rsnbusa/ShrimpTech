#ifndef UITLS_H_
#define UTILS_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

esp_err_t check_security(cJSON * cmd)
{
    uint32_t chaint=0,rsaint; 
    char *ptr;
    char *chKey=(char*)malloc(32);
    if(chKey)
    {
        memset(chKey,0x32,32);    // jaja, blank fill 32 chars
        if(cmd)
        {   
            char *que=cJSON_PrintUnformatted(cmd);
            memcpy(chKey,que,strlen(que)>32?32:strlen(que));        //get the key first 32 chars
            // printf("Key [%s]\n",chKey);
            free(que);
            cJSON *challenge= 		cJSON_GetObjectItem(cmd,"cha");
            if(!challenge)
            {
                free(chKey);
                // printf("No cha\n");
                return ESP_FAIL;
            }
            chaint=strtoul(challenge->valuestring, &ptr, 16);           //from string to HEX int
            // printf("Challenge %0x\n",chaint);
            char *aca=(char*)calloc (100,1);
            shrimp_aes_encrypt(SUPERSECRET,sizeof(SUPERSECRET),aca,chKey);
            memcpy((uint8_t*)&rsaint,aca,4);
            uint8_t *intt=(uint8_t*)&rsaint;
            memcpy(intt++,&aca[3],1);
            memcpy(intt++,&aca[2],1);
            memcpy(intt++,&aca[1],1);
            memcpy(intt,&aca[0],1);
            // printf("%02x%02x%02x%02x internal %x rsaint %x\n",aca[0],aca[1],aca[2],aca[3],chaint,rsaint);
            free(aca);
        }
        free(chKey);
        if(rsaint==chaint)
            return ESP_OK;
        else
            return ESP_FAIL;
    }
    return ESP_FAIL;
}

void read_flash()
{
	esp_err_t q ;
	size_t largo;

	if(xSemaphoreTake(flashSem, portMAX_DELAY/  portTICK_PERIOD_MS))
	{
		q = nvs_open("config", NVS_READONLY, &nvshandle);
		if(q!=ESP_OK)
		{
			ESP_LOGE(MESH_TAG,"Error opening NVS Read File %x",q);
			xSemaphoreGive(flashSem);
			return;
		}

		largo=sizeof(theConf);
		q=nvs_get_blob(nvshandle,"sysconf",(void*)&theConf,&largo);

		if (q !=ESP_OK)
			ESP_LOGE(MESH_TAG,"Error read %x largo %d aqui %d",q,largo,sizeof(theConf));
		nvs_close(nvshandle);
		xSemaphoreGive(flashSem);
	}
}

void write_to_flash() //save our configuration
{
	if(xSemaphoreTake(flashSem, portMAX_DELAY/  portTICK_PERIOD_MS))
	{
		esp_err_t q ;
		q = nvs_open("config", NVS_READWRITE, &nvshandle);
		if(q!=ESP_OK)
		{
			ESP_LOGE(MESH_TAG,"Error opening NVS File RW %x",q);
			xSemaphoreGive(flashSem);
			return;
		}
		size_t req=sizeof(theConf);
		q=nvs_set_blob(nvshandle,"sysconf",&theConf,req);
		if (q ==ESP_OK)
		{
			q = nvs_commit(nvshandle);
			if(q!=ESP_OK)
				ESP_LOGE(MESH_TAG,"Flash commit write failed %d",q);
		}
		else
			ESP_LOGE(MESH_TAG,"Fail to write flash %x",q);
		nvs_close(nvshandle);
		xSemaphoreGive(flashSem);
	}
}

void root_sntpget(void *pArg)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    timeval localtime;

    memset(&timeinfo,0,sizeof(timeinfo));
    
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) 
    {
        // printf("Time is not set yet. Connecting to WiFi and getting time over NTP.");
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();

        int retry = 0;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry <= SNTPTRY)
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        
        if(retry>=SNTPTRY)
        {
            ESP_LOGE(MESH_TAG,"SNTP failed retry count, using saved time");
            theConf.lastKnownDate=theMeter.getReservedDate();   
            localtime.tv_sec=theConf.lastKnownDate;
            localtime.tv_usec=0;                                      // no connection most likely
            settimeofday(&localtime,NULL);
            //let him fall thru and continue as if date was received
        }
        time(&now);
        setenv("TZ", LOCALTIME, 1);
        tzset();
        if(theConf.lastRebootTime==0)
            theConf.lastRebootTime=now;
        else
            theConf.downtime+=(uint32_t)now-theConf.lastRebootTime;
        theMeter.setReservedDate(now);

        ESP_LOGI(MESH_TAG,"SNTP Date %s",ctime((time_t*)&now));
        
        //start our Cycle calculations
        root_set_senddata_timer();
        write_to_flash();
// #ifdef LOGOPT
//         char * tmp=(char*)malloc(200);
//         sprintf(tmp,"System Reboot %d and active",theConf.bootcount);
//         writeLog(tmp);
//         free(tmp);
// #endif  

        #ifdef DISPLAY
        if(xSemaphoreTake(framSem, portMAX_DELAY/  portTICK_PERIOD_MS))		
        {  
            u8g2_ClearBuffer(&u8g2);
            ssdString(10,38,(char*)"ON",true);
            xSemaphoreGive(framSem);
        }
        #endif
    }
    vTaskDelete(NULL);

}

err_t write_log(char *LTAG,char *que)
{
    FILE* f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open Log for writting");
        return ESP_FAIL;
    }
    char * labola=(char*)calloc(strlen(que)+1,1);
    if(!labola)
        {
            ESP_LOGE(TAG, "No RAM writting");
            return ESP_FAIL;
        }
    sprintf(labola,"%s:%s:%s",LTAG,"1111",que);

    fprintf(f, labola);
    fclose(f);
    return ESP_OK;
}

err_t read_log(int nlines)
{
    char *buf=(char*)calloc(nlines+1);
    if (!buf)
    {
        ESP_LOGE(TAG, "Failed RAM log fro reading");
        return ESP_FAIL;
    }

    FILE* f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open Log for reading");
        return ESP_FAIL;
    }
	fgets(buf, nlines, f);
    printf("%s\n",buf);
    free(buf);
    return ESP_OK;
}
#endif;