TARGET = $(notdir $(CURDIR)).out
SRC_DIR = .
INC_DIR = ../common
BUILD_DIR = build
CC = gcc
CFLAGS = -Wall -I$(INC_DIR)
SRC_EXT = c
SOURCES = $(wildcard $(SRC_DIR)/*.$(SRC_EXT))
OBJECTS = $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SOURCES:.$(SRC_EXT)=.o))

all: build

build: $(OBJECTS)
	$(CC) $^ -o $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(SRC_EXT)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
