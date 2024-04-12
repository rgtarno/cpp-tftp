CXX=g++
CC=gcc

CLIENT:=tftp_client
SERVER:=tftp_server
TEST_BIN:=tests
BUILD:=./build
OBJ_DIR:=$(BUILD)/objects
APP_DIR:=$(BUILD)/apps
VERSION?=release

LDLAGS:=-lm -ldl -lpthread -lspdlog -lfmt
CXXFLAGS:=-std=c++17 -Wall -Wextra -Werror -Wswitch-enum -Wshadow -Woverloaded-virtual -Wnull-dereference -Wformat=2 -DSPDLOG_COMPILED_LIB
INCLUDE:=-Isrc/include/

ifeq ($(VERSION),release)
CXXFLAGS+= -O2 -DNDEBUG -DRELEASE
else
CXXFLAGS+= -ggdb -g -fsanitize=address,undefined
LDLAGS+= -fsanitize=address,undefined
endif
 

COMMON_SRCS := \
		$(wildcard src/common/*.cpp)

CLIENT_SRCS := $(wildcard src/client/*.cpp) $(COMMON_SRCS)
CLIENT_OBJECTS:=$(CLIENT_SRCS:%.cpp=$(OBJ_DIR)/%.o)

SERVER_SRCS := $(wildcard src/server/*.cpp) $(COMMON_SRCS)
SERVER_OBJECTS:=$(SERVER_SRCS:%.cpp=$(OBJ_DIR)/%.o)

TEST_SRCS := $(wildcard src/tests/*.cpp) $(COMMON_SRCS)
TEST_OBJECTS:=$(TEST_SRCS:%.cpp=$(OBJ_DIR)/%.o)

all: server client

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@
	@echo "Compiled: $@"

$(APP_DIR)/$(CLIENT): $(CLIENT_OBJECTS)
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ $^ $(LDLAGS)
	@echo "\033[32mBUILT: $@\033[0m"

$(APP_DIR)/$(SERVER): $(SERVER_OBJECTS)
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ $^ $(LDLAGS)
	@echo  "\033[32mBUILT: $@\033[0m"

$(APP_DIR)/$(TEST_BIN): $(TEST_OBJECTS)
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ $^ $(LDLAGS)
	@echo  "\033[32mBUILT: $@\033[0m"

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

$(APP_DIR)/$(TEST_BIN): $(TEST_OBJECTS)

tests: $(APP_DIR)/$(TEST_BIN)
	@echo Running tests
	@$(APP_DIR)/$(TEST_BIN)

clean:
	-@rm -rvf $(BUILD)

format:
	clang-format-13 -i --style=file $(shell find src -name *.cpp)
	clang-format-13 -i --style=file $(shell find src -name *.hpp)

client: build $(APP_DIR)/$(CLIENT)
server: build $(APP_DIR)/$(SERVER)

.PHONY: clean format server client tests 
