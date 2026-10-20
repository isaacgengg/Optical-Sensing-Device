// PLACEHOLDER FORMAT. The real MobileDetect message format is unknown and must
// come from DetectaChem. See docs/open-questions.md.

#include <string.h>
#include "mobiledetect_protocol.h"

// Little-endian float32, independent of host byte order.
static void put_f32(uint8_t *p, float v)
{
    uint32_t bits;
    memcpy(&bits, &v, sizeof(bits));
    p[0] = (uint8_t)(bits);
    p[1] = (uint8_t)(bits >> 8);
    p[2] = (uint8_t)(bits >> 16);
    p[3] = (uint8_t)(bits >> 24);
}

size_t mdp_encode_result(const strip_result_t *r, uint8_t *buf, size_t cap)
{
    if (r == NULL || buf == NULL || cap < MDP_RESULT_LEN) {
        return 0;
    }

    uint16_t samples = r->sample_count > UINT16_MAX ? UINT16_MAX : (uint16_t)r->sample_count;

    buf[0] = MDP_VERSION;
    buf[1] = (uint8_t)r->mode;
    buf[2] = (uint8_t)r->verdict;
    put_f32(&buf[3], r->score);
    put_f32(&buf[7], r->control_score);
    buf[11] = (uint8_t)(samples);
    buf[12] = (uint8_t)(samples >> 8);
    return MDP_RESULT_LEN;
}
