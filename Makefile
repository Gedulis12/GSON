CC=gcc
INCDIRS=-I./include
OPT=-O0
DEFINES=DDEBUG=0
CFLAGS=-Wall -Wextra -g $(INCDIRS) $(OPT) $(DEPFLAGS) $(foreach D,$(DEFINES),-D$(D))

SRC=src
CFILES=$(wildcard $(SRC)/*.c)

LIB=lib
OBJECTS=$(patsubst $(SRC)/%.c, $(LIB)/%.o, $(CFILES))
LIBRARY=$(LIB)/libgson.a

TEST_DIR=tests
TEST_SRC=$(TEST_DIR)/tests.c
TEST_BINARY=gson_test

BINARY=gson
FUZZER=fuzz

all: $(BINARY)

fuzz: $(FUZZER)

$(BINARY): $(LIBRARY)
	$(CC) -g -L$(LIB) $(INCDIRS) main.c -o $(BINARY) -l$(BINARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $(OBJECTS)

$(LIB)/%.o:$(SRC)/%.c
	mkdir -pv $(LIB)
	$(CC) $(CFLAGS) -c $^ -o $@

test: $(TEST_BINARY)
	./$(TEST_BINARY) 2>/dev/null

$(TEST_BINARY): $(LIBRARY)
	$(CC) -g -L$(LIB) $(INCDIRS) $(TEST_SRC) -o $(TEST_BINARY) -l$(BINARY)

$(FUZZER): $(LIBRARY)
	clang++ -g -fsanitize=address,fuzzer fuzz.cpp -o $(FUZZER) -Llib -lgson -I./include

clean:
	rm -rf $(OBJECTS) $(LIBRARY) $(BINARY) $(TEST_BINARY) $(FUZZER)
