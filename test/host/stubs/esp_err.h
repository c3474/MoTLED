// Stand-in for ESP-IDF's esp_err.h: just enough for app_nvs_config.h to parse.
#pragma once

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
