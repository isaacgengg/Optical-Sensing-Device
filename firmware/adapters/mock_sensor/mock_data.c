// Default mock readings, 8 channels.
// These values are MADE UP. They only need to look roughly like counts from a
// spectral sensor so the pipeline has something to chew on.
// TODO(team): replace with recorded readings once real data exists
// (see firmware/test/host/data/README.md).

#include "mock_sensor.h"

const channel_reading_t mock_default_table[] = {
    {.values = {812, 1034, 1420, 1688, 1502, 1211, 954, 640}, .count = 8},
    {.values = {808, 1041, 1415, 1693, 1497, 1219, 949, 645}, .count = 8},
    {.values = {815, 1029, 1428, 1680, 1510, 1206, 958, 637}, .count = 8},
    {.values = {810, 1037, 1419, 1690, 1505, 1214, 951, 642}, .count = 8},
    {.values = {806, 1032, 1423, 1685, 1499, 1209, 956, 639}, .count = 8},
};

const size_t mock_default_table_len = sizeof(mock_default_table) / sizeof(mock_default_table[0]);
