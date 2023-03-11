CXX=g++
CC=gcc

CLIENT:=tftp_client
TARGET:=tftp_server
BUILD:=./build
OBJ_DIR:=$(BUILD)/objects
APP_DIR:=$(BUILD)/apps

LDLAGS:=-lm -ldl -lpthread -lspdlog -lfmt
CXXFLAGS:=-std=c++17 -Wall -Wextra -Werror -Wswitch-enum -Wshadow -Woverloaded-virtual -Wnull-dereference -Wformat=2 -DSPDLOG_COMPILED_LIB
INCLUDE:=-Isrc/include/
 
SRC:= \
   $(wildcard src/*.cpp)

COMMON_SRCS := \
		$(wildcard src/common/*.cpp)

CLIENT_SRCS := $(wildcard src/client/*.cpp) $(COMMON_SRCS)
 
CLIENT_OBJECTS:=$(CLIENT_SRCS:%.cpp=$(OBJ_DIR)/%.o)


$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@
	@echo "Compiled: $@"

$(APP_DIR)/$(CLIENT): $(CLIENT_OBJECTS)
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(CLIENT) $^ $(LDLAGS)
	@echo "BUILT: $@"

# Rules
build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

$(APP_DIR)/$(TEST_BIN): $(TEST_OBJECTS)

format:
	@clang-format-13 -i `find src/ -name *.cpp`
	@clang-format-13 -i `find src/ -name *.hpp`

cppcheck:
	@cppcheck --enable=all src/ -I src/include/

debug: CXXFLAGS += -DRTDEBUG -ggdb -g -fsanitize=address
debug: all

release: CXXFLAGS += -O2 -DNDEBUG
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*


all: build $(APP_DIR)/$(CLIENT) 

.PHONY:


