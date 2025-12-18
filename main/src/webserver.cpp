/*
 * webserver.cpp
 *
 *  Created on: Dec 29, 2019
 *      Author: rsn
 */
#ifndef TYPESweb_H_
#define TYPESweb_H_
#define GLOBAL

#include "includes.h"
#include "globals.h"
#include "mongoose/mongoose_glue.h"

// mongoose glue structures. 
//
// Delete the STATIC in the mongoose_glue.c or linker fails cannot find externals below
//
//
extern struct settings s_settings;
extern struct system s_system;
extern struct sysset s_sysset;
extern struct meter s_meter;
extern struct profile s_profile;
extern struct limits s_limits;
extern uint64_t s_action_timeout_Done;  // Time when Done starts

extern int aes_encrypt(const char* src, size_t son, char *dst,const char *cualKey);
extern void write_to_flash();
extern void delay(uint32_t a);
extern char levels[6][10];			// log levels external in cmdConfig.cpp
static bool restartf=false;
extern 	int findLevel(char * cual);

bool check_key(uint64_t key)
{
	char kkey[17],laclave[33];
	int challenge;
	uint8_t *porg,*pdest;

	// ESP_LOG_BUFFER_HEX("KEY",&key,4);

	porg=(uint8_t*)&key;
	pdest=(uint8_t*)&challenge;
	pdest+=3;

	for (int a=0;a<4;a++)
	{
		*pdest=*porg;
		pdest--;
		porg++;
	}

	sprintf(kkey,"%016d",theConf.cid);
	// printf("num [%s]\n",kkey);
	sprintf(laclave,"%s%s",kkey,kkey);
	// printf("clave [%s] %d\n",laclave,strlen(laclave));
	char *aca=(char*)calloc (100,1);

	int ret=aes_encrypt(SUPERSECRET,sizeof(SUPERSECRET),aca,laclave);
	if(ret==ESP_FAIL)
	{
		ESP_LOGE("KEY","Fail encrypt");
		return false;
	}

	// ESP_LOG_BUFFER_HEX("ACA",aca,strlen(SUPERSECRET));
	// ESP_LOG_BUFFER_HEX("CHA",&challenge,4);

int como=memcmp((void*)aca,(void*)&challenge,4);
// printf("Como %d\n",como);
	free(aca);
	if(como==0)
		return true;

	return false;

}

void my_get_settings(struct settings *data) {

	//only 2 parameters, the cid (used for the challenge) and set disaabled if lifekwh in fram is > 0 but somehow got here
   if( theConf.meterconf>2)
   		s_settings.disable_val=1;
	else
   		s_settings.disable_val=0;

	sprintf(s_settings.mac_val,"%d",theConf.cid);		// challenge initiaol value
	if(restartf)
	{
		strcpy(s_settings.msg_val,"Listo");
		TimerHandle_t restartTimer=xTimerCreate("restart",pdMS_TO_TICKS(5000),pdFALSE,( void * ) 0, [] ( TimerHandle_t xTimer)
		{	
			theConf.meterconf=1;
			write_to_flash();
			esp_restart();});   //monitor activity and tiemout if no work done-> use lambda
		xTimerStart(restartTimer,0);
	}
	if(theConf.meterconf==0)
			strcpy(s_settings.msg_val,"ENTER KEY");


//current configuration and readigns

	// s_settings.mesh_val=theConf.controllerid;


	*data = s_settings;  // Sync with your device
}

void my_set_settings(struct settings *data) {
	// save Meter and theConf structures
	uint32_t keyread;
	char *ptr; 
	char *laclave=(char*)calloc(1,40);
	time_t now;

	s_settings=*data;
// check if challenge is met
	strcpy(laclave,data->challenge_val);
	keyread=strtoul(laclave, &ptr, 16);
	if(!check_key(keyread))
	{
		printf("Not same key %d %d\n",keyread,theConf.cid);
		strcpy(s_settings.msg_val,"Retry");
		free(laclave);
		return;
	}
	free(laclave);
//challenge OK
	theConf.masternode=s_settings.master_val;
	theConf.poolid=s_settings.pool_val;
	theConf.unitid=s_settings.unit_val;
	theConf.totalnodes=s_settings.nodes_val;
	theConf.delay_mesh=s_settings.delay_val;

	time(&now);
	// save data to flash and Fram
	theConf.meterconf=1; // configuration is done... next step (2) will send confirmation to COntroller DB and receive confirmation (state 3)
	theConf.ptch=1;		//Pop that Cherry :) 
	theConf.meterconfdate=now;	//configuration date... useless we have no NTP access so start of unix time
	theConf.bornDate=theConf.meterconfdate;
	theConf.cid=0;						// reset Challenge ID to 0, we are configured now
	// theConf.medback.backdate=now;
	write_to_flash();

	// save_inst_msg(theBlower.getMID(),theBlower.getBPK(),theBlower.getKstart(),(char*)"SYSTEM");

	s_settings = *data; 
	restartf=true;
}

