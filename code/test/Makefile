
.PHONY: all test

all: test

test: export CTEST_OUTPUT_ON_FAILURE=1
test: debug

release: CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Release
release: TARGET_DIR=build/release

debug: CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Debug
debug: TARGET_DIR=build/debug

release debug: testrunner

testrunner: $(TARGET_DIR)/testrunner
	cd $(TARGET_DIR) && $(MAKE) test
.PHONY: testrunner

$(TARGET_DIR)/testrunner: $(wildcard ../src/*cpp) $(wildcard ../src/*hpp) test.cpp $(TARGET_DIR)/Makefile
	cd $(TARGET_DIR) && $(MAKE)

$(TARGET_DIR)/Makefile:
	echo $0
	mkdir -p $(TARGET_DIR) \
		&& cd $(TARGET_DIR) \
		&& cmake $(CMAKE_ARGS) ../..


clean:
	[ -d build ] && rm -r build
