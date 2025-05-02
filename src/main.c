#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "img.h"
#include "iperlin.h"

int main(int argc, char** argv) {

    int octaves;
    char* endptr;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <int_octaves> <float_persistency> <base_bfreq> <base_bamp>\n", argv[0]);
        return EXIT_FAILURE;
    }

    errno = 0;

    unsigned long oct = strtoul(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr != '\0') {
        fprintf(stderr, "Invalid number format: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    octaves = (int)oct;

    double per = strtod(argv[2], &endptr);
    if (endptr == argv[2] || *endptr != '\0') {
        fprintf(stderr, "Invalid number format: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    double bfreq = strtod(argv[3], &endptr);
    if (endptr == argv[3] || *endptr != '\0') {
        fprintf(stderr, "Invalid number format: %s\n", argv[3]);
        return EXIT_FAILURE;
    }

    double bamp = strtod(argv[4], &endptr);
    if (endptr == argv[4] || *endptr != '\0') {
        fprintf(stderr, "Invalid number format: %s\n", argv[4]);
        return EXIT_FAILURE;
    }


    int width = 1024;
    int height = 1024;

    uint8_t* noise = (uint8_t*) malloc((size_t)(width * height));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t index = (size_t)(y*width + x);
            /*
            double n_v = (iperlin_at((double) x / 64.f, (double) y / 64.f, 0.0f) * 1.00 +
                iperlin_at((double) x / 32.f, (double) y / 32.f, 0.0f) * 0.5f +
                iperlin_at((double) x / 16.f, (double) y / 16.f, 0.0f) * 0.25f) / 1.75;
                */
            double n_v = octave_iperlin_at((double) x, (double) y, 0.0, octaves, per, bfreq, bamp);
            noise[index] = (uint8_t)((n_v * 0.5 + 0.5) * 255.0);
        }
    }

    if (write_image_to_ttf((const uint8_t*) noise, width, height, 96.0f, "example.tif") < 0) {
        free(noise);
        return EXIT_FAILURE;
    }

    free(noise);
    return EXIT_SUCCESS;
}
