#include "utils/shaders.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define D3DSI_OPCODE_MASK      0x0000FFFF
#define D3DSI_INSTLENGTH_MASK  0x0F000000
#define D3DSI_INSTLENGTH_SHIFT 24

#define D3DSHADER_VERSION_MAJOR(version) (((version) >> 8) & 0xFF)
#define D3DSHADER_VERSION_MINOR(version) (((version) >> 0) & 0xFF)

#define D3DSP_REGTYPE_SHIFT        28
#define D3DSP_REGTYPE_SHIFT2       8
#define D3DSP_REGTYPE_MASK         (0x7 << D3DSP_REGTYPE_SHIFT)
#define D3DSP_REGTYPE_MASK2        0x00001800
#define D3DSP_REGNUM_MASK          0x000007FF
#define D3DSP_SWIZZLE_SHIFT        16
#define D3DSP_SWIZZLE_MASK         (0xFF << D3DSP_SWIZZLE_SHIFT)
#define D3DSP_SRCMOD_SHIFT         24
#define D3DSP_SRCMOD_MASK          (0xF << D3DSP_SRCMOD_SHIFT)
#define D3DSP_TEXTURETYPE_SHIFT    27
#define D3DSP_TEXTURETYPE_MASK     0x78000000
#define D3DSP_DCL_USAGE_SHIFT      0
#define D3DSP_DCL_USAGE_MASK       0x0000000f
#define D3DSP_DCL_USAGEINDEX_SHIFT 16
#define D3DSP_DCL_USAGEINDEX_MASK  0x000f0000
#define D3DSP_WRITEMASK_0          0x00010000
#define D3DSP_WRITEMASK_1          0x00020000
#define D3DSP_WRITEMASK_2          0x00040000
#define D3DSP_WRITEMASK_3          0x00080000
#define D3DSP_WRITEMASK_ALL        (D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2 | D3DSP_WRITEMASK_3)
#define D3DSP_DSTMOD_SHIFT         20
#define D3DSP_DSTMOD_MASK          (0xF << D3DSP_DSTMOD_SHIFT)

