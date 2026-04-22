#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
#include "time_utils.h"

void showData(void * pArg)
{
	static _lock_t lvgl_api_lock;
	static lv_style_t style;

	lv_obj_clean(lv_scr_act());

	lv_obj_t *scr = lv_display_get_screen_active(disp);
	lv_obj_t *label = lv_label_create(scr);
	lv_obj_t *label2 = lv_label_create(scr);
	lv_obj_t *label3 = lv_label_create(scr);
	//get version
	const esp_app_desc_t *mip=esp_app_get_description();
	(void)mip;

	lv_style_init(&style);
	lv_style_set_text_font(&style, &lv_font_montserrat_14);
	lv_obj_add_style(label, &style, 0); 
	lv_obj_add_style(label2, &style, 0); 
	lv_obj_add_style(label3, &style, 0); 
	while(1)
	{
		_lock_acquire(&lvgl_api_lock);
		lv_label_cut_text(label,0,20);
		lv_label_cut_text(label2,0,20);
		lv_label_cut_text(label3,0,20);
		// lv_label_set_text_fmt(label,"Beats %d",theBlower.getBeats());
		// lv_label_set_text_fmt(label2,"kWh %d",theBlower.getLkwh());
		// lv_label_set_text_fmt(label3,"%s v%s",theBlower.getMID(),mip->version);
		lv_obj_align(label3,LV_ALIGN_TOP_MID, 0, 0);
		lv_obj_align(label,LV_ALIGN_TOP_MID, 0, 20);
		lv_obj_align(label2,LV_ALIGN_BOTTOM_MID ,0,-6);
		_lock_release(&lvgl_api_lock);
		delay(1000);
	}
}

static void IRAM_ATTR show_isr(void* arg)
{
	static TickType_t last= xTaskGetTickCountFromISR();   
    uint32_t ms;
    TickType_t rnow;
    BaseType_t mustYield = pdFALSE;

	if(!gpio_get_level((gpio_num_t)0))
	{
		rnow=xTaskGetTickCountFromISR();  
		ms=pdTICKS_TO_MS(rnow-last);
		if (ms>100)
			vTaskNotifyGiveFromISR(dispHandle,&mustYield);
	}
}

void displayManager(void *arg) 
{
  	gpio_config_t 	        io_conf;
    
    bzero(&io_conf,sizeof(io_conf));

	io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
	io_conf.pull_up_en =GPIO_PULLUP_ENABLE;     			
	io_conf.pull_down_en =GPIO_PULLDOWN_DISABLE;
	io_conf.pin_bit_mask = (1ULL<<0);    
	gpio_config(&io_conf);

    gpio_install_isr_service(0);								// in case its not installed yet

	gpio_isr_handler_add((gpio_num_t)0, show_isr, NULL);		//install our ISR

	while(true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  	// waiting on a pulse so no need to delay/sleep
		{
			if(showHandle)
			{
				ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
				vTaskDelete(showHandle);
				showHandle=NULL;
			}
			else
			{
				ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
				xTaskCreate(&showData,"sdata",1024*3,NULL, 5, &showHandle); 
				dispTimer=xTimerCreate("DispT",pdMS_TO_TICKS(30000),pdFALSE,NULL, []( TimerHandle_t xTimer)
				{ 				
					vTaskDelete(showHandle);
					ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
					showHandle=NULL;

				});   
				xTimerStart(dispTimer,0);
	        
			}       
		}
	}
}