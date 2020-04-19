
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lzkn.h"

/* Helper macros */
#define FAIL_IF_NONZERO(expr)	if ((expr) != 0) return -1;
#define FAIL_IF_ZERO(expr)		if (!(expr)) return -1;

/*
 * USAGE EXAMPLE:
 *	lzkn [-d|-c] input_file [output_file]
 *
 * 	-d 	Indicates decompression mode
 *	-c	Indicates compression mode
 *
 */

const char * usageMessageStr = 
	"Konami's LZ77 variant (LZKN1) compressor/decompressor\n"
	"(c) 2020, Vladikcomper\n"
	"\n"
	"USAGE:\n"
	"	lzkn [-d|-c] input_file [output_file]\n"
	"	\n"
	"	[-d|-c] selects operation mode: either decompression (-d) or \n"
	"	compression (-c). If ommited the compression mode is assumed.\n"
	"	\n"
	"	If [output_file] is not specified, it's constructed as follows:\n"
	"		- <input_file> + \".lzkn1\" extension in compression mode;\n"
	"		- <input_file> + \".unc\" extension in decompression mode.\n";

/*
 * Prints program usage
 */
void printUsage() {
	puts(usageMessageStr);
}

/*
 * Helper function to concat two given strings
 */
char * concatStrings(const char * str1, const char * str2) {
	const size_t str1_len = strlen(str1);
	const size_t str2_len = strlen(str2);

	char * result = malloc(str1_len + str2_len + 1);
	strncpy(result, str1, str1_len);
	strncpy(result + str1_len, str2, str2_len);

	return result;
}

/*
 * Reads a given file into the buffer
 */
int readFile(const char * path, uint8_t ** bufferPtr, size_t * bufferSize) {
	FILE * input;

	FAIL_IF_ZERO(input = fopen(path, "rb"));
	FAIL_IF_NONZERO(fseek(input, 0, SEEK_END));
	size_t inBuffSize = ftell(input);
	FAIL_IF_NONZERO(fseek(input, 0, SEEK_SET));

	uint8_t * inBuff;
	FAIL_IF_ZERO(inBuff = malloc(inBuffSize));

	if (fread(inBuff, 1, inBuffSize, input) != inBuffSize) {
		return -2;
	}

	fclose(input);

	*bufferPtr = inBuff;
	*bufferSize = inBuffSize;

	return 0;
}

/*
 * Writes the specified buffer to the file
 */
int writeFile(const char * path, uint8_t * buffer, size_t bufferSize) {
	FILE * output;

	FAIL_IF_ZERO(output = fopen(path, "wb"));

	if (fwrite(buffer, 1, bufferSize, output) != bufferSize) {
		return -2;
	}

	fclose(output);

	return 0;
}

int main(int argc, char ** argv) {
	
	enum { COMPRESSION, DECOMPRESSION } mode = COMPRESSION;

	if (argc < 2) {
		printUsage();
		fprintf(stderr, "ERROR: Too few arguments.\n");

		return 1;
	}

	// Check if the first argument specifies mode (-d|-c)
	const char * modeFlag;
	char * inputPath = argv[1];
	char * outputPath = (argc > 2) ? argv[2] : NULL;

	if (argv[1][0] == '-') {
		modeFlag = argv[1];
		inputPath = argv[2];
		outputPath = (argc > 3) ? argv[3] : NULL;

		if (modeFlag[1] == 'c') {
			mode = COMPRESSION;
		}
		else if (modeFlag[1] == 'd') {
			mode = DECOMPRESSION;
		}
		else {
			fprintf(stderr, "ERROR: Unknown mode flag \"%s\". Only -c and -d flags are supported.\n", modeFlag);
			return 2;
		}
	}

	// If the output path is not specified, re-use the input path and append an extension to it
	if (!outputPath) {
		const char * outputPathExtension = (mode == COMPRESSION) ? ".lzkn1" : ".unc";

		outputPath = concatStrings(inputPath, outputPathExtension);
	}

	// Initialize I/O buffers ...
	uint8_t * inBuff = NULL;
	size_t inBuffSize;
	uint8_t * outBuff = NULL;
	size_t outBuffSize;

	// Read input file to the buffer
	{
		int inputReadResult = readFile(inputPath, &inBuff, &inBuffSize);

		if (inputReadResult != 0) {
			fprintf(stderr, "ERROR: Unable to read the input file \"%s\" (code %d)\n", inputPath, inputReadResult);
			
			free(inBuff);
			return inputReadResult;
		}
	}

	// If mode is COMPRESSION, compress input buffer
	if (mode == COMPRESSION) {
		size_t compressedSize;

		outBuffSize = 0x10000;
		outBuff = malloc(outBuffSize);

		lz_error compressionResult = 
			lzkn1_compress(inBuff, inBuffSize, outBuff, outBuffSize, &compressedSize);

		if (compressionResult != 0) {
			fprintf(stderr, "Compression failed with return code %X\n", compressionResult);

			free(inBuff);
			free(outBuff);
			return compressionResult;
		}

		// Alter buffer size so only the compressed portion of the steam is written
		outBuffSize = compressedSize;

	}

	// If mode is DECOMPRESSION, decompress output buffer
	else if (mode == DECOMPRESSION) {
		lz_error decompressionResult =
			lzkn1_decompress(inBuff, inBuffSize, &outBuff, &outBuffSize);

		if (decompressionResult != 0) {
			fprintf(stderr, "Decompression failed with return code %X\n", decompressionResult);

			free(inBuff);
			free(outBuff);
			return decompressionResult;
		}
	}

	// 
	int outputWriteResult = writeFile(outputPath, outBuff, outBuffSize);
	
	if (outputWriteResult != 0) {
		fprintf(stderr, "ERROR: Unable to write to output file \"%s\" (code %d)\n", outputPath, outputWriteResult);

		free(inBuff);
		free(outBuff);
		return outputWriteResult;
	}

	free(inBuff);
	free(outBuff);
	return 0;

}
