#include <iostream>
#include <cassert>
#include <fstream>
#include <cmath>
#include "../src/geometry.h"

void test_basic_properties() {
    std::cout << "Testing basic properties..." << std::endl;
    int sx = 10, sy = 20, sz = 30;
    float dx = 0.1, dy = 0.2, dz = 0.3;
    Geometry geom(sx, sy, sz, dx, dy, dz);

    assert(geom.size_x == sx);
    assert(geom.size_y == sy);
    assert(geom.size_z == sz);
    assert(geom.dx == dx);
    assert(geom.dy == dy);
    assert(geom.dz == dz);
    
    // ext_size = size + 2*distance
    assert(geom.ext_size_x == sx + 2*Geometry::distance);
    assert(geom.ext_size_y == sy + 2*Geometry::distance);
    assert(geom.ext_size_z == sz + 2*Geometry::distance);
    
    std::cout << "Basic properties passed." << std::endl;
}

void test_coordinate_conversions() {
    std::cout << "Testing coordinate conversions..." << std::endl;
    int sx = 5, sy = 5, sz = 5;
    Geometry geom(sx, sy, sz, 1.0, 1.0, 1.0);

    // GridIndex <-> Coords
    int x = 2, y = 3, z = 4;
    size_t gidx = geom.GetGridIndex_from_Coords(x, y, z);
    Eigen::Vector3i coords = geom.GetCoords_from_GridIndex(gidx);
    assert(coords.x() == x);
    assert(coords.y() == y);
    assert(coords.z() == z);

    // ExtGridIndex <-> ExtCoords
    int ex = 2, ey = 3, ez = 4;
    size_t egidx = geom.GetExtGridIndex_from_ExtCoords(ex, ey, ez);
    Eigen::Vector3i ecoords = geom.GetExtCoords_from_ExtGridIndex(egidx);
    assert(ecoords.x() == ex);
    assert(ecoords.y() == ey);
    assert(ecoords.z() == ez);

    // GridIndex -> ExtGridIndex
    size_t egidx2 = geom.GetExtGridIndex_from_GridIndex(gidx);
    Eigen::Vector3i ecoords2 = geom.GetExtCoords_from_ExtGridIndex(egidx2);
    assert(ecoords2.x() == x + Geometry::distance);
    assert(ecoords2.y() == y + Geometry::distance);
    assert(ecoords2.z() == z + Geometry::distance);

    std::cout << "Coordinate conversions passed." << std::endl;
}

void test_memory_indexing() {
    std::cout << "Testing memory indexing..." << std::endl;
    int sx = 3, sy = 3, sz = 3;
    Geometry geom(sx, sy, sz, 1.0, 1.0, 1.0);

    // Check if live nodes (within boundaries) have correct indices
    for(int z = 0; z < sz; z++) {
        for(int y = 0; y < sy; y++) {
            for(int x = 0; x < sx; x++) {
                size_t gidx = geom.GetGridIndex_from_Coords(x, y, z);
                size_t egidx = geom.GetExtGridIndex_from_GridIndex(gidx);
                Index_t mem_idx = geom.GetMemIndex_from_GridIndex(egidx);
                assert(mem_idx == (Index_t)gidx);
            }
        }
    }

    // Check border (VOID/CORE nodes)
    // ext_index[0,0,0] should be NO_INDEX
    size_t border_egidx = geom.GetExtGridIndex_from_ExtCoords(0, 0, 0);
    assert(geom.GetMemIndex_from_GridIndex(border_egidx) == NO_INDEX);

    std::cout << "Memory indexing passed." << std::endl;
}

void test_neighbours() {
    std::cout << "Testing neighbours..." << std::endl;
    Geometry geom(10, 10, 10, 1.0, 1.0, 1.0);

    assert(geom.displacement.size() == Geometry::num_neighbours);
    assert(geom.relative_position.size() == Geometry::num_neighbours);
    assert(geom.distance_to_neighbour.size() == Geometry::num_neighbours);

    // Check relative position for some neighbour
    // Loop through the same logic as in geometry.h to verify
    size_t pos = 0;
    for(int i = -Geometry::distance; i <= Geometry::distance; ++i) {
        for(int j = -Geometry::distance; j <= Geometry::distance; ++j) {
            for(int k = -Geometry::distance; k <= Geometry::distance; ++k) {
                if(i == 0 && j == 0 && k == 0) continue;
                assert(geom.relative_position[pos].x() == k * geom.dx);
                assert(geom.relative_position[pos].y() == j * geom.dy);
                assert(geom.relative_position[pos].z() == i * geom.dz);
                
                int expected_displ = i*geom.ext_size_x*geom.ext_size_y + j*geom.ext_size_x + k;
                assert(geom.displacement[pos] == expected_displ);
                
                float expected_dist = geom.relative_position[pos].norm();
                assert(std::abs(geom.distance_to_neighbour[pos] - expected_dist) < 1e-6);
                
                pos++;
            }
        }
    }
    
    // Check axis displacement
    assert(geom.displ_axis.size() == Geometry::num_axis);

    std::cout << "Neighbours passed." << std::endl;
}

void test_save_load() {
    std::cout << "Testing save/load state..." << std::endl;
    int sx = 4, sy = 5, sz = 6;
    float dx = 0.5, dy = 0.6, dz = 0.7;
    Geometry geom_save(sx, sy, sz, dx, dy, dz);
    geom_save.origin = Vector3(1.0, 2.0, 3.0);

    const std::string filename = "test_geometry_state.bin";
    {
        std::ofstream ofs(filename, std::ios::binary);
        geom_save.SaveState(ofs);
    }

    Geometry geom_load(sx, sy, sz, dx, dy, dz); // Must be initialized with same sizes for LoadState to not fail error check
    {
        std::ifstream ifs(filename, std::ios::binary);
        geom_load.LoadState(ifs);
    }

    assert(geom_load.size_x == sx);
    assert(geom_load.size_y == sy);
    assert(geom_load.size_z == sz);
    assert(geom_load.dx == dx);
    assert(geom_load.dy == dy);
    assert(geom_load.dz == dz);
    assert(geom_load.origin.x() == 1.0f);
    assert(geom_load.origin.y() == 2.0f);
    assert(geom_load.origin.z() == 3.0f);

    std::remove(filename.c_str());
    std::cout << "Save/load state passed." << std::endl;
}

int main() {
    test_basic_properties();
    test_coordinate_conversions();
    test_memory_indexing();
    test_neighbours();
    test_save_load();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
