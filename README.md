📝  🚀🔥✨❤️💼🏠📣🧑‍💻

# Mesh Router and MQTT offline strategy

Experiments done to have final conditions for online and offline consistent work

## mqtt_start_app

    This is just the setup of the Mqtt server. Nothing expectacular leave as is.

    The auto_reconnect does not seem to have any effect

## MQTT Event Handler

***MQTT_EVENT_CONNECTED***

> - must subscribe to command topic to receive retained messages like commands
> - Second version should have a Batch of commands since we have very few opportunities to get commands, only when sending readings or booting
> - mqttf MUST be set here to inform other tasks the MQTT is now connected
> - mqtt_sender relies on this flag heavely

***MQTT_EVENT_DISCONNECTED***

> will reset the mqttf to false indicating no connection and try to start the mqtt again

***MQTT_EVENT_SUBSCRIBED***

> - subscribes to the CMD topic for incoming message
> - Has a EventGroup bit that could be used to indicate that the System Mqtt is ready

***MQTT_EVENT_PUBLISHED***

> - Very important since we are asking for confirmation, that should be standard in MQTT, that the message was delivered (published)
> - it does not work as expected.
>
>   - When connected online it does what it is supposed to do,
>     like turn PUB_BIT on and the ***mqtt_sender*** will detetc this NOT TO REQUEUE the lost mesasge.

***MQTT_EVENT_DATA***

> - get messages for the MCU and dispatches to a queue mqttQ where a task is waiting to handle the commands.
> - The message MUST be RETAIN type so it will reset the CloudMqtt via a blank message

***MQTT_EVENT_ERROR***

> just prints Error for debugging

***MQTT_EVENT_BEFORE_CONNECT***

    not used but interesting

## MQTT_SENDER

    is a TASK that is in charge of sending any messages to the MQTT HQ.

    No direct connection from other mesh nodes because we want very few connection to the MQTT Broker due to pricing.

    It waits on an xEventGroupWaitBits(wifi_event_group,SENDMQTT_BIT...) as the starting gun instead of a semaphore.

---

### **Router** Status and MQTT service

> We use **hostflag** for the Router status (true on) which is managed by the mesh_event handler
>
> **mqttf** status of the MQTT service managed by the mqtt-event handler (expalined above)

    if hostflag is not true (router is down) it waits for this flag to be changed (mesh event manager).

> The MQTT service BY DEFAULT is stopped, since every message needed to be sent will start a connection, send the message (if there is incoming message process it with no reply) and close the connection.
>
> It waits for the xEventGroupWaitBits(wifi_event_group, MQTT_BIT...) that the connection has been established.

