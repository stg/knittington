#ifdef SDL2
#include <stdint.h>
#include <stdbool.h>
uint8_t picture_load(const char* filename);
uint16_t picture_width();
uint16_t picture_height();
void picture_convert(uint8_t *p_data);
void picture_free();
#endif