/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */
#include <iostream>
#include <string>
#include "../src/definitions.h"
#include "../src/spline2D.h"

int main(int argc, char **argv)
{
    std::cout << "Begin" << std::endl;

    //SplineContainer2D splines;
    const std::string path = "restitutionSurfaces/";

    SplineContainer2D splines({{CellType::HEALTHY, TissueRegion::ENDO, path + "RestitutionSurface_Sanas_APD_Endo.csv"},
                                {CellType::BORDER_ZONE, TissueRegion::ENDO, path + "RestitutionSurface_BZ_APD_Endo.csv"},
                                {CellType::HEALTHY, TissueRegion::EPI, path + "RestitutionCurve_Sanas_APD_Epi.csv"},    // Test of 1D spline
                                    });

    std::cout << "-- Spline for healthy cells" << std::endl;
    Spline2D * s1 = splines.getSpline(CellType::HEALTHY, TissueRegion::ENDO);
    //s1->show();

    std::cout << "-- Spline for border zone cells" << std::endl;
    //Spline2D * s2 = splines.getSpline(CellType::BORDER_ZONE, TissueRegion::ENDO);
    Spline2D * s2 = splines.getSpline(CellType::HEALTHY, TissueRegion::EPI);
    //s2->show();

    std::cout << "-- Get Value: " << std::endl;
    std::cout << "(94.0, 20.0) ->" << s1->getValue(94.0, 20.0) << std::endl;
    std::cout << "(94.0, 40.0) ->" << s1->getValue(94.0, 40.0) << std::endl;
    std::cout << "(94.12, 41.2) ->" << s1->getValue(94.12, 41.2) << std::endl;

    std::cout << "(5.0) ->" << s2->getValue(5.0, 2) << std::endl;

    std::cout << "End" << std::endl;
    return 0;
}
