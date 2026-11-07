// Host stand-in for ESP-IDF's esp_log.h: print to stderr.
#pragma once
#include <stdio.h>

#define ESP_LOGW(tag, fmt, ...) fprintf(stderr, "W %s: " fmt "\n", tag, ##__VA_ARGS__)
