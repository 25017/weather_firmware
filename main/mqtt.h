/*
 * mqtt.h
 *
 *  Created on: 9 Feb 2021
 *      Author: zsolt
 */

#ifndef MAIN_MQTT_H_
#define MAIN_MQTT_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_sleep.h"

#include "mqtt_client.h"

#include "measure_msg.h"

#define MQTT_CONNECTED_BITS BIT(6)

void create_weather_msg(char *msg, struct MeasurementMessage *msg_struct);
esp_mqtt_client_handle_t mqtt_app_start(EventGroupHandle_t * event_group);
extern void send_weather_to_mqtt(esp_mqtt_client_handle_t *client, QueueHandle_t weather_msg_queue);

#endif /* MAIN_MQTT_H_ */
