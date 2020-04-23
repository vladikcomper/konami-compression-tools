
/* ================================================================================= *
 * Konami's LZSS variant 1 (LZKN1) compressor/decompressor							 *
 * Testing suite																	 *
 *																					 *
 * (c) 2020, Vladikcomper															 *
 * ================================================================================= */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lzkn.h"

/* Various structures and macros to handle tests */
typedef int (*testFunction)();

typedef struct {
	size_t dataSize;
	const uint8_t* data;
} testEntry;

typedef struct {
	const char * name;
	testFunction function;
} testExecutorData;

#define MAKE_TEST_ENTRY(ptr) { .dataSize = sizeof(ptr), .data = ptr }


/* Define test data */
const uint8_t _test0[] = { 0 };
const uint8_t _test1[] = { 1, 2, 3, 4 };
const uint8_t _test2[] = { 1, 1, 1, 1, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4 };
const uint8_t _test3[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
const uint8_t _test4[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 
						   21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 
						   39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 
						   57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 
						   75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 
						   93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 
						   109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 
						   124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 
						   139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 
						   154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 
						   169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 
						   184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 
						   199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 
						   214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 
						   229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 
						   244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255 };
const uint8_t _test5[] = "123456789";
const uint8_t _test6[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
						 "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
						 "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
						 "nisi ut aliquip ex ea commodo consequat.";

const testEntry testData[] = {
	MAKE_TEST_ENTRY(_test0),
	MAKE_TEST_ENTRY(_test1),
	MAKE_TEST_ENTRY(_test2),
	MAKE_TEST_ENTRY(_test3),
	MAKE_TEST_ENTRY(_test4),
	MAKE_TEST_ENTRY(_test5),
	MAKE_TEST_ENTRY(_test6)
};

/*
 * Helper function to print named data arrays
 */
void printData(const char* name, const uint8_t* data, const size_t size) {
	printf("%s (%ld):\n", name, size);
	
	for (size_t i = 0; i < size; ++i) {
		printf("%02x ", data[i]);
	}

	printf("\n");
}

/*
 *
 */
int validateDataRecompression(const uint8_t * sourceData, size_t sourceDataSize) {

	// Attempt compression on the source data
	const size_t compressedBufferSize = 0x10000;
	uint8_t * compressedData = malloc(compressedBufferSize);
	size_t compressedSize;

	lz_error compressionResult = 
		lzkn1_compress(sourceData, sourceDataSize, compressedData, compressedBufferSize, &compressedSize);

	if (compressionResult != 0) {
		printf("FAIL: lzkn1_compress() returned %X\n", compressionResult);
		return -1;
	}

	// Attempt decompression of the recently compressed data
	uint8_t * decompressedData = NULL;
	size_t decompressedSize;

	lz_error decompressionResult =
		lzkn1_decompress(compressedData, compressedSize, &decompressedData, &decompressedSize);

	if ((decompressionResult != 0) || (decompressedData == NULL)) {
		printf("FAIL: lzkn1_decompress() returned %X\n", decompressionResult);

		free(compressedData);
		free(decompressedData);
		return -1;
	}

	// Compare source data with the decompressed one
	#define RECOMPRESSION_OK				0
	#define RECOMPRESSION_SIZE_MISMATCH 	1
	#define RECOMPRESSION_DATA_MISMATCH 	2

	uint8_t recompressionResult = RECOMPRESSION_OK;

	if (sourceDataSize != decompressedSize) {
		recompressionResult = RECOMPRESSION_SIZE_MISMATCH;
	}
	else {
		for (size_t i = 0; i < sourceDataSize; ++i) {
			if (sourceData[i] != decompressedData[i]) {
				recompressionResult = RECOMPRESSION_DATA_MISMATCH;
				break;
			}
		}
	}

	if (recompressionResult == RECOMPRESSION_OK) {
		printf("PASS: Uncompressed: %ld, compressed: %ld\n", sourceDataSize, compressedSize);
	}
	else {
		const char * recompressionResultMessageData[] = { "", "SIZE MISMATCH", "DATA_MISMATCH" };

		printf("FAIL: %s\n", recompressionResultMessageData[recompressionResult]);
	}

	// If recompression failed, print out the results for analysis
	if (recompressionResult != RECOMPRESSION_OK) {
		printData("Source data", sourceData, sourceDataSize);
		printData("Compressed data", compressedData, compressedSize);
		printData("Decompressed data", decompressedData, decompressedSize);

		free(compressedData);
		free(decompressedData);
		return -2;
	}

	free(compressedData);
	free(decompressedData);
	return 0;

}

/*
 * Runs static (pre-defined) tests
 */
int runStaticTests() {

	for (size_t testId = 0; testId < sizeof(testData)/sizeof(testData[0]); ++testId ) {
		printf("TEST %ld... ", testId);

		const testEntry* entry = &testData[testId];

		int result = validateDataRecompression(entry->data, entry->dataSize);

		if (result != 0) {
			return result;
		}

	}

	return 0;
}

/*
 * Runs fuzzy tests
 */
int runFuzzyTests() {

	const size_t randomBufferSize = 0xFFFF;
	const size_t numTests = 100;

	uint8_t * randomBuffer = malloc(randomBufferSize);

	#define PROBABILITY(x) 		(rand() % 100) > (x)
	#define RAND_RANGE(a,b) 	(a) + (rand() % ((b) - (a)))
	#define MIN(a,b) 			(a) < (b) ? (a) : (b)

	for (size_t testId = 0; testId < numTests; ++testId) {
		printf("TEST %ld... ", testId);
		size_t buffPos;
		// Fill in random buffer
		for (buffPos = 0; buffPos < randomBufferSize; ) {
			const uint8_t byteValue = rand();
			const int byteShouldRepeat = PROBABILITY(30);

			if (byteShouldRepeat) {
				const size_t suggestedRepeatCount = RAND_RANGE(2,128);
				const size_t byteRepeatCount = MIN(suggestedRepeatCount, randomBufferSize - buffPos);

				for (size_t i = 0; i < byteRepeatCount; ++i) {
					randomBuffer[buffPos++] = byteValue;
				}
			}
			else {
				randomBuffer[buffPos++] = byteValue;
			}
		}

		// Test if the buffer recompresses properly
		int result = validateDataRecompression(randomBuffer, randomBufferSize);

		if (result != 0) {
			return result;
		}

	}

	free(randomBuffer);

	return 0;

}

/* Define test execution sequence ... */
const testExecutorData testsExecutorsSequence[] = {
	{ .name = "Static tests", .function = runStaticTests },
	{ .name = "Fuzzy tests", .function = runFuzzyTests }
};

/*
 * Main loop 
 */
int main() {

	for (int i = 0; i < sizeof(testsExecutorsSequence) / sizeof(testsExecutorsSequence[0]); ++i) {
		const testExecutorData * testExecutor = &testsExecutorsSequence[i];

		printf("Running %s...\n", testExecutor->name);

		int result = (*testExecutor->function)();

		if (result != 0) {
			return result;
		}
	}

	return 0;

}

