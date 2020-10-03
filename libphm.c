/* Basically unlicense, but you can choose MIT license also */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libphm.h"

#if defined(__sparcv9)
// Big endian
#else
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || MINIZ_X86_OR_X64_CPU
// Set MINIZ_LITTLE_ENDIAN to 1 if the processor is little endian.
#define LIBPHM_LITTLE_ENDIAN 1
#endif
#endif

static void swap2(unsigned short *val) {
  unsigned short tmp = *val;
  unsigned char *dst = (unsigned char *)(val);
  unsigned char *src = (unsigned char *)(&tmp);

  dst[0] = src[1];
  dst[1] = src[0];
}

// https://gist.github.com/rygorous/2156668
union FP32 {
  unsigned int u;
  float f;
  struct {
#if LIBPHM_LITTLE_ENDIAN
    unsigned int Mantissa : 23;
    unsigned int Exponent : 8;
    unsigned int Sign : 1;
#else
    unsigned int Sign : 1;
    unsigned int Exponent : 8;
    unsigned int Mantissa : 23;
#endif
  } s;
};


union FP16 {
  unsigned short u;
  struct {
#if LIBPHM_LITTLE_ENDIAN
    unsigned int Mantissa : 10;
    unsigned int Exponent : 5;
    unsigned int Sign : 1;
#else
    unsigned int Sign : 1;
    unsigned int Exponent : 5;
    unsigned int Mantissa : 10;
#endif
  } s;
};

static union FP32 half_to_float(union FP16 h) {
  static const union FP32 magic = {113 << 23};
  static const unsigned int shifted_exp = 0x7c00
                                          << 13;  // exponent mask after shift
  union FP32 o;

  o.u = (h.u & 0x7fffU) << 13U;           // exponent/mantissa bits
  unsigned int exp_ = shifted_exp & o.u;  // just the exponent
  o.u += (127 - 15) << 23;                // exponent adjust

  // handle exponent special cases
  if (exp_ == shifted_exp)    // Inf/NaN?
    o.u += (128 - 16) << 23;  // extra exp adjust
  else if (exp_ == 0)         // Zero/Denormal?
  {
    o.u += 1 << 23;  // extra exp adjust
    o.f -= magic.f;  // renormalize
  }

  o.u |= (h.u & 0x8000U) << 16U;  // sign bit
  return o;
}

static union FP16 float_to_half_full(union FP32 f) {
  union FP16 o = {0};

  // Based on ISPC reference code (with minor modifications)
  if (f.s.Exponent == 0)  // Signed zero/denormal (which will underflow)
    o.s.Exponent = 0;
  else if (f.s.Exponent == 255)  // Inf or NaN (all exponent bits set)
  {
    o.s.Exponent = 31;
    o.s.Mantissa = f.s.Mantissa ? 0x200 : 0;  // NaN->qNaN and Inf->Inf
  } else                                      // Normalized number
  {
    // Exponent unbias the single, then bias the halfp
    int newexp = f.s.Exponent - 127 + 15;
    if (newexp >= 31)  // Overflow, return signed infinity
      o.s.Exponent = 31;
    else if (newexp <= 0)  // Underflow
    {
      if ((14 - newexp) <= 24)  // Mantissa might be non-zero
      {
        unsigned int mant = f.s.Mantissa | 0x800000;  // Hidden 1 bit
        o.s.Mantissa = mant >> (14 - newexp);
        if ((mant >> (13 - newexp)) & 1)  // Check for rounding
          o.u++;  // Round, might overflow into exp bit, but this is OK
      }
    } else {
      o.s.Exponent = (unsigned int)(newexp);
      o.s.Mantissa = f.s.Mantissa >> 13;
      if (f.s.Mantissa & 0x1000)  // Check for rounding
        o.u++;                    // Round, might overflow to inf, this is OK
    }
  }

  o.s.Sign = f.s.Sign;
  return o;
}

/* --------------------------------------- */


int load_phm_as_float(const char *filename, int *w, int *h, int *c, float **out_image) {

  unsigned short *tmp = NULL;
  int ret = load_phm(filename, w, h, c, &tmp);
  if (ret <= 0) {
    if (tmp) free(tmp);
    return ret;
  }

  // to fp32
  size_t num_pixels = (size_t)((*w) * (*h) * (*c));
  float *dest = (float *)malloc(num_pixels * sizeof(float));

  for (size_t i = 0; i < num_pixels; i++) {

    union FP16 half;

    unsigned short us = tmp[i];

    half.u = us;

    union FP32 fp32 = half_to_float(half);

    dest[i] = fp32.f;
  }

  free(tmp);

  (*out_image) = dest;

  return (int)(num_pixels * sizeof(float)); // Assume < 2GB
}

