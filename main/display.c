/*
 * display.c
 *
 *  Created on: 8 Feb 2021
 *      Author: zsolt
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
#include <string.h>

#include "ssd1306.h"
#include "measure_msg.h"

#define tag_oled "SSD1306"

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

void display_task(void *pvParameter) {


	GenericParam_t * queues = (GenericParam_t *) pvParameter;

	//QueueHandle_t queue = (QueueHandle_t) pvParameter;

	QueueHandle_t mq = queues->mq;
	QueueHandle_t iq = queues->iq;
	char buf_ipaddr[16];

	struct MeasurementMessage msg;
	SSD1306_t dev_oled;

	ESP_LOGI(tag_oled, "Starting DISPLAY task");

	memset(&msg, 0, sizeof msg);
	memset(&dev_oled, 0, sizeof(SSD1306_t));
	vTaskDelay(500 / portTICK_PERIOD_MS);

	ESP_ERROR_CHECK(
			SSD1306_init_desc(&dev_oled, I2CAddress, 0, SDA_PIN, SCL_PIN));
	ESP_ERROR_CHECK(SSD1306_i2cdev_init(&dev_oled, 128, 32));
	ESP_LOGI(tag_oled, "_address=0x%x", dev_oled._address);
	ESP_LOGI(tag_oled, "_width=%i", dev_oled._width);
	ESP_LOGI(tag_oled, "_height=%i", dev_oled._height);
	ESP_LOGI(tag_oled, "_pages=%i", dev_oled._pages);

	ssd1306_clear_screen(&dev_oled, true);
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ssd1306_clear_screen(&dev_oled, false);
	ssd1306_contrast(&dev_oled, 0xff);

	/*	for(;;) {

	 ssd1306_display_text(&dev_oled, 0, "SSD1306 128x32", 14, false);
	 ssd1306_display_text(&dev_oled, 1, "Teszt 1 2 3 4 5", 16, true);

	 ssd1306_hardware_scroll(&dev_oled, SCROLL_RIGHT);
	 vTaskDelay(5000 / portTICK_PERIOD_MS);
	 ssd1306_hardware_scroll(&dev_oled, SCROLL_LEFT);
	 vTaskDelay(5000 / portTICK_PERIOD_MS);
	 ssd1306_hardware_scroll(&dev_oled, SCROLL_STOP);
	 ssd1306_clear_screen(&dev_oled, false);

	 ssd1306_display_text(&dev_oled, 0, "SSD1306 128x32", 14, true);
	 ssd1306_display_text(&dev_oled, 2, "Teszt 1 2 3 4 5", 16, false);
	 ssd1306_hardware_scroll(&dev_oled, SCROLL_DOWN);
	 vTaskDelay(5000 / portTICK_PERIOD_MS);
	 ssd1306_hardware_scroll(&dev_oled, SCROLL_UP);
	 vTaskDelay(5000 / portTICK_PERIOD_MS);
	 ssd1306_hardware_scroll(&dev_oled, SCROLL_STOP);
	 ssd1306_clear_screen(&dev_oled, false);

	 vTaskDelay(1000 / portTICK_PERIOD_MS);
	 }*/

	ssd1306_hardware_scroll(&dev_oled, SCROLL_DOWN);
	int i = 1;
	for (;;) {

		xQueueReceive(mq, &(msg), pdMS_TO_TICKS(100));
		xQueueReceive(iq, &buf_ipaddr, pdMS_TO_TICKS(100));

		char buf_temp[16];
		char buf_press[16];
		char buf_hum[16];

		memset(buf_temp, 0, sizeof buf_temp);
		memset(buf_press, 0, sizeof buf_press);
		memset(buf_hum, 0, sizeof buf_hum);

		snprintf(buf_temp, 16, "tmp : %4.2f *C", msg.temperature); // puts string into buffer
		snprintf(buf_press, 16, "prs : %4.2f hPa", msg.pressure / 100);
		snprintf(buf_hum, 16, "hum : %4.2f %%", msg.humidity);

		ssd1306_display_text(&dev_oled, 0, buf_temp, 16, false);
		ssd1306_display_text(&dev_oled, 1, buf_press, 16, false);
		ssd1306_display_text(&dev_oled, 2, buf_hum, 16, false);
		ssd1306_display_text(&dev_oled, 3, buf_ipaddr, 16, false);

/*		if ((i % 2) == 0) {
			ssd1306_clear_line(&dev_oled, 0, false);
			ssd1306_display_text(&dev_oled, 1, buf_temp, 16, false);
			ssd1306_display_text(&dev_oled, 2, buf_press, 16, false);
			ssd1306_display_text(&dev_oled, 3, buf_hum, 16, false);
		} else {
			ssd1306_clear_line(&dev_oled, 3, false);
			ssd1306_display_text(&dev_oled, 0, buf_temp, 16, false);
			ssd1306_display_text(&dev_oled, 1, buf_press, 16, false);
			ssd1306_display_text(&dev_oled, 2, buf_hum, 16, false);
		}*/
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		i++;
	}
}
