/**
 * ARRITMIC3D
 * Test of the CardiacTissue class with a cubic heart.
 *
 * (C) CoMMLab-UV 2023
 * */
#include <iostream>
#include <string>
#include "../src/tissue.h"
#include "../src/action_potential_rc.h"
#include "../src/conduction_velocity.h"

const int N_NODES = 50;
const int TOTAL_NODES = N_NODES*N_NODES*N_NODES;

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

    // Test of the CardiacTissue class
    // Number of nodes: 98x98x98 - 88x88x88 = 941192 - 681472 = 259720
    CardiacTissue<ActionPotentialRestCurve,ConductionVelocity> tissue(N_NODES, N_NODES, N_NODES, 0.1, 0.1, 0.1);
    std::vector<CellType> v_type(TOTAL_NODES, CellType::HEALTHY);
    //tissue.SetBorder(v_type, CellType::CORE);
    SetCore(tissue, v_type, N_NODES-10, N_NODES-10, N_NODES-10, CellType::CORE);

    vector<NodeParameters> v_np(1);
    Eigen::VectorXf fiber_dir = Eigen::Vector3f(0.7, 0.7, 0.0);
    tissue.Init(v_type, v_np, {fiber_dir});
    tissue.SetTimer(5);

    size_t initial_node = tissue.GetIndex(2,2,2);   //(1,2,2);
    int s1 = 300;
    float next_activation_time = 0.0;
    int beat = 0;

    tissue.ExternalActivation({initial_node}, next_activation_time, beat);
    tissue.SaveVTK("output/testb0.vtk");
    std::cout << 0 << std::endl;


    float t = tissue.GetTime();
    int i = 0;
    while( t < 1000.0)
    {
        bool tick = tissue.update();
        /*if(tick)
        {
            std::cout << "External activation: " << i << " " << tissue.GetTime() << std::endl;
            tissue.ExternalActivation({initial_node}, tissue.GetTime() );
        }*/
        t = tissue.GetTime();
        i++;
        if(tick)
        {
            std::cout << i << " " << t << std::endl;
            tissue.SaveVTK("output/testb"+ std::to_string(int(t)) +".vtk");
        }

        if (t >= next_activation_time)
        {
            beat++;
            next_activation_time = next_activation_time + s1;
            tissue.ExternalActivation({initial_node}, next_activation_time, beat);
            std::cout << "External activation schedulead for beat " << beat << " at time " << next_activation_time << std::endl;
        }

    }

    return 0;
}
