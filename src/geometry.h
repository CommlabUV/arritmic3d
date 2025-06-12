
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <array>
#include <eigen3/Eigen/Dense>

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
 * @todo The ideal is to put this class inside the CardiacTissue class, but it is necessary to change first the Node class.
 */

class Geometry
{
public:
    int size_x, size_y, size_z; // Number of nodes in each direction
    float dx, dy, dz;           // Spacing between nodes
    Vector3 origin;             // Origin of the tissue
    static constexpr int distance = 1;           // Maximun distance of the neighbours
    static constexpr size_t num_neighbours = calculate_neighbours(distance);  // Number of neighbours.
    static constexpr size_t num_axis = 6 * distance;  // Number of axis neighbours.
    std::array<int, num_neighbours> displacement = NeighboursDisplace<num_neighbours>(distance);
    std::array<Vector3, num_neighbours> relative_position = NeighboursRelativePos<num_neighbours>(distance);
    std::array<float, num_neighbours> distance_to_neighbour = NeighboursDistance<num_neighbours>();
    std::array<int, num_axis> displ_axis = DisplacementAxis<num_axis>(distance);

    Geometry()=default;

    Geometry(int size_x_, int size_y_, int size_z_, float dx_, float dy_, float dz_) :
        size_x(size_x_), size_y(size_y_), size_z(size_z_), dx(dx_), dy(dy_), dz(dz_)
    {
        origin = Vector3::Zero();
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
                    if(i == 0 and j == 0 and k == 0)
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
                    if(i == 0 and j == 0 and k == 0)
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

};

#endif // GEOMETRY_H
