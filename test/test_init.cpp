/**
 * ARRITMIC3D
 * Test init methods
 *
 * (C) CoMMLab-UV 2025
 * */
#include <iostream>
#include <string>
#include "../src/node.h"
#include "../src/tissue.h"
#include "../src/action_potential_rc.h"
#include "../src/action_potential_rs.h"
#include "../src/conduction_velocity.h"
#include "../src/conduction_velocity_simple.h"

enum CellTypeVentricle { HEALTHY_ENDO = 1, HEALTHY_MID, HEALTHY_EPI, BZ_ENDO, BZ_MID, BZ_EPI };

int main(int argc, char **argv)
{
    // Test of the CardiacTissue class. Units in mm.
    CardiacTissue<ActionPotentialRestSurface,ConductionVelocity> tissue(10, 6, 4, 0.1, 0.1, 0.1);
    std::vector<CellType> v_type(10*6*4, HEALTHY_ENDO);

    NodeParameters np;
    np.initial_apd = 100.0;
    vector<NodeParameters> v_np(tissue.size(), np);
    v_np.at(tissue.GetIndex(5, 3, 1)).sensor = 1;  // Set a sensor

    Eigen::VectorXf fiber_dir = Eigen::Vector3f(1.0, 0.0, 0.0);
    tissue.InitModels("restitutionModels/config_TenTuscher_APD.csv","restitutionModels/config_TenTuscher_CV.csv");
    tissue.Init(v_type, v_np, {fiber_dir});
    std::cout << "Tissue size: " << tissue.size() << std::endl;
    std::cout << "Tissue live nodes: " << tissue.GetNumLiveNodes() << std::endl;

    size_t initial_node = tissue.GetIndex(2,2,1);  // 1*6*6 + 2*6 + 2
    int beat = 0;
    tissue.SetSystemEvent(SystemEventType::EXT_ACTIVATION, 200);

    tissue.SaveVTK("output/test0.vtk");
    std::cout << "--- Begin simulation ---" << std::endl;

    for(int i = 1; i <= 1500; ++i)
    {
        auto tick = tissue.update();
        //std::cout << i << " " << tissue.GetTime() << std::endl;
        if(tick == SystemEventType::EXT_ACTIVATION)
        {
            if(beat == 4)
            {
                //break; // Stop after 4 beats

                NodeParameters np3;
                np3.correction_factor_apd = 0.5;
                vector<NodeParameters> v_np3(tissue.size(), np3);
                v_np3.at(tissue.GetIndex(5, 2, 1)).sensor = 1;  // Set a sensor
                tissue.ChangeParameters(v_np3);
                std::cout << "--- Changed tissue parameters " << std::endl;
            }
            beat++;
            std::cout << "Mean APD variation: " << tissue.GetAPDMeanVariation() << std::endl;
            tissue.ResetVariations();

            std::cout << "External activation for beat " << beat << " at time " << tissue.GetTime() << std::endl;
            tissue.ExternalActivation({initial_node}, tissue.GetTime(), beat);
            tissue.SetSystemEvent(SystemEventType::EXT_ACTIVATION, tissue.GetTime() + 300);
            // Write VTK file after activation
            //tissue.SetSystemEvent(SystemEventType::FILE_WRITE, tissue.GetTime() + 20);
        }
        if(tick == SystemEventType::FILE_WRITE)
            tissue.SaveVTK("output/test"+ std::to_string(i) +".vtk");

        //std::cout << "State of initial node: " << tissue.GetStates()[initial_node] << std::endl;
        // Write after each event
        //tissue.SaveVTK("output/test"+ std::to_string(i) +".vtk");
    }
    tissue.ShowSensorData();

    // Re-initialize tissue with different parameters
    std::cout << "--- Re-initialized tissue " << std::endl;
    std::vector<CellType> v_type2(10*6*4, BZ_ENDO);
    NodeParameters np2;
    np2.initial_apd = 300.0;
    vector<NodeParameters> v_np2(tissue.size(), np2);
    v_np2.at(tissue.GetIndex(5, 3, 1)).sensor = 1;  // Set a sensor

    tissue.Init(v_type2, v_np2, {fiber_dir});
    tissue.SetSystemEvent(SystemEventType::EXT_ACTIVATION, tissue.GetTime() );

    for(int i = 1; i <= 1500; ++i)
    {
        auto tick = tissue.update();
        //std::cout << i << " " << tissue.GetTime() << std::endl;
        if(tick == SystemEventType::EXT_ACTIVATION)
        {
            beat++;
            std::cout << "Mean APD variation: " << tissue.GetAPDMeanVariation() << std::endl;
            tissue.ResetVariations();

            std::cout << "External activation for beat " << beat << " at time " << tissue.GetTime() << std::endl;
            tissue.ExternalActivation({initial_node}, tissue.GetTime(), beat);
            tissue.SetSystemEvent(SystemEventType::EXT_ACTIVATION, tissue.GetTime() + 300);
            // Write VTK file after activation
            //tissue.SetSystemEvent(SystemEventType::FILE_WRITE, tissue.GetTime() + 20);
        }
        if(tick == SystemEventType::FILE_WRITE)
            tissue.SaveVTK("output/test"+ std::to_string(i) +".vtk");

        //std::cout << "State of initial node: " << tissue.GetStates()[initial_node] << std::endl;
        // Write after each event
        //tissue.SaveVTK("output/test"+ std::to_string(i) +".vtk");
    }

    tissue.ShowSensorData();


    return 0;
}
