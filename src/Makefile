# Build the random forest example program

HEADERS := $(wildcard *.hpp)
RFTEST_SRCS := rftest.cpp
CPP_FLAGS += -Wall -ggdb
CC := g++

rftest:	$(RFTEST_SRCS) $(HEADERS)
	$(CC) $(CPP_FLAGS) $(RFTEST_SRCS) -o $@
