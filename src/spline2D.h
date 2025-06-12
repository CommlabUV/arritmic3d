/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef SPLINE2D_H
#define SPLINE2D_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <map>
#include <cmath>
#include <initializer_list>
#include "../src/rapidcsv.h"
#include <eigen3/Eigen/Dense>

#include "definitions.h"

using std::vector;

/**
 * @brief 2D spline class for 2D interpolation.
 */
class Spline2D
{
private:
    Eigen::ArrayXf x[2];
    Eigen::ArrayXXf y;
    //Eigen::ArrayXXf m;
    bool same_interval[2] = {false,false};
    float d[2] = {0.0,0.0};
    bool monotonic = false;

    const float ALMOST_ZERO = 1e-8;

public:

    float getValue(float row, float col) const
    {
        unsigned int n0 = x[0].size();
        unsigned int n1 = x[1].size();
        int x0 = 0;
        int x1 = 0;

        if(std::isnan(row) || std::isnan(col))
            return NAN;

        // Find row index (x0)
        if (row <= x[0][0] )   // Check if the point is out of the spline
            x0 = 0;
        else if (row >= x[0][n0-1])
            x0 = n0-1;
        else
        {
            if(same_interval[0])
                x0 = (row - x[0][0]) / d[0];

            //std::cout << "row: " << x0 << std::endl;
            while(row < x[0][x0])
                x0 -= 1;
            while (row > x[0][x0+1])
                x0 += 1;

            // Take the nearest point. Can be done with interpolation instead.
            if(abs(row - x[0][x0]) > abs(row - x[0][x0+1]))
                x0++;

            //std::cout << "row: " << x0 << " " << d[0] << std::endl;

        }

        // Find column index (x1)
        if (col <= x[1][0] )   // Check if the point is out of the spline
            x1 = 0;
        else if (col >= x[1][n1-1])
            x1 = n1-1;
        else
        {
            if(same_interval[1])
                x1 = (col - x[1][0]) / d[1];

            //std::cout << "col: " << x1 << std::endl;
            while(col < x[1][x1])
                x1 -= 1;
            while (col > x[1][x1+1])
                x1 += 1;

            // Take the nearest point. Can be done with interpolation instead.
            if(abs(col - x[1][x1]) > abs(col - x[1][x1+1]))
                x1++;

            //std::cout << "col: " << x1 << std::endl;

        }

        return y(x0, x1);

    }

    bool IsSameInterval(const Eigen::ArrayXf & x)
    {
        if (x.size() < 2)
            return false;

        float d = x[1] - x[0];
        float small_error = d / 20.0;

        for (int i = 2; i < x.size(); i++)
        {
            if (abs(x[i] - x[i-1] - d) > small_error)
                return false;
        }
        return true;
    }

    void setPoints(Eigen::ArrayXf x_row, Eigen::ArrayXf x_column,Eigen::ArrayXXf y_)
    {
        assert(x_row.size() == y_.rows() && x_row.size() > 0);
        assert(x_column.size() == y_.cols() && x_column.size() > 0);
        x[0] = x_row;
        x[1] = x_column;
        y = y_;

        same_interval[0] = IsSameInterval(x[0]);
        // If same_interval is true, there are at least two points
        if(same_interval[0])
            d[0] = x[0][1] - x[0][0];
        same_interval[1] = IsSameInterval(x[1]);
        if(same_interval[1])
            d[1] = x[1][1] - x[1][0];
        //monotonic = IsMonotonic(x[0]) && IsMonotonic(x[1]);
        //ComputeSlopes();

        //std::cout << "\nRow info: " << x[0][0] << " - " << x[0][x[0].size() - 1] << " " << d[0] << std::endl;
        //std::cout << "Col info: " << x[1][0] << " - " << x[1][x[1].size() - 1] << " " << d[1] << std::endl;
    }

    void show()
    {
        std::cout << "Same interval: " << same_interval[0] << " - " << same_interval[1] << std::endl;
        std::cout << "Rows: " << x[0].transpose() << std::endl;
        std::cout << "Columns: " << x[1].transpose() << std::endl;
        std::cout << "Values: \n" << y << std::endl;
    }

};

class SplineContainer2D
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
    SplineContainer2D(const std::string path = "restitutionSurfaces/")
    {

        //addSpline(CellType::HEALTHY, TissueRegion::ENDO, {5.0, 700.0}, {133.0, 290.0}); // Endo sano
        addSpline(CellType::HEALTHY, TissueRegion::ENDO, path + "RestitutionSurface_Sanas_APD_Endo.csv"); // Endo sano
        addSpline(CellType::BORDER_ZONE , TissueRegion::ENDO, path + "RestitutionSurface_BZ_APD_Endo.csv"); // Endo BZ
    }

    SplineContainer2D(std::initializer_list<std::tuple<CellType, TissueRegion, std::string>> splines)
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
    void addSpline(CellType type, TissueRegion region, Eigen::ArrayXf x_row, Eigen::ArrayXf x_column,Eigen::ArrayXXf y_)
    {
        Spline2D s;
        s.setPoints(x_row, x_column, y_);
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

            // Get column ids
            vector<float> x1_ = doc.GetRow<float>(0);
            if(x1_.size() < 2)
                throw std::runtime_error("Not enough columns in: " + filename);
            if(x1_[0] != 0.0)
                throw std::runtime_error("Incorrect format in: " + filename);
            x1_.erase(x1_.begin());
            Eigen::ArrayXf x1 = Eigen::Map<Eigen::ArrayXf>(x1_.data(), x1_.size());

            // Get row ids
            vector<float> x0_ = doc.GetColumn<float>(0);
            if(x0_.size() < 2)
                throw std::runtime_error("Not enough rows in: " + filename);
            x0_.erase(x0_.begin());
            Eigen::ArrayXf x0 = Eigen::Map<Eigen::ArrayXf>(x0_.data(), x0_.size());

            // Get values
            Eigen::ArrayXXf y(doc.GetRowCount()-1, doc.GetColumnCount()-1);
            for (unsigned i = 1; i < doc.GetRowCount(); i++)
            {
                vector<float> row = doc.GetRow<float>(i);
                row.erase(row.begin());
                y.row(i-1) = Eigen::Map<Eigen::ArrayXf>(row.data(), row.size());
            }

            addSpline(type, region, x0, x1, y);
        }
        catch (std::exception &e)
        {
            std::cerr << "Error reading " << filename << std::endl;
            throw;
        }
    }

    Spline2D * getSpline(CellType type, TissueRegion region)
    {
        auto it = splines.find({type,region});
        if (it == splines.end())
            return nullptr;
        return &it->second;
    }

private:
    std::map<par,Spline2D> splines;
};

#endif // SPLINE2D_H
