/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef VTK_OUTPUT_H
#define VTK_OUTPUT_H

#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>
#include <Eigen/Dense>

#include "basic_tissue.h"
#include "node.h"
#include "error.h"

using std::vector;

/**
 * @brief Save the state of the tissue in a VTK file for visualization.
 * @param filename Name of the file to save. It should end with .vtk.
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::SaveVTK(const std::string & filename) const
{
    std::ofstream vtk_file;
    vtk_file.open(filename);
    if(!vtk_file)
    {
        LOG::Error(true, "Could not open file " + filename + " for writing.");
        return;
    }
    // Write the header
    vtk_file << "# vtk DataFile Version 3.0\n";
    vtk_file << "Cardiac Tissue\n";
    vtk_file << "ASCII\n";
    vtk_file << "DATASET RECTILINEAR_GRID\n";
    vtk_file << "DIMENSIONS " << tissue_geometry.size_x << " " << tissue_geometry.size_y << " " << tissue_geometry.size_z << std::endl;
    vtk_file << "X_COORDINATES " << tissue_geometry.size_x << " float" << std::endl;
    for(int i = 0; i < tissue_geometry.size_x; i++)
    {
        vtk_file << tissue_geometry.origin[0] + i*tissue_geometry.dx << " ";
        if((i+1) % 10 == 0)
            vtk_file << "\n";
    }
    vtk_file << std::endl;
    vtk_file << "Y_COORDINATES " << tissue_geometry.size_y << " float" << std::endl;
    for(int i = 0; i < tissue_geometry.size_y; i++)
    {
        vtk_file << tissue_geometry.origin[1] + i*tissue_geometry.dy << " ";
        if((i+1) % 10 == 0)
            vtk_file << "\n";
    }
    vtk_file << std::endl;
    vtk_file << "Z_COORDINATES " << tissue_geometry.size_z << " float" << std::endl;
    for(int i = 0; i < tissue_geometry.size_z; i++)
    {
        vtk_file << tissue_geometry.origin[2] + i*tissue_geometry.dz << " ";
        if((i+1) % 10 == 0)
            vtk_file << "\n";
    }
    vtk_file << std::endl;

    // Write the data
    vtk_file << "\nPOINT_DATA " << tissue_geometry.size_x * tissue_geometry.size_y * tissue_geometry.size_z << std::endl;
    vtk_file << "SCALARS Type int 1\n";
    vtk_file << "LOOKUP_TABLE default" << std::endl;
    for(int i = 0; i < int(this->size()); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
        {
            vtk_file << int(tissue_nodes[index].type) << " ";
        }
        else
        {
            vtk_file << int(CELL_TYPE_VOID) << " ";
        }
        if((i+1) % 10 == 0)
            vtk_file << "\n";
    }
    vtk_file << std::endl;

    vtk_file << "SCALARS State int 1\n";
    vtk_file << "LOOKUP_TABLE default" << std::endl;
    for(int i = 0; i < int(this->size()); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
        {
            vtk_file << int(tissue_nodes[index].GetState(tissue_time) ) << " ";
        }
        else
        {
            vtk_file << 0 << " ";
        }
        if((i+1) % 10 == 0)
            vtk_file << "\n";
    }
    vtk_file << std::endl;

    vtk_file.close();
}

/**
 * @brief Save the state of the tissue in a VTK file for visualization.
 * @param filename Name of the file to save. It should end with .vtk.
 * @param data_id Bitwise OR of NodeDataId values to select which data to save.
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::SaveVTKPoints(const std::string & filename, const int data_id) const
{
    std::ofstream vtk_file;
    vtk_file.open(filename);
    if(!vtk_file)
    {
        LOG::Error(true, "Could not open file " + filename + " for writing.");
        return;
    }
    // Write the header
    vtk_file << "# vtk DataFile Version 3.0\n";
    vtk_file << "Cardiac Tissue\n";
    vtk_file << "ASCII\n";
    vtk_file << "DATASET POLYDATA\n";

    // Write the points
    vtk_file << "POINTS " << this->tissue_nodes.size() << " float" << std::endl;
    for(size_t i = 0; i < this->tissue_nodes.size(); i++)
    {
        auto grid_index = tissue_nodes[i].id;
        auto coords = tissue_geometry.GetCoords_from_GridIndex(grid_index);
        vtk_file << coords[0] << " " << coords[1] << " " << coords[2] << "\n";
    }

    vtk_file << std::endl;

    // Write the data
    vtk_file << "\nPOINT_DATA " << this->tissue_nodes.size() << std::endl;

    if(data_id & NodeDataId::ID)
    {
        vtk_file << "SCALARS ID int 1\n";
        vtk_file << "LOOKUP_TABLE default" << std::endl;
        for(int i = 0; i < int(this->tissue_nodes.size()); i++)
        {
            vtk_file << int(tissue_nodes[i].id) << "\n";
        }
        vtk_file << std::endl;
    }

    if(data_id & NodeDataId::TYPE)
    {
        vtk_file << "SCALARS Type int 1\n";
        vtk_file << "LOOKUP_TABLE default" << std::endl;
        for(int i = 0; i < int(this->tissue_nodes.size()); i++)
        {
            vtk_file << int(tissue_nodes[i].type) << "\n";
        }
        vtk_file << std::endl;
    }

    if(data_id & NodeDataId::STATE)
    {
        vtk_file << "SCALARS State int 1\n";
        vtk_file << "LOOKUP_TABLE default" << std::endl;
        for(int i = 0; i < int(this->tissue_nodes.size()); i++)
        {
            vtk_file << int(tissue_nodes[i].GetState(tissue_time) ) << "\n";
        }
        vtk_file << std::endl;
    }

    if(data_id & NodeDataId::BEAT)
    {
        vtk_file << "SCALARS Beat int 1\n";
        vtk_file << "LOOKUP_TABLE default" << std::endl;
        for(int i = 0; i < int(this->tissue_nodes.size()); i++)
        {
            vtk_file << int(tissue_nodes[i].GetBeat() ) << "\n";
        }
        vtk_file << std::endl;
    }

     if(data_id & NodeDataId::APD)
    {
        vtk_file << "SCALARS APD float 1\n";
        vtk_file << "LOOKUP_TABLE default" << std::endl;
        for(int i = 0; i < int(this->tissue_nodes.size()); i++)
        {
            vtk_file << tissue_nodes[i].apd_model.getAPD() << "\n";
        }
        vtk_file << std::endl;
    }

     if(data_id & NodeDataId::CV)
    {
        vtk_file << "SCALARS CV float 1\n";
        vtk_file << "LOOKUP_TABLE default" << std::endl;
        for(int i = 0; i < int(this->tissue_nodes.size()); i++)
        {
            vtk_file << tissue_nodes[i].conduction_vel << "\n";
        }
        vtk_file << std::endl;
    }

     if(data_id & NodeDataId::LAT)
    {
        vtk_file << "SCALARS LAT float 1\n";
        vtk_file << "LOOKUP_TABLE default" << std::endl;
        for(int i = 0; i < int(this->tissue_nodes.size()); i++)
        {
            vtk_file << tissue_nodes[i].apd_model.getActivationTime() << "\n";
        }
        vtk_file << std::endl;
    }

     if(data_id & NodeDataId::LIFE)
    {
        vtk_file << "SCALARS Life float 1\n";
        vtk_file << "LOOKUP_TABLE default" << std::endl;
        for(int i = 0; i < int(this->tissue_nodes.size()); i++)
        {
            vtk_file << tissue_nodes[i].apd_model.getLife(tissue_time) << "\n";
        }
        vtk_file << std::endl;
    }


    vtk_file.close();
}

#endif
