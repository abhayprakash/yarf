# Build the random forest example program

HEADERS := $(wildcard *.hpp)
RFTEST_SRCS := rftest.cpp
IMTEST_SRCS := imtest.cpp
CPP_FLAGS += -Wall
CXX ?= g++

default: debug

debug: CPP_FLAGS += -g
debug: rftest imtest

release: CPP_FLAGS += -O3
release: rftest imtest


rftest:	$(RFTEST_SRCS) $(HEADERS)
	$(CXX) $(CPP_FLAGS) $(RFTEST_SRCS) -o $@

imtest:	$(IMTEST_SRCS) $(HEADERS)
	$(CXX) $(CPP_FLAGS) $(IMTEST_SRCS) -o $@ -g

clean:
	rm -f rftest imtest
