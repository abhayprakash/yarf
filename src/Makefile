# Build the random forest example program

HEADERS := $(wildcard *.hpp)
RFTEST_SRCS := rftest.cpp
CPP_FLAGS += -Wall
CXX ?= g++

default: debug

debug: CPP_FLAGS += -g
debug: rftest

release: rftest

rftest:	$(RFTEST_SRCS) $(HEADERS)
	$(CXX) $(CPP_FLAGS) $(RFTEST_SRCS) -o $@

clean:
	rm -f rftest
