/*
 * Wii U 'GTX' Texture Extractor
 * Created by Ninji Vahran / Treeki; 2014-10-31
 *   ( https://github.com/Treeki )
 * This software is released into the public domain.
 *
 * Dependencies: libtxc_dxtn, libpng
 * Tested with GCC on Arch Linux. May fail elsewhere.
 * Expect it to fail anywhere, really ;)
 *
 * gcc -lpng -ltxc_dxtn -o gtx_extract gtx_extract.c
 *
 * This tool currently supports RGBA8 (format 0x1A) and DXT5 (format 0x33)
 * textures.
 * The former is known to work with 2048x512 textures.
 * The latter has been tested successfully with 512x320 and 2048x512 textures,
 * and is known to be broken with 384x256 textures.
 *
 * Why so complex?
 * Wii U textures appear to be packed using a complex 'texture swizzling'
 * algorithm, presumably for faster access.
 *
 * With no publicly known details that I could find, I had to attempt to
 * recreate it myself - with a limited set of sample data to examine.
 *
 * This tool's implementation is sufficient to unpack the textures I wanted,
 * but it's likely to fail on others.
 * Feel free to throw a pull request at me if you improve it!
 */

#include <txc_dxtn.h>
#include <png.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>

typedef struct _GTXData {
	uint32_t width, height;
	uint32_t format;
	uint32_t dataSize;
	uint8_t *data;
} GTXData;

typedef struct _GTXRawHeader {
	char magic[4];
	uint32_t _4, _8, _C, _10, _14, _18, _1C;
} GTXRawHeader;
typedef struct _GTXRawSectionHeader {
	char magic[4];
	uint32_t _4, _8, _C, _10;
	uint32_t size;
	uint32_t _18, _1C;
} GTXRawSectionHeader;
typedef struct _GTXRawTextureInfo {
	uint32_t _0, width, height, _C;
	uint32_t _10, formatMaybe, _18, _1C;
	uint32_t sizeMaybe, _24, _28, _2C;
	uint32_t _30, _34, _38, _3C;
	uint32_t _40, _44, _48, _4C;
	uint32_t _50, _54, _58, _5C;
	uint32_t _60, _64, _68, _6C;
	uint32_t _70, _74, _78, _7C;
	uint32_t _80, _84, _88, _8C;
	uint32_t _90, _94, _98;
} GTXRawTextureInfo;

uint32_t swap32(uint32_t v) {
	uint32_t a = (v & 0xFF000000) >> 24;
	uint32_t b = (v & 0x00FF0000) >> 8;
	uint32_t c = (v & 0x0000FF00) << 8;
	uint32_t d = (v & 0x000000FF) << 24;
	return a|b|c|d;
}

uint32_t swapRB(uint32_t argb) {
	uint32_t r = (argb & 0x00FF0000) >> 16;
	uint32_t b = (argb & 0x000000FF) << 16;
	uint32_t ag = (argb & 0xFF00FF00);
	return ag|r|b;
}

void writeBMPHeader(FILE *f, int width, int height) {
	uint16_t u16;
	uint32_t u32;

	fwrite("BM", 1, 2, f);
	u32 = 122 + (width*height*4); fwrite(&u32, 1, 4, f);
	u16 = 0; fwrite(&u16, 1, 2, f);
	u16 = 0; fwrite(&u16, 1, 2, f);
	u32 = 122; fwrite(&u32, 1, 4, f);

	u32 = 108; fwrite(&u32, 1, 4, f);
	u32 = width; fwrite(&u32, 1, 4, f);
	u32 = height; fwrite(&u32, 1, 4, f);
	u16 = 1; fwrite(&u16, 1, 2, f);
	u16 = 32; fwrite(&u16, 1, 2, f);
	u32 = 3; fwrite(&u32, 1, 4, f);
	u32 = width*height*4; fwrite(&u32, 1, 4, f);
	u32 = 2835; fwrite(&u32, 1, 4, f);
	u32 = 2835; fwrite(&u32, 1, 4, f);
	u32 = 0; fwrite(&u32, 1, 4, f);
	u32 = 0; fwrite(&u32, 1, 4, f);
	u32 = 0xFF0000; fwrite(&u32, 1, 4, f);
	u32 = 0xFF00; fwrite(&u32, 1, 4, f);
	u32 = 0xFF; fwrite(&u32, 1, 4, f);
	u32 = 0xFF000000; fwrite(&u32, 1, 4, f);
	u32 = 0x57696E20; fwrite(&u32, 1, 4, f);

	uint8_t thing[0x24];
	memset(thing, 0, 0x24);
	fwrite(thing, 1, 0x24, f);

	u32 = 0; fwrite(&u32, 1, 4, f);
	u32 = 0; fwrite(&u32, 1, 4, f);
	u32 = 0; fwrite(&u32, 1, 4, f);
}

int readGTX(GTXData *gtx, FILE *f) {
	GTXRawHeader header;

	// This is kinda bad. Don't really care right now >.>
	gtx->width = 0;
	gtx->height = 0;
	gtx->data = NULL;

	if (fread(&header, 1, sizeof(header), f) != sizeof(header))
		return -1;

	if (memcmp(header.magic, "Gfx2", 4) != 0)
		return -2;

	while (!feof(f)) {
		GTXRawSectionHeader section;
		if (fread(&section, 1, sizeof(section), f) != sizeof(section))
			break;

		if (memcmp(section.magic, "BLK{", 4) != 0)
			return -100;

		if (swap32(section._10) == 0xB) {
			GTXRawTextureInfo info;

			if (swap32(section.size) != 0x9C)
				return -200;

			if (fread(&info, 1, sizeof(info), f) != sizeof(info))
				return -201;

			gtx->width = swap32(info.width);
			gtx->height = swap32(info.height);
			gtx->format = swap32(info.formatMaybe);

		} else if (swap32(section._10) == 0xC && gtx->data == NULL) {
			gtx->dataSize = swap32(section.size);
			gtx->data = malloc(gtx->dataSize);
			if (!gtx->data)
				return -300;

			if (fread(gtx->data, 1, gtx->dataSize, f) != gtx->dataSize)
				return -301;

		} else {
			fseek(f, swap32(section.size), SEEK_CUR);
		}
	}

	return 1;
}

