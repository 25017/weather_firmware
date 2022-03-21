/*
 * wifi.c
 *
 *  Created on: 8 Feb 2021
 *      Author: zsolt
 */

#include <stdint.h>
#include <machine/endian.h>
#include <string.h>
#include "wifi.h"
#include "measure_msg.h"

#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define DEFAULT_RSSI -127
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA2_PSK
#define WIFI_CONNECTED_BITS BIT(1)
#define CONFIG_WIFI_SSID "SSID"
#define CONFIG_WIFI_PASSWORD "PASSWORD"
#define TAG "WIFI"

/**
 * @brief Initialize Wi-Fi as sta and set scan method
 *
 * @param arg a pointer to EventGroupHandle_t that will be used to send connected event
 * @param event_base set by ESP-IDF
 * @param event_id set by ESP-IDF
 * @param event_data set by ESP-IDF
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    // cast our handler argument to QueueEvent_t (a struct contains two members, one for queue and one for event group
	QueueEvent_t * queue_event = (QueueEvent_t *) arg;
	char buf_ipaddr[16];
	const TickType_t xQueueBlockTime = pdMS_TO_TICKS(200);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // try to connect to WiFi network if STAtion is started
    	ESP_LOGI(TAG, "Connecting");
    	memset(buf_ipaddr, 0, sizeof buf_ipaddr);
    	snprintf(buf_ipaddr, 16, "Connecting");
    	xQueueSendToBack(queue_event->iq, (void *)&buf_ipaddr, xQueueBlockTime);
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // in case we were disconnected, try to connect again
    	ESP_LOGI(TAG, "Reconnecting...");
    	memset(buf_ipaddr, 0, sizeof buf_ipaddr);
    	snprintf(buf_ipaddr, 16, "Reconnecting...");
    	xQueueSendToBack(queue_event->iq, (void *)&buf_ipaddr, xQueueBlockTime);

        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        // log IP and fire our connected_event_group if the connection was successful
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        uint32_t ipa = event->ip_info.ip.addr;
        memset(buf_ipaddr, 0, sizeof buf_ipaddr);
        snprintf(buf_ipaddr,sizeof buf_ipaddr,"%u.%u.%u.%u" ,(ipa & 0x000000ff)
        													,(ipa & 0x0000ff00) >> 8
                                                            ,(ipa & 0x00ff0000) >> 16
                                                            ,(ipa & 0xff000000) >> 24);
        ESP_LOGI(TAG, "got ip: %s", buf_ipaddr);
        xQueueSendToBack(queue_event->iq, (void *)&buf_ipaddr, xQueueBlockTime);
        xEventGroupSetBits(queue_event->s_connect_event_group, WIFI_CONNECTED_BITS);
    }
}

/**
 * @brief connect to WiFi network. Connection settings will be taken from sdkconfig file
 *
 * @param connected_event_group will block on this event group until the connection is established
 */
//void wifi_connect_blocking(EventGroupHandle_t* connected_event_group) {
void wifi_connect_blocking(QueueEvent_t* queue_event) {
	// initialize TCP adapter
    tcpip_adapter_init();

    // ESP_ERROR_CHECK is just a useful macro that checks that the function returned ESP_OK and logs error otherwise
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // initialize WiFi adapter and set default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // here we register our connected_event_group as a handler for all WiFi related events,
    // as well as IP_EVENT that indicates that connection was established successfully
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, queue_event));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, queue_event));

    // set up some settings from sdkconfig. You can use esp-idf.py menuconfig to change those variables interactively
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .scan_method = DEFAULT_SCAN_METHOD,
            .sort_method = DEFAULT_SORT_METHOD,
            .threshold.rssi = DEFAULT_RSSI,
            .threshold.authmode = DEFAULT_AUTHMODE,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    // start up the WiFi module
    ESP_ERROR_CHECK(esp_wifi_start());

    // wait for connection
    xEventGroupWaitBits(queue_event->s_connect_event_group, WIFI_CONNECTED_BITS, true, true, portMAX_DELAY);



}
