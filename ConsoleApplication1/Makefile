CC = g++

# Get GTK flags from pkg-config
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
GTK_LDFLAGS := $(shell pkg-config --libs gtk+-3.0)

CFLAGS = -Wall -I/usr/include -I./Nodes -I./ -std=c++20 $(GTK_CFLAGS)
LDFLAGS = -L/usr/lib -lraylib -luuid $(GTK_LDFLAGS)
TARGET = out
SRC = $(wildcard *.cpp ./Nodes/*.cpp)
OBJ = $(SRC:.cpp=.o)

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(TARGET)

# Compile source files to object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean object files and target executable
clean:
	rm -f $(TARGET) $(OBJ)
