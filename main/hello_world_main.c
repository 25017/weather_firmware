/* Hello World Example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include <bmp280.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"

#include "ssd1306.h"
#include "sensor.h"
#include "display.h"
#include "measure_msg.h"
#include "wifi.h"
#include "mqtt.h"

#define tag "app_main"






void app_main(void) {
	char buf_ipaddr[16];
	esp_chip_info_t chip_info;
	EventGroupHandle_t s_connect_event_group = xEventGroupCreate();
	QueueHandle_t measure_queue = xQueueCreate(2, sizeof(struct MeasurementMessage));
	QueueHandle_t mqtt_queue    = xQueueCreate(2, sizeof(struct MeasurementMessage));
	QueueHandle_t ipaddr_queue  = xQueueCreate(2, sizeof(buf_ipaddr));

	esp_mqtt_client_handle_t client_ref;

	GenericParam_t queues = {measure_queue, ipaddr_queue };
	GenericParam_t mqtt_sensor =  {measure_queue, mqtt_queue };
	QueueEvent_t queue_event = {ipaddr_queue, s_connect_event_group };

	/* Print chip information */

	esp_chip_info(&chip_info);
	ESP_LOGI(tag,"This is %s chip with %d CPU core(s), WiFi%s%s, ",
	CONFIG_IDF_TARGET, chip_info.cores,
			(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
			(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

	ESP_LOGI(tag,"silicon revision %d, ", chip_info.revision);

	ESP_LOGI(tag,"%dMB %s flash", spi_flash_get_chip_size() / (1024 * 1024),
			(chip_info.features & CHIP_FEATURE_EMB_FLASH) ?
					"embedded" : "external");

	ESP_LOGI(tag,"Minimum free heap size: %d bytes",
			esp_get_minimum_free_heap_size());

	ESP_LOGI(tag, "QueueHandle:                 measure: %p", measure_queue);
	ESP_LOGI(tag, "GenericParam_t for sensor  : measure: %p ,  2mqtt:     %p ", mqtt_sensor.mq, mqtt_sensor.iq);
	ESP_LOGI(tag, "GenericParam_t for display : measure: %p , ipaddr:     %p ", queues.mq, queues.iq);
	ESP_LOGI(tag, "QueueEvent_t: ipaddr: %p , eventgroup: %p",queue_event.iq, queue_event.s_connect_event_group);

	ESP_LOGI(tag,"i2cdev_init()");
	ESP_ERROR_CHECK(i2cdev_init());

	//sensor task
	xTaskCreate(sensor_task,
	                "sensor_task",
	                4096,
					(void *)&mqtt_sensor,
	                1,
	                NULL);

	//display task
		xTaskCreate(display_task,
		                "display_task",
		                4096,
						(void *)&queues,
		                1,
		                NULL);

		ESP_LOGI(tag,"Tasks started");
		ESP_LOGI(tag,"Initializing Non Volatile Storage");
		// Initialize the NonVolatileStorage
		esp_err_t ret = nvs_flash_init();
		if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
		{
			ESP_ERROR_CHECK(nvs_flash_erase());
			ret = nvs_flash_init();
		}
		ESP_ERROR_CHECK(ret);

		ESP_LOGI(tag,"Starting WIFI");
		wifi_connect_blocking(&queue_event);
		ESP_LOGI(tag,"Started WIFI");

		client_ref = mqtt_app_start(&s_connect_event_group);

		ESP_LOGI(tag,"MQTT Connected, client ref: %p", client_ref);

		send_weather_to_mqtt(&client_ref, mqtt_queue);



/*	for (int i = 10; i >= 0; i--) {
		printf("Restarting in %d seconds...\n", i);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	printf("Restarting now.\n");
	fflush(stdout);
	esp_restart();*/

}