void my_set_system(struct system *data) {
	//set/save genreal paramenter in theConf structure
	s_system=*data;

	// theConf.allowSkips=s_system.skipopt_val;
	// theConf.maxSkips=s_system.skip_val;
	theConf.repeat=s_system.repeat_val;
	theConf.baset=s_system.baset_val;
	// theConf.otaOnTheFly=s_system.otf_val;
	strcpy(theConf.kpass,s_system. password_val);
	int cual=findLevel(s_system.loglevel_val);
	if(cual>=0)
		theConf.loglevel=cual;
	theConf.loginwait=s_system.logtime_val;
	strcpy(theConf.mqttPass,s_system. mqttpass_val);
	strcpy(theConf.mqttUser,s_system. mqttuser_val);
	strcpy(theConf.mqttServer,s_system. mqttserver_val);
	strcpy(theConf.mqttcert,s_system. mqttcert_val);
	// strcpy(theConf.OTAURL,s_system. otaurl_val);
	strcpy(theConf.thepass,s_system. ssidpass_val);
	strcpy(theConf.thessid,s_system. ssid_val);
	theConf.poolid=s_system. meshid_val;
	theConf.useSec=s_system. security_val;
	theConf.totalnodes=s_system. nodes_val;
	theConf.conns=s_system. conns_val;
	theConf.mqttDiscoRetry=s_system.mqttreco_val;
	if(theConf.meterconf==3)
		theConf.meterconf=2;
	write_to_flash();
	// xTimerReset(webTimer,0);
}

void my_get_system(struct system *data) 
{
	   if(theConf.meterconf>3)				// CONF is 4
	   
			s_system.disable_val=1;
	else
   		s_system.disable_val=0;

	s_system.meshid_val=theConf.poolid;
	const esp_app_desc_t *mip=esp_app_get_description();
	strcpy(s_system.version_val,mip->version);
	s_system.meshid_val=theConf.poolid;
	// s_system.skipopt_val=theConf.allowSkips;
	// s_system.skip_val=theConf.maxSkips;
	s_system.repeat_val=theConf.repeat;
	s_system.baset_val=theConf.baset;
	// s_system.otf_val=theConf.otaOnTheFly;
	strcpy(s_system. password_val,theConf.kpass);
	strcpy(s_system. loglevel_val,levels[theConf.loglevel]);
	strcpy(s_system. mqttpass_val,theConf.mqttPass);
	strcpy(s_system. mqttuser_val,theConf.mqttUser);
	strcpy(s_system. mqttserver_val,theConf.mqttServer);
	strcpy(s_system. mqttcert_val,theConf.mqttcert);
	// strcpy(s_system. otaurl_val,theConf.OTAURL);
	strcpy(s_system. ssidpass_val,theConf.thepass);
	strcpy(s_system. ssid_val,theConf.thessid);
	s_system. security_val=theConf.useSec;
	s_system. nodes_val=theConf.totalnodes;
	s_system. conns_val=theConf.conns;
	s_system.mqttreco_val=theConf.mqttDiscoRetry;

	*data = s_system;
	// xTimerReset(webTimer,0);

}

