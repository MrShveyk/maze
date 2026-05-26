CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
LDFLAGS  ?=

SRC_DIR  := src
BUILD    := build
TARGET   := maze

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

IMAGE    ?= maze

.PHONY: all clean docs diagrams run-server run-client docker-build docker-server docker-client docker-down

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD)/%.o: $(SRC_DIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) $(TARGET) doxygen

docs:
	doxygen Doxyfile

diagrams:
	plantuml -tpng diagrams/*.puml

docker-build:
	docker build -t $(IMAGE) .

docker-server:
	docker compose up -d --build server

docker-client:
	docker compose run --rm client -c -a server -p 4321 -n $(or $(NAME),player)

docker-down:
	docker compose down

-include $(DEPS)
