/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */
#include <iostream>
#include <string>
#include "../src/definitions.h"
#include "../src/spline2D.h"

enum CellTypeVentricle { HEALTHY_ENDO = 1, HEALTHY_MID, HEALTHY_EPI, BZ_ENDO, BZ_MID, BZ_EPI };

int main(int argc, char **argv)
{
    std::cout << "Begin" << std::endl;

    //SplineContainer2D splines;
    const std::string path = "restitutionModels/";

    SplineContainer2D splines({{HEALTHY_ENDO, path + "TenTuscher_APD_HE_Endo.csv"},
                                {HEALTHY_MID, path + "TenTuscher_APD_HE_Mid.csv"},
                                {HEALTHY_EPI, path + "TenTuscher_APD_HE_Epi.csv"},    // Test of 2D spline
                                    });

    std::cout << "-- Spline for endo cells" << std::endl;
    Spline2D * s1 = splines.getSpline(HEALTHY_ENDO);
    s1->show();
    for(int i = 0; i < s1->size(0); i++)
    {
        std::cout << "Pos no-value row " << i << ": " << s1->GetPosNoValue(0, i) << std::endl;
    }
    for(int i = 0; i < std::min(s1->size(1), 6); i++)
    {
        std::cout << "Pos no-value column " << i << ": " << s1->GetPosNoValue(1, i) << std::endl;
    }
    std::cout << "-- Get Label No Value APD 200: " << s1->GetLabelNoValue(0, 200) << std::endl;
    std::cout << "-- Get Label No Value APD 400: " << s1->GetLabelNoValue(0, 400) << std::endl;

    std::cout << "-- Spline for epi cells" << std::endl;
    //Spline2D * s2 = splines.getSpline(HEALTHY_MID);
    Spline2D * s2 = splines.getSpline(HEALTHY_EPI);
    //s2->show();

    std::cout << "-- Get Value endo: " << std::endl;
    std::cout << "(94.0, 20.0) -> " << s1->getValue(94.0, 20.0) << std::endl;
    std::cout << "(94.0, 40.0) -> " << s1->getValue(94.0, 40.0) << std::endl;
    std::cout << "(204.0, 40.0) -> " << s1->getValue(204.0, 40.0) << std::endl;
    std::cout << "(94.12, 41.2) -> " << s1->getValue(94.12, 41.2) << std::endl;

    std::cout << "-- Get Value epi: " << std::endl;
    std::cout << "(5.0) -> " << s2->getValue(5.0, 2) << std::endl;

    std::cout << "End" << std::endl;
    return 0;
}
