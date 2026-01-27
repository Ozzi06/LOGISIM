CC = clang++

# Get GTK flags from pkg-config
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
GTK_LDFLAGS := $(shell pkg-config --libs gtk+-3.0)

OPT ?= -O0
CFLAGS = -g -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -I/usr/include -Isrc/Nodes -I./src/ -std=c++20 $(OPT) $(GTK_CFLAGS)
LDFLAGS = -L/usr/lib -lraylib -luuid -lm $(GTK_LDFLAGS)

TARGET = out
OBJDIR = build
SRC = $(wildcard src/*.cpp src/Nodes/*.cpp)
OBJ = $(SRC:%.cpp=$(OBJDIR)/%.o)

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(TARGET)

# Compile source files to object files
$(OBJDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean object files and target executable
clean:
	rm -rf $(OBJDIR) $(TARGET)
