
.PHONY: all release debug clean test

all: release

release: CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Release
release: TARGET_DIR=build/release

debug: CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Debug
debug: TARGET_DIR=build/debug

release debug: lifter

lifter: $(TARGET_DIR)/lifter
.PHONY: lifter

$(TARGET_DIR)/lifter: $(wildcard src/*cpp) $(wildcard src/*hpp) $(TARGET_DIR)/Makefile
	cd $(TARGET_DIR) && $(MAKE)

$(TARGET_DIR)/Makefile:
	echo $0
	mkdir -p $(TARGET_DIR) \
		&& cd $(TARGET_DIR) \
		&& cmake $(CMAKE_ARGS) ../../src


test:
	cd test && $(MAKE)


clean:
	[ -d build ] && rm -r build
