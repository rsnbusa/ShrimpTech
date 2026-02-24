#define GLOBAL
#include "globals.h"
extern void write_to_flash();

void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	struct tm timeinfo;
	bzero(&timeinfo,sizeof(timeinfo));

    gps = NULL;
    switch (event_id) {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        /* print information parsed from GPS statements */
    if ((theConf.debug_flags >> dGPS) & 1U) 
		{
			printf("%04d/%02d/%02d %02d:%02d:%02d => \n"
					"\t\t\t\t\t\tlatitude   = %.05f°N\n"
					"\t\t\t\t\t\tlongitude = %.05f°E\n"
					"\t\t\t\t\t\taltitude   = %.02fm\n"
					"\t\t\t\t\t\tspeed      = %fm/s\n",
					gps->date.year+YEAR_BASE, gps->date.month, gps->date.day,
					gps->tim.hour , gps->tim.minute, gps->tim.second,
					gps->latitude, gps->longitude, gps->altitude, gps->speed);
		}
		if(gps->latitude<-90.0 || gps->latitude>90.0)
			return;
		if(gps->longitude<-180.0 || gps->longitude>180.0)
			return; 
			if(abs(gps->latitude)>0.01)
			{
				// printf("GPS to Flash and die\n");
				theConf.lat=gps->latitude;
				theConf.longi=gps->longitude;
				timeinfo.tm_hour=gps->tim.hour+TIME_ZONE;
				timeinfo.tm_min=gps->tim.minute;
				timeinfo.tm_sec=gps->tim.second;
				timeinfo.tm_year=gps->date.year+YEAR_BASE-1900;
				timeinfo.tm_mon=gps->date.month-1;
				timeinfo.tm_mday=gps->date.day;
				theConf.gpsDateTime=mktime(&timeinfo);
				if(!gpsFlag)
				{
					write_to_flash();
					gpsFlag=true;
				}
				vTaskDelete(gpsH);
			}
        break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        // printf( "Unknown statement:%s\n", (char *)event_data);
        break;
    default:
        break;
    }
}

int getGpsTime(void *argument)
{
    // we have a uart to read gps data

}
