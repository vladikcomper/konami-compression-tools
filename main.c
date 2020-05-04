
/* ================================================================================= *
 * Konami's LZSS variant 1 (LZKN1) compressor/decompressor							 *
 * Command line interface implementation											 *
 *																					 *
 * (c) 2020, Vladikcomper															 *
 * ================================================================================= */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Include compress/decompress functions */
#include "lzkn.h"

/* Helper macros */
#define FAIL_IF_NONZERO(expr)	if ((expr) != 0) return -1;
#define FAIL_IF_ZERO(expr)		if (!(expr)) return -1;

/* Helper types */
typedef enum { COMPRESS, DECOMPRESS, RECOMPRESS } operationMode;

/* Program usage */
const char * usageMessageStr = 
	"Konami's LZSS variant 1 (LZKN1) compressor/decompressor v.1.5.1\n"
	"(c) 2020, Vladikcomper\n"
	"\n"
	"USAGE:\n"
	"	lzkn [-c|-d|-r] input_path [output_path]\n"
	"	\n"
	"	The first optional argument, if present, selects operation mode:\n"
	"		-c	Compress <input_path>;\n"
	"		-d	Decompress <input_path>;\n"
	"		-r	Recompress <input_path> (decompress and compress again).\n\n"
	"	If flag is ommited, compression mode is assumed."
	"	\n"
	"	If [output_path] is not specified, it's set as follows:\n"
	"		= <input_path> + \".lzkn1\" extension if in compression mode;\n"
	"		= <input_path> + \".unc\" extension if in decompression mode;\n"
	"		= <input_path> (w/o changes) in recompression mode.\n";

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

	char * result = malloc(str1_len + str2_len + 1);	// WARNING! Causes a tiny memory leak!
	strncpy(result, str1, str1_len + 1);
	strncpy(result + str1_len, str2, str2_len + 1);

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

/*
 * Parses command line arguments
 */
int parseAgrs(int argc, char ** argv, operationMode * mode, char ** inputPathPtr, char ** outputPathPtr) {

	// Handle "too few" arguments error
	if (argc < 2) {
		printUsage();
		fprintf(stderr, "ERROR: Too few arguments.\n");

		return 1;
	}

	// Handle "too many" arguments warning
	else if (argc > 4) {
		fprintf(stderr, "WARNING: Unexpected arguments found.\n");
	}

	// Check if the first argument specifies an operation mode flag ...
	if (argv[1][0] == '-') {
		const char * modeFlag = argv[1];
		int modeFlagSet = 0;

		*inputPathPtr = argv[2];						// the second argument specifies <input_path> then
		*outputPathPtr = (argc > 3) ? argv[3] : NULL;	// the third argument (if present) specifies <output_path>

		if (modeFlag[1] == 'c') {
			*mode = COMPRESS;
			modeFlagSet = 1;
		}
		else if (modeFlag[1] == 'd') {
			*mode = DECOMPRESS;
			modeFlagSet = 1;
		}
		else if (modeFlag[1] == 'r') {
			*mode = RECOMPRESS;
			modeFlagSet = 1;
		}
		
		if (!modeFlagSet || modeFlag[2] != 0x00) {
			fprintf(stderr, "ERROR: Unknown mode flag \"%s\". Only -c, -d and -r flags are supported.\n", modeFlag);
			return 2;
		}
	}

	// If the first argument is not an operation mode flag ...
	else {
		*inputPathPtr = argv[1];						// the fist argument specifies <input_path> then
		*outputPathPtr = (argc > 2) ? argv[2] : NULL;	// the third argument (if present) specifies <output_path>
	}

	return 0;

}

/*
 * The main function
 */
int main(int argc, char ** argv) {
		
	// Parse input arguments
	char * inputPath;
	char * outputPath;
	operationMode mode = COMPRESS;

	int argParseResult = parseAgrs(argc, argv, &mode, &inputPath, &outputPath);

	if (argParseResult != 0) {
		return argParseResult;
	}

	// If the output path is not specified ...
	if (!outputPath) {

		// If RECOMPRESSION mode, assume input and output are the same
		if (mode == RECOMPRESS) {
			outputPath = inputPath;
		}

		// Otherwise, output file name is the same as input, but with the extension appended
		else {
			const char * outputPathExtension = (mode == COMPRESS) ? ".lzkn1" : ".unc";

			outputPath = concatStrings(inputPath, outputPathExtension);
		}
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

	// If mode is DECOMPRESS or RECOMPRESS, decompress to the output buffer
	if (mode == DECOMPRESS || mode == RECOMPRESS) {
		lz_error decompressionResult =
			lzkn1_decompress(inBuff, inBuffSize, &outBuff, &outBuffSize);

		if (decompressionResult != 0) {
			fprintf(stderr, "Decompression failed with return code %X\n", decompressionResult);

			free(inBuff);
			free(outBuff);
			return decompressionResult;
		}
	}

	// If mode is COMPRESS or RECOMPRESS, compress to the output buffer
	if (mode == COMPRESS || mode == RECOMPRESS) {
		size_t compressedSize;

		if (mode == RECOMPRESS) {
			free(inBuff);

			inBuff = outBuff;
			inBuffSize = outBuffSize;
		}

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


	// Write down the resulting buffer to the output file
	{
		int outputWriteResult = writeFile(outputPath, outBuff, outBuffSize);
		
		if (outputWriteResult != 0) {
			fprintf(stderr, "ERROR: Unable to write to output file \"%s\" (code %d)\n", outputPath, outputWriteResult);

			free(inBuff);
			free(outBuff);
			return outputWriteResult;
		}
	}

	free(inBuff);
	free(outBuff);
	return 0;

}