Now we have the MQTTF loop[ that needs to be told that a connection is acrtive, as explained above.

Then an loop to read and send any queued messages. The idea is to connect once and send any number (20) of messasges any task may have requested IN ONE CONNECTION.

if the Queue has entries (at least one by design) it will publish the message to the Mqtt service AND wait for confirmation uxBits=xEventGroupWaitBits(wifi_event_group, PUB_BIT,...).

HERE IS WHERE EVERYTHING GOES TITS UP

If there is no confirmation we assume Router or MQTT service (internet conecction for example) is down so we queue the message again xQueueSend(mqttSender,&mensaje,0). If this fails, we need to free the original malloc in the free(mensaje.msg) or we will eventually crash. We continue wiating now for the mqttf if is was not true initially.

Eventually the mqttf is True and we got confirmation the message was sent. and finally free the malloc form the original requester. Next message in queue if any, same idea.

When ALL queued messages have been read, we STOP the MQTT service and disconnect.

MESH Router Logic

The mesh has been started and usually working when we have a Rotuer failure. This will be viewed as a MESH_EVENT_PARENT_DISCONNECTED if we are ROOT (which is the only one in charge of AP messages in and out).

If this event triggers, the hostflag and mqttf are set to false for the mqtt_sender task as expalined above.

The effect is a Router malfunction, damaged, powered off, etc. We should get an IP_VENT_LOST_IP but NO, we get the above Mesh event PARENT DISCONNECTED, same thing.

Connection and reconnection of Rotuer

We consider the router up when we get a IP_EVENT_STA_GOT_IP in the ip_event_handler.

It sets the hostflag to true and has certain logic explained below.

If this was the first tiem we got the IP, we set a *loadedf* to true and start secondary important tasks which are the mqtt_app_start, some post_root start and an emergency task. This is so that we do not start this tasks AGAIN in a recconection.

## Version Review

### Version month-data

* Mayor change making communication structures inside de Mesh binary due to max size restriction of MTU 1500. see typedef.h structure **meshunion_t**
* Data sent from nodes include Node idetification and MONTH and MONTHHOURS of the CURRENT month only of ALL Meters (even if not configured ei, nodeid= blank )
* Data is collected from all nodes into a SINGLE cJSON structure and sent to MQtt server. Potentially a lot of data, considering that a sample Node is about 2500 bytes, so multiplied by X Nodes can be very big. Mqtt will segment message transparently for us, but still is a considertaion for next version or option that will send MQtt server in BINARY instead of cJson, which adds to security
* Connection Sharing is implemented based on Number of estimated Units divede by number of estimated mqtt connectios

  * Used 2.5MM and 10K conns given 250 slots that makes every node able to transmit and receive a message every 8.7 minutes (500 seconds). Tested interval and reception

  As of Jul 20 can not assert massive test due to no Nodes to test malloc/free leaks and performance. -> Pending at least 3 nodes = 24 Meters

### Version Binary Mqtt

Send Mqtt in binary format with an array of MAXNODES of 10, hence 80 meters max, with one single binary mqtt message

## Version Prepaid

Including logic for prepaid option

## AES Encryption for Initial Configuration

The displayed number on the OLED (ex 5546600) will be expanded to be the KEY for AES. Thats 32 bytes so it needs first to be padded to 16 bytes with left 0 -> 0000000005546600 and then appenede onto itself, 00000000055466000000000005546600. This is the secret key. The Text to be encrypted is SUPERSECRET. This will generate a HEX block of which the first 4Bytes (hex format) is used by the WebConfigurator as password. 

Complete example:

System was PARKED and then via Bluetooth was provisioned to a Network. Then there is a Configuration phase that will display on the OLED a number, in this case 5546600, Now Command Center will receive this number VIA CELL PHONE or Whatever and input it to a AES Encryptor, like https://anycript.com/crypto, put the SUPERSECRET text in place, and set the secret key to the generated key as explained above. Output from the online/whatever encryptor should be in HEX and give the first 4 bytes (8 chars) as password for the Webiste Configuration, in this case 462b1e82.

## WARNING
The espidf 5.4 as dwonload via VSC DOES NOT have the latest I2C Driver and files (includes) so the code and text(.h) was copied from website and put in i2c_master.h and i2c_master.c

Hence, a espidf update will kill it, cannot compile due to references and types

## Mongoose web server integration
Using mongoose.ws create the Dashboard/Web site
in settings the option to generate must be complete project
in the esp32 app, add the monghoose dir that comes out of that generation directory
in the mongoose folder (in espidf) change in mongoose_glue.c take out STATIC from the structures created
thats is all
the code must have  extern to these structures and the routine/task that call mongosee have the call backs for each strucutes like the following

## void my_get_settings(struct settings *data) {
      printf("MID %s Start %d Chall %s prepago %d bpk %d slot %d node %d ssid %s\n",s_settings.mid_val,s_settings.kwhstart_val,
   s_settings.challenge_val,s_settings.prepago_val,s_settings.bpk_val,s_settings.slot_val,s_settings.node_val,
   s_settings.ssid_val);
   sprintf(s_settings.ssid_val,"%d",theConf.cid);
  *data = s_settings;  // Sync with your device
}
## void my_set_settings(struct settings *data) {
	// char laclave[20];
	uint32_t keyread;
	char *ptr; 
	char *laclave=(char*)calloc(1,40);

	printf("mid [%s] len %d\n",data->mid_val,strlen(data->mid_val));
	strcpy(laclave,data->challenge_val);
	ESP_LOG_BUFFER_HEX(MESH_TAG,laclave,strlen(laclave));
	keyread=strtoul(laclave, &ptr, 16);
	printf("Key %x %d\n",keyread,theConf.cid);
	ESP_LOG_BUFFER_HEX(MESH_TAG,&keyread,sizeof(keyread));

	if(!check_key(keyread))
	{
		printf("Not same key %d %d\n",keyread,theConf.cid);
		// sendnak(req);
	}
	free(laclave);
  s_settings = *data; // Sync with your device
  printf("savesettings\n");
}



## void start_webserver(void *pArg)
{
	ESP_LOGW(MESH_TAG,"Stating webserver");
	mongoose_init();
  	mongoose_set_http_handlers("settings", my_get_settings, my_set_settings);

  for (;;) {
    mongoose_poll();
  }

}
## 