static void append(char **buffer, size_t *buffer_size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

static void append(char **buffer, size_t *buffer_size, const char *fmt, ...)
{
	va_list ap;

	if (*buffer_size <= 1)
		return;
	va_start(ap, fmt);
	size_t len = vsnprintf(*buffer, *buffer_size, fmt, ap);
	if (len >= *buffer_size)
		len = *buffer_size - 1;
	*buffer += len;
	*buffer_size -= len;
	va_end(ap);
}

static const char *decode_dx9_swizzle_part(uint8_t swizzle)
{
	switch (swizzle)
	{
		case 0:
			return "x";
		case 1:
			return "y";
		case 2:
			return "z";
		case 3:
			return "w";
	}
	return "";
}

static void decode_dx9_swizzle(char **buffer, size_t *buffer_size, uint32_t data)
{
	uint32_t swizzle = (data & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
	if (swizzle == 0xE4) /* .xyzw */
		return;
	uint32_t sx = (swizzle >> 0) & 0x3;
	uint32_t sy = (swizzle >> 2) & 0x3;
	uint32_t sz = (swizzle >> 4) & 0x3;
	uint32_t sw = (swizzle >> 6) & 0x3;
	if (sx == sy && sy == sz && sz == sw)
	{
		append(buffer, buffer_size, "%s", decode_dx9_swizzle_part(sx));
		return;
	}
	append(buffer, buffer_size, "%s", decode_dx9_swizzle_part(sx));
	append(buffer, buffer_size, "%s", decode_dx9_swizzle_part(sy));
	append(buffer, buffer_size, "%s", decode_dx9_swizzle_part(sz));
	append(buffer, buffer_size, "%s", decode_dx9_swizzle_part(sw));
}

static void decode_dx9_write_mask(char **buffer, size_t *buffer_size, uint32_t data)
{
	if ((data & D3DSP_WRITEMASK_ALL) == D3DSP_WRITEMASK_ALL)
		return;
	if (data & D3DSP_WRITEMASK_0)
		append(buffer, buffer_size, "x");
	if (data & D3DSP_WRITEMASK_1)
		append(buffer, buffer_size, "y");
	if (data & D3DSP_WRITEMASK_2)
		append(buffer, buffer_size, "z");
	if (data & D3DSP_WRITEMASK_3)
		append(buffer, buffer_size, "w");
}

static void decode_dx9_operand(char **buffer, size_t *buffer_size, uint32_t data, const char *swizzle)
{
	uint32_t regtype = ((data & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) | ((data & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2);
	uint32_t regnum = data & D3DSP_REGNUM_MASK;
	switch (regtype)
	{
		case 0:
			append(buffer, buffer_size, "r%" PRIu32, regnum);
			if (swizzle[0])
				append(buffer, buffer_size, ".%s", swizzle);
			break;
		case 1:
			append(buffer, buffer_size, "v%" PRIu32, regnum);
			if (swizzle[0])
				append(buffer, buffer_size, ".%s", swizzle);
			break;
		case 2:
			append(buffer, buffer_size, "c%" PRIu32, regnum);
			if (swizzle[0])
				append(buffer, buffer_size, ".%s", swizzle);
			break;
		case 3:
			append(buffer, buffer_size, "t%" PRIu32, regnum);
			break;
		case 4:
			append(buffer, buffer_size, "oFog%" PRIu32, regnum);
			if (swizzle[0])
				append(buffer, buffer_size, ".%s", swizzle);
			break;
		case 5:
			append(buffer, buffer_size, "oPos%" PRIu32, regnum);
			if (swizzle[0])
				append(buffer, buffer_size, ".%s", swizzle);
			break;
		case 6:
			append(buffer, buffer_size, "oT%" PRIu32, regnum);
			if (swizzle[0])
				append(buffer, buffer_size, ".%s", swizzle);
			break;
		case 7:
			append(buffer, buffer_size, "i%" PRIu32, regnum);
			if (swizzle[0])
				append(buffer, buffer_size, ".%s", swizzle);
			break;
		case 8:
			append(buffer, buffer_size, "oC%" PRIu32, regnum);
			if (swizzle[0])
				append(buffer, buffer_size, ".%s", swizzle);
			break;
		case 9:
			append(buffer, buffer_size, "oDepth");
			break;
		case 10:
			append(buffer, buffer_size, "s%" PRIu32, regnum);
			break;
		case 14:
			append(buffer, buffer_size, "b%" PRIu32, regnum);
			break;
		case 15:
			append(buffer, buffer_size, "aL");
			break;
		case 18:
			append(buffer, buffer_size, "l%" PRIu32, regnum);
			break;
		case 19:
			append(buffer, buffer_size, "p%" PRIu32, regnum);
			break;
		default:
			append(buffer, buffer_size, "unk(%" PRIu32 ")%" PRIu32, regtype, regnum);
	}
}

static void decode_dx9_operand_src(char **buffer, size_t *buffer_size, uint32_t data, bool display_swizzle)
{
	uint32_t mod = (data & D3DSP_SRCMOD_MASK) >> D3DSP_SRCMOD_SHIFT;
	switch (mod)
	{
		case 1:
		case 3:
		case 5:
		case 8:
		case 12:
			append(buffer, buffer_size, "-");
			break;
		case 6:
			append(buffer, buffer_size, "~");
			break;
		case 13:
			append(buffer, buffer_size, "!");
			break;
	}
	switch (mod)
	{
		case 11:
		case 12:
			append(buffer, buffer_size, "abs(");
			break;
	}
	char swizzle_buf[8] = "";
	char *swizzle = swizzle_buf;
	size_t swizzle_size = sizeof(swizzle_buf);
	if (display_swizzle)
		decode_dx9_swizzle(&swizzle, &swizzle_size, data);
	decode_dx9_operand(buffer, buffer_size, data, swizzle_buf);
	switch (mod)
	{
		case 2:
		case 3:
			append(buffer, buffer_size, "_bias");
			break;
		case 4:
		case 5:
			append(buffer, buffer_size, "_bx2");
			break;
		case 7:
		case 8:
			append(buffer, buffer_size, "_x2");
			break;
		case 9:
			append(buffer, buffer_size, "_dz");
			break;
		case 10:
			append(buffer, buffer_size, "_dw");
			break;
		case 11:
		case 12:
			append(buffer, buffer_size, ")");
			break;
	}
}

static void decode_dx9_operand_dst(char **buffer, size_t *buffer_size, uint32_t data, bool display_swizzle)
{
	char swizzle_buf[8] = "";
	char *swizzle = swizzle_buf;
	size_t swizzle_size = sizeof(swizzle_buf);
	if (display_swizzle)
		decode_dx9_write_mask(&swizzle, &swizzle_size, data);
	decode_dx9_operand(buffer, buffer_size, data, swizzle_buf);
}

static void decode_dx9_instr(char **buffer, size_t *buffer_size, const char *instr, const uint32_t *data, size_t *pos, size_t size, bool dst, uint8_t src)
{
	append(buffer, buffer_size, "%s", instr);
	if (dst)
	{
		(*pos)++;
		if (*pos >= size)
			return;
		switch ((data[*pos] & D3DSP_DSTMOD_MASK) >> D3DSP_DSTMOD_SHIFT)
		{
			case 1:
				append(buffer, buffer_size, "_SAT");
				break;
			case 2:
				append(buffer, buffer_size, "_PARTIAL");
				break;
			case 3:
				append(buffer, buffer_size, "_CENTROID");
				break;
		}
		append(buffer, buffer_size, " ");
		decode_dx9_operand_dst(buffer, buffer_size, data[*pos], true);
	}
	for (uint8_t i = 0; i < src; ++i)
	{
		(*pos)++;
		if (*pos >= size)
			return;
		if (dst || i != 0)
			append(buffer, buffer_size, ",");
		append(buffer, buffer_size, " ");
		decode_dx9_operand_src(buffer, buffer_size, data[*pos], true);
	}
	(*pos)++;
}

static void decode_dx9_def(char **buffer, size_t *buffer_size, const uint32_t *data, size_t *pos, size_t size)
{
	if (size - *pos < 5)
		return;
	append(buffer, buffer_size, "DEF ");
	(*pos)++;
	decode_dx9_operand(buffer, buffer_size, data[*pos], "");
	(*pos)++;
	append(buffer, buffer_size, ", %f", *(float*)&data[(*pos)++]);
	append(buffer, buffer_size, ", %f", *(float*)&data[(*pos)++]);
	append(buffer, buffer_size, ", %f", *(float*)&data[(*pos)++]);
	append(buffer, buffer_size, ", %f", *(float*)&data[(*pos)++]);
}

static void decode_dx9_dcl_sampler(char **buffer, size_t *buffer_size, const uint32_t *data, size_t *pos, size_t size)
{
	if (size - *pos < 2)
		return;
	append(buffer, buffer_size, "DCL_");
	(*pos)++;
	switch ((data[*pos] & D3DSP_TEXTURETYPE_MASK) >> D3DSP_TEXTURETYPE_SHIFT)
	{
		case 0:
			append(buffer, buffer_size, "unknown");
			break;
		case 1:
			append(buffer, buffer_size, "2d");
			break;
		case 2:
			append(buffer, buffer_size, "cube");
			break;
		case 3:
			append(buffer, buffer_size, "volume");
			break;
	}
	(*pos)++;
	append(buffer, buffer_size, " ");
	char mask_buf[8] = "";
	char *mask = mask_buf;
	size_t mask_size = sizeof(mask_buf);
	decode_dx9_write_mask(&mask, &mask_size, data[*pos]);
	decode_dx9_operand(buffer, buffer_size, data[*pos], mask_buf);
	(*pos)++;
}

static void decode_dx9_dcl_attribute(char **buffer, size_t *buffer_size, const uint32_t *data, size_t *pos, size_t size)
{
	if (size - *pos < 2)
		return;
	append(buffer, buffer_size, "DCL_");
	(*pos)++;
	switch ((data[*pos] & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT)
	{
		case 0:
			append(buffer, buffer_size, "position");
			break;
		case 1:
			append(buffer, buffer_size, "blendweight");
			break;
		case 2:
			append(buffer, buffer_size, "blendindices");
			break;
		case 3:
			append(buffer, buffer_size, "normal");
			break;
		case 4:
			append(buffer, buffer_size, "psize");
			break;
		case 5:
			append(buffer, buffer_size, "texcoord");
			break;
		case 6:
			append(buffer, buffer_size, "tangent");
			break;
		case 7:
			append(buffer, buffer_size, "binormal");
			break;
		case 8:
			append(buffer, buffer_size, "tessfactor");
			break;
		case 9:
			append(buffer, buffer_size, "posititiont");
			break;
		case 10:
			append(buffer, buffer_size, "color");
			break;
		case 11:
			append(buffer, buffer_size, "fog");
			break;
		case 12:
			append(buffer, buffer_size, "depth");
			break;
		case 13:
			append(buffer, buffer_size, "sample");
			break;
	}
	append(buffer, buffer_size, "%d", (data[*pos] & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT);
	(*pos)++;
	append(buffer, buffer_size, " ");
	char mask_buf[8] = "";
	char *mask = mask_buf;
	size_t mask_size = sizeof(mask_buf);
	decode_dx9_write_mask(&mask, &mask_size, data[*pos]);
	decode_dx9_operand(buffer, buffer_size, data[*pos], mask_buf);
	(*pos)++;
}

static void decode_dx9_dcl(char **buffer, size_t *buffer_size, const uint32_t *data, size_t *pos, size_t size)
{
	if (size - *pos < 2)
		return;
	if ((((data[*pos + 2] & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) | ((data[*pos + 2] & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2)) == 10)
		decode_dx9_dcl_sampler(buffer, buffer_size, data, pos, size);
	decode_dx9_dcl_attribute(buffer, buffer_size, data, pos, size);
}

void decode_dx9_shader(char *buffer, size_t buffer_size, const void *d, size_t s)
{
	if (s < 4)
		return;
	const uint32_t *data = (const uint32_t*)d;
	size_t size = s / 4;
	uint32_t major = D3DSHADER_VERSION_MAJOR(data[0]);
	uint32_t minor = D3DSHADER_VERSION_MINOR(data[0]);
	switch (data[0] & 0xFFFF0000)
	{
		case 0xFFFF0000:
			append(&buffer, &buffer_size, "ps_%d_%d\n", major, minor);
			break;
		case 0xFFFE0000:
			append(&buffer, &buffer_size, "vs_%d_%d\n", major, minor);
			break;
		default:
			return;
	}
	size_t i = 1;
	while (i < size)
	{
		char line_buf[128];
		char *line = line_buf;
		size_t line_size = sizeof(line_buf);
		size_t prev = i;
		uint32_t opcode = data[i] & D3DSI_OPCODE_MASK;
		uint32_t instlen = (data[i] & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
		switch (opcode)
		{
			case 0:
				decode_dx9_instr(&line, &line_size, "NOP", data, &i, size, false, 0);
				break;
			case 1:
				decode_dx9_instr(&line, &line_size, "MOV", data, &i, size, true, 1);
				break;
			case 2:
				decode_dx9_instr(&line, &line_size, "ADD", data, &i, size, true, 2);
				break;
			case 3:
				decode_dx9_instr(&line, &line_size, "SUB", data, &i, size, true, 2);
				break;
			case 4:
				decode_dx9_instr(&line, &line_size, "MAD", data, &i, size, true, 3);
				break;
			case 5:
				decode_dx9_instr(&line, &line_size, "MUL", data, &i, size, true, 2);
				break;
			case 6:
				decode_dx9_instr(&line, &line_size, "RCP", data, &i, size, true, 1);
				break;
			case 7:
				decode_dx9_instr(&line, &line_size, "RSQ", data, &i, size, true, 1);
				break;
			case 8:
				decode_dx9_instr(&line, &line_size, "DP3", data, &i, size, true, 2);
				break;
			case 9:
				decode_dx9_instr(&line, &line_size, "DP4", data, &i, size, true, 2);
				break;
			case 10:
				decode_dx9_instr(&line, &line_size, "MIN", data, &i, size, true, 2);
				break;
			case 11:
				decode_dx9_instr(&line, &line_size, "MAX", data, &i, size, true, 2);
				break;
			case 12:
				decode_dx9_instr(&line, &line_size, "SLT", data, &i, size, true, 2);
				break;
			case 13:
				decode_dx9_instr(&line, &line_size, "SGE", data, &i, size, true, 2);
				break;
			case 14:
				decode_dx9_instr(&line, &line_size, "SLT", data, &i, size, true, 1);
				break;
			case 15:
				decode_dx9_instr(&line, &line_size, "LOG", data, &i, size, true, 1);
				break;
			case 16:
				decode_dx9_instr(&line, &line_size, "LIT", data, &i, size, true, 1);
				break;
			case 17:
				decode_dx9_instr(&line, &line_size, "DST", data, &i, size, true, 2);
				break;
			case 18:
				decode_dx9_instr(&line, &line_size, "LRP", data, &i, size, true, 3);
				break;
			case 19:
				decode_dx9_instr(&line, &line_size, "FRC", data, &i, size, true, 1);
				break;
			case 20:
				decode_dx9_instr(&line, &line_size, "M4X4", data, &i, size, true, 2);
				break;
			case 21:
				decode_dx9_instr(&line, &line_size, "M4X3", data, &i, size, true, 2);
				break;
			case 22:
				decode_dx9_instr(&line, &line_size, "M3X4", data, &i, size, true, 2);
				break;
			case 23:
				decode_dx9_instr(&line, &line_size, "M3X3", data, &i, size, true, 2);
				break;
			case 24:
				decode_dx9_instr(&line, &line_size, "M3X2", data, &i, size, true, 2);
				break;
			case 25:
				decode_dx9_instr(&line, &line_size, "CALL", data, &i, size, false, 1);
				break;
			case 26:
				decode_dx9_instr(&line, &line_size, "CALLNZ", data, &i, size, false, 2);
				break;
			case 27:
				decode_dx9_instr(&line, &line_size, "LOOP", data, &i, size, false, 2);
				break;
			case 28:
				decode_dx9_instr(&line, &line_size, "RET", data, &i, size, false, 0);
				break;
			case 29:
				decode_dx9_instr(&line, &line_size, "ENDLOOP", data, &i, size, false, 0);
				break;
			case 30:
				decode_dx9_instr(&line, &line_size, "LABEL", data, &i, size, false, 1);
				break;
			case 31:
				decode_dx9_dcl(&line, &line_size, data, &i, size);
				break;
			case 32:
				decode_dx9_instr(&line, &line_size, "POW", data, &i, size, true, 2);
				break;
			case 33:
				decode_dx9_instr(&line, &line_size, "CRS", data, &i, size, true, 2);
				break;
			case 34:
				decode_dx9_instr(&line, &line_size, "SIGN", data, &i, size, true, 3);
				break;
			case 35:
				decode_dx9_instr(&line, &line_size, "ABS", data, &i, size, true, 1);
				break;
			case 36:
				decode_dx9_instr(&line, &line_size, "NRM", data, &i, size, true, 1);
				break;
			case 37:
				decode_dx9_instr(&line, &line_size, "SINCOS", data, &i, size, true, 3);
				break;
			case 38:
				decode_dx9_instr(&line, &line_size, "REP", data, &i, size, false, 1);
				break;
			case 39:
				decode_dx9_instr(&line, &line_size, "ENDREP", data, &i, size, false, 0);
				break;
			case 46:
				decode_dx9_instr(&line, &line_size, "MOVA", data, &i, size, true, 1);
				break;
			case 66:
				if (major >= 2)
					decode_dx9_instr(&line, &line_size, "TEXID", data, &i, size, true, 2);
				else if (minor == 4)
					decode_dx9_instr(&line, &line_size, "TEXID", data, &i, size, true, 1);
				else
					decode_dx9_instr(&line, &line_size, "TEX", data, &i, size, false, 1);
				break;
			case 81:
				decode_dx9_def(&line, &line_size, data, &i, size);
				break;
			case 65533:
				decode_dx9_instr(&line, &line_size, "PHASE", data, &i, size, 0, 0);
				break;
			case 65535:
				append(&line, &line_size, "END");
				i++;
				break;
			default:
				i++;
				break;
		}
		size_t j;
		for (j = strlen(line_buf); j < 60; ++j)
			line_buf[j] = ' ';
		line_buf[j] = '\0';
		append(&buffer, &buffer_size, "%s; opcode: %#6" PRIx32 ", len: %2" PRIu32 ", val: %#10" PRIx32 "\n", line_buf, opcode, instlen, data[prev]);
	}
}
