#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#define HEADER_SIZE 14
#define END_MARKER_SIZE 8

typedef struct {
    char magic[4];
    uint32_t width;
    uint32_t height;
    uint8_t channels;
    uint8_t colorspace;
} qoi_header;

typedef struct {
  uint8_t r, g, b, a;
} rgb_vals;

void read_bytes(uint8_t *bytes, size_t num_bytes, FILE *file) {
  size_t num_bytes_read = fread(bytes, 1, num_bytes, file);
  if (num_bytes_read != num_bytes) {
    if (ferror(file)) {
      fprintf(stderr, "ERROR: could not find next bytes: %s\n", strerror(errno));
      exit(1);
    } else if (feof(file)) {
      fprintf(stderr, "ERROR: could not find next bytes: reached end of file\n");
      exit(1);
    } else {
      fprintf(stderr, "ERROR: unknown error when reading file.\n");
      exit(1);
    }
  }
}

void reverse_bytes(uint8_t *bytes, size_t size) {
  for (size_t i = 0; i < size / 2; ++i) {
    uint8_t temp = bytes[i];
    bytes[i] = bytes[size - i - 1];
    bytes[size - i - 1] = temp;
  }
}

void print_image_details(qoi_header *header) {
  // reverse width and height bytes (big-endian to little-endian)
  uint8_t *width_ptr = (uint8_t *) &header->width;
  uint8_t *height_ptr = (uint8_t *) &header->height;
  reverse_bytes(width_ptr, sizeof(header->width));
  reverse_bytes(height_ptr, sizeof(header->height));

  // print image details
  printf("IMAGE DETAILS:\n");
  printf("  Magic bytes: %.4s\n", header->magic);
  printf("  Channels: %d\n", header->channels);
  printf("  Colorspace: %d\n", header->colorspace);
  printf("  Image width: %d\n", header->width);
  printf("  Image height: %d\n", header->height);
}

int get_file_size(FILE *file) {
  fseek(file, 0, SEEK_END);
  int input_size = ftell(file);
  if (input_size <= 0) {
    fprintf(stderr, "ERROR: could not open the file (maybe the file is empty?): %s\n", strerror(errno));
  }
  fseek(file, 0, SEEK_SET);
  return input_size;
}

void clear_file_contents(const char *filepath) {
  FILE *output_fp = fopen(filepath, "w");
  if (output_fp == NULL) {
    fprintf(stderr, "ERROR: could not open %s: %s\n", filepath, strerror(errno));
    exit(1);
  }
  fclose(output_fp);
}

void write_pixel(rgb_vals curr_pixel, FILE *file) {
    //float target_r = (1 - (curr_pixel.a / 255.0)) * 40.0 / 255.0 + (curr_pixel.a / 255.0) * (curr_pixel.r / 255.0);
    //float target_g = (1 - (curr_pixel.a / 255.0)) * 40.0 / 255.0 + (curr_pixel.a / 255.0) * (curr_pixel.g / 255.0);
    //float target_b = (1 - (curr_pixel.a / 255.0)) * 40.0 / 255.0 + (curr_pixel.a / 255.0) * (curr_pixel.b / 255.0);
    //uint8_t pixels[] = {(uint8_t) (target_r * 255), (uint8_t) (target_g * 255), (uint8_t) (target_b * 255)};
    uint8_t pixels[] = {curr_pixel.r, curr_pixel.g, curr_pixel.b};
    size_t num_bytes_read = fwrite(pixels, 1, 3, file);
    if (num_bytes_read != 3) {
      fprintf(stderr, "ERROR: Could not write to file: %s\n", strerror(errno));
      exit(1);
    }
}

