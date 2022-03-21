/*
 * wifi.h
 *
 *  Created on: 8 Feb 2021
 *      Author: zsolt
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_log.h"
#include "measure_msg.h"

//void wifi_connect_blocking(EventGroupHandle_t* connected_event_group);
void wifi_connect_blocking(QueueEvent_t* queue_event);

#endif /* MAIN_WIFI_H_ */
