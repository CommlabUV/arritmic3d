/**
 * ARRITMIC3D
 * Test save, load and init methods
 *
 * (C) CoMMLab-UV 2025
 * */
#include <iostream>
#include <string>
#include "../src/node.h"
#include "../src/tissue.h"
#include "../src/action_potential_rs.h"
#include "../src/conduction_velocity.h"

enum CellTypeVentricle { HEALTHY_ENDO = 1, HEALTHY_MID, HEALTHY_EPI, BZ_ENDO, BZ_MID, BZ_EPI };

int main(int argc, char **argv)
{
    // Test of the CardiacTissue class. Units in mm.
    CardiacTissue<ActionPotentialRestSurface,ConductionVelocity> tissue(10, 6, 4, 0.1, 0.1, 0.1);

    tissue.InitModels("restitutionModels/config_TenTuscher_APD.csv","restitutionModels/config_TenTuscher_CV.csv");
    //tissue.Init(v_type, v_np, {fiber_dir});
    tissue.LoadState("tissue_state.bin");
    std::cout << "Tissue size: " << tissue.size() << std::endl;
    std::cout << "Tissue live nodes: " << tissue.GetNumLiveNodes() << std::endl;

    size_t initial_node = tissue.GetIndex(2,2,1);  // 1*6*6 + 2*6 + 2
    int beat = 5;   // Start from beat 5, as the first 4 beats are already in the loaded state
    std::cout << "Time: " << tissue.GetTime() << std::endl;
    //tissue.SetSystemEvent(SystemEventType::EXT_ACTIVATION, tissue.GetTime() );

    std::cout << "--- Begin simulation ---" << std::endl;

    for(int i = 1; i <= 1200; ++i)
    {
        auto tick = tissue.update(0);
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
        tissue.SaveVTK("output/test"+ std::to_string(i) +".vtk");
    }

    tissue.ShowSensorData();

    return 0;
}
