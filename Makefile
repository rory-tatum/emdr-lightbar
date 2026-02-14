CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Iinclude
EMCC = emcc

UNITY_SRC = lib/unity/src/unity.c
UNITY_INC = -Ilib/unity/src

SRC = src/main.c
TEST_SRC = test/test_main.c

LIGHTBAR_SRC = src/lightbar.c
LIGHTBAR_TEST_SRC = test/test_lightbar.c

.PHONY: native test wasm clean

native: build/main
	@echo "Native build complete: build/main"

build/main: $(SRC) include/main.h | build
	$(CC) $(CFLAGS) -o $@ $(SRC)

build:
	mkdir -p build

test: build/test_main build/test_lightbar
	./build/test_main
	./build/test_lightbar

build/test_main: $(TEST_SRC) $(SRC) include/main.h | build
	$(CC) $(CFLAGS) $(UNITY_INC) -DUNITY_INCLUDE_DOUBLE -Dmain=__original_main -c src/main.c -o build/main_under_test.o
	$(CC) $(CFLAGS) $(UNITY_INC) -DUNITY_INCLUDE_DOUBLE -o $@ \
		$(TEST_SRC) build/main_under_test.o $(UNITY_SRC)

build/test_lightbar: $(LIGHTBAR_TEST_SRC) $(LIGHTBAR_SRC) include/lightbar.h | build
	$(CC) $(CFLAGS) $(UNITY_INC) -DUNITY_INCLUDE_DOUBLE -o $@ \
		$(LIGHTBAR_TEST_SRC) $(LIGHTBAR_SRC) $(UNITY_SRC)

wasm: web/main.js
	@echo "WASM build complete: web/main.js web/main.wasm"

web/main.js: $(SRC) include/main.h
	$(EMCC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -rf build/
	rm -f web/main.js web/main.wasm
