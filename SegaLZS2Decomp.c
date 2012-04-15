#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef unsigned char u8;

static u8 decBuffer[0x1000];
static u8 decBuffer2[0x32000];

int main(int argc, const char **argv) {
	if (argc != 3) {
		printf("usage: %s <source> <destination>\n", argv[0]);
		return 1;
	}

	// load src file
	FILE *fSrc = fopen(argv[1], "rb");

	if (fSrc == NULL) {
		printf("could not open source file: %s\n", argv[1]);
		return 1;
	}

	fseek(fSrc, 0, SEEK_END);
	int srcSize = ftell(fSrc);
	rewind(fSrc);

	printf("source size: 0x%x bytes\n", srcSize);

	u8 *srcBuffer = malloc(srcSize);
	fread(srcBuffer, 1, srcSize, fSrc);

	fclose(fSrc);

	printf("successfully read\n");

	// prepare dest file
	FILE *fDest = fopen(argv[2], "wb");

	if (fDest == NULL) {
		printf("could not open destination file: %s\n", argv[2]);
		return 1;
	}

	// read header
	u8 *src = srcBuffer + 4;
	int decompSize = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];

	printf("output size: 0x%x bytes\n", decompSize);

	src += 4;

	u8 *buf2ptr = decBuffer2;
	u8 *buf2end = &decBuffer2[decompSize];

	// uhhhhh, what a pointless check
	if (buf2ptr < buf2end) {
		// magic happens here

		u8 *realBuf2End = &decBuffer2[sizeof(decBuffer2)];

		int currentControlBit = 0;
		int controlByte = 0;

		int dbIndex = 0;

		do {
			// figure out remaining space
			int remainingSpace = (int)(realBuf2End - buf2ptr);

			if (currentControlBit == 0) {
				controlByte = *src;
				currentControlBit = 8;
				src++;
			}

			if (remainingSpace < 0x12) {
				// If we're running out of space, start over in this buffer
				printf("0x%x bytes written\n", (int)(buf2ptr - decBuffer2));
				fwrite(decBuffer2, (int)(buf2ptr - decBuffer2), 1, fDest);

				// the assembly does some weird stuff involving the output buffer
				// but we don't have it here, we're just writing to files --
				// so we don't need that. I'm not even sure what it's for,
				// I'm pretty sure this should serve the same purpose.
				int howMuchIsLeft = (int)(buf2end - buf2ptr);
				printf("0x%x bytes remain\n", howMuchIsLeft);
				buf2end = &decBuffer2[howMuchIsLeft];
				buf2ptr = decBuffer2;
			}

			// now process the control bit
			if (controlByte & 0x80) {
				// CONTROL BIT SET

				unsigned short value = (src[0] << 8) | src[1];
				src += 2;

				u8 *endCopyAt = buf2ptr + (value >> 12) + 3;

				if (endCopyAt >= buf2ptr) {
					int dunno = value & 0xFFF;
					u8 *dontReallyKnow = decBuffer;

					do {
						u8 thisByte = decBuffer[dunno];
						dunno = (dunno + 1) & 0xFFF;

						decBuffer[dbIndex] = thisByte;
						*buf2ptr = thisByte;

						buf2ptr++;
						dbIndex = (dbIndex + 1) & 0xFFF;
					} while (buf2ptr < endCopyAt);
				}

			} else {
				u8 thisByte = *src;
				thisByte ^= dbIndex;

				decBuffer[dbIndex] = thisByte;

				dbIndex = (dbIndex + 1) & 0xFFF;

				src++;

				*buf2ptr = thisByte;
				buf2ptr++;
			}

			controlByte <<= 1;

			currentControlBit--;
		} while (buf2ptr < buf2end);
	}

	int remainingAmount = (int)(buf2ptr - decBuffer2);

	if (remainingAmount > 0)
		fwrite(decBuffer2, remainingAmount, 1, fDest);
	fclose(fDest);

	free(srcBuffer);

	printf("0x%x bytes written from last chunk\n", (int)(buf2ptr - decBuffer2));
	return 0;
}

