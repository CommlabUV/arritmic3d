/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef BASIC_TISSUE_H
#define BASIC_TISSUE_H

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <cassert>
#include <eigen3/Eigen/Dense>

#include "geometry.h"
#include "node.h"
#include "cell_event_queue.h"
#include "error.h"
#include "sensor_dict.h"

using std::vector;

/**
 * @brief Class to model cardiac tissue. It does not include propagation functions.
 *
 * It contains a vector of nodes, that represent cardiac cells, and
 * the functions to perform the simulation using the fast reaction
 * diffusion model.
 *
 * If no fiber orientation is given, isotropic tissue is assumed.
 */
template <typename APM, typename CVM>
class BasicTissue
{
public:

    enum class FiberOrientation {ISOTROPIC, HOMOGENEOUS, HETEROGENEOUS};
    using Node = NodeT<APM,CVM>;

    BasicTissue(int size_x_, int size_y_, int size_z_, float dx_, float dy_, float dz_) :
        tissue_geometry(size_x_, size_y_, size_z_, dx_, dy_, dz_),
        tissue_nodes(size_x_ * size_y_ * size_z_),
        event_queue(tissue_nodes),
        sensor_dict(Node::GetDataNames())
    {
        tissue_time = 0.0;
    }

    void Init(const vector<CellType> & cell_types_, const vector<NodeParameters> & parameters_, const vector<Eigen::Vector3f> & fiber_orientation_ = {Eigen::Vector3f::Zero()});
    void InitPy(const vector<CellType> & cell_types_, const vector<TissueRegion> & tissue_region_ , std::map<std::string, std::vector<float> > & parameters_, const std::vector<vector<float>> & fiber_orientation_);
    void Reset();
    vector<int> GetStates() const;
    vector<float> GetAPD() const;
    vector<float> GetCV() const;
    vector<float> GetDI() const;
    void SaveVTK(const std::string & filename) const;
    /** Get the current time of the tissue */
    float GetTime() const { return tissue_time; }
    void SetBorder(vector<CellType> & cell_types_, CellType border_type);
    /** Get the id (index) of node with coordinates (x, y, z) */
    size_t GetIndex(int x, int y, int z) const  { return tissue_geometry.GetIndex(x, y, z);}
    /** Get the number of nodes in the tissue */
    size_t size() const { return tissue_nodes.size(); }
    /** Get the number of nodes in the X direction */
    size_t GetSizeX() const { return tissue_geometry.size_x; }
    /** Get the number of nodes in the Y direction */
    size_t GetSizeY() const { return tissue_geometry.size_y; }
    /** Get the number of nodes in the Z direction */
    size_t GetSizeZ() const { return tissue_geometry.size_z; }
    /** Set a timer for the simulation
     * @param t Period (time between events) in milliseconds.
    */
    void SetTimer(float t);
    void ShowSensorData() const
    {
        sensor_dict.Show();
    }

    /**
     * @brief Get the information of all sensors.
     * @return A map where the key is the node ID and the value is a vector of sensor data.
     */
    std::map<int, std::vector<typename Node::NodeData>> GetSensorInfo() const
    {
        return sensor_dict.GetSensorInfo();
    }

    /**
     * @brief Get the names of the data stored in the sensors.
     * @return A vector of strings containing the names of the data.
     */
    std::vector<std::string> GetSensorDataNames() const
    {
        return sensor_dict.GetDataNames();
    }

    /**
     * @brief Return a dictionary with the default parameters for the nodes.
     * int values are converted to float.
     * @return Dictionary with the parameters.
    */
    std::map<std::string, float> GetDefaultParameters() const
    {
        NodeParameters n;
        return n.GetParameters();
    }

protected:

    // Geometry
    FiberOrientation    tissue_fiber_orientation;
    Geometry      tissue_geometry;
    vector<Node>        tissue_nodes;
    CellEventQueue<Node>  event_queue;

    // Parameters
    ParametersPool      parameters_pool;

    // Simulation
    float           tissue_time;
    float           timer;
    int             debug_level = 0;

    SensorDict<typename Node::NodeData> sensor_dict;  ///< Dictionary to store sensor data
};

