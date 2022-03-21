/*
 * mqtt.c
 *
 *  Created on: 9 Feb 2021
 *      Author: zsolt
 */

#include "mqtt.h"

#define TAG  "MQTT"
#define CONFIG_BROKER_URL "mqtt://192.168.178.38"

void create_weather_msg(char *msg, struct MeasurementMessage *msg_struct) {
	sprintf(msg,
			"{\"temp\":%4.2f,\"pressure\":%4.2f,\"humidity\":%4.2f}",
			msg_struct->temperature, msg_struct->pressure /100,
			msg_struct->humidity);
}

/**
 * @brief read message from the queue and send it to the MQTT topic
 *
 * @param client MQTT client
 * @param weather_msg_queue FreeRTOS Queue which contains updates
 */
void send_weather_to_mqtt(esp_mqtt_client_handle_t *client,
		QueueHandle_t weather_msg_queue) {
	int msg_id;
	// if our queue handle points to somewhere
	if (weather_msg_queue != 0) {
		// wait for the new message to arrive

		for (;;) {
			//ESP_LOGI(TAG, "Receiving message from the queue");
			struct MeasurementMessage msg;
			//if (xQueueReceive(weather_msg_queue, &(msg), pdMS_TO_TICKS(100))) {  //portMAX_DELAY
			if (xQueueReceive(weather_msg_queue, &(msg), portMAX_DELAY)) {  //publish only when something received in the queue
				//ESP_LOGI(TAG, "Got new message from the queue");

				// encode WeatherMessage struct in JSON format using a helper function
				char json_msg[90];
				create_weather_msg(json_msg, &msg);

				// publish JSON message to the MQTT topic
				ESP_LOGI(TAG, "Sending JSON message: %s", json_msg);
				msg_id = esp_mqtt_client_publish(*client, "weather", json_msg,
						0, 1, 0);
					ESP_LOGD(TAG, "publish successful, msg_id=%d", msg_id);

				//vTaskDelay(1000 / portTICK_PERIOD_MS);

			}
		}
	}
}

/**
 * @brief MQTT event processing handler
 *
 * @param event set by ESP-IDF
 * @param weather_msg_queue the queue to publish WeatherMessages
 * @return esp_err_t
 */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event,
		EventGroupHandle_t *event_group) {

	// let's get MQTT client from our event handle
	//esp_mqtt_client_handle_t client = event->client;
	switch (event->event_id) {
	case MQTT_EVENT_CONNECTED:
		// push WeatherMessage to the MQTT topic if we have connected to the server
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		xEventGroupSetBits(*event_group, MQTT_CONNECTED_BITS);
		break;
	case MQTT_EVENT_DISCONNECTED:
		// other events are handled mostly for logging and debugging,
		// the error messages should be self-explanatory
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
		if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
			ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x",
					event->error_handle->esp_tls_last_esp_err);
			ESP_LOGE(TAG, "Last tls stack error number: 0x%x",
					event->error_handle->esp_tls_stack_err);
		} else if (event->error_handle->error_type
				== MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
			ESP_LOGE(TAG, "Connection refused error: 0x%x",
					event->error_handle->connect_return_code);
		} else {
			ESP_LOGW(TAG, "Unknown error type: 0x%x",
					event->error_handle->error_type);
		}
		break;

	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED msg_id=%i",event->msg_id);
		break;

	case MQTT_EVENT_BEFORE_CONNECT:
		ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");

		break;

	default:
		ESP_LOGI(TAG, "MQTT recieved event id:%d", event->event_id);
		break;
	}
	return ESP_OK;
}

/**
 * @brief A top-level handler that wraps around mqtt_event_handler_cb
 *
 * @param handler_args QueueHandle_t queue for WeatherMessages
 * @param base set by ESP-IDF
 * @param event_id set by ESP-IDF
 * @param event_data set by ESP-IDF
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data) {
	EventGroupHandle_t event_group = (EventGroupHandle_t) handler_args;
	//ESP_LOGI(TAG, "mqtt_event_handler EventGroupHandle_t: %p", event_group);
	//ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
	//		event_id);
	mqtt_event_handler_cb(event_data, event_group);
}

/**
 * @brief starts the MQTT component
 *
 * @param queue QueueHandle_t for WeatherMessages
 */
esp_mqtt_client_handle_t mqtt_app_start(EventGroupHandle_t *event_group) {

	ESP_LOGI(TAG, "mqtt_app_start event_group: %p", *event_group);
	ESP_LOGI(TAG, "Connecting to MQTT server at %s", CONFIG_BROKER_URL);
	esp_mqtt_client_config_t mqtt_cfg = { .uri = CONFIG_BROKER_URL, .username =
			"weather", .password = "weather" };

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	// let's register our handler
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
			(void*) event_group);
	esp_mqtt_client_start(client);

	ESP_LOGI(TAG, "client ref is: %p", client);
	ESP_LOGI(TAG, "Waiting for MQTT_CONNECTED_EVENT");

	xEventGroupWaitBits(*event_group, MQTT_CONNECTED_BITS, true, true,
			portMAX_DELAY);

	return client;
}
