/*
 * measure_msg.h
 *
 *  Created on: 8 Feb 2021
 *      Author: zsolt
 */

#ifndef MAIN_MEASURE_MSG_H_
#define MAIN_MEASURE_MSG_H_

#include "freertos/queue.h"
#include "freertos/event_groups.h"

struct MeasurementMessage
{
    float temperature;
    float pressure;
    float humidity;
};

typedef struct
	{
		QueueHandle_t mq;
		QueueHandle_t iq;

	} GenericParam_t;

typedef struct
{
	QueueHandle_t iq;
	EventGroupHandle_t s_connect_event_group;
} QueueEvent_t;




#endif /* MAIN_MEASURE_MSG_H_ */
