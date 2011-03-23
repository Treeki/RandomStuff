#include <iostream>
#include <fstream>
using namespace std;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct LHContext {
	u8 buf1[0x800];
	u8 buf2[0x80];
};

u32 GetUncompressedSize(u8 *inData) {
	u32 outSize = inData[1] | (inData[2] << 8) | (inData[3] << 16);

	if (outSize == 0) {
		outSize = inData[4] | (inData[5] << 8) | (inData[6] << 16) | (inData[7] << 24);
	}

	return outSize;
}

u32 LoadLHPiece(u8 *buf, u8 *inData, u8 unk) {
	u32 r0, r4, r6, r7, r9, r10, r11, r12, r30;
	u32 inOffset, dataSize, copiedAmount;

	r6 = 1 << unk;
	r7 = 2;
	r9 = 1;
	r10 = 0;
	r11 = 0;
	r12 = r6 - 1;
	r30 = r6 << 1;

	if (unk <= 8) {
		r6 = inData[0];
		inOffset = 1;
		copiedAmount = 1;
	} else {
		r6 = inData[0] | (inData[1] << 8);
		inOffset = 2;
		copiedAmount = 2;
	}

	dataSize = (r6 + 1) << 2;
	goto startLoop;
loop:
	r6 = unk + 7;
	r6 = (r6 - r11) >> 3;

	if (r11 < unk) {
		for (int i = 0; i < r6; i++) {
			r4 = inData[inOffset];
			r10 <<= 8;
			r10 |= r4;
			copiedAmount++;
			inOffset++;
		}
		r11 += (r6 << 3);
	}

	if (r9 < r30) {
		r0 = r11 - unk;
		r9++;
		r0 = r10 >> r0;
		r0 &= r12;
		buf[r7] = r0 >> 8;
		buf[r7+1] = r0 & 0xFF;
		r7 += 2;
	}

	r11 -= unk;
startLoop:
	if (copiedAmount < dataSize)
		goto loop;

	return copiedAmount;
}

void UncompressLH(u8 *inData, u8 *outData, LHContext *context) {
	u32 outIndex = 0;
	u32 outSize = inData[1] | (inData[2] << 8) | (inData[3] << 16);
	inData += 4;

	if (outSize == 0) {
		outSize = inData[0] | (inData[1] << 8) | (inData[2] << 16) | (inData[3] << 24);
		inData += 4;
	}

	inData += LoadLHPiece(context->buf1, inData, 9);
	inData += LoadLHPiece(context->buf2, inData, 5);

	// this is a direct conversion of the PPC ASM, pretty much
	u32 r0 = 0x10;
	u32 r3 = 0x100;
	u32 r4 = 0;
	u32 r5 = 0;
	u32 r6 = 0;
	u32 r7, r8, r9, r10, r11, r12, r25;
	bool flag;

	goto startOrContinueLoop;

loop:
	r12 = 2; // Used as an offset into context->buf1
	r7 = r4; // Used as an offset into inData

_5BA0:
	if (r6 == 0) {
		r5 = inData[r7];
		r6 = 8;
		r4++;
		r7++;
	}

	r11 = (context->buf1[r12] << 8) | context->buf1[r12 + 1];
	r8 = r5 >> (r6 - 1);
	r6--;

	r9 = r8 & 1;
	r10 = r11 & 0x7F;
	r8 = r3 >> r9; // sraw?
	r8 = r11 & r8;
	flag = (r8 == 0);
	r8 = (r10 + 1) << 1;
	r9 += r8;

	if (flag) {
		r12 &= ~3;
		r8 = r9 << 1;
		r12 += r8;
		goto _5BA0;
	} else {
		r8 = r12 & ~3; // offset into buf1
		r7 = r9 << 1;
		r7 = (context->buf1[r8+r7] << 8) | context->buf1[r8+r7+1];
	}

	if (r7 < 0x100) {
		outData[outIndex] = (u8)r7;
		outIndex += 1;
		goto startOrContinueLoop;
	}

	// block copy?
	r7 &= 0xFF;
	r25 = 2; // used as an offset into context->buf2
	r7 += 3;
	r7 &= 0xFFFF; // r7 is really an ushort, probably
	r8 = r4; // used as an offset into inData

_5C30:
	if (r6 == 0) {
		r5 = inData[r8];
		r6 = 8;
		r4++;
		r8++;
	}

	r12 = (context->buf2[r25] << 8) | context->buf2[r25+1];
	r9 = r5 >> (r6 - 1);
	r6--;
	r10 = r9 & 1;
	r11 = r12 & 7;
	r9 = r0 >> r10; // sraw
	r9 = r12 & r9;
	flag = (r9 == 0);
	r9 = r11 + 1;
	r9 <<= 1;
	r10 += r9;

	if (flag) {
		r25 &= ~3;
		r9 = r10 << 1;
		r25 += r9;
		goto _5C30;
	} else {
		r9 = r25 & ~3;
		r8 = r10 << 1;
		r11 = (context->buf2[r9+r8] << 8) | context->buf2[r9+r8+1];
	}

	r10 = 0;
	if (r11 != 0) {
		r8 = r4; // offset into inData
		r10 = 1;

		goto _5CE0;
	_5CB0:
		r10 = (r10 << 1) & 0xFFFF;
		if (r6 == 0) {
			r5 = inData[r8];
			r6 = 8;
			r4++;
			r8++;
		}

		r6--;
		r9 = r5 >> r6;
		r9 &= 1;
		r10 |= r9;
	_5CE0:
		r11--;
		r9 = r11 & 0xFFFF;
		if (r9 != 0)
			goto _5CB0;
	}

	if ((outIndex + r7) > outSize) {
		r7 = outSize - outIndex;
		r7 &= 0xFFFF;
	}

	r9 = r10 + 1;
	r8 = outIndex; // offset into outData
	r10 = r9 & 0xFFFF;
	goto startBlockCopyLoop;
blockCopyLoop:
	r9 = outIndex - r10;
	outIndex++;
	outData[r8] = outData[r9];
	r8++;
startBlockCopyLoop:
	r9 = r7 & 0xFFFF;
	r7--;
	if (r9 != 0)
		goto blockCopyLoop;


startOrContinueLoop:
	if (outIndex < outSize)
		goto loop;
}



int main(int argc, char **argv) {
	u8 *inBuf, *outBuf;
	int inLength, outLength;
	LHContext context;
	
	// sanity check -- todo, more error checking..
	if (argc < 3) {
		cout << "no filenames specified. to run: " << argv[0] << " CompFile.bin UncompFile.bin" << endl;
		return 1;
	}

	//ifstream loadFile("SportsMix/c00B00.mdl", ifstream::in|ifstream::binary);
	ifstream loadFile(argv[1], ifstream::in|ifstream::binary);

	loadFile.seekg(0, ios::end);
	inLength = loadFile.tellg();
	loadFile.seekg(0, ios::beg);

	inBuf = new u8[inLength];
	loadFile.read((char*)inBuf, inLength);
	loadFile.close();

	outLength = GetUncompressedSize(inBuf);
	outBuf = new u8[outLength];
	UncompressLH(inBuf, outBuf, &context);

	//ofstream outFile("SportsMix/c00B00.mdl_decomp", ifstream::out|ifstream::binary);
	ofstream outFile(argv[2], ifstream::out|ifstream::binary);
	outFile.write((char*)outBuf, outLength);
	outFile.close();

	delete inBuf;
	delete outBuf;
}

