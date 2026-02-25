import arritmic3d
import numpy as np

HEALTHY_ENDO = 1

def main():
    print("-- Initialization --", flush=True)
    tissue = arritmic3d.CardiacTissue(10, 6, 4, 0.1, 0.1, 0.1)
    v_type = [HEALTHY_ENDO] * tissue.size()
    initial_apd = 200.0
    apd_correction_factor = 1.0
    v_cf = [apd_correction_factor] * tissue.size()
    v_sensor = [0.0] * tissue.size()
    v_sensor[tissue.GetIndex(5, 3, 1)] = 1.0
    parameters = {"CORRECTION_FACTOR_APD" : v_cf, "SENSOR" : v_sensor}
    tissue.InitModels("restitutionModels/config_TenTuscher_APD.csv","restitutionModels/config_TenTuscher_CV.csv")
    #tissue.InitModels("restitutionModels/config_TorOrd_APD.csv","restitutionModels/config_TorOrd_CV.csv")
    tissue.SetInitialAPD(initial_apd)
    tissue.InitPy(v_type, parameters)
    print("Tissue size: ", tissue.size())
    print("Tissue live nodes: ", tissue.GetNumLiveNodes())

    initial_node = tissue.GetIndex(2, 2, 1)
    beat = 0
    CL = 300.0  # cycle length in ms
    tissue.SetSystemEvent(arritmic3d.SystemEventType.EXT_ACTIVATION, 1)
    #tissue.SaveVTK("output/test0.vtk")

    print("-- Begin Simulation --", flush=True)

    for i in range(1, 1000):
        tick = tissue.update()
        #print(i, tissue.GetTime())

        if tick == arritmic3d.SystemEventType.EXT_ACTIVATION:
            beat += 1
            print("Mean APD variation: ", tissue.GetAPDMeanVariation())
            tissue.ResetVariations()

            print("External activation for beat ", beat, " at time ", tissue.GetTime())
            tissue.ExternalActivation([initial_node], tissue.GetTime(), beat)
            tissue.SetSystemEvent(arritmic3d.SystemEventType.EXT_ACTIVATION, tissue.GetTime() + CL)

        if tick == arritmic3d.SystemEventType.FILE_WRITE:
            tissue.SaveVTK(f"output/test{i}.vtk")

    Sensors = tissue.GetSensorInfo()
    print("\n", Sensors)


if __name__ == "__main__":
    main()