// Default mock readings, 8 channels.
// These values are MADE UP. They only need to look roughly like counts from a
// spectral sensor so the pipeline has something to chew on.
// TODO(team): replace with recorded readings once real data exists
// (see firmware/test/host/data/README.md).
//
// Laid out for lateral flow with sample_count = 5: each measurement reads 5 T
// rows, then 5 C rows, so repeated 'l' presses cycle POSITIVE, NEGATIVE,
// INVALID with the thresholds in main.c (T >= 1200, C >= 100). A colorimetric
// measurement reads only 5 rows, so it shifts the cycle.
// ponytail: higher = line present matches the placeholder metric, not physics
// (a real line darkens the strip).

#include "mock_sensor.h"

const channel_reading_t mock_default_table[] = {
    // 1. POSITIVE: T line strong, C line present
    // T
    {.values = {934, 1189, 1633, 1941, 1727, 1393, 1097, 736}, .count = 8},
    {.values = {929, 1197, 1627, 1947, 1722, 1402, 1091, 742}, .count = 8},
    {.values = {937, 1183, 1642, 1932, 1736, 1387, 1102, 733}, .count = 8},
    {.values = {931, 1193, 1632, 1943, 1731, 1396, 1094, 738}, .count = 8},
    {.values = {927, 1187, 1636, 1938, 1724, 1390, 1099, 735}, .count = 8},
    // C
    {.values = {812, 1034, 1420, 1688, 1502, 1211, 954, 640}, .count = 8},
    {.values = {808, 1041, 1415, 1693, 1497, 1219, 949, 645}, .count = 8},
    {.values = {815, 1029, 1428, 1680, 1510, 1206, 958, 637}, .count = 8},
    {.values = {810, 1037, 1419, 1690, 1505, 1214, 951, 642}, .count = 8},
    {.values = {806, 1032, 1423, 1685, 1499, 1209, 956, 639}, .count = 8},
    // 2. NEGATIVE: T line weak, C line present
    // T
    {.values = {812, 1034, 1420, 1688, 1502, 1211, 954, 640}, .count = 8},
    {.values = {808, 1041, 1415, 1693, 1497, 1219, 949, 645}, .count = 8},
    {.values = {815, 1029, 1428, 1680, 1510, 1206, 958, 637}, .count = 8},
    {.values = {810, 1037, 1419, 1690, 1505, 1214, 951, 642}, .count = 8},
    {.values = {806, 1032, 1423, 1685, 1499, 1209, 956, 639}, .count = 8},
    // C
    {.values = {812, 1034, 1420, 1688, 1502, 1211, 954, 640}, .count = 8},
    {.values = {808, 1041, 1415, 1693, 1497, 1219, 949, 645}, .count = 8},
    {.values = {815, 1029, 1428, 1680, 1510, 1206, 958, 637}, .count = 8},
    {.values = {810, 1037, 1419, 1690, 1505, 1214, 951, 642}, .count = 8},
    {.values = {806, 1032, 1423, 1685, 1499, 1209, 956, 639}, .count = 8},
    // 3. INVALID: C line missing
    // T
    {.values = {812, 1034, 1420, 1688, 1502, 1211, 954, 640}, .count = 8},
    {.values = {808, 1041, 1415, 1693, 1497, 1219, 949, 645}, .count = 8},
    {.values = {815, 1029, 1428, 1680, 1510, 1206, 958, 637}, .count = 8},
    {.values = {810, 1037, 1419, 1690, 1505, 1214, 951, 642}, .count = 8},
    {.values = {806, 1032, 1423, 1685, 1499, 1209, 956, 639}, .count = 8},
    // C
    {.values = {41, 52, 71, 84, 75, 61, 48, 32}, .count = 8},
    {.values = {40, 52, 71, 85, 75, 61, 47, 32}, .count = 8},
    {.values = {41, 51, 71, 84, 76, 60, 48, 32}, .count = 8},
    {.values = {40, 52, 71, 84, 75, 61, 48, 32}, .count = 8},
    {.values = {40, 52, 71, 84, 75, 60, 48, 32}, .count = 8},
};

const size_t mock_default_table_len = sizeof(mock_default_table) / sizeof(mock_default_table[0]);
