// User-labeled red-marker paper captures, 2026-11-23. See data/README.md.
// All values are rounded averages from five hardware samples per zone.
// Batches11-15 fit the profile. Batch16 is a later stationary check.
// Placement repeatability is untested; this is not assay validation.
// Frozen fit SHA256: b72ddcd68d3ad06b0df33abed1a7a26198b42aef5a44b160f93ee6a3078acef5
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    unsigned batch;
    uint32_t uptime_ms;
    float test_raw[14];
    float control_raw[14];
    bool test_line, control_line;
    float test_contrast, control_contrast;
} lfa_bench_reading_t;

static const lfa_bench_reading_t LFA_BENCH_READINGS[] = {
    // test-only-11-tcs128-as64.json
    {
        .batch = 11, .uptime_ms = 131173,
        .test_raw = {25, 59, 307, 407, 349, 129, 502, 469, 469, 323, 48, 25, 632, 15089},
        .control_raw = {19, 49, 178, 440, 270, 144, 343, 367, 412, 228, 28, 19, 743, 22176},
        .test_line = true, .control_line = false,
        .test_contrast = 0.390379209f, .control_contrast = -0.077069527f,
    },
    {
        .batch = 11, .uptime_ms = 136413,
        .test_raw = {25, 59, 306, 407, 348, 129, 500, 468, 468, 322, 48, 25, 630, 15023},
        .control_raw = {19, 49, 177, 440, 269, 143, 342, 365, 411, 228, 28, 19, 741, 22104},
        .test_line = true, .control_line = false,
        .test_contrast = 0.391598780f, .control_contrast = -0.073281649f,
    },
    {
        .batch = 11, .uptime_ms = 140863,
        .test_raw = {25, 59, 306, 406, 347, 128, 499, 466, 466, 321, 47, 25, 628, 14962},
        .control_raw = {19, 48, 177, 439, 268, 143, 341, 364, 409, 227, 28, 19, 738, 22022},
        .test_line = true, .control_line = false,
        .test_contrast = 0.392199922f, .control_contrast = -0.072073433f,
    },
    {
        .batch = 11, .uptime_ms = 145103,
        .test_raw = {25, 59, 304, 405, 346, 128, 497, 464, 465, 320, 47, 25, 626, 14912},
        .control_raw = {19, 48, 177, 438, 267, 142, 340, 363, 408, 226, 28, 19, 736, 21967},
        .test_line = true, .control_line = false,
        .test_contrast = 0.394037923f, .control_contrast = -0.067077338f,
    },
    {
        .batch = 11, .uptime_ms = 149353,
        .test_raw = {25, 59, 304, 405, 345, 128, 496, 464, 464, 320, 47, 25, 625, 14882},
        .control_raw = {19, 48, 176, 437, 267, 142, 339, 362, 407, 226, 28, 18, 735, 21920},
        .test_line = true, .control_line = false,
        .test_contrast = 0.394639065f, .control_contrast = -0.065869122f,
    },
    // blank-white-12-tcs128-as64.json
    {
        .batch = 12, .uptime_ms = 309423,
        .test_raw = {29, 94, 542, 579, 559, 201, 698, 622, 537, 355, 51, 28, 851, 18174},
        .control_raw = {19, 42, 179, 416, 259, 133, 345, 351, 388, 226, 28, 20, 686, 24250},
        .test_line = false, .control_line = false,
        .test_contrast = -0.005746667f, .control_contrast = -0.006405680f,
    },
    {
        .batch = 12, .uptime_ms = 313433,
        .test_raw = {29, 94, 540, 579, 556, 200, 694, 618, 534, 354, 51, 28, 847, 18066},
        .control_raw = {19, 42, 179, 415, 257, 133, 344, 350, 386, 225, 28, 20, 683, 24131},
        .test_line = false, .control_line = false,
        .test_contrast = -0.002706382f, .control_contrast = -0.005197464f,
    },
    {
        .batch = 12, .uptime_ms = 317333,
        .test_raw = {29, 94, 539, 578, 554, 199, 692, 616, 533, 353, 51, 28, 844, 18000},
        .control_raw = {19, 42, 178, 414, 257, 132, 343, 349, 385, 225, 28, 20, 681, 24059},
        .test_line = false, .control_line = false,
        .test_contrast = -0.000885669f, .control_contrast = -0.000201369f,
    },
    {
        .batch = 12, .uptime_ms = 321153,
        .test_raw = {29, 94, 538, 578, 553, 199, 691, 615, 532, 352, 51, 28, 843, 17955},
        .control_raw = {19, 42, 178, 413, 256, 132, 342, 348, 384, 224, 28, 20, 679, 23996},
        .test_line = false, .control_line = false,
        .test_contrast = 0.000333903f, .control_contrast = 0.001006847f,
    },
    {
        .batch = 12, .uptime_ms = 325383,
        .test_raw = {29, 93, 538, 577, 553, 199, 691, 615, 531, 352, 51, 28, 842, 17933},
        .control_raw = {19, 42, 178, 413, 256, 131, 341, 348, 384, 224, 28, 19, 677, 23953},
        .test_line = false, .control_line = false,
        .test_contrast = 0.003892622f, .control_contrast = 0.004794725f,
    },
    {
        .batch = 12, .uptime_ms = 329183,
        .test_raw = {29, 93, 537, 577, 552, 199, 689, 613, 530, 351, 51, 28, 841, 17888},
        .control_raw = {19, 42, 177, 412, 255, 131, 341, 347, 383, 223, 28, 19, 675, 23893},
        .test_line = false, .control_line = false,
        .test_contrast = 0.005112193f, .control_contrast = 0.006002941f,
    },
    // control-only-13-tcs128-as64.json
    {
        .batch = 13, .uptime_ms = 404803,
        .test_raw = {28, 88, 506, 550, 528, 191, 667, 593, 513, 343, 50, 28, 801, 17560},
        .control_raw = {19, 45, 177, 405, 262, 128, 341, 358, 415, 227, 28, 21, 674, 22815},
        .test_line = false, .control_line = true,
        .test_contrast = 0.056504504f, .control_contrast = 0.025824089f,
    },
    {
        .batch = 13, .uptime_ms = 414183,
        .test_raw = {28, 88, 505, 549, 528, 190, 666, 592, 512, 342, 50, 27, 800, 17517},
        .control_raw = {19, 45, 176, 405, 261, 127, 341, 357, 414, 226, 28, 21, 673, 22768},
        .test_line = false, .control_line = true,
        .test_contrast = 0.057122934f, .control_contrast = 0.029611967f,
    },
    {
        .batch = 13, .uptime_ms = 420923,
        .test_raw = {28, 88, 504, 549, 526, 190, 665, 590, 511, 342, 50, 27, 798, 17470},
        .control_raw = {19, 45, 176, 404, 261, 127, 340, 356, 413, 226, 28, 21, 671, 22714},
        .test_line = false, .control_line = true,
        .test_contrast = 0.058943647f, .control_contrast = 0.030820183f,
    },
    {
        .batch = 13, .uptime_ms = 426073,
        .test_raw = {28, 88, 503, 548, 525, 189, 663, 589, 510, 341, 50, 27, 797, 17420},
        .control_raw = {19, 45, 176, 404, 260, 127, 339, 355, 412, 225, 28, 21, 670, 22654},
        .test_line = false, .control_line = true,
        .test_contrast = 0.060163218f, .control_contrast = 0.030820183f,
    },
    {
        .batch = 13, .uptime_ms = 430493,
        .test_raw = {28, 87, 503, 548, 524, 189, 662, 588, 509, 341, 50, 27, 795, 17378},
        .control_raw = {19, 45, 176, 403, 259, 127, 339, 354, 411, 225, 28, 21, 668, 22595},
        .test_line = false, .control_line = true,
        .test_contrast = 0.064323079f, .control_contrast = 0.032028399f,
    },
    {
        .batch = 13, .uptime_ms = 435343,
        .test_raw = {28, 88, 503, 548, 524, 188, 662, 588, 509, 340, 50, 27, 795, 17366},
        .control_raw = {19, 45, 175, 403, 259, 127, 338, 353, 411, 225, 28, 21, 668, 22569},
        .test_line = false, .control_line = true,
        .test_contrast = 0.060764361f, .control_contrast = 0.032028399f,
    },
    {
        .batch = 13, .uptime_ms = 440073,
        .test_raw = {28, 87, 502, 548, 523, 188, 661, 587, 508, 340, 50, 27, 794, 17337},
        .control_raw = {19, 45, 175, 403, 258, 126, 338, 353, 410, 224, 28, 21, 667, 22529},
        .test_line = false, .control_line = true,
        .test_contrast = 0.065542651f, .control_contrast = 0.035816278f,
    },
    {
        .batch = 13, .uptime_ms = 443493,
        .test_raw = {28, 88, 501, 547, 521, 188, 659, 585, 507, 339, 50, 27, 792, 17264},
        .control_raw = {18, 45, 175, 402, 258, 126, 336, 351, 408, 224, 28, 20, 664, 22449},
        .test_line = false, .control_line = true,
        .test_contrast = 0.063804645f, .control_contrast = 0.037024494f,
    },
    {
        .batch = 13, .uptime_ms = 447913,
        .test_raw = {28, 87, 501, 547, 522, 188, 659, 585, 507, 339, 50, 27, 792, 17268},
        .control_raw = {18, 45, 175, 402, 258, 126, 337, 352, 409, 224, 28, 20, 665, 22487},
        .test_line = false, .control_line = true,
        .test_contrast = 0.066762222f, .control_contrast = 0.037024494f,
    },
    // two-red-lines-14-tcs128-as64.json
    {
        .batch = 14, .uptime_ms = 585303,
        .test_raw = {25, 59, 306, 405, 348, 129, 499, 465, 467, 321, 47, 25, 629, 14947},
        .control_raw = {19, 44, 176, 369, 258, 114, 340, 356, 402, 228, 28, 17, 618, 18284},
        .test_line = true, .control_line = true,
        .test_contrast = 0.391598780f, .control_contrast = 0.122350163f,
    },
    {
        .batch = 14, .uptime_ms = 589273,
        .test_raw = {25, 59, 305, 404, 346, 128, 496, 463, 465, 320, 47, 25, 626, 14854},
        .control_raw = {19, 44, 175, 368, 256, 113, 338, 355, 401, 226, 28, 17, 615, 18191},
        .test_line = true, .control_line = true,
        .test_contrast = 0.393419493f, .control_contrast = 0.127346258f,
    },
    {
        .batch = 14, .uptime_ms = 592973,
        .test_raw = {25, 58, 304, 403, 345, 128, 495, 462, 464, 319, 47, 24, 625, 14803},
        .control_raw = {19, 44, 175, 368, 256, 113, 337, 353, 399, 226, 28, 17, 613, 18130},
        .test_line = true, .control_line = true,
        .test_contrast = 0.398197784f, .control_contrast = 0.127346258f,
    },
    {
        .batch = 14, .uptime_ms = 596623,
        .test_raw = {25, 58, 303, 403, 344, 127, 494, 460, 463, 319, 47, 24, 623, 14765},
        .control_raw = {18, 44, 174, 367, 255, 113, 337, 352, 398, 225, 28, 17, 612, 18085},
        .test_line = true, .control_line = true,
        .test_contrast = 0.399417355f, .control_contrast = 0.128554473f,
    },
    {
        .batch = 14, .uptime_ms = 600353,
        .test_raw = {25, 58, 303, 402, 344, 127, 493, 460, 463, 318, 47, 24, 623, 14734},
        .control_raw = {18, 44, 174, 367, 255, 112, 336, 352, 398, 225, 28, 17, 611, 18048},
        .test_line = true, .control_line = true,
        .test_contrast = 0.399417355f, .control_contrast = 0.132342352f,
    },
    {
        .batch = 14, .uptime_ms = 606293,
        .test_raw = {25, 58, 303, 402, 345, 127, 495, 460, 463, 319, 47, 24, 623, 14763},
        .control_raw = {18, 44, 174, 366, 255, 112, 337, 352, 398, 225, 28, 17, 611, 18065},
        .test_line = true, .control_line = true,
        .test_contrast = 0.398816213f, .control_contrast = 0.133550568f,
    },
    // blank-white-15-tcs128-as64.json
    {
        .batch = 15, .uptime_ms = 852493,
        .test_raw = {29, 93, 537, 575, 555, 200, 693, 616, 532, 353, 51, 28, 845, 18039},
        .control_raw = {19, 42, 179, 416, 259, 133, 345, 351, 386, 226, 28, 19, 687, 23725},
        .test_line = false, .control_line = false,
        .test_contrast = 0.003308767f, .control_contrast = -0.006405680f,
    },
    {
        .batch = 15, .uptime_ms = 856783,
        .test_raw = {29, 93, 536, 574, 552, 198, 689, 613, 530, 351, 51, 28, 842, 17942},
        .control_raw = {19, 42, 179, 415, 258, 132, 344, 350, 385, 225, 28, 19, 684, 23613},
        .test_line = false, .control_line = false,
        .test_contrast = 0.005730622f, .control_contrast = -0.001409585f,
    },
    {
        .batch = 15, .uptime_ms = 861733,
        .test_raw = {29, 93, 535, 574, 552, 198, 689, 612, 529, 351, 51, 28, 841, 17908},
        .control_raw = {19, 42, 178, 414, 257, 132, 343, 350, 384, 225, 28, 19, 683, 23573},
        .test_line = false, .control_line = false,
        .test_contrast = 0.006349051f, .control_contrast = -0.000201369f,
    },
    {
        .batch = 15, .uptime_ms = 867843,
        .test_raw = {29, 93, 535, 574, 552, 198, 688, 612, 529, 351, 51, 28, 841, 17900},
        .control_raw = {19, 42, 178, 414, 257, 132, 343, 349, 383, 224, 28, 19, 681, 23532},
        .test_line = false, .control_line = false,
        .test_contrast = 0.006349051f, .control_contrast = -0.000201369f,
    },
    {
        .batch = 15, .uptime_ms = 871113,
        .test_raw = {29, 93, 533, 573, 549, 197, 685, 609, 526, 349, 51, 28, 837, 17786},
        .control_raw = {19, 42, 178, 413, 256, 131, 341, 347, 382, 223, 28, 19, 678, 23411},
        .test_line = false, .control_line = false,
        .test_contrast = 0.009389336f, .control_contrast = 0.004794725f,
    },
    {
        .batch = 15, .uptime_ms = 876643,
        .test_raw = {29, 93, 534, 573, 550, 197, 687, 610, 527, 350, 51, 28, 838, 17824},
        .control_raw = {19, 42, 178, 412, 256, 131, 342, 348, 382, 224, 28, 19, 678, 23431},
        .test_line = false, .control_line = false,
        .test_contrast = 0.008169765f, .control_contrast = 0.006002941f,
    },
    {
        .batch = 15, .uptime_ms = 881263,
        .test_raw = {29, 93, 533, 573, 549, 197, 685, 609, 526, 349, 51, 28, 837, 17784},
        .control_raw = {19, 42, 177, 411, 255, 130, 341, 347, 381, 223, 28, 19, 677, 23378},
        .test_line = false, .control_line = false,
        .test_contrast = 0.009389336f, .control_contrast = 0.010999036f,
    },
    {
        .batch = 15, .uptime_ms = 890023,
        .test_raw = {29, 93, 534, 572, 551, 197, 687, 610, 527, 349, 51, 28, 839, 17821},
        .control_raw = {19, 42, 178, 411, 256, 130, 341, 347, 381, 223, 28, 19, 677, 23401},
        .test_line = false, .control_line = false,
        .test_contrast = 0.007568623f, .control_contrast = 0.010999036f,
    },
    // control-only-16-stationary-tcs128-as64.json
    {
        .batch = 16, .uptime_ms = 1063273,
        .test_raw = {29, 97, 561, 590, 568, 203, 705, 626, 542, 357, 52, 28, 871, 18161},
        .control_raw = {19, 41, 179, 403, 256, 127, 344, 349, 377, 226, 28, 20, 660, 23768},
        .test_line = false, .control_line = true,
        .test_contrast = -0.033583258f, .control_contrast = 0.032028399f,
    },
    {
        .batch = 16, .uptime_ms = 1067463,
        .test_raw = {29, 97, 560, 590, 565, 202, 701, 623, 539, 356, 51, 28, 867, 18064},
        .control_raw = {19, 41, 179, 402, 255, 127, 343, 348, 375, 225, 28, 19, 657, 23655},
        .test_line = false, .control_line = true,
        .test_contrast = -0.031161402f, .control_contrast = 0.033236615f,
    },
    {
        .batch = 16, .uptime_ms = 1071243,
        .test_raw = {29, 97, 559, 590, 564, 201, 699, 621, 538, 355, 51, 28, 865, 17993},
        .control_raw = {19, 41, 178, 401, 254, 126, 342, 346, 374, 224, 28, 19, 655, 23564},
        .test_line = false, .control_line = true,
        .test_contrast = -0.029941831f, .control_contrast = 0.038232710f,
    },
    {
        .batch = 16, .uptime_ms = 1075073,
        .test_raw = {29, 97, 558, 589, 563, 201, 698, 620, 537, 354, 51, 28, 864, 17955},
        .control_raw = {19, 41, 178, 400, 254, 126, 341, 346, 373, 224, 28, 19, 653, 23505},
        .test_line = false, .control_line = true,
        .test_contrast = -0.028722259f, .control_contrast = 0.039440926f,
    },
    {
        .batch = 16, .uptime_ms = 1080773,
        .test_raw = {29, 97, 559, 589, 564, 201, 699, 621, 537, 355, 51, 28, 865, 17977},
        .control_raw = {19, 41, 178, 399, 254, 126, 341, 346, 373, 224, 28, 19, 653, 23502},
        .test_line = false, .control_line = true,
        .test_contrast = -0.029941831f, .control_contrast = 0.040649141f,
    },
    {
        .batch = 16, .uptime_ms = 1084053,
        .test_raw = {29, 97, 557, 588, 561, 200, 695, 618, 535, 353, 51, 28, 862, 17876},
        .control_raw = {19, 41, 177, 398, 253, 125, 340, 344, 371, 223, 28, 19, 650, 23385},
        .test_line = false, .control_line = true,
        .test_contrast = -0.026901546f, .control_contrast = 0.045645236f,
    },
};