template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::Init(const vector<CellType> & cell_types_, const vector<NodeParameters> & parameters_, const vector<Eigen::Vector3f> & fiber_orientation_)
{
    // First, check if data vectors are consistent
    size_t n_nodes = tissue_nodes.size();
    if( cell_types_.size() != n_nodes )
    {
        LOG::Error(true, "Number of cell types (", cell_types_.size(), ") does not match number of nodes (", n_nodes, ").");
        return;
    }

    // Set borders to CORE type
    auto cell_types2 = cell_types_;
    SetBorder(cell_types2, CellType::CORE);

    // Fiber orientation
    if( fiber_orientation_.size() == n_nodes )
        this->tissue_fiber_orientation = FiberOrientation::HETEROGENEOUS;
    else
        if( fiber_orientation_.size() == 1 and fiber_orientation_.at(0).norm() > ALMOST_ZERO )
            this->tissue_fiber_orientation = FiberOrientation::HOMOGENEOUS;
        else
            this->tissue_fiber_orientation = FiberOrientation::ISOTROPIC;

    // Initialize nodes

    // Node parameters
    assert(parameters_.size() == n_nodes || parameters_.size() == 1);
    parameters_pool.Init(parameters_);
    LOG::Info(debug_level > 0, parameters_pool.Info());

    tissue_time = 0.0;

    for(size_t i = 0; i < n_nodes; i++)
    {
        tissue_nodes[i].id = i;
        if(parameters_.size() == 1)
            tissue_nodes[i].parameters = parameters_pool.Find(parameters_[0]);
        else
            tissue_nodes[i].parameters = parameters_pool.Find(parameters_[i]);

        tissue_nodes[i].next_event = event_queue.GetEvent(i);
        tissue_nodes[i].type = cell_types2[i];
        // Set the fiber orientation, default is isotropic
        if(this->tissue_fiber_orientation == FiberOrientation::HOMOGENEOUS)
        {
            tissue_nodes[i].orientation = fiber_orientation_.at(0);
            tissue_nodes[i].parameters->isotropic_diffusion = false;
        }
        else if(this->tissue_fiber_orientation == FiberOrientation::HETEROGENEOUS)
        {
            tissue_nodes[i].orientation = fiber_orientation_.at(i);
            tissue_nodes[i].parameters->isotropic_diffusion = false;
        }

        // Reset should only be called after the Node parameters are set.
        tissue_nodes[i].Reset(tissue_time);
    }
}

