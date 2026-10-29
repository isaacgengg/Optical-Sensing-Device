// Default mock readings, 14 channels in the TCS3448 driver's order
// (see adapters/tcs3448/include/tcs3448.h):
//   F1 F2 FZ F3 F4 F5 FY FXL F6 F7 F8 NIR VIS FD
// These values are MADE UP, shaped like a white LED reflected off a white
// membrane (blue LED peak at FZ, broad yellow hump, little NIR).
// TODO(team): replace with recorded readings once real data exists
// (see firmware/test/host/data/README.md).
//
// Laid out for lateral flow with sample_count = 5: each measurement reads 5 T
// rows, then 5 C rows, so repeated 'l' presses cycle POSITIVE, NEGATIVE,
// INVALID with the thresholds in main.c (T >= 1200, C >= 100; channel means
// here are ~1300 strong, ~929 baseline, ~46 missing). A colorimetric
// measurement reads only 5 rows, so it shifts the cycle.
// ponytail: higher = line present matches the placeholder metric, not physics
// (a real line darkens the strip).

#include "mock_sensor.h"

const channel_reading_t mock_default_table[] = {
    // 1. POSITIVE: T line strong, C line present
    // T
    {.values = {251, 590, 2312, 1543, 1063, 1466, 1926, 1982, 1376, 731, 294, 126, 4324, 196}, .count = 14},
    {.values = {251, 589, 2309, 1547, 1062, 1475, 1923, 1980, 1371, 728, 293, 126, 4359, 196}, .count = 14},
    {.values = {252, 587, 2317, 1542, 1063, 1477, 1922, 1979, 1366, 730, 293, 126, 4326, 195}, .count = 14},
    {.values = {253, 586, 2311, 1544, 1066, 1476, 1932, 1982, 1373, 728, 295, 126, 4357, 197}, .count = 14},
    {.values = {252, 590, 2313, 1545, 1061, 1468, 1923, 1985, 1374, 725, 295, 126, 4332, 196}, .count = 14},
    // C
    {.values = {179, 422, 1649, 1103, 763, 1046, 1380, 1416, 977, 517, 211, 90, 3113, 140}, .count = 14},
    {.values = {180, 419, 1642, 1100, 759, 1046, 1379, 1414, 978, 519, 211, 90, 3097, 140}, .count = 14},
    {.values = {179, 422, 1649, 1104, 761, 1055, 1380, 1418, 978, 521, 210, 90, 3100, 140}, .count = 14},
    {.values = {179, 422, 1652, 1099, 759, 1048, 1376, 1414, 982, 518, 210, 90, 3095, 141}, .count = 14},
    {.values = {179, 421, 1657, 1096, 758, 1047, 1373, 1418, 983, 521, 210, 90, 3112, 140}, .count = 14},
    // 2. NEGATIVE: T line weak, C line present
    // T
    {.values = {179, 422, 1649, 1103, 763, 1046, 1380, 1416, 977, 517, 211, 90, 3113, 140}, .count = 14},
    {.values = {180, 419, 1642, 1100, 759, 1046, 1379, 1414, 978, 519, 211, 90, 3097, 140}, .count = 14},
    {.values = {179, 422, 1649, 1104, 761, 1055, 1380, 1418, 978, 521, 210, 90, 3100, 140}, .count = 14},
    {.values = {179, 422, 1652, 1099, 759, 1048, 1376, 1414, 982, 518, 210, 90, 3095, 141}, .count = 14},
    {.values = {179, 421, 1657, 1096, 758, 1047, 1373, 1418, 983, 521, 210, 90, 3112, 140}, .count = 14},
    // C
    {.values = {179, 422, 1649, 1103, 763, 1046, 1380, 1416, 977, 517, 211, 90, 3113, 140}, .count = 14},
    {.values = {180, 419, 1642, 1100, 759, 1046, 1379, 1414, 978, 519, 211, 90, 3097, 140}, .count = 14},
    {.values = {179, 422, 1649, 1104, 761, 1055, 1380, 1418, 978, 521, 210, 90, 3100, 140}, .count = 14},
    {.values = {179, 422, 1652, 1099, 759, 1048, 1376, 1414, 982, 518, 210, 90, 3095, 141}, .count = 14},
    {.values = {179, 421, 1657, 1096, 758, 1047, 1373, 1418, 983, 521, 210, 90, 3112, 140}, .count = 14},
    // 3. INVALID: C line missing
    // T
    {.values = {179, 422, 1649, 1103, 763, 1046, 1380, 1416, 977, 517, 211, 90, 3113, 140}, .count = 14},
    {.values = {180, 419, 1642, 1100, 759, 1046, 1379, 1414, 978, 519, 211, 90, 3097, 140}, .count = 14},
    {.values = {179, 422, 1649, 1104, 761, 1055, 1380, 1418, 978, 521, 210, 90, 3100, 140}, .count = 14},
    {.values = {179, 422, 1652, 1099, 759, 1048, 1376, 1414, 982, 518, 210, 90, 3095, 141}, .count = 14},
    {.values = {179, 421, 1657, 1096, 758, 1047, 1373, 1418, 983, 521, 210, 90, 3112, 140}, .count = 14},
    // C
    {.values = {9, 21, 83, 55, 38, 53, 69, 71, 49, 26, 11, 4, 155, 7}, .count = 14},
    {.values = {9, 21, 83, 55, 38, 52, 69, 71, 49, 26, 11, 5, 155, 7}, .count = 14},
    {.values = {9, 21, 82, 55, 38, 53, 69, 71, 49, 26, 11, 4, 156, 7}, .count = 14},
    {.values = {9, 21, 83, 55, 38, 53, 69, 71, 49, 26, 11, 5, 155, 7}, .count = 14},
    {.values = {9, 21, 82, 55, 38, 53, 69, 71, 49, 26, 10, 5, 155, 7}, .count = 14},
};

const size_t mock_default_table_len = sizeof(mock_default_table) / sizeof(mock_default_table[0]);
