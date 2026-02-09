#include "stb_image.h"

#include <cstdlib>

unsigned char* stbi_load(const char* filename, int* x, int* y, int* channels_in_file, int desired_channels) {
  (void)filename;
  (void)x;
  (void)y;
  (void)channels_in_file;
  (void)desired_channels;
  return nullptr;
}

void stbi_image_free(void* retval_from_stbi_load) {
  std::free(retval_from_stbi_load);
}

void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip) {
  (void)flag_true_if_should_flip;
}
