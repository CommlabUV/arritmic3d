import tissue_module
import numpy as np

def main():
    print("0", flush=True)
    tissue = tissue_module.CardiacTissue(6, 6, 6, 0.1, 0.1, 0.2)
    v_type = [tissue_module.CellType.HEALTHY] * (6 * 6 * 6)
    initial_apd = 1.0
    #v_apd = np.full((6 * 6 * 6), initial_apd)
    v_apd = [initial_apd] * (6 * 6 * 6)
    parameters = {"INITIAL_APD" : v_apd}
    v_region = [tissue_module.TissueRegion.ENDO] * (6 * 6 * 6)
    tissue.InitPy(v_type, v_region, parameters)
    print("tissue initialized", flush=True)

    initial_node = tissue.GetIndex(2, 2, 1)
    tissue.ExternalActivation([initial_node], 0.0)
    tissue.SaveVTK("output/test0.vtk")
    print(0)

    for i in range(1, 141):
        if i == 120:
            tissue.ExternalActivation([initial_node], tissue.GetTime())
        tissue.update(1)
        print(i, tissue.GetTime())
        if i % 4 == 0:
            tissue.SaveVTK(f"output/test{i}.vtk")

if __name__ == "__main__":
    main()