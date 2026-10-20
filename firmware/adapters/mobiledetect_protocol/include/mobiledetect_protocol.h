// PLACEHOLDER FORMAT. The real MobileDetect message format is unknown and must
// come from DetectaChem. See docs/open-questions.md.
// TODO(team): replace with the real MobileDetect message format.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "strip_types.h"

#define MDP_VERSION 1

// v1 layout, little-endian, 13 bytes:
//   [0]     version        u8   MDP_VERSION
//   [1]     mode           u8   strip_mode_t (0 = lateral flow, 1 = colorimetric)
//   [2]     verdict        u8   strip_verdict_t (0 = negative, 1 = positive, 2 = invalid)
//   [3..6]  score          f32  lateral flow: T line signal
//   [7..10] control_score  f32  lateral flow: C line signal; 0 for colorimetric
//   [11..12] sample_count  u16
// v0 (8 bytes) had no mode or control_score.
#define MDP_RESULT_LEN 13

// Returns bytes written, or 0 if `cap` is too small.
size_t mdp_encode_result(const strip_result_t *r, uint8_t *buf, size_t cap);
