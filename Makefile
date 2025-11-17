# Set the name of the Python interpreter to use
PY = python3

CXX = g++
PY_MOD_NAME = tissue_module
SRC_PY = src/bindings.cpp

# Set the C++ compiler flags
CXXFLAGS = -O3 -Wall
# Use python3-config (or <python>-config) to obtain compiler/linker flags for Python
PYTHON_CONFIG := $(shell command -v python3-config 2>/dev/null || command -v $(PY)-config 2>/dev/null)
ifeq ($(PYTHON_CONFIG),)
$(error "python3-config not found. Install python3-dev or provide a python-config tool in PATH.")
endif

CXXFLAGS_PY = -shared -fPIC $(shell $(PYTHON_CONFIG) --includes)

DEPEND = src/cell_event_queue.h src/tissue.h src/basic_tissue.h src/action_potential_rc.h src/conduction_velocity.h src/geometry.h src/node.h src/node.cpp src/definitions.h src/spline.h src/spline2D.h src/sensor_dict.h src/node_parameters.h


# Set the name of the compiled module
TARGET_PY = $(PY_MOD_NAME)$(shell $(PY) -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")

# Set the source files
SRC1 = test/test.cpp
TARGET1 = t
SRC2 = test/test2.cpp
TARGET2 = t2
SRC3 = test/test_reentry.cpp
TARGET3 = tr
SRC4 = test/test_spline.cpp
TARGET4 = ts
SRC5 = test/test_spline2d.cpp
TARGET5 = ts2

all: $(TARGET_PY) $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5)

# Define the build rule for the module
$(TARGET_PY): $(SRC_PY) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_PY) $(SRC_PY) -o $(TARGET_PY)

$(TARGET1): $(SRC1) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(SRC1) -o $(TARGET1)
$(TARGET2): $(SRC2) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(SRC2) -o $(TARGET2)
$(TARGET3): $(SRC3) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(SRC3) -o $(TARGET3)
$(TARGET4): $(SRC4) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(SRC4) -o $(TARGET4)
$(TARGET5): $(SRC5) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(SRC5) -o $(TARGET5)


.PHONY: clean
clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET_PY)
