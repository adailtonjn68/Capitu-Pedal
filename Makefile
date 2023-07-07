PROJECT_NAME:=capitu_pedal

TOOLCHAIN:=gcc

STANDARD:=c17
DEBUG_LEVEL:=-g3
OPTIM_LEVEL:=-O0
WARNINGS:=-Wall -Wextra -Wpedantic
SYNTAX:=
C_FLAGS:=-c $(WARNINGS) -std=$(STANDARD) $(DEBUG_LEVEL) $(OPTIM_LEVEL) -MMD
LINK_FLAGS:= $(WARNINGS) -lportaudio -lsndfile -lm -lncurses

SRC_DIR:=Core/Src
INC_DIR:=Core/Inc
INCLUDES:=$(addprefix -I,$(INC_DIR))

C_FILES:=$(shell find $(SRC_DIR) -name '*.c')
OBJ_FILES:=$(C_FILES:%.c=Debug/%.o)
DEP_FILES = $(OBJ_FILES:%.o=%.d)

all: Debug/$(PROJECT_NAME)

Debug/$(PROJECT_NAME): $(OBJ_FILES)
	$(TOOLCHAIN) $(OBJ_FILES) $(LINK_FLAGS) -o $@

Debug/%.o: %.c
	@mkdir -p $(addprefix Debug/,$(SRC_DIR))
	$(TOOLCHAIN) $(C_FLAGS) $(INCLUDES) $< -o $@

clean:
	rm -rf Debug/*

run: Debug/$(PROJECT_NAME)
	@#pulseaudio --kill
	@#jack_control start
	./Debug/$(PROJECT_NAME)
	@#jack_control exit
	@#pulseaudio --start

debug: Debug/$(PROJECT_NAME)
	@#pulseaudio --kill
	@#jack_control start
	gdb -tui ./Debug/$(PROJECT_NAME)
	@#jack_control exit
	@#pulseaudio --start



check_memory: Debug/$(PROJECT_NAME)
	valgrind --leak-check=full -s ./Debug/$(PROJECT_NAME) 
dependencies:
	sudo apt install libsndfile1-dev libportaudio2

.PHONY: all clean run debug dependencies check_memory

-include $(DEP_FILES)
