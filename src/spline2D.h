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
#include <filesystem>
#include <initializer_list>
#include "../src/rapidcsv.h"
#include <Eigen/Dense>
#include "definitions.h"

using std::vector;

namespace fs = std::filesystem;


/**
 * @brief 2D spline class for 2D interpolation.
 */
class Spline2D
{
private:
    Eigen::ArrayXf x[2];    // 0: rows, 1: columns
    Eigen::ArrayXXf y;
    //Eigen::ArrayXXf m;
    Eigen::ArrayXi first_novalue_pos[2];
    bool same_interval[2] = {false,false};
    float d[2] = {0.0,0.0};
    bool monotonic = false;

    const float ALMOST_ZERO = 1e-8;

public:
    bool is_novalue(float x) const
    {
        return x < 0;
    }

    int size(int dim) const
    {
        assert(dim == 0 || dim == 1);
        return x[dim].size();
    }

    /**
     * @brief Find the index of the given value in the given dimension.
     * @param dim Dimension (0: rows, 1: columns)
     * @param value Value to find
     * @return Index of the value
     */
    int FindIndex(int dim, float value) const
    {
        assert(dim >= 0 && dim <= 1);
        unsigned int n = x[dim].size();

        int index = 0;
        if (value <= x[dim][0] )   // Check if the point is out of the spline
            index = 0;
        else if (value >= x[dim][ n - 1 ])
            index = n - 1;
        else
        {
            if(same_interval[dim])
                index = (value - x[dim][0]) / d[dim];

            while(value < x[dim][index])
                index -= 1;
            while (value > x[dim][index+1])
                index += 1;

            // Take the nearest point. Can be done with interpolation instead.
            if(abs(value - x[dim][index]) > abs(value - x[dim][index+1]))
                index++;

        }
        return index;
    }

    float getValue(float row, float col) const
    {
        // @TODO Erase this assert when checked that nan is impossible
        assert(! std::isnan(row) && ! std::isnan(col));

        int x0 = FindIndex(0, row);
        int x1 = FindIndex(1, col);

        return y(x0, x1);

    }

    float getEquilibrium(int dim, float value) const
    {
        assert(dim == 0 || dim == 1);
        int other_dim = 1 - dim;

        int index = FindIndex(dim, value);
        // Find the value in the other dimension for which the result of getValue is the same.
        // We assume that the value inceases monotonically. @todo Generalizes or check it doesn't matter.
        int i = 0;
        if(dim == 0)
            while(i < x[other_dim].size() - 1 && value > y(index,i) )
                i++;
        else
            while(i < x[other_dim].size() - 1 && value > y(i,index) )
                i++;
        return x[other_dim][i];
    }

    /**
     * @brief Get the position of the first no-value in the given row or column beginning from the end.
     * @param row_or_col 0 for rows, 1 for columns
     * @param index Index of the row or column
     * @return Position of the first no-value. -1 if all values are valid.
     */
    int GetPosNoValue(int row_or_col, int index) const
    {
        assert(row_or_col >= 0 && row_or_col <= 1);
        return first_novalue_pos[row_or_col][index];
    }

