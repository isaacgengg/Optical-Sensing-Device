#include "unity.h"
#include <math.h>
#include <string.h>
#include "strip_analysis.h"
#include "color_refs.h"
#include "../data/five_color_2026_11_23.h"

void setUp(void) {}
void tearDown(void) {}

static const strip_config_t CFG = {
    .mode = STRIP_MODE_COLORIMETRIC, .white = &COLOR_WHITE,
    .refs = COLOR_REFS, .ref_count = sizeof(COLOR_REFS) / sizeof(COLOR_REFS[0]),
    .max_distance = COLOR_MAX_DISTANCE, .min_color_margin = COLOR_MIN_MARGIN,
};

static const struct {
    const char *color;
    const char *match;
} EXPECTED_LABELS[] = {
    {"white", "control"},
    {"yellow", "psychedelics"},
    {"turquoise", "fentanyl"},
    {"orange", "cocaine"},
    {"deep_purple", "explosives"},
};

static void test_all_28_captured_scans_match_their_user_labels(void)
{
    const size_t count = sizeof(COLOR_BENCH_READINGS) / sizeof(COLOR_BENCH_READINGS[0]);
    TEST_ASSERT_EQUAL_size_t(28, count);
    for (size_t i = 0; i < count; i++) {
        channel_reading_t sample = {.count = 14};
        memcpy(sample.values, COLOR_BENCH_READINGS[i].raw, sizeof(COLOR_BENCH_READINGS[i].raw));
        strip_result_t r = strip_analyze(&sample, 1, &CFG);
        const char *expected = COLOR_BENCH_READINGS[i].color;
        TEST_ASSERT_EQUAL_STRING(expected, r.color);
        const char *expected_match = NULL;
        for (size_t j = 0; j < sizeof(EXPECTED_LABELS) / sizeof(EXPECTED_LABELS[0]); j++)
            if (strcmp(expected, EXPECTED_LABELS[j].color) == 0)
                expected_match = EXPECTED_LABELS[j].match;
        TEST_ASSERT_NOT_NULL(expected_match);
        TEST_ASSERT_EQUAL_STRING(expected_match, r.match);
        TEST_ASSERT_EQUAL(COLOR_MATCH_ACCEPTED, r.color_match.status);
        TEST_ASSERT_EQUAL(strcmp(expected, "white") == 0 ? STRIP_NEGATIVE : STRIP_POSITIVE, r.verdict);
        TEST_ASSERT_TRUE(r.score <= COLOR_MAX_DISTANCE);
        TEST_ASSERT_TRUE(r.color_match.margin >= COLOR_MIN_MARGIN);
    }
}

static void test_close_color_midpoints_are_ambiguous(void)
{
    const size_t pairs[][2] = {{1, 3}, {2, 4}}; // yellow/orange; turquoise/deep_purple
    for (size_t p = 0; p < sizeof(pairs) / sizeof(pairs[0]); p++) {
        const color_ref_t *a = &COLOR_REFS[pairs[p][0]], *b = &COLOR_REFS[pairs[p][1]];
        float am = 0, bm = 0;
        for (size_t c = 0; c < 14; c++) { am += a->reflectance[c]; bm += b->reflectance[c]; }
        am /= 14; bm /= 14;
        channel_reading_t sample = {.count = 14};
        for (size_t c = 0; c < 14; c++)
            sample.values[c] = COLOR_WHITE.values[c] * (a->reflectance[c] / am + b->reflectance[c] / bm) / 2;
        strip_result_t r = strip_analyze(&sample, 1, &CFG);
        TEST_ASSERT_TRUE(r.score < COLOR_MAX_DISTANCE);
        TEST_ASSERT_EQUAL(COLOR_MATCH_AMBIGUOUS, r.color_match.status);
        TEST_ASSERT_EQUAL(STRIP_INVALID, r.verdict);
        TEST_ASSERT_NULL(r.color);
        TEST_ASSERT_NULL(r.match);
    }
}

static void test_unmatched_and_dark_scans_remain_invalid(void)
{
    channel_reading_t sample = {.count = 14};
    TEST_ASSERT_EQUAL(STRIP_INVALID, strip_analyze(&sample, 1, &CFG).verdict);
    sample = COLOR_WHITE;
    sample.values[0] *= 10;
    strip_result_t r = strip_analyze(&sample, 1, &CFG);
    TEST_ASSERT_EQUAL(STRIP_INVALID, r.verdict);
    TEST_ASSERT_EQUAL(COLOR_MATCH_TOO_FAR, r.color_match.status);
    TEST_ASSERT_NULL(r.color);
}

static void test_profile_has_only_the_five_selected_colors(void)
{
    TEST_ASSERT_EQUAL_size_t(5, CFG.ref_count);
    for (size_t i = 0; i < CFG.ref_count; i++) {
        TEST_ASSERT_EQUAL_STRING(EXPECTED_LABELS[i].color, COLOR_REFS[i].color);
        TEST_ASSERT_EQUAL_STRING(EXPECTED_LABELS[i].match, COLOR_REFS[i].name);
        TEST_ASSERT_EQUAL_FLOAT(0.025f, COLOR_REFS[i].max_distance);
    }
    TEST_ASSERT_EQUAL_FLOAT(0.010f, CFG.min_color_margin);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_all_28_captured_scans_match_their_user_labels);
    RUN_TEST(test_close_color_midpoints_are_ambiguous);
    RUN_TEST(test_unmatched_and_dark_scans_remain_invalid);
    RUN_TEST(test_profile_has_only_the_five_selected_colors);
    return UNITY_END();
}
