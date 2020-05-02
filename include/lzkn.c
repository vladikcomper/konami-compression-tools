
/* ================================================================================= *
 * Konami's LZSS variant 1 (LZKN1) compressor/decompressor							 *
 * API implementation																 *
 *																					 *
 * (c) 2020, Vladikcomper															 *
 * ================================================================================= */

#include <stdlib.h>		// for "malloc"
#include <stdio.h>		// for "size_t", "printf" etc
#include <stdint.h>		// for "uint8_t" etc.

#include "lzkn.h"

/**
 * Compression function
 * 
 * Returns size of the compressed buffer
 */
lz_error lzkn1_compress(const uint8_t *inBuff, const size_t inBuffSize, uint8_t *outBuff, size_t outBuffSize, size_t* compressedSize) {

	lz_error result = 0;					// default return value (success)

	const int32_t sizeWindow = 0x3FF;		// sliding window (displacement) maximum size
	const int32_t sizeCopy = 0x21;			// maximum size of the bytes (sting) to copy

	#define FLAG_COPY_MODE1		0x00
	#define FLAG_COPY_MODE2		0x80
	#define FLAG_COPY_RAW		0xC0

	int32_t inBuffPos = 0;					// input buffer position
	int32_t inBuffLastCopyPos = 0;			// position of the last copied byte to the uncompressed stream
	int32_t outBuffPos = 0;					// output buffer position

	// Define basic helper macros
	#define MIN(a,b)	((a) < (b) ? (a) : (b))
	#define MAX(a,b)	((a) > (b) ? (a) : (b))

	// Description field helpers
	uint8_t * descFieldPtr = NULL;
	int descFieldCurrentBit = 0;

	#define BYTE_FLAG	1
	#define BYTE_RAW	0

	#define PUSH_DESC_FIELD_BIT(BIT)	\
		if (descFieldPtr == NULL) { \
			descFieldPtr = outBuff + outBuffPos++; \
			*descFieldPtr = BIT; \
			descFieldCurrentBit = 1; \
		} \
		else { \
			*descFieldPtr |= (BIT << descFieldCurrentBit++); \
			if (descFieldCurrentBit >= 8) { \
				descFieldPtr = NULL; \
			} \
		}

	// Put uncompressed size ...
	outBuff[outBuffPos++] = inBuffSize >> 8;
	outBuff[outBuffPos++] = inBuffSize & 0xFF;

	// Main compression loop ...
	while ((inBuffPos < inBuffSize) && (outBuffPos < outBuffSize)) {

		// Attempt to find the longest matching string in the input buffer ...
		int32_t matchStrPos = -1;
		int32_t matchStrSize = 0;
		const int32_t matchStrMaxCopy = MIN(sizeCopy, inBuffSize - inBuffPos);
		const int32_t matchWindowBoundary = MAX(inBuffPos - sizeWindow, 0);

		for (int32_t matchPos = inBuffPos - 1; matchPos >= matchWindowBoundary; --matchPos) {
			int32_t currentMatchSize = 0;

			while (inBuff[matchPos + currentMatchSize] == inBuff[inBuffPos + currentMatchSize]) {
				++currentMatchSize;

				if (currentMatchSize >= matchStrMaxCopy) {
					break;
				}
			}

			if (currentMatchSize > matchStrSize) {
				matchStrSize = currentMatchSize;
				matchStrPos = matchPos;
			}
		}

		int32_t matchStrDisp = inBuffPos - matchStrPos;	// matching string displacement

		// Now, decide on the compression mode ...
		int32_t queuedRawCopySize = inBuffPos - inBuffLastCopyPos;
		uint8_t suggestedMode = 0xFF;

		if ((matchStrSize >= 2) && (matchStrSize <= 5) && (matchStrDisp < 16)) {		// Uncompressed stream copy (Mode 2)
			suggestedMode = FLAG_COPY_MODE2;
		}
		else if ((matchStrSize >= 3)) {
			suggestedMode = FLAG_COPY_MODE1;
		}

		// Initiate raw bytes transfer until the current location in the following cases:
		//	-- If the copy mode was suggested, but there are raw bytes queued, render them first
		//	-- If the raw bytes queue is too large to store in a single flag (FLAG_COPY_RAW)
		//	-- If the input buffer exhausted and should be flushed immidiately
		if (((suggestedMode != 0xFF) && (queuedRawCopySize >= 1)) 
				|| (queuedRawCopySize >= 0x47 || (inBuffPos + 1 == inBuffSize))) {

			// If on the last cycle, correct transfer size ...
			if ((inBuffPos + 1 == inBuffSize)) {
				queuedRawCopySize = inBuffSize - inBuffLastCopyPos;
			}

			// When transferring more than 8 bytes, use "FLAG_COPY_RAW" flag instead of plain bit fields
			if (queuedRawCopySize > 8) {
				PUSH_DESC_FIELD_BIT(BYTE_FLAG);					// set the following data as a flag
				outBuff[outBuffPos++] = (FLAG_COPY_RAW) | (queuedRawCopySize - 8);

				for (int32_t i = 0; i < queuedRawCopySize; ++i) {
					outBuff[outBuffPos++] = inBuff[inBuffLastCopyPos++];
				}
			}

			// If less than 8 bytes should be transferred, store raw bytes info in the description field directly ...
			else {
				for (int32_t i = 0; i < queuedRawCopySize; ++i) {
					PUSH_DESC_FIELD_BIT(BYTE_RAW);
					outBuff[outBuffPos++] = inBuff[inBuffLastCopyPos++];
				}
			}
		}

		// Now, render compression modes, if any was suggested ...
		if (suggestedMode == FLAG_COPY_MODE1) {
			PUSH_DESC_FIELD_BIT(BYTE_FLAG);
			outBuff[outBuffPos++] = (FLAG_COPY_MODE1) | ((matchStrDisp & 0x300) >> 3) | (matchStrSize - 3);
			outBuff[outBuffPos++] = (matchStrDisp & 0xFF);
			inBuffPos += matchStrSize;
			inBuffLastCopyPos = inBuffPos;
		}

		else if (suggestedMode == FLAG_COPY_MODE2) {
			PUSH_DESC_FIELD_BIT(BYTE_FLAG);
			outBuff[outBuffPos++] = (FLAG_COPY_MODE2) | (matchStrDisp & 0xF) | ((matchStrSize - 2) << 4);
			inBuffPos += matchStrSize;
			inBuffLastCopyPos = inBuffPos;
		}
		else {
			inBuffPos += 1;
		}

	}

	// Detect buffer overflow errors
	if (inBuffPos > inBuffSize) {
		result |= LZ_INBUFF_OVERFLOW;
	}
	if (outBuffPos > outBuffSize) {
		result |= LZ_OUTBUFF_OVERFLOW;
	}

	// Finalize compression buffer
	PUSH_DESC_FIELD_BIT(BYTE_FLAG);
	outBuff[outBuffPos++] = 0x1F;

	// Return compressed data size
	*compressedSize = outBuffPos;

	return result;

}



