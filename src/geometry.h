
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <array>
#include <Eigen/Dense>
#include "error.h"

// Define the maximum distance to consider neighbours. Can be set during compilation with -DNEIGHBOURS_DISTANCE=X
#ifndef NEIGHBOURS_DISTANCE
#define NEIGHBOURS_DISTANCE 1
#endif

using Vector3 = Eigen::Vector3f;

/**
 * @brief Calculate the number of neighbours given the maximun distance from the central node
*/
constexpr size_t calculate_neighbours(size_t distance)
{
    return (2*distance+1)*(2*distance+1)*(2*distance+1)-1;
}

/**
 * @brief Class to model the geometry of the tissue
 */

class Geometry
{
public:
    int size_x, size_y, size_z; // Number of nodes in each direction
    float dx, dy, dz;           // Spacing between nodes
    Vector3 origin;             // Origin of the tissue
    static constexpr int distance = NEIGHBOURS_DISTANCE;           // Maximun distance of the neighbours
    static constexpr size_t num_neighbours = calculate_neighbours(distance);  // Number of neighbours.
    static constexpr size_t num_axis = 6 * distance;  // Number of axis neighbours.
    std::array<int, num_neighbours> displacement;
    std::array<Vector3, num_neighbours> relative_position;
    std::array<float, num_neighbours> distance_to_neighbour;
    std::array<int, num_axis> displ_axis;

    Geometry()=default;

    Geometry(int size_x_, int size_y_, int size_z_, float dx_, float dy_, float dz_) :
        size_x(size_x_), size_y(size_y_), size_z(size_z_), dx(dx_), dy(dy_), dz(dz_)
    {
        origin = Vector3::Zero();
        displacement = NeighboursDisplace<num_neighbours>(distance);
        relative_position = NeighboursRelativePos<num_neighbours>(distance);
        distance_to_neighbour = NeighboursDistance<num_neighbours>();
        displ_axis = DisplacementAxis<num_axis>(distance);
    }

    /**
     * @brief Calculate the index displacement of the neighbours
     * @param distance Maximum distance to consider a neighbour
    */
    template<size_t Size>
    std::array<int, Size> NeighboursDisplace(const int distance)
    {
        std::array<int, Size> neighbours;
        size_t pos = 0;

        for(int i = -distance; i <= distance; ++i)  // z
        {
            for(int j = -distance; j <= distance; ++j)  // y
            {
                for(int k = -distance; k <= distance; ++k)  // x
                {
                    if(i == 0 && j == 0 && k == 0)
                        continue;
                    neighbours[pos] = i*size_x*size_y + j*size_x + k;
                    ++pos;
                }
            }
        }
        //for(auto n : neighbours) std::cout << n << " ";

        return neighbours;
    }

    /**
     * @brief Calculate the index displacement of the axis neighbours
     * @param distance Maximum distance to consider a neighbour
    */
    template<size_t Size>
    std::array<int, Size> DisplacementAxis(const int distance)
    {
        std::array<int, Size> neighbours;
        size_t pos = 0;

        for(int k = -distance; k <= distance; ++k)  // x
        {
                    if(k == 0)
                        continue;
                    neighbours[pos] = k;
                    ++pos;
        }

        for(int j = -distance; j <= distance; ++j)  // y
        {
                    if(j == 0)
                        continue;
                    neighbours[pos] = j*size_x;
                    ++pos;
        }

        for(int i = -distance; i <= distance; ++i)  // z
        {
                    if(i == 0)
                        continue;
                    neighbours[pos] = i*size_x*size_y;
                    ++pos;
        }

        //for(auto n : neighbours) std::cout << n << " ";

        return neighbours;
    }

    /**
     * @brief Calculate the index displacement of the neighbours
     * @param distance Maximum distance to consider a neighbour
    */
    template<size_t Size>
    std::array<Vector3, Size> NeighboursRelativePos(const int distance)
    {
        std::array<Vector3, Size> neighbours;
        size_t pos = 0;

        for(int i = -distance; i <= distance; ++i)  // z
        {
            for(int j = -distance; j <= distance; ++j)  // y
            {
                for(int k = -distance; k <= distance; ++k)  // x
                {
                    if(i == 0 && j == 0 && k == 0)
                        continue;
                    neighbours[pos] = Vector3(k*dx, j*dy, i*dz);
                    ++pos;
                }
            }
        }
        //for(auto n : neighbours) std::cout << n << std::endl;

        return neighbours;
    }

    template<size_t Size>
    std::array<float, Size> NeighboursDistance()
    {
        std::array<float, Size> neighbours;
        size_t pos = 0;

        for(Vector3 n : relative_position)
        {
            neighbours[pos] = n.norm();
            ++pos;
        }
        //for(auto n : neighbours) std::cout << n << std::endl;

        return neighbours;
    }

    /**
     * @brief Get the coordinates of a node given its index inside the array
    */
    Eigen::Vector3i GetCoords(size_t index) const
    {
        int z = index / (size_x*size_y);
        int y = (index - z*size_x*size_y) / size_x;
        int x = index - z*size_x*size_y - y*size_x;

        return Eigen::Vector3i(x, y, z);
    }

    /**
     * @brief Get the index inside the array of a node given its coordinates
    */
    size_t GetIndex(int x, int y, int z) const
    {
        return z*size_x*size_y + y*size_x + x;
    }

    /**
     * @brief Get the physical central position of a node given its index
    */
    Vector3 GetPos(size_t index) const
    {
        Eigen::Vector3i coords = GetCoords(index);
        return origin + Vector3(0.5*dx, 0.5*dy, 0.5*dz) + Vector3(coords[0]*dx, coords[1]*dy, coords[2]*dz);
    }

    void SaveState(std::ofstream & f) const
    {
        f.write( (const char*) (&size_x), sizeof(size_x) );
        f.write( (const char*) (&size_y), sizeof(size_y) );
        f.write( (const char*) (&size_z), sizeof(size_z) );
        f.write( (const char*) (&dx), sizeof(dx) );
        f.write( (const char*) (&dy), sizeof(dy) );
        f.write( (const char*) (&dz), sizeof(dz) );
        f.write( (const char*) (origin.data()), sizeof(float)*3 );
    }

    void LoadState(std::ifstream & f)
    {
        // Just check sizes
        int size;
        f.read( (char*) (&size), sizeof(size) );
        LOG::Error(size != size_x, "Geometry::LoadState: Wrong size_x in file.");
        f.read( (char*) (&size), sizeof(size) );
        LOG::Error(size != size_y, "Geometry::LoadState: Wrong size_y in file.");
        f.read( (char*) (&size), sizeof(size) );
        LOG::Error(size != size_z, "Geometry::LoadState: Wrong size_z in file.");
        float d;
        f.read( (char*) (&d), sizeof(d) );
        LOG::Error(d != dx, "Geometry::LoadState: Wrong dx in file.");
        f.read( (char*) (&d), sizeof(d) );
        LOG::Error(d != dy, "Geometry::LoadState: Wrong dy in file.");
        f.read( (char*) (&d), sizeof(d) );
        LOG::Error(d != dz, "Geometry::LoadState: Wrong dz in file.");
        f.read( (char*) (origin.data()), sizeof(float)*3 );
    }

};

#endif // GEOMETRY_H
