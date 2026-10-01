// PLACEHOLDER FORMAT. The real MobileDetect message format is unknown and must
// come from DetectaChem. See docs/open-questions.md.
// TODO(team): replace with the real MobileDetect message format.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "strip_types.h"

#define MDP_VERSION 0

// v0 layout, little-endian: [version:u8][verdict:u8][score:float32][sample_count:u16]
#define MDP_RESULT_V0_LEN 8

// Returns bytes written, or 0 if `cap` is too small.
size_t mdp_encode_result(const strip_result_t *r, uint8_t *buf, size_t cap);
