# Makefile for BotiEyes network-layer host-native tests.
# Builds against the platform-agnostic sources in BotiEyes/src/net/.

CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O0 -g -I../../BotiEyes/src/net

NET_SRC = ../../BotiEyes/src/net/CommandCodec.cpp ../../BotiEyes/src/net/SessionManager.cpp

TESTS = test_command_codec test_session_manager

.PHONY: all clean
all: $(TESTS)
	@echo "---- running test_command_codec ----"
	@./test_command_codec
	@echo "---- running test_session_manager ----"
	@./test_session_manager

test_command_codec: test_command_codec.cpp $(NET_SRC) test_harness.h
	$(CXX) $(CXXFLAGS) -o $@ test_command_codec.cpp $(NET_SRC)

test_session_manager: test_session_manager.cpp $(NET_SRC) test_harness.h
	$(CXX) $(CXXFLAGS) -o $@ test_session_manager.cpp $(NET_SRC)

clean:
	rm -f $(TESTS)
	rm -rf *.dSYM