int load_phm(const char *filename, int *w, int *h, int *c, unsigned short **out_image) {

  if ((filename == NULL) || (w == NULL) || (h == NULL) || (c == NULL) || (out_image == NULL)) {
    fprintf(stderr, "Invalid argument\n");
    return -100;
  }

  FILE *fp = NULL;

#if defined(_MSC_VER)
  errno_t err = fopen_s(&fp, filename, "rb");
  if (err != 0) {
    return 0;
  }
#else
  fp = fopen(filename, "rb");
#endif

  if (fp == NULL) {
    fprintf(stderr, "Failed to open a file: %s\n", filename);
    return 0;
  }

  char header[8];

  if (fgets(header, 7, fp) == NULL) {
    fprintf(stderr, "Failed to read a header\n");
    return -1;
  }

  printf("loc = %ld\n", ftell(fp));

  if ((header[0] == 'P') && (header[1] == 'h')) {
    (*c) = 1;
  } else if ((header[0] == 'P') && (header[1] == 'H')) {
    (*c) = 3;
  } else {
    fprintf(stderr, "Invalid header: %s\n", header);
    return -2;
  }


  char buf[1024];
  if (fgets(buf, 1023, fp) == NULL) {
    fprintf(stderr, "Failed to read resolution line\n");
    return -3;
  }

  sscanf(buf, "%d %d", w, h);

  if (((*w) <= 0)  || ((*h) <= 0)) {
    fprintf(stderr, "Invalid width or height\n");
    return -3;
  }

  if (fgets(buf, 1023, fp) == NULL) {
    fprintf(stderr, "Failed to read byte_order line\n");
    return -3;
  }

  float byte_order = -1.0f;
  sscanf(buf, "%f", &byte_order);

  int swap_endian = 0;

  if (byte_order < 0.0f) {
    // little endian
#if !defined(LIBPHM_LITTLE_ENDIAN)
    swap_endian = 1;
#endif
  } else {
    // big endian
#if defined(LIBPHM_LITTLE_ENDIAN)
    swap_endian = 1;
#endif
  }

  size_t num_pixels = (size_t)((*w) * (*h) * (*c));

  unsigned short *img = (unsigned short *)malloc(num_pixels * sizeof(unsigned short));
  if (!img) {
    return -4;
  }

  size_t n_count = fread((void *)img, 1, num_pixels * 2, fp);
  if (n_count != (num_pixels * 2)) {
    fprintf(stderr, "Failed to read image data.\n");
    return -5;
  }

  if (swap_endian) {
    for (size_t i = 0; i < num_pixels; i++) {
      swap2(&img[i]);
    }
  }

  (*out_image) = img;

  fclose(fp);

  return (int)(n_count); // Assume < 2GB
}

int save_phm_from_float(const char *filename, const int w, const int h, const int c, const float *image) {
  if (image == NULL) {
    return -1;
  }

  if (filename == NULL) {
    return -1;
  }

  if ((w <= 0) || (h <= 0)) {
    // invalid extent
    return -2;
  }

  if ((c == 1) || (c == 3)) {
    // ok
  } else {
    fprintf(stderr, "Invalid channels: %d\n", c);
    return -2;
  }

  FILE *fp = NULL;

#if defined(_MSC_VER)
  errno_t err = fopen_s(&fp, filename, "wb");
  if (err != 0) {
    return 0;
  }
#else
  fp = fopen(filename, "wb");
#endif

  if (c == 1) {
    fprintf(fp, "Ph\n");
  } else {
    fprintf(fp, "PH\n");
  }

  fprintf(fp, "%d %d\n", w, h);

#if LIBPHM_LITTLE_ENDIAN
  fprintf(fp, "-1.0\n");
#else
  // TODO(syoyo): swap endian if we want to save as little endian format
  fprintf(fp, "1.0\n");
#endif


  size_t n_pixels = (size_t)(w * h * c);

  // to fp16
  unsigned short *fp16_image = (unsigned short *)malloc(n_pixels * 2);
  if (!fp16_image) {
    return -3;
  }

  for (size_t i = 0; i < n_pixels; i++) {
    union FP32 fp32;

    fp32.f = image[i];

    union FP16 half = float_to_half_full(fp32);

    fp16_image[i] = half.u;
  }

  size_t n_count = fwrite((const void *)fp16_image, 1, n_pixels * 2, fp);
  if (n_count != (n_pixels * 2)) {
    fprintf(stderr, "Failed to write image data\n");
    return -4;
  }

  free(fp16_image);

  return (int)n_count;
}

int save_phm(const char *filename, const int w, const int h, const int c, const unsigned short *image) {
  if (image == NULL) {
    return -1;
  }

  if (filename == NULL) {
    return -1;
  }

  if ((w <= 0) || (h <= 0)) {
    // invalid extent
    return -2;
  }

  if ((c == 1) || (c == 3)) {
    // ok
  } else {
    fprintf(stderr, "Invalid channels: %d\n", c);
    return -2;
  }

  FILE *fp = NULL;

#if defined(_MSC_VER)
  errno_t err = fopen_s(&fp, filename, "wb");
  if (err != 0) {
    return 0;
  }
#else
  fp = fopen(filename, "wb");
#endif

  if (c == 1) {
    fprintf(fp, "Ph\n");
  } else {
    fprintf(fp, "PH\n");
  }

  fprintf(fp, "%d %d\n", w, h);

#if LIBPHM_LITTLE_ENDIAN
  fprintf(fp, "-1.0\n");
#else
  // TODO(syoyo): swap endian if we want to save as little endian format
  fprintf(fp, "1.0\n");
#endif


  size_t n_pixels = (size_t)(w * h * c);

  size_t n_count = fwrite((const void *)image, 1, n_pixels * 2, fp);
  if (n_count != (n_pixels * 2)) {
    fprintf(stderr, "Failed to write image data\n");
    return -4;
  }

  fclose(fp);

  return (int)n_count;
}

