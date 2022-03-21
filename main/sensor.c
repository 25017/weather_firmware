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

#include "measure_msg.h"

#define tag "BME280"



#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

const TickType_t xQueueBlockTime = pdMS_TO_TICKS(200);

void sensor_task(void *pvParameter) {

	bmp280_params_t params;
	bmp280_init_default_params(&params);
	bmp280_t dev;

	GenericParam_t * queues = (GenericParam_t *) pvParameter;
	QueueHandle_t mq   = queues->mq;
	QueueHandle_t mqtt = queues->iq;


	struct MeasurementMessage msg;
	float pressure, temperature, humidity;

	ESP_LOGI(tag, "Starting SENSOR task");

	memset(&msg, 0, sizeof msg);
	memset(&dev, 0, sizeof(bmp280_t));

	// we use pins 21 and 22 for data transfer between sensor and ESP32
	// note that BMP280_I2C_ADDRESS_0 holds a constant preset address 0x76
	// which is used by all BMP280 sensors
	ESP_ERROR_CHECK(
			bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, 0, SDA_PIN, SCL_PIN));
	vTaskDelay(500 / portTICK_PERIOD_MS);
	ESP_ERROR_CHECK(bmp280_init(&dev, &params));

	// check if we have detected BME280 or BMP280 modification of the sensor
	bool bme280p = dev.id == BME280_CHIP_ID;
	ESP_LOGI(tag,"BMP280: found %s", bme280p ? "BME280" : "BMP280");

	vTaskDelay(1500 / portTICK_PERIOD_MS);

	int i = 0;
	for (;;) {

		if (bmp280_read_float(&dev, &temperature, &pressure,
				&humidity) != ESP_OK) {
			ESP_LOGE(tag, "Temperature/pressure reading failed");
		} else {
			ESP_LOGI(tag,"%3i: temp: %f, press.hPa:%f, press:hgmm:%f hum: %f", i,
					temperature, pressure / 100, pressure / 133.322, humidity);

			msg.temperature = temperature;
			msg.humidity = humidity;
			msg.pressure = pressure;

			xQueueSendToFront(mq  , (void *)&msg, xQueueBlockTime); // send to display Q.
			xQueueSendToFront(mqtt, (void *)&msg, xQueueBlockTime); // send to MQTT Q.
		}
		i++;
		vTaskDelay(60000 / portTICK_PERIOD_MS);
	}
}
