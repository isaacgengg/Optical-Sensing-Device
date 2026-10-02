// Command source that reads keys from the serial console (idf.py monitor).
// Stand-in for the device keypad: 'l' = lateral flow, 'c' = colorimetric.
#pragma once
#include "command_source.h"

command_source_t *console_input_create(void);
