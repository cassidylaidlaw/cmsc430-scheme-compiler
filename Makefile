
CLANG=clang++
RM=rm
TARGETS=header.ll

.PHONY : all

%.ll : %.cpp
	$(CLANG) $^ -S -emit-llvm -o $@

all : $(TARGETS)

clean :
	$(RM) $(TARGETS)

