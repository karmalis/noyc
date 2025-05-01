#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define TIFF_BYTE_ORDER_LL 0x4949;
#define TIFF_VERSION 42;

#define TIFF_TAG_IMAGE_WIDTH 256;
#define TIFF_TAG_IMAGE_LENGTH 257;
#define TIFF_TAG_BITS_PER_SAMPLE 258;
#define TIFF_TAG_COMPRESSION 259;
#define TIFF_TAG_PHOTOMERIC_INTERPRETATION 262;
#define TIFF_TAG_STRIP_OFFSETS 273;
#define TIFF_TAG_SAMPLES_PER_PIXEL 277;
#define TIFF_TAG_ROWS_PER_STRIP 278;
#define TIFF_TAG_STRIP_BYTE_COUNTS 279;
#define TIFF_TAG_X_RESOLUTION 282;
#define TIFF_TAG_Y_RESOLUTION 283;
#define TIFF_TAG_RESOLUTION_UNIT 296;
#define TIFF_TAG_SAMPLE_FORMAT 339;


#define TIFF_TYPE_BYTE 1;
#define TIFF_TYPE_ASCII 2;
#define TIFF_TYPE_SHORT 3;
#define TIFF_TYPE_LONG 4;
#define TIFF_TYPE_RATIONAL 5;

typedef struct {
    uint16_t tag;
    uint16_t type;
    uint32_t count;
    uint32_t value_offset;
} TiffIFDEntry;

int write_image_to_ttf(const uint8_t* data, int width, int height, float dpi, const char* filename) {
    FILE* fhandle = fopen(filename, "wb");
    if (!fhandle) {
        perror("Could not open file for writing");
        return -1;
    }

    uint16_t byte_order = TIFF_BYTE_ORDER_LL;
    fwrite(&byte_order, 2, 1, fhandle);

    uint16_t tiff_version = TIFF_VERSION;
    fwrite(&tiff_version, 2, 1, fhandle);

    uint32_t tiff_data_offset = 8; // Right after the header
    fwrite(&tiff_data_offset, 4, 1, fhandle);

    size_t ifd_start_offset = 8;
    uint16_t number_of_entries = 13;
    //size_t ifd_size = number_of_entries * sizeof(TiffIFDEntry);
    size_t ifd_size = 2 + number_of_entries * sizeof(TiffIFDEntry) + 4;

    size_t rational_data_offset = ifd_start_offset + ifd_size;
    if (rational_data_offset % 4 != 0) {
        rational_data_offset += (4 - (rational_data_offset % 4));
    }
    size_t x_res_offset = rational_data_offset;
    size_t y_res_offset = rational_data_offset + 8;
    size_t image_data_offset = y_res_offset + 8;
    size_t bits_per_sample = 8; // Floats size
    size_t bytes_per_sample = bits_per_sample / 8;

    size_t image_data_size = (size_t) (width * height * bytes_per_sample);

    fseek(fhandle, ifd_start_offset, SEEK_SET);

    // Start to write IDF entries
    fwrite(&number_of_entries, 2, 1, fhandle);

    TiffIFDEntry entry;
    entry.count = 1;

    entry.tag = TIFF_TAG_IMAGE_WIDTH;
    entry.type = TIFF_TYPE_LONG;
    entry.value_offset = (uint32_t) width;
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_IMAGE_LENGTH;
    entry.type = TIFF_TYPE_LONG;
    entry.value_offset = (uint32_t) height;
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_BITS_PER_SAMPLE;
    entry.type = TIFF_TYPE_SHORT;
    entry.value_offset = (uint32_t) bits_per_sample;
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_COMPRESSION;
    entry.type = TIFF_TYPE_SHORT;
    entry.value_offset = 1; // No compression
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_PHOTOMERIC_INTERPRETATION;
    entry.type = TIFF_TYPE_SHORT;
    entry.value_offset = 1; // value 1 means 0.0 is black
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);


    entry.tag = TIFF_TAG_STRIP_OFFSETS;
    entry.type = TIFF_TYPE_LONG;
    entry.value_offset = (uint32_t) image_data_offset;
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_SAMPLES_PER_PIXEL;
    entry.type = TIFF_TYPE_SHORT;
    entry.value_offset = 1;
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_ROWS_PER_STRIP;
    entry.type = TIFF_TYPE_LONG;
    entry.value_offset = (uint32_t) height;
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_STRIP_BYTE_COUNTS;
    entry.type = TIFF_TYPE_LONG;
    entry.value_offset = (uint32_t) image_data_size;
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);


    entry.tag = TIFF_TAG_X_RESOLUTION;
    entry.type = TIFF_TYPE_RATIONAL;
    entry.value_offset = (uint32_t) x_res_offset;
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_Y_RESOLUTION;
    entry.type = TIFF_TYPE_RATIONAL;
    entry.value_offset = (uint32_t) y_res_offset;
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_RESOLUTION_UNIT;
    entry.type = TIFF_TYPE_SHORT;
    entry.value_offset = 1; // No absolute inch / cm unit of measurement
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    entry.tag = TIFF_TAG_SAMPLE_FORMAT;
    entry.type = TIFF_TYPE_SHORT;
    entry.value_offset = 1; // unsigned int
    fwrite(&entry, sizeof(TiffIFDEntry), 1, fhandle);

    uint32_t next_ifd = 0;
    fwrite(&next_ifd, 4, 1, fhandle);

    uint32_t numerator = (uint32_t) dpi;
    uint32_t denominator = 1;

    fseek(fhandle, x_res_offset, SEEK_SET);
    fwrite(&numerator, 4, 1, fhandle);
    fwrite(&denominator, 4, 1, fhandle);

    numerator = (uint32_t) dpi;
    fseek(fhandle, y_res_offset, SEEK_SET);
    fwrite(&numerator, 4, 1, fhandle);
    fwrite(&denominator, 4, 1, fhandle);

    fseek(fhandle, image_data_offset, SEEK_SET);
    fwrite(data, bytes_per_sample, (size_t) width * height, fhandle);

    fclose(fhandle);

    return 0;
}

int main(int argc, char** argv) {

    int width = 1024;
    int height = 1024;

    uint8_t* noise = (uint8_t*) malloc((size_t)(width * height));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t index = (size_t)(y*width + x);
            float r_v = (rand() % 201 - 100) / 100.f;
            noise[index] = (uint8_t)((r_v + 1.f)*255.f)/2.f;
        }
    }

    if (write_image_to_ttf((const uint8_t*) noise, width, height, 96.0f, "example.tif") < 0) {
        free(noise);
        return EXIT_FAILURE;
    }

    free(noise);
    return EXIT_SUCCESS;
}
