#ifndef _ESP_CAM_H_
#define _ESP_CAM_H_

#include <esp_system.h>

void take_picture();
esp_err_t init_camera();
void init_sdcard();

#endif