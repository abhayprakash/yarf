# Build the random forest example program

HEADERS := $(wildcard *.hpp)
RFTEST_SRCS := rftest.cpp
IMTEST_SRCS := imtest.cpp
CPP_FLAGS += -Wall
CPP_BUILD_FLAGS := -g
CXX ?= g++

default: debug

debug: CPP_BUILD_FLAGS := -g
debug: rftest imtest

release: CPP_BUILD_FLAGS := -O3
release: rftest imtest

pch: CPP_FLAGS += -O3
pch: pch.hpp.gch

# Precompiled headers?
GCHS := $(HEADERS:.hpp=.hpp.gch)

pch.hpp.gch: $(HEADERS)
	$(CXX) $(CPP_FLAGS) $(CPP_BUILD_FLAGS) -x c++-header -o $@ pch.hpp

rftest:	$(RFTEST_SRCS) $(HEADERS)
	$(CXX) $(CPP_FLAGS) $(CPP_BUILD_FLAGS) $(RFTEST_SRCS) -o $@

imtest:	$(IMTEST_SRCS) $(HEADERS)
	$(CXX) $(CPP_FLAGS) $(CPP_BUILD_FLAGS) $(IMTEST_SRCS) -o $@ -g

clean:
	rm -f rftest imtest $(GCHS)
