import arritmic
import numpy as np

HEALTHY_ENDO = 1

def main():
    print("0", flush=True)
    tissue = arritmic.CardiacTissue(6, 6, 6, 0.1, 0.1, 0.2)
    v_type = [HEALTHY_ENDO] * (6 * 6 * 6)
    initial_apd = 1.0
    #v_apd = np.full((6 * 6 * 6), initial_apd)
    v_apd = [initial_apd] * (6 * 6 * 6)
    parameters = {"INITIAL_APD" : v_apd}
    tissue.InitModels("restitutionModels/config_TenTuscher_APD.csv","restitutionModels/config_TenTuscher_CV.csv")
    tissue.InitPy(v_type, parameters)
    print("tissue initialized", flush=True)

    initial_node = tissue.GetIndex(2, 2, 1)
    beat = 1
    tissue.ExternalActivation([initial_node], 0.0, beat)
    tissue.SaveVTK("output/test0.vtk")
    print(0)

    for i in range(1, 400):
        if i == 20:
            tissue.ExternalActivation([initial_node], tissue.GetTime(), beat)
        tissue.update(1)
        print(i, tissue.GetTime())
        if i % 4 == 0:
            tissue.SaveVTK(f"output/test_py_{i}.vtk")

if __name__ == "__main__":
    main()