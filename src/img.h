#ifndef IMG_H_
#define IMG_H_

#include <stdint.h>

int write_image_to_ttf(const uint8_t* data, int width, int height, float dpi, const char* filename);

#endif // IMG_H_
