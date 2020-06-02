
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#include "driver/gpio.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>


#include "src/utils/includes.h"
#include "os.h"
#include "src/utils/base64.h"
 
#include <stdio.h>
#include <unistd.h>

#include "esp_camera.h"

#include "define.h"
#include "uart_task.h"
#include "wifi_ota.h"
#include "mqtt_task.h"
#include "esp_cam.h"
// #include "aws_mqtt_task.h"

static const char *TAG = "esp-cam";

void app_main()
{
  app_start();
  printf("\nSEBELUM v %f\n",FIRMWARE_VERSION);
  init_sdcard();
  init_camera();
  mqtt_app_start(); // connect server 
  uart_init();
}
