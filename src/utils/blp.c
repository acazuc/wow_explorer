#include "utils/blp.h"
#include "utils/bc.h"

#include <libwow/blp.h>

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

bool blp_decode_rgba(const struct wow_blp_file *file, uint8_t mipmap_id, uint32_t *width, uint32_t *height, uint8_t **data)
{
	if (file->header.type != 1)
	{
		fprintf(stderr, "unsupported BLP type %" PRIu32 "\n", file->header.type);
		return false;
	}
	if (mipmap_id >= file->mipmaps_nb)
	{
		fprintf(stderr, "invalid mipmap id\n");
		return false;
	}
	const struct wow_blp_mipmap *mipmap = &file->mipmaps[mipmap_id];
	*width = mipmap->width;
	*height = mipmap->height;
	switch (file->header.compression)
	{
		case 1:
		{
			*data = malloc(*width * *height * 4);
			if (!*data)
			{
				fprintf(stderr, "failed to malloc data\n");
				break;
			}
			const uint8_t *indexes = mipmap->data;
			const uint8_t *alphas = indexes + *width * *height;
			uint32_t idx = 0;
			for (uint32_t i = 0; i < *width * *height; ++i)
			{
				uint32_t p = file->header.palette[indexes[i]];
				uint8_t *r = &(*data)[idx++];
				uint8_t *g = &(*data)[idx++];
				uint8_t *b = &(*data)[idx++];
				uint8_t *a = &(*data)[idx++];
				*r = p >> 16;
				*g = p >> 8;
				*b = p >> 0;
				switch (file->header.alpha_depth)
				{
					case 0:
						*a = 0xff;
						break;
					case 1:
						*a = ((alphas[i / 8] >> (i % 8)) & 1) * 0xff;
						break;
					case 4:
						*a = ((alphas[i / 2] >> ((i % 2) * 4)) & 0xf);
						*a |= *a << 4;
						break;
					case 8:
						*a = alphas[i];
						break;
					default:
						*a = 0xff;
						fprintf(stderr, "unsupported BLP alpha depth: %" PRIu32 "\n", file->header.alpha_depth);
						break;
				}
			}
			break;
		}
		case 2:
		{
			size_t size = *width * *height * 4;
			if (*width < 4)
				size += (4 - *width) * *height * 4;
			if (*height < 4)
				size += (4 - *height) * *width * 4;
			switch (file->header.alpha_type)
			{
				case 0:
					*data = malloc(size);
					if (*data)
						unpack_bc1(*width, *height, mipmap->data, *data);
					else
						fprintf(stderr, "failed to malloc BC1\n");
					break;
				case 1:
					*data = malloc(size);
					if (data)
						unpack_bc2(*width, *height, mipmap->data, *data);
					else
						fprintf(stderr, "failed to malloc BC2\n");
					break;
				case 7:
					*data = malloc(size);
					if (*data)
						unpack_bc3(*width, *height, mipmap->data, *data);
					else
						fprintf(stderr, "failed to malloc BC3\n");
					break;
				default:
					return false;
			}
			break;
		}
		case 3:
		{
			*data = malloc(*width * *height * 4);
			if (!*data)
			{
				fprintf(stderr, "failed to malloc data\n");
				break;
			}
			for (uint32_t i = 0; i < *width * *height * 4; i += 4)
			{
				(*data)[i + 0] = mipmap->data[i + 2];
				(*data)[i + 1] = mipmap->data[i + 1];
				(*data)[i + 2] = mipmap->data[i + 0];
				(*data)[i + 3] = mipmap->data[i + 3];
			}
			break;
		}
		default:
			return false;
	}
	return true;
}
