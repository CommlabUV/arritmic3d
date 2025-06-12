 # Set the name of the Python interpreter to use
PY = python3

CXX = g++-12
PY_MOD_NAME = tissue_module
SRC_PY = src/bindings.cpp

# Set the C++ compiler flags
CXXFLAGS = -O3 -Wall
CXXFLAGS_PY = -I /usr/include/$(PY) -shared -fPIC $(shell $(PY) -m pybind11 --includes)

DEPEND = src/cell_event_queue.h src/tissue.h src/basic_tissue.h src/action_potential_rc.h src/conduction_velocity.h src/geometry.h src/node.h src/node.cpp src/definitions.h src/spline2D.h src/sensor_dict.h src/node_parameters.h


# Set the name of the compiled module
TARGET_PY = $(PY_MOD_NAME)$(shell $(PY) -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")

# Set the source files
SRC1 = test/test.cpp
TARGET1 = t
SRC2 = test/test2.cpp
TARGET2 = t2
SRC3 = test/test_reentry.cpp
TARGET3 = tr

all: $(TARGET_PY) $(TARGET1) $(TARGET2) $(TARGET3)

# Define the build rule for the module
$(TARGET_PY): $(SRC_PY) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_PY) $(SRC_PY) -o $(TARGET_PY)

$(TARGET1): $(SRC1) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(SRC1) -o $(TARGET1)
$(TARGET2): $(SRC2) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(SRC2) -o $(TARGET2)
$(TARGET3): $(SRC3) $(DEPEND)
	$(CXX) $(CXXFLAGS) $(SRC3) -o $(TARGET3)

.PHONY: clean
clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET_PY)
