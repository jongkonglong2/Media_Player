#ifndef IMAGE_FILTER_H
#define IMAGE_FILTER_H
#include <stdint.h>
/* Convert one RGB565 scanline to a black/white Sobel edge scanline.
 * prev/current/next each contain width pixels; output contains width pixels.
 * Caller supplies zero-filled boundary rows for the top and bottom edges.
 */
void Image_EdgeRowRGB565(const uint16_t *prev,
                         const uint16_t *current,
                         const uint16_t *next,
                         uint16_t *output,
                         uint16_t width,
                         uint8_t threshold);
#endif
