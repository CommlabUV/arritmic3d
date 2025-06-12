/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */
#include <iostream>
#include <string>
#include "../src/node.h"
#include "../src/spline.h"
#include "../src/cell_event_queue.h"
#include "../src/tissue.h"
#include "../src/action_potential_rc.h"
#include "../src/conduction_velocity.h"
#include "../src/conduction_velocity_simple.h"

int main(int argc, char **argv)
{
    // Test of the CardiacTissue class. Units in meters.
    //CardiacTissue<ActionPotentialRestCurve,ConductionVelocitySimple> tissue(6, 6, 6, 0.1, 0.1, 0.2);  // Constant CV
    //CardiacTissue<ActionPotentialRestCurve,ConductionVelocity> tissue(6, 6, 6, 0.1, 0.1, 0.2);
    CardiacTissue<ActionPotentialRestCurve,ConductionVelocity> tissue(10, 6, 4, 0.1, 0.1, 0.1);
    std::vector<CellType> v_type(10*6*4, CellType::HEALTHY);

    NodeParameters np;
    np.initial_apd = 100.0;
    vector<NodeParameters> v_np(tissue.size(), np);
    v_np.at(tissue.GetIndex(5, 3, 1)).sensor = 1;  // Set a sensor

    Eigen::VectorXf fiber_dir = Eigen::Vector3f(1.0, 0.0, 0.0);
    tissue.Init(v_type, v_np, {fiber_dir});

    size_t initial_node = tissue.GetIndex(2,2,1);  // 1*6*6 + 2*6 + 2
    int beat = 0;
    tissue.ExternalActivation({initial_node}, 100.0, beat);
    beat++;
    tissue.SaveVTK("output/test0.vtk");
    std::cout << 0 << std::endl;

    for(int i = 1; i <= 200; ++i)
    {
        if(i == 120)
            tissue.ExternalActivation({initial_node}, tissue.GetTime() + 100.0, beat);
        tissue.update(1);
        std::cout << i << " " << tissue.GetTime() << std::endl;
        if(i % 1 == 0)
            tissue.SaveVTK("output/test"+ std::to_string(i) +".vtk");
    }

    tissue.ShowSensorData();

    return 0;
}
