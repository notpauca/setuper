Compiler = clang++ -std=gnu++11
CCompiler = clang -std=gnu11
FLAGS = -O3 -Wall -Wextra -pedantic -Wno-newline-eof -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-unused-function --include-directory=include/ --include-directory=/usr/local/include/ --include-directory=/opt/homebrew/include/ --library-directory=/usr/local/lib/ --library-directory=/opt/homebrew/lib/ --library-directory=lib/
FUSEFLAGS = -DFUSE_USE_VERSION=28 -D_FILE_OFFSET_BITS=64 -DHAVE_OSBYTEORDER_H -DHAVE_BIRTHTIME -DHAVE_STAT_FLAGS -DHAVE_STAT_BLKSIZE -DHAVE_STAT_BLOCKS -DHAVE_PREAD -DUTF8PROC_EXPORTS -DHFSFUSE_VERSION_STRING=\"0.190-8b3cb2c\"


build: 
	mkdir bin
	mkdir lib
	$(CCompiler) $(FLAGS) $(FUSEFLAGS) HFS/HFSOperations.c -c
	$(Compiler) $(FLAGS) -c DMGConverting/*.cpp -v
	$(Compiler) $(FLAGS) $(FUSEFLAGS) -c src/setuper.cpp
	mv *.o lib/
	cp DMGConverting/liblzfse.dylib bin/
	$(Compiler) $(FLAGS) $(FUSEFLAGS) -lfuse -lhfs -lhfsuser -lublio -lutf8proc -pthread -lz -lbz2 -lcurl -llzfse lib/* -o bin/setuper
	cp src/programs.csv bin/programs.csv

clean: 
	rm -rf bin/
	rm -rf lib/
	rm -rf *.o