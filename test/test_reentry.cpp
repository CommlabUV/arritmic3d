/**
 * ARRITMIC3D
 * Test of the detection of reentry.
 *
 * */
#include <iostream>
#include <string>
#include "../src/tissue.h"
#include "../src/action_potential_rc.h"
#include "../src/conduction_velocity.h"

const int N_NODES = 20;
const int THICKNESS = 4;

/**
 * @brief Set the core of the tissue to a given type.
 * @param tissue The tissue to modify.
 * @param v_type The vector of cell types.
 * @param size_x The size in the x direction of the core.
 * @param size_y The size in the y direction of the core.
 * @param size_z The size in the z direction of the core.
 * @param type The type to set the core to.
 */
template<typename T>
void SetCore(const T & tissue, std::vector<CellType> &v_type, int size_x, int size_y, int size_z, CellType type)
{
    int sx = (tissue.GetSizeX()-size_x)/2;
    int sy = (tissue.GetSizeY()-size_y)/2;
    int sz = (tissue.GetSizeZ()-size_z)/2;

    for(int i = 0+sx; i < int(tissue.GetSizeX())-sx; ++i)
    {
        for(int j = 0+sy; j < int(tissue.GetSizeY())-sy; ++j)
        {
            for(int k = 0+sz; k < int(tissue.GetSizeZ())-sz; ++k)
            {
                v_type.at(tissue.GetIndex(i,j,k)) = type;
            }
        }
    }
}

int main(int argc, char **argv)
{
    CardiacTissue<ActionPotentialRestCurve,ConductionVelocity> tissue(N_NODES, N_NODES, THICKNESS, 0.1, 0.1, 0.1);
    int total_nodes = tissue.size();
    std::vector<CellType> v_type(total_nodes, CellType::HEALTHY);

    SetCore(tissue, v_type, 8, 8, THICKNESS, CellType::BORDER_ZONE);

    NodeParameters np;
    np.initial_apd = 300.0;
    vector<NodeParameters> v_np = {np};
    //Eigen::VectorXf fiber_dir = Eigen::Vector3f(0.7, 0.7, 0.0);
    Eigen::VectorXf fiber_dir = Eigen::Vector3f(0, 0, 0.0);
    tissue.Init(v_type, v_np, {fiber_dir});

    tissue.SetTimer(SystemEventType::FILE_WRITE, 800);
    tissue.SetTimer(SystemEventType::EXT_ACTIVATION, 200);

    size_t initial_node = tissue.GetIndex(10,2,1);
    int beat = 0;
    tissue.ExternalActivation({initial_node}, 0.0, beat);
    tissue.SaveVTK("output/testr0.vtk");
    std::cout << 0 << std::endl;

    int i = 0;
    while (tissue.GetTime() < 500)
    {
        auto tick = tissue.update(1);
        if(tick == SystemEventType::EXT_ACTIVATION)
        {
            beat++;
            std::cout << "\nExternal activation: " << beat << " " << tissue.GetTime() << std::endl;
            tissue.ExternalActivation({initial_node}, tissue.GetTime(), beat);
        }


        if(tick == SystemEventType::FILE_WRITE)
        {
            //std::cout << i << " " << tissue.GetTime() << std::endl;
            tissue.SaveVTK("output/testr"+ std::to_string(i) +".vtk");
        }
        i++;
    }

    return 0;
}
