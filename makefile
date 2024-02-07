Compiler = clang++ -std=gnu++11
CCompiler = clang -std=gnu11
FLAGS = -O3 -Wall -Wextra -pedantic -Wno-newline-eof -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-unused-function -Wno-unused-command-line-argument --include-directory=include/ --include-directory=/usr/local/include/ --include-directory=/opt/homebrew/include/ --library-directory=/usr/local/lib/ --library-directory=/opt/homebrew/lib/ --library-directory=lib/
FUSEFLAGS = -DFUSE_USE_VERSION=28 -D_FILE_OFFSET_BITS=64 -DHAVE_OSBYTEORDER_H -DHAVE_BIRTHTIME -DHAVE_STAT_FLAGS -DHAVE_STAT_BLKSIZE -DHAVE_STAT_BLOCKS -DHAVE_PREAD -DUTF8PROC_EXPORTS -DHFSFUSE_VERSION_STRING=\"0.190-8b3cb2c\"


build: 
	mkdir bin
	mkdir lib
	$(CCompiler) $(FLAGS) $(FUSEFLAGS) HFS/HFSOperations.c -c
	$(Compiler) $(FLAGS) DMGConverting/*.cpp -c
	$(Compiler) $(FLAGS) $(FUSEFLAGS) src/setuper.cpp -c
	mv *.o lib/
	$(Compiler) $(FLAGS) $(FUSEFLAGS) -lfuse -lhfs -lhfsuser -lublio -lutf8proc -pthread -lcurl -lz -lbz2 /usr/local/lib/liblzfse.a lib/* -o bin/setuper
	cp src/programs.csv bin/programs.csv

clean: 
	rm -rf bin/
	rm -rf lib/
	rm -rf *.o