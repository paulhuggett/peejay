CXXFLAGS = -g -std=c++11 -D KLEE=1 -emit-llvm -I /home/klee/klee_src/include/
CXX = clang

OBJS = main_klee.o json.o utf.o

.PHONY: all
all: main_klee.bc
.PHONY: clean
clean:
	-rm $(OBJS) main_klee.bc

main_klee.bc: $(OBJS)
	llvm-link -o $@ $^

#eof:Makefile