void my_get_sysset(struct sysset *data) 		// get data for general configuration and display
{
	// set s_sysset strucutre to stored values in Meter or theConf structures
	const esp_app_desc_t *mip=esp_app_get_description();
	strcpy(s_sysset.idf_val,mip->version);
	strcpy(s_sysset.compile_val,mip->idf_ver);
	s_sysset.boot_val=theConf.bootcount;
	s_sysset.lreason_val=theConf.lastResetCode;
	s_sysset.security_val=theConf.useSec;
	s_sysset.display_val=gdispf;
	// s_sysset.lost_val=theBlower.getLost_Pulses();
	s_sysset.writes_val=theBlower.getFram_Writes();
	s_sysset.reads_val=theBlower.getFram_Reads();
	strcpy(s_sysset.nodetype_val,esp_mesh_is_root()?"ROOT":"NODE");		//not usefull since mesh is not active but...
	mesh_addr_t mmeshid;
	esp_mesh_get_id(&mmeshid);
	sprintf(s_sysset.mesh_val,MACSTR,MAC2STR(mmeshid.addr));			// mac ok
	unsigned char mac_base[6] = {0};
	esp_efuse_mac_get_default(mac_base);
	esp_read_mac(mac_base, ESP_MAC_WIFI_STA);
	sprintf(s_sysset.mac_val,MACSTR,MAC2STR(mac_base));
	s_sysset.bytesout_val=theBlower.getStatsBytesOut();
	s_sysset.bytesin_val=theBlower.getStatsBytesIn();
	s_sysset.msgin_val=theBlower.getStatsMsgIn();	
	s_sysset.msgout_val=theBlower.getStatsMsgOut();
	s_sysset.conns_val=theBlower.getStatsStaConns();
	s_sysset.disco_val=theBlower.getStatsStaDiscos();
    time_t  ahora=theBlower.getStatsLastCountTS();
	strcpy(s_sysset.ldate_val,ctime(&ahora));
	*data = s_sysset;
}

void my_set_sysset(struct sysset *data) // there is no setting in this menu option
{
}

