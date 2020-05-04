
# Compression Tools for Konami Compression variant 1 (LZKN1)

**IMPORTANT NOTICE:** 
These tools are aimed for Sega Mega-Drive / Genesis ROM hackers, homebrew developers and researchers. This *is not* a modern general-purpose compression library for your everyday use.

Konami compression algorithms are several internally similar compression formats based on the LZSS data compression scheme. These were found in several Sega Mega-Drive/Genesis games developed by Konami in the 1990s.


## Introduction

This repository includes my own tools for compressing, decompressing and recompressing data in Konami Compression format Variant 1 (__LZKN1__ for short), as well as the disassembly of the original decompression code in the M68K assembly language, found in Konami games.

My implementation of the compressor is confirmed to perform _even better_ in compression ratio than the original compressor used by Konami. 
Hence, the _recompression_ functionality was introduced.



## Historical notice

They are based on my research and reverse-engineering of a Konami game, _Contra Hard Corps_, for the Sega Mega Drive / Genesis from back in 2013. The tools I developed, however, were not released until 2020, when I decided to completely overhaul the compressor implementation to achieve the desired performance and stability.

Pior to releasing these tools, I found out that the Konami formats were already reverse-engineered and cracked by other people independently, and I only discovered one of the several compression variants used by Konami. Thus for the sake of simplicity and compatibility, I decided to re-use previously established name of the format, _LZKN1_ (my implementation originally refereed to it just as _"Konami compression"_), and mention the other formats (although not discovered by me) in the project's description.

This implementation may be further expanded to support other Konami compression variants (namely LZKN2, LKN3) in the future.

## LZKN1 format description

The compressed data starts with a ***header***, followed by the ***compressed stream***:

	<data> = <header> <compressed stream>

The 2-byte ***header*** merely encodes uncompressed data size, in bytes. Size is a 16-bit Big-endian integer, meaning up to 64 kb worth of data may be compressed.

	<header> = <uncompressed size>

The ***compressed stream*** consists of ***description fields***, followed by several ***data blocks***.

***Description fields*** are 8-bit bitfields that encode whether the following ***data blocks*** are *uncompressed bytes* or *command*:

* If bit == 0, the next ***data block*** to read is considered an *uncompressed byte*. One byte is read from the compressed stream. This byte is appended to the uncompressed stream directly;
* If bit == 1, the next ***data block*** to read is considered a *command*. One or two bytes are read from the compressed stream, depending on the first bits of the command itself. The *command* usually initiate copying of the previously uncompressed data, if this data is going to be repeated.

The ***description field*** bits are processed _right to left_ (least significant bits first).
When all bits are exhausted (and _after_ the last corresponding ***data block*** is fetched), the next byte in the compressed stream is read and considered a new *description field*.

Thus, the **compressed stream** may look something like this:

	<compressed stream> = 
		<description field 1> <data block 1> <data block 2> ... <data block 8> 
		<description field 2> <data block 9> <data block 10> ...

If ***data block*** represents a *command*, the following formats are possible, depending on the 2 most significant bits in the first byte read (formats below er represented as bitfields for clarity):

	%00011111			Stop flag

	%0ddnnnnn dddddddd	Uncompressed stream copy (Mode 1)
						dd..dddddddd = Displacement (0..-1023)
						nnnnn = Number of bytes to copy (3..33)
	
	%10nndddd			Uncompressed stream copy (Mode 2)
						nn = Number of bytes to copy (2..5)
						dddd = Displacement (0..-15)
				
	%11nnnnnn			Compressed stream copy
						nnnnnn = Number of bytes to copy (8..71)

As seen above, if *command* with a value of `$1F` (`%00011111`) is met, the decompression stops immediately. Each compressed stream should include the stop command at the end.


## Repository structure

This repository includes:

* Compression and decompression function headers and source files (see __include/__ directory), for use in other C/C++ projects;
* The disassembled source code of original decompressor used by Konami in the M68K assembly language (see __m68k/__ directory);
* Source code for `lzkn`, a command-line tool, used to perform compression, decompression and recompression on the individual files. For more information, see [How to use](#How-to-use) section;
* `test.c`, an automated testing suite used through the development to ensure implementation performance and stability.


## Building from the source code and installation

The implementation is written in pure C, doesn't have any dependencies other than the standard C library and includes automatic tests.

It can be easily built under __Linux__ and __MacOS__ via _Makefile_. Native __Windows__ builds are also possible, but POSIX-like environment is necessary (possibly, MinGW or Cygwin) to get the existing build system working.

To build from the source, just run:

	make

The resulting binaries, if the build succeeds, will appear in the __bin/__ directory.

If you wish to additionally test the compression integrity and performance on your system, run: 

	make test

These tests were introduced solely for debugging purposes during the development; they should always succeed. If you see any failures during their execution, please report an issue immediately.

To install compression tools on your system, run the following command as root (or use `sudo` on Debian-based systems):

	make install

The binaries will be copied to the `/usr/local/bin` directory by default (you may override this by passing your own `PREFIX` to `make install`, please consult the GNU `make` manual for more information).


## How to use

This repository builds and installs `lzkn`, a command-line tool that accepts the following arguments:

	lzkn [-c|-d|-r] input_path [output_path]

The first optional argument, if present, selects operation mode:
* `-c`	Compress `<input_path>`;
* `-d`	Decompress `<input_path>`;
* `-r`	Recompress `<input_path>` (decompress and compress again).

If flag is ommited, _compression mode_ is assumed.

If `[output_path]` is not specified, it's set as follows:
* `.lzkn1` extension is appended to the `<input_path>` in compression mode;
* `.unc` extension is appended to the `<input_path>` in decompression mode;
* In recompression mode, the same filepath is used.

### Examples

The following command compresses `file.bin` to `file.bin.lzkn1`:

	lzkn file.bin

In decompression mode, the same `file.bin.lzkn1` may be decompressed to `file.bin.lzkn1.unc` by default:

	lzkn -d file.bin.lzkn1

Recompress `old-compressed.bin` to itself:

	lzkn -r old-compressed.bin

Compress `uncompressed.bin` to `compressed.bin`:

	lzkn -c uncompressed.bin compressed.bin

Please note that `-c` is the default mode and may be omitted.


# Licensing

The code is distributed under the MIT license. Please see the `LICENSE` file for more information.
