#ifndef BLP_H
#define BLP_H

#include <stdbool.h>
#include <stdint.h>

struct wow_blp_file;

bool blp_decode_rgba(const struct wow_blp_file *file, uint8_t mipmap_id, uint32_t *width, uint32_t *height, uint8_t **data);

#endif
