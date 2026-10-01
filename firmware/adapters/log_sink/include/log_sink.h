// Result sink that writes results to the serial log. For bring-up without a phone.
#pragma once
#include "result_sink.h"

result_sink_t *log_sink_create(void);