void decode(uint8_t *image_data, int image_size, FILE *output_fp) {
  rgb_vals curr_pixel = {0, 0, 0, 255}, pixel_arr[64];
  uint32_t run = 0;

  for (size_t i = 0; i < image_size; ++i) {
    uint8_t curr_byte = image_data[i];

    //uint8_t temp_byte = curr_byte;
    //size_t temp_i = i;
    //size_t end_counter = 0;
    //while (temp_byte == 0x00) {
    //  end_counter++;
    //  if (end_counter == 7) {
    //    break;
    //  }
    //  temp_byte = image_data[++temp_i];
    //}

    if (curr_byte == 0xFE) {
      curr_pixel.r = image_data[++i];
      curr_pixel.g = image_data[++i];
      curr_pixel.b = image_data[++i];
    } else if (curr_byte == 0xFF) {
      curr_pixel.r = image_data[++i];
      curr_pixel.g = image_data[++i];
      curr_pixel.b = image_data[++i];
      curr_pixel.a = image_data[++i];
    } else if ((curr_byte >> 6) == 0x00) {
      uint32_t index = curr_byte & 0x3F;
      curr_pixel = pixel_arr[index];
    } else if ((curr_byte >> 6) == 0x01) {
      int8_t dr = ((curr_byte >> 4) & 0x03) - 2;
      int8_t dg = ((curr_byte >> 2) & 0x03) - 2;
      int8_t db = (curr_byte & 0x03) - 2;
      curr_pixel.r += dr;
      curr_pixel.g += dg;
      curr_pixel.b += db;
    } else if ((curr_byte >> 6) == 0x02) {
      uint8_t next_byte = image_data[++i];
      int8_t dg = (curr_byte & 0x3F) - 32;
      int8_t dr = ((next_byte >> 4) & 0x0F) - 8 + dg;
      int8_t db = (next_byte & 0x0F) - 8 + dg;
      curr_pixel.r += dr;
      curr_pixel.g += dg;
      curr_pixel.b += db;
    } else if ((curr_byte >> 6) == 0x03) {
      run = (curr_byte & 0x3F);
      while (run--) {
        write_pixel(curr_pixel, output_fp);
      }
    } else {
      fprintf(stderr, "ERROR: unable to parse byte: %hhu\n", curr_byte);
      exit(1);
    }

    uint32_t prev_index = (curr_pixel.r * 3 + curr_pixel.g * 5 + curr_pixel.b * 7 + curr_pixel.a * 11) % 64;
    pixel_arr[prev_index] = curr_pixel;

    write_pixel(curr_pixel, output_fp);
  }
}

int main(int argc, char **argv) {
  char *program = *argv++;
  if (argc == 1){
    fprintf(stderr, "Usage: %s <input.png>\n", program);
    fprintf(stderr, "ERROR: no input file was provided\n");
    exit(1);
  }

  // open QOI image file
  char *input_filepath = *argv++;
  FILE *input_fp = fopen(input_filepath, "rb");
  if (input_fp == NULL) {
    fprintf(stderr, "ERROR: could not open %s: %s\n", input_filepath, strerror(errno));
    exit(1);
  }

  // load image data
  int input_size = get_file_size(input_fp);
  uint8_t *input_data = malloc(input_size);
  if (!input_data) {
    fprintf(stderr, "ERROR: malloc failed: %s\n", strerror(errno));
    exit(1);
  }
  read_bytes(input_data, input_size, input_fp);

  fclose(input_fp);

  // interpret first 14 bytes as header
  uint8_t *header_bytes = input_data;
  qoi_header *header = (qoi_header *) header_bytes;
  const char *qoi_magic = "qoif";
  if (strcmp(header->magic, qoi_magic) != 0) {
    fprintf(stderr, "ERROR: this does not seem to be a QOI image file.\n");
    exit(1);
  }

  print_image_details(header);

  // set up output file
  const char *output_filepath = "output.ppm";
  clear_file_contents(output_filepath);
  FILE *output_fp = fopen(output_filepath, "a");
  if (output_fp == NULL) {
    fprintf(stderr, "ERROR: could not open %s: %s\n", output_filepath, strerror(errno));
    exit(1);
  }

  // write header for PPM format
  fprintf(output_fp, "P6 %d %d 255\n", header->width, header->height);

  decode(input_data + HEADER_SIZE, input_size - HEADER_SIZE - END_MARKER_SIZE, output_fp);

  free(input_data);
  fclose(output_fp);

  return 0;
}