void show_profiles()
{
	for (int a=0;a<2;a++)
	{
		printf("======== Profile[%d] %s %s =========\n",a,theConf.profiles[a].name,theConf.profiles[a].version);
		printf("\tIssued %s",ctime(&theConf.profiles[a].issued));
		printf("\tExpires %s",ctime(&theConf.profiles[a].expires));
		printf("\tNumber of Cycles is %d\n",theConf.profiles[a].numCycles);
		for (int c=0;c<theConf.profiles[a].numCycles;c++)
		{
			printf(" \t\t---- Cycle[%d] Day %d Duration %d Hours %d\n",c,theConf.profiles[a].cycle[c].day,
					theConf.profiles[a].cycle[c].duration,theConf.profiles[a].cycle[c].numHorarios);
		
			for (int h=0;h<theConf.profiles[a].cycle[c].numHorarios;h++)
			{
				printf("\t\t\t• Horario[%d] Start %d Duration %d PWD %d\n",h,theConf.profiles[a].cycle[c].horarios[h].hourStart,
					theConf.profiles[a].cycle[c].horarios[h].horarioLen,theConf.profiles[a].cycle[c].horarios[h].pwmDuty);
			}
		printf("\n\n");
		}
	}	
}
err_t make_profile(char * prof)
{
	struct tm tm={0};
	time_t tt;

	cJSON *root=cJSON_Parse(prof);
	if (!root) 
	{
		ESP_LOGE(TAG, "Internal cJSON Make");
        return ESP_FAIL;
	}
	// lets parse it
	//get profile array
	cJSON *profiles_array=cJSON_GetObjectItem(root,"profiles");
	if(!profiles_array) {
		ESP_LOGE(TAG, "Make Profile no profiles");
        return ESP_FAIL;
	}
	int sonp=cJSON_GetArraySize(profiles_array);
	if(sonp>0 && sonp<3) // wierd its not > 0 but...
	{
		for(int a=0;a<sonp;a++)
		{
			cJSON *pitem=cJSON_GetArrayItem(profiles_array, a);//next item
			if(pitem)
			{
				// got a valid profile
				// save metadata Name, issued, expires and number of cycles
				cJSON *metad= cJSON_GetObjectItem(pitem,"name");
				if(metad)
					strcpy(theConf.profiles[a].name,metad->valuestring);

				metad= cJSON_GetObjectItem(pitem,"version");
				if(metad)
					strcpy(theConf.profiles[a].version,metad->valuestring);
				
				// printf("Profile Name %s Version %s\n",theConf.profiles[a].name,theConf.profiles[a].version);

				metad= cJSON_GetObjectItem(pitem,"issued");
				if(metad)
				{

				    if (strptime(metad->valuestring, "%d-%m-%Y", &tm) != NULL) 
						// Convert to time_t
						theConf.profiles[a].issued= mktime(&tm);

				}

				metad= cJSON_GetObjectItem(pitem,"expires");
				if(metad)
				{
				    if (strptime(metad->valuestring, "%d-%m-%Y", &tm) != NULL) 
						// Convert to time_t
						theConf.profiles[a].expires= mktime(&tm);	
				}
			// get cycles
				cJSON *ciclos=cJSON_GetObjectItem(pitem,"ciclos");
                if(!ciclos)
				{
					ESP_LOGE(TAG, "Make Profile no Cycles");
					cJSON_Delete(root);
        			return ESP_FAIL;
				}

				int sonc=cJSON_GetArraySize(ciclos);
				if(sonc<1 || sonc>MAXCICLOS)
				{
					ESP_LOGE(TAG, "Make Profile Invalid Number of Cycles %d",sonc);
					cJSON_Delete(root);
        			return ESP_FAIL;
				}
				// set count of cycles
				theConf.profiles[a].numCycles=sonc;
				// printf("Profile[%d] ciclos %d\n",a,sonc);
				// loop for cycles
				for (int c=0;c<sonc;c++)
				{
					// get next cycle 
					cJSON *cycle_item=cJSON_GetArrayItem(ciclos, c);//next item
					if(!cycle_item)
					{
						ESP_LOGE(TAG, "Make Profile % d no cycle item  %d",a,c);
						cJSON_Delete(root);
						return ESP_FAIL;
					}
					// get cycle data
					cJSON *cycle_meta=cJSON_GetObjectItem(cycle_item,"startday");
					if(!cycle_meta)
					{
						ESP_LOGE(TAG, "Make Profile %d Cycles %d no Startday",a,c);
						cJSON_Delete(root);
						return ESP_FAIL;
					}
					theConf.profiles[a].cycle[c].day=cycle_meta->valueint; //later check sanity

					cycle_meta=cJSON_GetObjectItem(cycle_item,"dias");
					if(!cycle_meta)
					{
						ESP_LOGE(TAG, "Make Profile %d Cycles %d no Days",a,c);
						cJSON_Delete(root);
						return ESP_FAIL;
					}
					theConf.profiles[a].cycle[c].duration=cycle_meta->valueint; //later check sanity

					// get horarios
					cJSON *horas=cJSON_GetObjectItem(cycle_item,"horario");
					if(!horas)
					{
						ESP_LOGE(TAG, "Make Profile %d Cycle %d no Hour Schedule",a,c);
						cJSON_Delete(root);
						return ESP_FAIL;
					}
					// get count Hours
					int sonh=cJSON_GetArraySize(horas);
					// printf("Profile[%d] Ciclo[%d] horarios %d\n",a,c,sonh);
					if(sonh<1 || sonh>MAXHORARIOS)
					{
						ESP_LOGE(TAG, "Make Profile %d Cycles %d Hours Invalid %d",a,c,sonh);
						cJSON_Delete(root);
						return ESP_FAIL;
					}
					theConf.profiles[a].cycle[c].numHorarios=sonh;

					//loop for hours
					for (int h=0;h<sonh;h++)
					{
						cJSON *hour_item=cJSON_GetArrayItem(horas, h);//next item
						if(!hour_item)
						{
							ESP_LOGE(TAG, "Make Profile %d Cycles %d no Hour %d",a,c,h);
							cJSON_Delete(root);
							return ESP_FAIL;
						}
						// get hour start data
						cJSON *hmeta=cJSON_GetObjectItem(hour_item,"h_start");
						if(!hmeta)
						{
							ESP_LOGE(TAG, "Make Profile %d Cycle %d Hour %d no start hour",a,c,h);
							cJSON_Delete(root);
							return ESP_FAIL;
						}	
						theConf.profiles[a].cycle[c].horarios[h].hourStart=hmeta->valueint;
						// get hour duration data
						hmeta=cJSON_GetObjectItem(hour_item,"h_secs");
						if(!hmeta)
						{
							ESP_LOGE(TAG, "Make Profile %d Cycle %d Hour %d no start duration",a,c,h);
							cJSON_Delete(root);
							return ESP_FAIL;

						}	
						theConf.profiles[a].cycle[c].horarios[h].horarioLen=hmeta->valueint;
						// printf("Horario len %d - %d\n",theConf.profiles[a].cycle[c].c_horarios[h].horario_len,hmeta->valueint);
						// get hour pwm data
						hmeta=cJSON_GetObjectItem(hour_item,"pwm_duty");
						if(!hmeta)
						{
							ESP_LOGE(TAG, "Make Profile %d Cycle %d Hour %d no PWM",a,c,h);
							cJSON_Delete(root);
							return ESP_FAIL;
;
						}	
						theConf.profiles[a].cycle[c].horarios[h].pwmDuty=hmeta->valueint;
					}	// horarios
				}	// ciclos
			}	// pitme
		}	// profiles
	}	// array count profiles > 0
	else
	{
		ESP_LOGE(TAG, "Make Profile range invalid %d",sonp);
		cJSON_Delete(root);
		return ESP_FAIL;
	}
	return ESP_OK;
}