    float GetLabelNoValue(int row_or_col, float index_label) const
    {
        assert(row_or_col >= 0 && row_or_col <= 1);
        int x_dim = 1 - row_or_col;
        int index = FindIndex(row_or_col, index_label);

        if(first_novalue_pos[row_or_col][index] >= 0)
        {
            return x[x_dim][ first_novalue_pos[row_or_col][index] ];
        }
        else
        {
            float label = x[x_dim][ 0 ] - d[x_dim];
            return label;
        }
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
        if(x[0].size() > 1)
            d[0] = x[0][1] - x[0][0];

        same_interval[1] = IsSameInterval(x[1]);
        if(x[1].size() > 1)
            d[1] = x[1][1] - x[1][0];
        //monotonic = IsMonotonic(x[0]) && IsMonotonic(x[1]);
        //ComputeSlopes();

        // Find the first non-value position for each dimension
        first_novalue_pos[1] = Eigen::ArrayXi::Constant(x[1].size(), -1);
        for (int i = 0; i < x[1].size(); i++)
        {
            int pos = 0;
            while(pos < y.rows() && is_novalue(y(pos,i)) )
                pos++;
            first_novalue_pos[1][i] = pos-1;
        }

        first_novalue_pos[0] = Eigen::ArrayXi::Constant(x[0].size(), -1);
        for (int i = 0; i < x[0].size(); i++)
        {
            int pos = 0;
            while(pos < y.cols() && is_novalue(y(i,pos)) )
                pos++;
            first_novalue_pos[0][i] = pos-1;
        }

        //std::cout << "\nRow info: " << x[0][0] << " - " << x[0][x[0].size() - 1] << " " << d[0] << std::endl;
        //std::cout << "Col info: " << x[1][0] << " - " << x[1][x[1].size() - 1] << " " << d[1] << std::endl;
    }

    void show()
    {
        std::cout << "Same interval: " << same_interval[0] << " - " << same_interval[1] << std::endl;
        std::cout << "Rows: " << x[0].transpose() << std::endl;
        std::cout << "Columns: " << x[1].transpose() << std::endl;
        ///< @todo This throws a compilation error.
        //std::cout << "Values: \n" << y << std::endl;
        std::cout << "First no-value positions (rows): " << first_novalue_pos[0].transpose() << std::endl;
        std::cout << "First no-value positions (columns): " << first_novalue_pos[1].transpose() << std::endl;
    }

};

class SplineContainer2D
{

public:
    SplineContainer2D() = default;

    SplineContainer2D(std::initializer_list<std::tuple<CellType, std::string>> splines)
    {
        for (auto s : splines)
        {
            addSpline(std::get<0>(s), std::get<1>(s));
        }
    }

    /**
     * @brief Initialize the container from a configuration file.
     * @param filename Name of the configuration file containing the list of restitution models.
     */
    void Init(const fs::path &filename)
    {
        fs::path models_directory = filename.parent_path();  // directory containing the config file
        try {
            rapidcsv::Document doc(filename.string(), rapidcsv::LabelParams(-1,-1) ); // No header
            vector<int> type = doc.GetColumn<int>(0);
            vector<std::string> file = doc.GetColumn<std::string>(1);
            if(type.size() != file.size())
                throw std::runtime_error("Different number of elements in " + filename.string());
            for(size_t i = 0; i < type.size(); i++)
                addSpline(static_cast<CellType>(type[i]), models_directory / file[i]);
        }
        catch (std::exception &e)
        {
            std::cerr << "Error processing " << filename.string() << std::endl;
            throw;
        }
    }

    /**
     * @brief Add a new spline using the given points.
     * @param type Cell type
     * @param region Cell region
    */
    void addSpline(CellType type, Eigen::ArrayXf x_row, Eigen::ArrayXf x_column,Eigen::ArrayXXf y_)
    {
        Spline2D s;
        s.setPoints(x_row, x_column, y_);
        splines.insert({type, s});
    }

    /**
     * @brief Add a new spline using a csv file.
     * @param type Cell type
     * @param region Cell region
     * @param filename Name of the csv file
    */
    void addSpline(CellType type, std::filesystem::path filename)
    {
        addSpline(type, filename.string());
    }

    /**
     * @brief Add a new spline using a csv file.
     * @param type Cell type
     * @param region Cell region
     * @param filename Name of the csv file
    */
    void addSpline(CellType type, std::string filename)
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

            addSpline(type, x0, x1, y);
        }
        catch (std::exception &e)
        {
            std::cerr << "Error reading " << filename << std::endl;
            throw;
        }
    }

    Spline2D * getSpline(CellType type)
    {
        auto it = splines.find(type);
        if (it == splines.end())
            return nullptr;
        return &it->second;
    }

private:
    std::map<CellType,Spline2D> splines;
};

#endif // SPLINE2D_H
