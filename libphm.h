/* simple PHM library */
/* Basically unlicense, but you can choose MIT license also */

#ifndef LIBPHM_H_
#define LIBPHM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Load PHM(Portable Half float Map) data and convert it to float32.
 * Endian swap is considered and resulting `out_image` has correct endianess
 *
 * @param[in] filename Input Filename
 * @param[out] w Image width
 * @param[out] h Image height
 * @param[out] c Image channels(1 or 3)
 * @param[out] out_image Loaded image(in float32 format).
 *
 * @return Image bytes. w x h x c x sizeof(float). 0 or negative = failed to read
 */
int load_phm_as_float(const char *filename, int *w, int *h, int *c, float **out_image);

/*
 * Load PHM(Portable Half float Map) data and return it as-is(fp16. use ushort as a storage format)
 * Endian swap is considered and resulting `out_image` has correct endianess
 *
 * @param[in] filename Input Filename
 * @param[out] w Image width
 * @param[out] h Image height
 * @param[out] c Image channels(1 or 3)
 * @param[out] out_image Loaded image(in fp16 format).
 *
 * @return Image bytes. w x h x c x sizeof(float). 0 or negative = failed to read
 */
int load_phm(const char *filename, int *w, int *h, int *c, unsigned short **out_image);

/*
 * Save floating point image as PHM(Portable Half float Map) data
 * Data is saved as host CPU's endian
 *
 * @param[in] filename Output Filename
 * @param[in] w Image width
 * @param[in] h Image height
 * @param[in] c Image channels(1 or 3)
 * @param[in] image Input image(in float32 format).
 *
 * @return Image bytes written. w x h x c x sizeof(fp16). 0 or native = failed to save.
 */
int save_phm_from_float(const char *filename, const int w, const int h, const int c, const float *image);


/*
 * Save fp16(ushort as storage format) image as PHM(Portable Half float Map) data
 * Data is saved as host CPU's endian
 *
 * @param[in] filename Output Filename
 * @param[in] w Image width
 * @param[in] h Image height
 * @param[in] c Image channels(1 or 3)
 * @param[in] image Input image(in fp16 format).
 *
 * @return Image bytes written. w x h x c x sizeof(fp16). 0 or native = failed to save.
 */
int save_phm(const char *filename, const int w, const int h, const int c, const unsigned short *image);

#ifdef __cplusplus
}
#endif

#endif // LIBPHM_H_