void my_set_profile(struct profile *data) // there is no setting in this menu option
{

	s_profile=*data;
	cJSON *prof_root= cJSON_Parse(s_profile.schedule);
	if(!prof_root)
	{
        ESP_LOGE(TAG, "Invalid JSON Profile");
		return;
	}
	
	FILE* f = fopen(PROFILE_FILE, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing Profile");
        return;
    }
	fprintf(f, s_profile.schedule);
    fclose(f);
	// printf("Profile saved len %d\n",strlen(s_profile.schedule));
	err_t err=make_profile(s_profile.schedule);
	if(err==ESP_OK)
	{
		write_to_flash();
		show_profiles();
	}
	cJSON_Delete(prof_root);
}

void my_get_profile(struct profile *data) 
{
	size_t largo;

	FILE* f = fopen(PROFILE_FILE, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
	fgets(s_profile.schedule, sizeof(s_profile.schedule), f);
    fclose(f);
	*data=s_profile;
}

void my_set_limits(struct limits *data) // save limits from web to theblower
{
	int local[21][2];

		// void * aca=theBlower.getLimits();
		// memcpy(aca,data,sizeof(struct limits));
	//	memcpy(local,data,sizeof(struct limits));
	// for (int a=0;a<21;a++)
	// 	printf("Pos[%d] Min %d Max %d\n",a,local[a][0],local[a][1]);
		theBlower.setLimits((void*)data);
	// 	memcpy(local,theBlower.getLimits(),sizeof(struct limits));
	// for (int a=0;a<21;a++)
	// 	printf("Web rePos[%d] Min %d Max %d\n",a,local[a][0],local[a][1]);
}


void my_get_limits(struct limits *data) // return limits saved in theblower
{
	// int local[21][2];
	// memcpy(&local,theBlower.getLimits(),sizeof(local));
	// for (int a=0;a<21;a++)
	// 	printf("Web Get[%d] min %d max %d\n",a,local[a][0],local[a][1]);
 	// data=(struct limits*)theBlower.getLimits();
	memcpy(data,theBlower.getLimits(),sizeof(struct limits));
}

void app_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "profile",
      .max_files = 2,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }


    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }


    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partition size info.
    if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

}


// When webserver started the meter controller is ACTIVE so beats are counted and saved
void start_webserver(void *pArg)
{
	ESP_LOGW(MESH_TAG,"Starting webserver");
	mongoose_init();
	// app_spiffs();
  	mongoose_set_http_handlers("settings", my_get_settings, my_set_settings);		// when virgin chip or first run
  	mongoose_set_http_handlers("system", my_get_system ,my_set_system);				// additonal system settings
  	mongoose_set_http_handlers("sysset", my_get_sysset ,my_set_sysset);				// general data just for information not mutable
  	mongoose_set_http_handlers("profile", my_get_profile ,my_set_profile);				// working meter data
  	mongoose_set_http_handlers("limits", my_get_limits ,my_set_limits);				// working meter data
	//web timeout if not done in 2 minutes restart
	// webTimer=xTimerCreate("restart",pdMS_TO_TICKS(120000),pdFALSE,( void * ) 0, [] ( TimerHandle_t xTimer){	
	// 	// if(strlen(theBlower.getMID())!=0)		// if meter already configured then return to configu4red state (2)
	// 	// 	theConf.meterconf=2;
	// write_to_flash();
	// esp_restart();});   
// start the meter now
	// xTimerStart(webTimer,0);

	for (;;) {
    mongoose_poll();
  	}

}



#endif