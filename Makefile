.DEFAULT_GOAL := all

CXX ?= g++
COMMON_STD := -std=c++17
COMMON_WARNINGS := -Wall -Wextra
THREAD_FLAG := -pthread
RELEASE_OPT := -O3
DEBUG_OPT := -O0 -g

CXXFLAGS ?= $(COMMON_STD) $(COMMON_WARNINGS) $(RELEASE_OPT) $(THREAD_FLAG)
DEBUG_CXXFLAGS := $(COMMON_STD) $(COMMON_WARNINGS) $(DEBUG_OPT) $(THREAD_FLAG)

SRC_DIR := src
BUILD_DIR := build
OUTPUT_DIR := output
DATA_DIR := data

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)

TARGET_NAME := spell_checker

DICT := $(DATA_DIR)/words_alpha.txt
DATA_INPUT := $(DATA_DIR)/input.txt
TEST_INPUT := tests/test.txt
TEST_OUTPUT := $(OUTPUT_DIR)/test.html

ifeq ($(OS),Windows_NT)
  EXE_EXT := .exe
  define MKDIR_P
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\\,$(abspath $(1)))' | Out-Null"
  endef
  define RM_RF
	powershell -NoProfile -Command "if (Test-Path '$(subst /,\\,$(abspath $(1)))') { Remove-Item -Recurse -Force '$(subst /,\\,$(abspath $(1)))' }"
  endef
  define OPEN_FILE
	powershell -NoProfile -Command "Start-Process -FilePath '$(subst /,\\,$(abspath $(1)))'"
  endef
else
  EXE_EXT :=
  define MKDIR_P
	mkdir -p "$(1)"
  endef
  define RM_RF
	rm -rf "$(1)"
  endef
  UNAME_S := $(shell uname -s)
  define OPEN_FILE
	open "$(abspath $(1))"
  endef
endif

TARGET := $(BUILD_DIR)/$(TARGET_NAME)$(EXE_EXT)
TARGET_DEBUG := $(BUILD_DIR)/$(TARGET_NAME)_debug$(EXE_EXT)

all: $(TARGET)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES)

$(TARGET_DEBUG): $(SOURCES) | $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) -o $@ $(SOURCES)

$(BUILD_DIR):
	@$(call MKDIR_P,$@)

$(OUTPUT_DIR):
	@$(call MKDIR_P,$@)

$(DICT):
	@echo "Downloading dictionary..."
ifeq ($(OS),Windows_NT)
	@powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(DATA_DIR)' | Out-Null; Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/dwyl/english-words/master/words_alpha.txt' -OutFile '$(DICT)'"
else
	@mkdir -p $(DATA_DIR)
	@curl -L -o $(DICT) https://raw.githubusercontent.com/dwyl/english-words/master/words_alpha.txt
endif
	@echo "Dictionary downloaded to $(DICT)"

run: $(TARGET) $(OUTPUT_DIR) $(DICT)
	$(TARGET) $(DICT) < $(DATA_INPUT) > $(TEST_OUTPUT)
	@$(call OPEN_FILE,$(TEST_OUTPUT))

test: $(TARGET) $(OUTPUT_DIR) $(DICT)
	@echo "Running spell checker test..."
ifeq ($(OS),Windows_NT)
	@cmd /c "$(subst /,\,$(TARGET)) $(subst /,\,$(DICT)) < $(subst /,\,$(TEST_INPUT)) > $(subst /,\,$(TEST_OUTPUT)) 2>nul"
else
	@$(TARGET) $(DICT) < $(TEST_INPUT) > $(TEST_OUTPUT) 2>/dev/null
endif
	@echo "Comparing output with expected result..."
ifeq ($(OS),Windows_NT)
	@powershell -NoProfile -Command "if (Compare-Object (Get-Content tests/test.html) (Get-Content $(TEST_OUTPUT))) { Write-Host 'FAIL: Output differs from expected' -ForegroundColor Red; exit 1 } else { Write-Host 'PASS: Output matches expected result' -ForegroundColor Green }"
else
	@diff -q tests/test.html $(TEST_OUTPUT) && echo "PASS: Output matches expected result" || (echo "FAIL: Output differs from expected" && exit 1)
endif

debug: $(TARGET_DEBUG)

clean:
	@$(call RM_RF,$(BUILD_DIR))
	@$(call RM_RF,$(OUTPUT_DIR))

.PHONY: all clean run debug test download-dict
