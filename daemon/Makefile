CXX := zig c++
CC := zig cc

# hint: make TARGET=aarch64-linux DEBUG=0 -j$(nproc)

PROJECT := encored
DEBUG ?= 0
TARGET ?= aarch64-linux
TARGET_FLAGS := -target $(TARGET)

OBJ_DIR := ../obj/$(TARGET)
BIN_DIR := ../libs/$(TARGET)

LINK_FLAGS := -static

ifeq ($(DEBUG),1)
	OPT_FLAGS := -O0 -g -fno-omit-frame-pointer
else
	OPT_FLAGS := -Os -flto -DNDEBUG
	LINK_FLAGS += -s -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables
endif

CXXFLAGS := $(TARGET_FLAGS) $(OPT_FLAGS) -std=c++23 -fexceptions \
            -Wpedantic -Wall -Wextra -Werror -Wformat -Wuninitialized \
            -Wno-date-time

SRC_FILES := Main.cpp

rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
SRC_FILES += $(call rwildcard,src/,*.cpp)

INCLUDES := -I./include \
            -I./external/rapidjson/include \
            -I./external/spdlog/include

HEADER_DIRS := $(sort $(dir $(call rwildcard,src/,*.hpp)))
INCLUDES += $(addprefix -I,$(HEADER_DIRS))

OBJ_FILES := $(addprefix $(OBJ_DIR)/, $(SRC_FILES:.cpp=.o))

all: $(BIN_DIR)/$(PROJECT)

$(BIN_DIR)/$(PROJECT): $(OBJ_FILES) | $(BIN_DIR)
	$(CXX) $(TARGET_FLAGS) $(CXXFLAGS) $(LINK_FLAGS) -o $@ $^

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(@D)
	$(CXX) $(TARGET_FLAGS) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(patsubst %/$(TARGET),%, $(BIN_DIR))
	rm -rf $(patsubst %/$(TARGET),%, $(OBJ_DIR))

.PHONY: all clean
