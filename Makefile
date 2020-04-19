
# Compiler settings
CC=cc
CFLAGS = -std=c99 -Iinclude -Wall -O3

# Required object files
OBJFILES = bin/lzkn.o

.PHONY : lzkn clean test

# Main target
lzkn: bin/lzkn

# Target: test
test: bin/test
	./bin/test

# Target: clean
clean :
	-rm -f $(OBJFILES)
	-rm -f bin/lzkn
	-rm -f bin/test

# Binary files rules
bin/lzkn: main.c $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o bin/lzkn

bin/test: test.c $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o bin/test

# Object files rules
bin/%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@ -c
