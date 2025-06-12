/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */
#include <iostream>
#include <string>
#include "../src/definitions.h"
#include "../src/spline.h"

int main(int argc, char **argv)
{
    std::cout << "Begin" << std::endl;

    SplineContainer splines;

    std::cout << "-- Spline for healthy cells" << std::endl;
    Spline * s1 = splines.getSpline(CellType::HEALTHY, TissueRegion::ENDO);
    s1->show();

    std::cout << "-- Spline for border zone cells" << std::endl;
    Spline * s2 = splines.getSpline(CellType::BORDER_ZONE, TissueRegion::ENDO);
    s2->show();

    std::cout << "End" << std::endl;
    return 0;
}
