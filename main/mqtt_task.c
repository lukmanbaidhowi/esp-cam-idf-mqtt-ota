#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
// #include "protocol_examples_common.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

#include "define.h"
#include "wifi_ota.h"

static const char *TAG = "mqtt";

/////////////////////////////////////////////////////////////////////////
///// MQTT////////

long unsigned int delayTimeSend = 1000 * 10 *60; //ms

esp_mqtt_client_handle_t client;

static void publish_mqtt_task(){
    static const char *PUBISH_TAG = "P_TASK";
    char * temp = malloc(1024+1); 
    int msg_id;
    while(1){
        sprintf(temp,"{\"device id\":%s,\"version\":%.1f,}",CONFIG_DEVICE_ID,FIRMWARE_VERSION);
        msg_id = esp_mqtt_client_publish(client, CONFIG_MQTT_TOPIC_PUBLISH, temp, 0, 1, 0);
        ESP_LOGI(PUBISH_TAG, "sent publish successful, msg_id=%d", msg_id);
        vTaskDelay(delayTimeSend / portTICK_PERIOD_MS);
    }
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    int msg_id;
    // TaskHandle_t xHandle;
    client = event->client;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(client, CONFIG_MQTT_TOPIC_PUBLISH, "initialization", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, CONFIG_MQTT_TOPIC_SUBS, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            xTaskCreate(publish_mqtt_task, "publish_mqtt_task", 1024*2, NULL, 6, NULL);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, CONFIG_MQTT_TOPIC_PUBLISH, "Device recv_data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            cJSON * root   = cJSON_Parse(event->data);
            int statusOta = cJSON_GetObjectItem(root,"ota")->valueint;
            char * rendered = cJSON_Print(root);
            printf("%s\n ",rendered);
            free(rendered);
            cJSON_Delete(root);
            if(statusOta){
                // start cek ota update
                start_ota_task();
                delayTimeSend = 10000*100; //ms
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
    
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_URL,
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void start_log_mqtt(void){
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
}

void app_start(void){
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    start_log_mqtt();
    ESP_ERROR_CHECK( nvs_flash_init() );

    printf("HTTPS OTA, firmware %.1f\n\n", FIRMWARE_VERSION);

    initialise_wifi();
    wifi_wait_connected();
	printf("Connected to wifi network\n");
    
}

/////////////////////////////////////////////////////////////////////////

