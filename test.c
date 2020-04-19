
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
const uint8_t _test4[] = "123456789";
const uint8_t _test5[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
						 "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
						 "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
						 "nisi ut aliquip ex ea commodo consequat.";

const testEntry testData[] = {
	MAKE_TEST_ENTRY(_test0),
	MAKE_TEST_ENTRY(_test1),
	MAKE_TEST_ENTRY(_test2),
	MAKE_TEST_ENTRY(_test3),
	MAKE_TEST_ENTRY(_test4),
	MAKE_TEST_ENTRY(_test5)
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

const testExecutorData testsExecutorsSequence[] = {
	{ .name = "Static tests", .function = runStaticTests },
	{ .name = "Fuzzy tests", .function = runFuzzyTests }
};

int main() {

	for (int i = 0; i < sizeof(testsExecutorsSequence)/sizeof(testsExecutorsSequence[0]); ++i) {
		const testExecutorData * testExecutor = &testsExecutorsSequence[i];

		printf("Running %s...\n", testExecutor->name);

		int result = (*testExecutor->function)();

		if (result != 0) {
			return result;
		}
	}

	return 0;

}

