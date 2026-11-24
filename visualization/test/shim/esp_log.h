// Host capture for the real log_sink.c; never part of the device build.
#pragma once
#include <stdio.h>
#define ESP_LOGI(tag, fmt, ...) printf("I (100) %s: " fmt "\n", tag, ##__VA_ARGS__)
