
CLANG=clang++

%.ll : %.cpp
	$(CLANG) $^ -S -emit-llvm -o $@
