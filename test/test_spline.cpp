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

    SplineContainer splines({{1,"restitutionModels/TenTuscher_CV_HE_Mid.csv"},{2,"restitutionModels/TenTuscher_CV_HE_Epi.csv"}});

    std::cout << "-- Spline for healthy cells" << std::endl;
    Spline * s1 = splines.getSpline(1);
    s1->show();

    std::cout << "-- Spline for border zone cells" << std::endl;
    Spline * s2 = splines.getSpline(2);
    s2->show();

    std::cout << "End" << std::endl;
    return 0;
}
