
/* ================================================================================= *
 * Konami's LZSS variant 1 (LZKN1) compressor/decompressor							 *
 * API implementation																 *
 *																					 *
 * (c) 2020, Vladikcomper															 *
 * ================================================================================= */


#include <stdio.h>
#include <stdint.h>

typedef uint32_t lz_error;

// Error codes
#define LZ_ALLOC_FAILED				0x1
#define LZ_INBUFF_OVERFLOW			0x2
#define LZ_INBUFF_UNDERFLOW			0x4
#define LZ_OUTBUFF_OVERFLOW			0x8
#define LZ_OUTBUFF_UNDERFLOW		0x10

lz_error lzkn1_compress(
	const uint8_t *inBuff, 
	const size_t inBuffSize, 
	uint8_t *outBuff, 
	size_t outBuffSize, 
	size_t *compressedSize
);

lz_error lzkn1_decompress(
	uint8_t *inBuff, 
	size_t inBuffSize, 
	uint8_t **outBuffPtr, 
	size_t *decompressedSize
);