void writeFile(FILE *f, int width, int height, uint8_t *output, int isPNG) {
	int row;

	if (isPNG == 0) {
		writeBMPHeader(f, width, height);

		for (row = height - 1; row >= 0; row--) {
			fwrite(&output[row * width * 4], 1, width * 4, f);
		}
	} else {
		png_structp png_ptr;
		png_infop info_ptr;

		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		info_ptr = png_create_info_struct(png_ptr);

		if (setjmp(png_jmpbuf(png_ptr))) {
			png_destroy_write_struct(&png_ptr, &info_ptr);
			return;
		}

		png_init_io(png_ptr, f);
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr);
		png_set_bgr(png_ptr);

		for (row = 0; row < height; row++) {
			png_write_row(png_ptr, &output[row * width * 4]);
		}

		png_write_end(png_ptr, info_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);
	}
}

void export_RGBA8(GTXData *gtx, FILE *f, int isPNG) {
	uint32_t pos, x, y;
	uint32_t *source, *output;

	source = (uint32_t *)gtx->data;
	output = malloc(gtx->width * gtx->height * 4);
	pos = 0;

	for (y = 0; y < gtx->height; y++) {
		for (x = 0; x < gtx->width; x++) {
			pos = (y & ~15) * gtx->width;
			pos ^= (x & 3);
			pos ^= ((x >> 2) & 1) << 3;
			pos ^= ((x >> 3) & 1) << 6;
			pos ^= ((x >> 3) & 1) << 7;
			pos ^= (x & ~0xF) << 4;
			pos ^= (y & 1) << 2;
			pos ^= ((y >> 1) & 7) << 4;
			pos ^= (y & 0x10) << 4;
			pos ^= (y & 0x20) << 2;
			output[y * gtx->width + x] = swapRB(source[pos]);
		}
	}

	writeFile(f, gtx->width, gtx->height, (uint8_t *)output, isPNG);

	free(output);
}

void export_DXT5(GTXData *gtx, FILE *f, int isPNG) {
	uint32_t pos, x, y;
	uint32_t *output, outValue;
	uint32_t blobWidth = gtx->width / 4, blobHeight = gtx->height / 4;
	uint8_t bits[4];
	__uint128_t *src = (__uint128_t *)gtx->data;
	__uint128_t *work = malloc(gtx->width * gtx->height);

	for (y = 0; y < blobHeight; y++) {
		for (x = 0; x < blobWidth; x++) {
			pos = (y >> 4) * (blobWidth * 16);
			pos ^= (y & 1);
			pos ^= (x & 7) << 1;
			pos ^= (x & 8) << 1;
			pos ^= (x & 8) << 2;
			pos ^= (x & 0x10) << 2;
			pos ^= (x & ~0x1F) << 4;
			pos ^= (y & 2) << 6;
			pos ^= (y & 4) << 6;
			pos ^= (y & 8) << 1;
			pos ^= (y & 0x10) << 2;
			pos ^= (y & 0x20);

			work[(y*blobWidth)+x] = src[pos];
		}
	}

	output = malloc(gtx->width * gtx->height * 4);

	for (y = 0; y < gtx->height; y++) {
		for (x = 0; x < gtx->width; x++) {
			fetch_2d_texel_rgba_dxt5(gtx->width, (uint8_t *)work, x, y, bits);

			outValue = (bits[ACOMP] << 24);
			outValue |= (bits[RCOMP] << 16);
			outValue |= (bits[GCOMP] << 8);
			outValue |= bits[BCOMP];

			output[(y * gtx->width) + x] = outValue;
		}
	}

	writeFile(f, gtx->width, gtx->height, (uint8_t *)output, isPNG);

	free(output);
	free(work);
}

int main(int argc, char **argv) {
	GTXData data;
	FILE *f;
	int result;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s [input.gtx] [output.png]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!(f = fopen(argv[1], "rb"))) {
		fprintf(stderr, "Cannot open %s for reading\n", argv[1]);
		return EXIT_FAILURE;
	}

	if ((result = readGTX(&data, f)) != 1) {
		fprintf(stderr, "Error %d while parsing GTX file %s\n", result, argv[1]);
		fclose(f);
		return EXIT_FAILURE;
	}
	fclose(f);

	if (!(f = fopen(argv[2], "wb"))) {
		fprintf(stderr, "Cannot open %s for writing\n", argv[2]);
		return EXIT_FAILURE;
	}

	printf("Width: %d - Height: %d - Format: 0x%x - Size: %d (%x)\n", data.width, data.height, data.format, data.dataSize, data.dataSize);

	data.width = (data.width + 63) & ~63;
	data.height = (data.height + 63) & ~63;
	printf("Padded Width: %d - Padded Height: %d\n", data.width, data.height);

	if (data.format == 0x1A)
		export_RGBA8(&data, f, 1);
	else if (data.format == 0x33)
		export_DXT5(&data, f, 1);
	fclose(f);

	return EXIT_SUCCESS;
}

