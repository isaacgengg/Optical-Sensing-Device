// Mock optical sensor: replays a fixed table of readings. Lets the full
// pipeline run on a devkit (or on the host) without the real sensor.
#pragma once
#include <stddef.h>
#include "optical_sensor.h"

typedef struct {
    const channel_reading_t *table;
    size_t table_len;
    size_t next;
    optical_sensor_t sensor;
} mock_sensor_storage_t;
optical_sensor_t *mock_sensor_create_in(mock_sensor_storage_t *storage,
                                       const channel_reading_t *table, size_t table_len);

// Replays `table` in order, wrapping around at the end. Single instance.
optical_sensor_t *mock_sensor_create(const channel_reading_t *table, size_t table_len);

extern const channel_reading_t mock_default_table[];
extern const size_t mock_default_table_len;
