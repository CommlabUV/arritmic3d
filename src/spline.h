/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef SPLINE_H
#define SPLINE_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <map>
#include <cmath>
#include <initializer_list>
#include "../src/rapidcsv.h"

#include "definitions.h"

using std::vector;


class Spline
{
private:
    vector<float> x;
    vector<float> y;
    vector<float> m;

    const float ALMOST_ZERO = 1e-8;

    void ComputeSlopes()
    {
        m.clear();
        for (int i = 1; i < int(x.size()); i++)
        {
            float d = (x[i] - x[i-1]);
            if (abs(d) > ALMOST_ZERO )
                m.push_back( (y[i] - y[i-1])/d );
            else
                m.push_back(0.0);
        }

    }

public:

    Spline()
    {
    }

    float getValue(float t) const
    {
        unsigned int n = x.size();

        if ( n == 0 ) return 0.0;
        if ( n == 1 ) return x[0];

        if (t <= x[0]  ) return y[0];
        if (t >= x[n-1]) return y[n-1];

        unsigned int i = 1;
        while (t >= x[i])
            i += 1;

        return y.at(i-1) + m.at(i-1) * (t - x.at(i-1));

    }

    void setPoints(vector<float> x_,vector<float> y_)
    {
        assert(x_.size() == y_.size());
        x = x_;
        y = y_;
        ComputeSlopes();
    }

    void show()
    {
        for (int i = 0; i < int(x.size()); i++)
            std::cout << x[i] << " " << y[i] << " " << m[i] << std::endl;
    }

};

class SplineContainer
{
    struct par
    {
        CellType type;
        TissueRegion region;
        bool operator<(const par & other) const
        {
            return (type < other.type) || (type == other.type && region < other.region);
        }
    };

public:
    SplineContainer()
    {
        // @TODO More general way to load the splines
        std::string path = "restitutionCurves/";

        //addSpline(CellType::HEALTHY, 0, {5.0, 700.0}, {133.0, 290.0}); // Endo sano
        addSpline(CellType::HEALTHY, TissueRegion::ENDO, path + "RestitutionCurve_Sanas_APD_Endo.csv"); // Endo sano
        addSpline(CellType::BORDER_ZONE , TissueRegion::ENDO, path + "RestitutionCurve_BZ_APD_Endo.csv"); // Endo BZ
    }

    SplineContainer(std::initializer_list<std::tuple<CellType, TissueRegion, std::string>> splines)
    {
        for (auto s : splines)
        {
            addSpline(std::get<0>(s), std::get<1>(s), std::get<2>(s));
        }
    }

    /**
     * @brief Add a new spline using the given points.
     * @param type Cell type
     * @param region Cell region
    */
    void addSpline(CellType type, TissueRegion region, vector<float> x, vector<float> y)
    {
        Spline s;
        s.setPoints(x,y);
        splines.insert({{type,region}, s});
    }

    /**
     * @brief Add a new spline using a csv file.
     * @param type Cell type
     * @param region Cell region
     * @param filename Name of the csv file
    */
    void addSpline(CellType type, TissueRegion region, std::string filename)
    {
        try{
            rapidcsv::Document doc(filename, rapidcsv::LabelParams(-1,-1) ); // No header
            vector<float> x = doc.GetColumn<float>(0);
            vector<float> y = doc.GetColumn<float>(1);
            if(x.size() != y.size())
                throw std::runtime_error("Different number of points in " + filename);
            addSpline(type, region, x, y);
        }
        catch (std::exception &e)
        {
            std::cerr << "Error reading " << filename << std::endl;
            throw;
        }
    }

    Spline * getSpline(CellType type, TissueRegion region)
    {
        auto it = splines.find({type,region});
        if (it == splines.end())
            return nullptr;
        return &it->second;
    }

private:
    std::map<par,Spline> splines;
};

#endif // SPLINE_H
