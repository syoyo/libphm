#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "libphm.h"

#if defined(__sparcv9)
// Big endian
#else
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || MINIZ_X86_OR_X64_CPU
// Set MINIZ_LITTLE_ENDIAN to 1 if the processor is little endian.
#define MY_LITTLE_ENDIAN 1
#endif
#endif

static void swap4(unsigned int *val) {
  unsigned int tmp = *val;
  unsigned char *dst = (unsigned char *)(val);
  unsigned char *src = (unsigned char *)(&tmp);

  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
}

static float *gen_image(int w, int h, int c)
{
  float *img;

  if (c == 1) {
    img = (float *)malloc((size_t)(w * h) * sizeof(float));

    for (size_t y = 0; y < (size_t)(h); y++) {
      for (size_t x = 0; x < (size_t)(w); x++) {
        img[(y * (size_t)(w) + x)] = (y * (size_t)(w) + x) / (float)(w * h);
      }
    }

  } else {
    // assume 3
    img = (float *)malloc((size_t)(w * h) * 3 * sizeof(float));

    for (size_t y = 0; y < (size_t)(h); y++) {
      for (size_t x = 0; x < (size_t)(w); x++) {
        img[3 * (y * (size_t)(w) + x) + 0] = x / (float)(w);
        img[3 * (y * (size_t)(w) + x) + 1] = y / (float)(h);
        img[3 * (y * (size_t)(w) + x) + 2] = 0.5f;
     }
    }
  }

  return img;
}

// Load PFM(portable float map) image. float32 pixel data type.
static float *load_pfm(const char *filename, int *w, int *h, int *c) {
  FILE *fp = NULL;
#if defined(_MSC_VER)
  errno_t err = fopen_s(&fp, filename, "rb");
  if (err != 0) {
    fprintf(stderr, "Failed to open file: %s\n", filename);
    return NULL;
  }
#else
  fp = fopen(filename, "rb");
#endif

  if (!fp) {
    fprintf(stderr, "Failed to open file: %s\n", filename);
    return NULL;
  }

  char buf[1024];
  fscanf(fp, "%s\n", buf);

  int channels = 0;
  if (strcmp(buf, "PF") == 0) {
    channels = 3;
  } else if (strcmp(buf, "Pf") == 0) {
    channels = 1;
  } else {
    fprintf(stderr, "Unsupported type: %s\n", buf);
    return NULL;
  }

  int width, height;
  fscanf(fp, "%d %d\n", &width, &height);

  float endian_flag = 0.0f;
  fscanf(fp, "%f\n", &endian_flag);

  int swap_endian = 0;

  if (endian_flag < 0.0f) {
#if !defined(MY_LITTLE_ENDIAN)
    swap_endian = 1;
#endif
  } else {
#if defined(MY_LITTLE_ENDIAN)
    swap_endian = 1;
#endif
  }

  // TODO(syoyo): Consider endianness
  size_t img_size = (size_t)(width * height * channels) * sizeof(float);
  float *img = (float *)malloc(img_size);

  size_t n_read = fread((void *)img, 1, img_size, fp);

  if (n_read != img_size) {
    fprintf(stderr, "Failed to read image data\n");
    return NULL;
  }

  fclose(fp);

  if (swap_endian) {
    for (size_t i = 0; i < (size_t)(width * height * channels); i++) {
      swap4((unsigned int *)(&img[i]));
    }
  }

  (*w) = width;
  (*h) = height;
  (*c) = channels;

  return img;

}

int main(int argc, char **argv)
{
  int w, h, c;
  float *img = NULL;

  if (argc < 2) {
    printf("Usage: sample input.pfm\n\n  Use synthetically generated image.\n");
    w = 256; h = 256; c = 3;
    img = gen_image(w, h, c);
  } else {

    img = load_pfm(argv[1], &w, &h, &c);
    if (!img) {
      return -1;
    }
  }

  // save
  if (save_phm_from_float("output.phm", w, h, c, img) <= 0) {
    fprintf(stderr, "Failed to save PHM data\n");
    return -1;
  }

  printf("Wrote phm image as \"output.phm\"\n");

  if (img) {
    free(img);
  }

  return EXIT_SUCCESS;
}