/**
 * Reset the tissue to the initial state.
 * This function resets the time, all nodes and the event queue.
 * It does not change the parameters of the nodes.
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::Reset()
{
    // Reset the tissue time
    tissue_time = 0.0;

    // Reset all nodes
    for(auto & node : tissue_nodes)
    {
        node.Reset(tissue_time);
    }

    // Reset the timer
    this->timer = 0.0;
}

/**
 * Initialize the tissue from Python. Calls Init with a vector of parameters.
 * @param cell_types_ Vector of cell types.
 * @param tissue_region_ Vector of tissue regions.
 * @param parameters_ Dictionary with the parameters.
 * @param fiber_orientation_ Vector of fiber orientations.
 *
 * @todo Why parameters_ can't be const?
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::InitPy(const vector<CellType> & cell_types_, const vector<TissueRegion> & tissue_region_ , std::map<std::string, std::vector<float> > & parameters_, const std::vector<vector<float> > & fiber_orientation_)
{
    vector<NodeParameters> parameters(tissue_nodes.size() );

    for(size_t i = 0; i < parameters.size(); i++)
        parameters[i].tissue_region = tissue_region_[i];

    for(size_t param = 0; param < NodeParameters::names.size(); param++)
    {
        if(parameters_.count(NodeParameters::names[param]))
        {
            //std::assert(parameters_[NodeParameters::names[param]].size() == parameters.size());
            for(size_t i = 0; i < parameters.size(); i++)
                parameters[i].SetParameter(param, parameters_[NodeParameters::names[param]][i]);
        }
    }

    // Set the fiber orientation
    vector<Eigen::Vector3f> fiber_orientation(tissue_nodes.size(), Eigen::Vector3f::Zero());
    if(fiber_orientation_.size() == 1)
    {
        // If only one fiber orientation is given, use it for all nodes
        for(size_t i = 0; i < tissue_nodes.size(); i++)
            fiber_orientation[i] = Eigen::Vector3f(fiber_orientation_[0].data());
    }
    else if(fiber_orientation_.size() == tissue_nodes.size())
    {
        // If fiber orientation is given for each node, use it
        for(size_t i = 0; i < tissue_nodes.size(); i++)
            fiber_orientation[i] = Eigen::Vector3f(fiber_orientation_[i].data());
    }
    else
    {
        LOG::Error(true, " Number of fiber orientations (", fiber_orientation_.size(), ") does not match number of nodes (", tissue_nodes.size(), " or 1).");
        return;
    }


    Init(cell_types_, parameters, fiber_orientation);
}

/**
 * Get the states of the tissue nodes.
 * @return Vector of states of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<int> BasicTissue<APM,CVM>::GetStates() const
{
    vector<int> state(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        state[i] = int(tissue_nodes[i].GetState(tissue_time));
    return state;
}

/**
 * Get the APD of the tissue nodes.
 * @return Vector of APD of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetAPD() const
{
    vector<float> apd(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        apd[i] = tissue_nodes[i].apd_model.getAPD();
    return apd;
}

/**
 * Get the conduction velocity of the tissue nodes.
 * @return Vector of conduction velocity of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetCV() const
{
    vector<float> cv(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        cv[i] = tissue_nodes[i].conduction_vel;
    return cv;
}

/**
 * Get the DI (diastolic interval) of the tissue nodes.
 * @return Vector of DI of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetDI() const
{
    vector<float> di(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        di[i] = tissue_nodes[i].apd_model.getDI(GetTime());
    return di;
}

/**
 * Set a timer for the simulation.
 * @param t Period (time between events) in milliseconds.
*/
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::SetTimer(float t)
{
    assert(tissue_nodes.size() > 0 && tissue_nodes[0].next_event != nullptr);
    this->timer = t;

    tissue_nodes[0].next_event->ChangeEvent(t);
    event_queue.InsertEvent(tissue_nodes[0].next_event);
}

/**
 * Set the border of the tissue to a given type. The border thickness is given by Geometry::distance.
 * @param cell_types_ Vector of cell types.
 * @param border_type Type of the border.
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::SetBorder(vector<CellType> & cell_types_, CellType border_type)
{
    int dist = Geometry::distance;

    for(int x = 0; x < tissue_geometry.size_x; x++)
        for(int y = 0; y < tissue_geometry.size_y; y++)
            for(int k = 0; k < dist; k++)
            {
                cell_types_[tissue_geometry.GetIndex(x, y, k)] = border_type;
                cell_types_[tissue_geometry.GetIndex(x, y, tissue_geometry.size_z-1-k)] = border_type;
            }

    for(int x = 0; x < tissue_geometry.size_x; x++)
        for(int z = 0; z < tissue_geometry.size_z; z++)
            for(int k = 0; k < dist; k++)
            {
                cell_types_[tissue_geometry.GetIndex(x, k, z)] = border_type;
                cell_types_[tissue_geometry.GetIndex(x, tissue_geometry.size_y-1-k, z)] = border_type;
            }

    for(int y = 0; y < tissue_geometry.size_y; y++)
        for(int z = 0; z < tissue_geometry.size_z; z++)
            for(int k = 0; k < dist; k++)
            {
                cell_types_[tissue_geometry.GetIndex(k, y, z)] = border_type;
                cell_types_[tissue_geometry.GetIndex(tissue_geometry.size_x-1-k, y, z)] = border_type;
            }
}

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
    for(int i = 0; i < int(tissue_nodes.size()); i++)
    {
        vtk_file << int(tissue_nodes[i].type) << " ";
        if((i+1) % 10 == 0)
            vtk_file << "\n";
    }
    vtk_file << std::endl;

    vtk_file << "SCALARS State int 1\n";
    vtk_file << "LOOKUP_TABLE default" << std::endl;
    for(int i = 0; i < int(tissue_nodes.size()); i++)
    {
        vtk_file << int(tissue_nodes[i].GetState(tissue_time) ) << " ";
        if((i+1) % 10 == 0)
            vtk_file << "\n";
    }
    vtk_file << std::endl;

    vtk_file.close();
}

#endif
