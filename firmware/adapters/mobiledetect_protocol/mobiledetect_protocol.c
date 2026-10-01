// PLACEHOLDER FORMAT. The real MobileDetect message format is unknown and must
// come from DetectaChem. See docs/open-questions.md.

#include <string.h>
#include "mobiledetect_protocol.h"

size_t mdp_encode_result(const strip_result_t *r, uint8_t *buf, size_t cap)
{
    if (r == NULL || buf == NULL || cap < MDP_RESULT_V0_LEN) {
        return 0;
    }

    uint32_t score_bits;
    memcpy(&score_bits, &r->score, sizeof(score_bits));
    uint16_t samples = r->sample_count > UINT16_MAX ? UINT16_MAX : (uint16_t)r->sample_count;

    buf[0] = MDP_VERSION;
    buf[1] = (uint8_t)r->verdict;
    buf[2] = (uint8_t)(score_bits);
    buf[3] = (uint8_t)(score_bits >> 8);
    buf[4] = (uint8_t)(score_bits >> 16);
    buf[5] = (uint8_t)(score_bits >> 24);
    buf[6] = (uint8_t)(samples);
    buf[7] = (uint8_t)(samples >> 8);
    return MDP_RESULT_V0_LEN;
}
