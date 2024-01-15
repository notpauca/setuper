Compiler = clang++
CPPFLAGS = -O3 -Wall --std=c++17 --include-directory=Include/ --library-directory=lib/ --library-directory=/opt/homebrew/lib/

build: 
	mkdir bin
	$(Compiler) $(CPPFLAGS) -o bin/setuper -lcurl -lz -lbz2 lib/*.cpp src/setuper.cpp
	cp src/programs.csv bin/programs.csv

clean: 
	rm -rf bin/