/**
 * Compression function
 * 
 * Returns size of the compressed buffer
 */
lz_error lzkn1_decompress(uint8_t *inBuff, size_t inBuffSize, uint8_t** outBuffPtr, size_t *decompressedSize) {
	
	lz_error result = 0;

	int32_t inBuffPos = 0;
	int32_t outBuffPos = 0;

	#define FLAG_COPY_MODE1		0x00
	#define FLAG_COPY_MODE2		0x80
	#define FLAG_COPY_RAW		0xC0

	#define BYTE_FLAG	1
	#define BYTE_RAW	0

	// Get uncompressed buffer size and allocate the buffer
	size_t outBuffSize = (inBuff[inBuffPos] << 8) + inBuff[inBuffPos+1];
	uint8_t *outBuff = malloc(outBuffSize);
	inBuffPos += 2;

	if (!outBuff) {
		result |= LZ_ALLOC_FAILED;
		return result;
	}

	uint8_t done = 0;
	uint8_t descField;
	int8_t descFieldRemainingBits = 0;

	while (!done && (outBuffPos <= outBuffSize)) {

		// Fetch a new description field if necessary
		if (!descFieldRemainingBits--) {
			descField = inBuff[inBuffPos++];
			descFieldRemainingBits = 7;
		}

		// Get successive description field bit, rotate the field
		uint8_t bit = descField & 1;
		descField = descField >> 1;

		// If bit indicates a raw byte ("BYTE_RAW") in the stream, copy it over ...
		if (bit == BYTE_RAW) {
			outBuff[outBuffPos++] = inBuff[inBuffPos++];
		}

		// Otherwise, it indicates a flag ("BYTE_FLAG"), so decode it ...
		else {
			uint8_t flag = inBuff[inBuffPos++];

			if (flag == 0x1F) {
				done = 1;
			}
			else if (flag >= FLAG_COPY_RAW) {
				int32_t copySize = (int32_t)flag - (int32_t)FLAG_COPY_RAW + 8;

            	for (int32_t i = 0; i < copySize; ++i) {
					outBuff[outBuffPos++] = inBuff[inBuffPos++];
            	}
			}
			else if (flag >= FLAG_COPY_MODE2) {
				int32_t copyDisp = ((int32_t)flag & 0xF);
				int32_t copySize = ((int32_t)flag >> 4) - 6;

            	for (int32_t i = 0; i < copySize; ++i) {
					outBuff[outBuffPos] = outBuff[outBuffPos - copyDisp];
					outBuffPos++;
            	}
			}
			else {	// "FLAG_COPY_MODE1"
            	int32_t copyDisp = (inBuff[inBuffPos++]) | (((int32_t)flag << 3) & 0x300);
            	int32_t copySize = (flag & 0x1F) + 3;

            	for (int32_t i = 0; i < copySize; ++i) {
					outBuff[outBuffPos] = outBuff[outBuffPos - copyDisp];
					outBuffPos++;
            	}
			}
		}

	}

	// Detect buffer errors
	if (outBuffPos < outBuffSize) {
		result |= LZ_OUTBUFF_UNDERFLOW;
	}
	else if (outBuffPos > outBuffSize) {
		result |= LZ_OUTBUFF_OVERFLOW;
	}

	if (inBuffPos < inBuffSize) {
		result |= LZ_INBUFF_UNDERFLOW;
	}
	else if (inBuffPos > inBuffSize) {
		result |= LZ_INBUFF_OVERFLOW;
	}

	// Return pointer to the uncompressed buffer and its size
	*outBuffPtr = outBuff;
	*decompressedSize = outBuffSize;

	return result;
}
