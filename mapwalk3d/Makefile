CC      := gcc
CFLAGS  := -std=c99 -Wall
LDLIBS  := -lSDL2 -lm

SRC_DIR := src
BIN_DIR := bin
TARGET  := $(BIN_DIR)/mapwalk3d
SRC     := main.c game.c render.c
OBJ     := $(addprefix $(BIN_DIR)/,$(SRC:.c=.o))
DEP     := $(OBJ:.o=.d)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BIN_DIR):
	mkdir -p $@

MAP ?= short.txt

run: $(TARGET)
	./$(TARGET) $(MAP)

clean:
	rm -rf $(BIN_DIR)

-include $(DEP)
.PHONY: run clean
