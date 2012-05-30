#ifdef SDL2
#include <stdint.h>
#include <stdbool.h>
bool image_load(const char* filename,int flag);
uint16_t image_width();
uint16_t image_height();
void image_convert(uint8_t *p_data);
void image_free();
#endif