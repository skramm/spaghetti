
SRC_DIR=.
BIN_DIR=.

SRC_FILES    := $(wildcard $(SRC_DIR)/*.cpp)
EXEC_FILES   := $(patsubst $(SRC_DIR)/%.cpp,   $(BIN_DIR)/%,   $(SRC_FILES))
LDFLAGS += -lboost_system -lboost_thread -pthread

% :%.cpp
	$(CXX) $< -o $@ $(LDFLAGS)
	@echo done

all: $(EXEC_FILES)
	@echo done

