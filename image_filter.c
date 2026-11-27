#include "image_filter.h"

static uint8_t gray565(uint16_t pixel) {
    uint16_t r = (pixel >> 11) & 0x1FU;
    uint16_t g = (pixel >> 5) & 0x3FU;
    uint16_t b = pixel & 0x1FU;
    /* Weighted luminance approximation, scaled to 8 bits. */
    return (uint8_t)((r * 2U + g * 4U + b * 2U) >> 1);
}

void Image_EdgeRowRGB565(const uint16_t *prev,
                         const uint16_t *current,
                         const uint16_t *next,
                         uint16_t *output,
                         uint16_t width,
                         uint8_t threshold) {
    uint16_t x;
    if (width == 0U) return;
    output[0] = 0x0000U;
    if (width > 1U) output[width - 1U] = 0x0000U;

    for (x = 1U; x + 1U < width; x++) {
        int p00 = gray565(prev[x - 1U]);
        int p01 = gray565(prev[x]);
        int p02 = gray565(prev[x + 1U]);
        int p10 = gray565(current[x - 1U]);
        int p12 = gray565(current[x + 1U]);
        int p20 = gray565(next[x - 1U]);
        int p21 = gray565(next[x]);
        int p22 = gray565(next[x + 1U]);
        int gx = -p00 + p02 - 2 * p10 + 2 * p12 - p20 + p22;
        int gy = -p00 - 2 * p01 - p02 + p20 + 2 * p21 + p22;
        int magnitude = (gx < 0 ? -gx : gx) + (gy < 0 ? -gy : gy);
        output[x] = (magnitude > (int)threshold * 4) ? 0xFFFFU : 0x0000U;
    }
}
