# =========================================================
# Makefile: C++ (header-only/templates)
#
# Usage:
#   make                    # builds default mode (release)
#   make MODE=debug         # builds debug mode
#   make executable
#   make clean              # removes all builds
#   make clean_cpp          # removes all test executables
#   make help               # shows this help message
# =========================================================

# ---------- Build mode ----------
MODE ?= release

# Compiler
CXX = g++

# ---------- Mode-specific flags ----------
ifeq ($(MODE),debug)
    CXXFLAGS_MODE := -O0 -g -DDEBUG -fno-omit-frame-pointer \
                     -fsanitize=address,undefined
else ifeq ($(MODE),release)
    CXXFLAGS_MODE := -O3
else ifeq ($(MODE),fast)
	# TODO try -flto -s
    CXXFLAGS_MODE := -O3 -DNDEBUG -mavx -ffast-math
else
    $(error MODE must be 'debug', 'release', or 'fast' (got '$(MODE)'))
endif

# Set the C++ compiler flags
CXXFLAGS_EIGEN = -I /usr/include/eigen3
INCLUDES = $(CXXFLAGS_EIGEN) #-Iinclude
CXXFLAGS := -std=c++17 -Wall $(INCLUDES) $(CXXFLAGS_MODE)

# ---------- Python settings ----------
# Set the name of the Python interpreter to use
PY = python3

PY_MOD_NAME = tissue_module
SRC_PY = src/bindings.cpp

# Use python3-config (or <python>-config) to obtain compiler/linker flags for Python
PYTHON_CONFIG := $(shell command -v python3-config 2>/dev/null || command -v $(PY)-config 2>/dev/null)
ifeq ($(PYTHON_CONFIG),)
$(error "python3-config not found. Install python3-dev or provide a python-config tool in PATH.")
endif

CXXFLAGS_PY = -shared -fPIC $(shell $(PYTHON_CONFIG) --includes)

# Set the name of the compiled module
TARGET_PY = $(PY_MOD_NAME)$(shell $(PY) -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")

#HEADERS = src/cell_event_queue.h src/tissue.h src/basic_tissue.h src/action_potential_rc.h src/conduction_velocity.h src/geometry.h src/node.h src/node.cpp src/definitions.h src/spline.h src/spline2D.h src/sensor_dict.h src/node_parameters.h
# Collect all headers; changing any should rebuild executables
HEADERS := $(shell find src -type f \( -name '*.hpp' -o -name '*.h' \))

# Set the source files
TEST_DIR = test
TARGET_CPP = test1 test2 test_reentry test_spline test_spline2d test_save

all: $(TARGET_PY) $(TARGET_CPP)

# Define the build rule for the module
$(TARGET_PY): $(SRC_PY) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_PY) $(SRC_PY) -o $(TARGET_PY)

$(TARGET_CPP): %: $(TEST_DIR)/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean clean_cpp help
clean:
	rm -f $(TARGET_PY) $(TARGET_CPP)

clean_cpp:
	rm -f $(TARGET_CPP)

# ---------- Help ----------
help:
	@echo "Targets:"
	@echo "  make [MODE=release|debug|fast]    Build default target (all)"
	@echo "  make app1 / make app2        Build a single executable"
	@echo "  make clean                   Remove all"
	@echo "  make clean_cpp               Remove all test executables"
