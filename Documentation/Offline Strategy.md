 📝  🚀🔥✨❤️💼🏠📣🧑‍💻

# Mesh Router and MQTT offline strategy

Experiments done to have final conditions for online and offline consistent work

## mqtt_start_app

This is just the setup of the Mqtt server. Nothing expectacular leave as is.

- The auto_reconnect does not seem to have any effect

### MQTT Event Handler

***MQTT_EVENT_CONNECTED***  must *subscribe* to command topic to receive retained messages like commands

* Second version should have a Batch of commands since we have very few opportunities to get commands, only when sending readings or booting
* mqttf MUST be set here as indicating other tasks the MQTT is now connected
* mqtt_sender relies on this flag heavely

MQTT_EVENT_DISCONNECTED will reset the mqttf to false indicating no connection and try to start the mqtt again

MQTT_EVENT_SUBSCRIBED subscribes to the CMD topic for incoming messages

* Has a EventGroup bit that could be used to indicate that the System Mqtt is ready

MQTT_EVENT_PUBLISHED is important since we are asking for confirmation, that should be standard in MQTT, that the message was delivered (published).

It does not work as expected. When connected online it does what it is supposed to do, like turn PUB_BIT on and the *mqtt_sender* will detetc this NOT TO REQUEUE the lost mesasge.

MQTT_EVENT_DATA get messages for the MCU and dispatches to a queue mqttQ where a task is waiting to handle the commands. The message MUST be RETAIN type so it will reset the CloudMqtt via a blank message

MQTT_EVENT_ERROR just prints Error for debugging

MQTT_EVENT_BEFORE_CONNECT not used but interesting

MQTT_SENDER

    is a TASK that is in charge of sending any messages to the MQTT HQ.

    No direct contact from other mesh nodes because we want very few connection to the MQTT 		Broker due to pricing.

    It waits on an xEventGroupWaitBits(wifi_event_group,SENDMQTT_BIT...) as the starting gun, vs a semaphore.

Needs to know 2 things: Router status and MQTT status.

    We use hostflag for the Router status (true on) which is managed by the mesh_event handler and the status of the MQTT service via mqttf, managed by the mqtt-event handler (expalined above)

if hostflag is not true (router is down) it waits for this flag to be changed (mesh event manager).

The MQTT service BY DEFAULT is stopped, since every message needed to be sent will start a connection, send the message (if there is incoming message process it with no reply) and close the connection.

It waits for the xEventGroupWaitBits(wifi_event_group, MQTT_BIT...) that the connection has been established.

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
