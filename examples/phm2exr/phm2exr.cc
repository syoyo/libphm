#include <iostream>
#include <cstdlib>

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#include "libphm.h"

int main(int argc, char **argv)
{
  if (argc < 3) {
    std::cout << "Need input.phm output.exr\n";
    return -1;
  }

  std::string input_filename = argv[1];
  std::string output_filename = argv[2];

  float *img;
  int w, h, c;
  if (load_phm_as_float(input_filename.c_str(), &w, &h, &c, &img) <= 0) {
    return -1;
  }

  printf("w = %d, h = %d, c = %d\n", w, h, c);

  const char *err = nullptr;

  int ret = SaveEXR(img, w, h, c, /* fp16 */1, output_filename.c_str(), &err);

  if (ret != TINYEXR_SUCCESS) {
    if (err) {
      fprintf(stderr, "Save EXR err: %s(code %d)\n", err, ret);
    } else {
      fprintf(stderr, "Failed to save EXR image. code = %d\n", ret);
    }
  }

  free(img);

  return EXIT_SUCCESS;
}
