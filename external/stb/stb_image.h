#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned char* stbi_load(const char* filename, int* x, int* y, int* channels_in_file, int desired_channels);
void stbi_image_free(void* retval_from_stbi_load);
void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

#ifdef __cplusplus
}
#endif

#endif
