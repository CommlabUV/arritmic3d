/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef BASIC_TISSUE_H
#define BASIC_TISSUE_H

#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <fstream>
#include <cassert>
#include <Eigen/Dense>

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
    constexpr static int SAVE_VERSION = 1;  ///< Version of the BasicTissue class for state saving/loading.
    using Node = NodeT<APM,CVM>;

    BasicTissue(int size_x_, int size_y_, int size_z_, float dx_, float dy_, float dz_) :
        tissue_geometry(size_x_, size_y_, size_z_, dx_, dy_, dz_),
        tissue_nodes(size_x_ * size_y_ * size_z_),
        sensor_dict(Node::GetDataNames())
    {
        tissue_time = 0.0;
        // Initialize the timer for system events
        timer.fill(0.0f);
    }

    void InitModels(const std::string &fileAP, const std::string &fileCV)
    {
        APM::InitModel(fileAP);
        CVM::InitModel(fileCV);
    }

    void Init(const vector<CellType> & cell_types_, vector<NodeParameters> & parameters_, const vector<Eigen::Vector3f> & fiber_orientation_ = {Eigen::Vector3f::Zero()});
    void InitPy(const vector<CellType> & cell_types_ , std::map<std::string, std::vector<float> > & parameters_, const std::vector<vector<float>> & fiber_orientation_);
    void Reset();
    void ChangeParameters(vector<NodeParameters> & parameters_);
    vector<int> GetStates() const;
    vector<float> GetAPD() const;
    vector<float> GetCV() const;
    vector<float> GetDI() const;
    vector<float> GetLastDI() const;
    vector<float> GetLAT() const;
    vector<float> GetLT() const;
    vector<int> GetBeat() const;
    vector<float> GetAPDVariation() const;
    /** Get the current time of the tissue */
    float GetTime() const { return tissue_time; }
    void SetBorder(vector<CellType> & cell_types_, CellType border_type);
    /** Get the id (index) of node with coordinates (x, y, z) */
    size_t GetIndex(int x, int y, int z) const  { return tissue_geometry.GetIndex(x, y, z);}
    /** Get the number of nodes in the tissue */
    size_t size() const { return tissue_nodes.size(); }
    /** Get the number of live nodes (not CORE) in the tissue */
    int GetNumLiveNodes() const { return n_live_nodes; }

    /** Get the number of nodes in the X direction */
    size_t GetSizeX() const { return tissue_geometry.size_x; }
    /** Get the number of nodes in the Y direction */
    size_t GetSizeY() const { return tissue_geometry.size_y; }
    /** Get the number of nodes in the Z direction */
    size_t GetSizeZ() const { return tissue_geometry.size_z; }
    /** Set a timer for the simulation
     * @param t Period (time between events) in milliseconds.
    */
    void SetTimer(SystemEventType type, float t);
    void SetSystemEvent(SystemEventType type, float t);

    void SaveVTK(const std::string & filename) const;
    void SaveState(const std::string & filename) const;
    void LoadState(const std::string & filename);

    void ShowSensorData(std::ostream& os = std::cout) const
    {
        sensor_dict.Show(os);
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
    int           n_live_nodes = 0;   ///< Number of nodes that are not CORE

    // Parameters
    ParametersPool      parameters_pool;

    // Simulation
    float           tissue_time;
    int             debug_level = 0;
    std::array<float, int(SystemEventType::SIZE)> timer; ///< Timer for each of the different system events. 0 unused.

    SensorDict<typename Node::NodeData> sensor_dict;  ///< Dictionary to store sensor data
};

/**
 * Initialize the tissue.
 * @param cell_types_ Vector of cell types.
 * @param parameters_ Vector of parameters for each node. Isotropic diffusion is set according to fiber orientation.
 * @param fiber_orientation_ Vector of fiber orientations.
 *
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::Init(const vector<CellType> & cell_types_, vector<NodeParameters> & parameters_, const vector<Eigen::Vector3f> & fiber_orientation_)
{
    // First, check if data vectors are consistent
    size_t n_nodes = tissue_nodes.size();
    LOG::Error(cell_types_.size() != n_nodes, "Number of cell types (", cell_types_.size(), ") does not match number of nodes (", n_nodes, ").");
    assert(cell_types_.size() == n_nodes);

    // Reset basic variables
    n_live_nodes = 0;
    tissue_time = 0.0;

    sensor_dict.Init();

    // Reset the timer
    timer.fill(0.0f);

    // Set borders to VOID type
    auto cell_types2 = cell_types_;
    SetBorder(cell_types2, CELL_TYPE_VOID);

    // Fiber orientation
    if( fiber_orientation_.size() == n_nodes )
        this->tissue_fiber_orientation = FiberOrientation::HETEROGENEOUS;
    else
        if( fiber_orientation_.size() == 1 && fiber_orientation_.at(0).norm() > ALMOST_ZERO )
            this->tissue_fiber_orientation = FiberOrientation::HOMOGENEOUS;
        else
            this->tissue_fiber_orientation = FiberOrientation::ISOTROPIC;

    // Initialize nodes
    assert(parameters_.size() == n_nodes || parameters_.size() == 1);
    for(size_t i = 0; i < n_nodes; i++)
    {
        tissue_nodes[i] = Node();  // Totally reset the node

        tissue_nodes[i].id = i;
        tissue_nodes[i].type = cell_types2[i];
        // Set the fiber orientation, default is isotropic
        if(this->tissue_fiber_orientation == FiberOrientation::HOMOGENEOUS)
        {
            tissue_nodes[i].orientation = fiber_orientation_.at(0);
        }
        else if(this->tissue_fiber_orientation == FiberOrientation::HETEROGENEOUS)
        {
            tissue_nodes[i].orientation = fiber_orientation_.at(i);
        }

        if(tissue_nodes[i].type != CELL_TYPE_VOID)
            n_live_nodes++;
    }

    LOG::Warning(n_live_nodes == 0, "Tissue has no live cells (all cells are VOID).");

    // Node parameters
    ChangeParameters(parameters_);

    // Initialize the event queue
    event_queue.Init(tissue_nodes, n_live_nodes);

    // Link each node with its events.
    for(size_t i = 0; i < n_nodes; i++)
    {
        tissue_nodes[i].next_activation_event = event_queue.GetEvent(i,CellEventType::ACTIVATION);
        tissue_nodes[i].next_deactivation_event = event_queue.GetEvent(i,CellEventType::DEACTIVATION);

        // Reset should only be called after the Node parameters are set.
        tissue_nodes[i].Reset(tissue_time);
    }
}

/**
 * Change the parameters of the tissue nodes. Simulation can continue normally.
 * @param parameters_ Vector of new parameters. Isotropic diffusion is set according to fiber orientation.
 *
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::ChangeParameters(vector<NodeParameters> & parameters_)
{
    size_t n_nodes = tissue_nodes.size();
    assert(parameters_.size() == n_nodes || parameters_.size() == 1);

    // Set isotropic diffusion, default is true
    bool isotropic = true;
    if(this->tissue_fiber_orientation == FiberOrientation::HOMOGENEOUS || this->tissue_fiber_orientation == FiberOrientation::HETEROGENEOUS)
        isotropic = false;
    for(size_t i = 0; i < parameters_.size(); i++)
    {
        parameters_[i].isotropic_diffusion = isotropic;
    }

    parameters_pool.Init(parameters_);
    LOG::Info(debug_level > 0, parameters_pool.Info());

    for(size_t i = 0; i < n_nodes; i++)
    {
        if(parameters_.size() == 1)
            tissue_nodes[i].parameters = parameters_pool.Find(parameters_[0]);
        else
            tissue_nodes[i].parameters = parameters_pool.Find(parameters_[i]);

        if(tissue_nodes[i].type != CELL_TYPE_VOID)
            tissue_nodes[i].ReApplyParam(tissue_time);
    }


    // Clear the finder map in the parameters pool to save memory
    parameters_pool.FinderClear();
}

/**
 * Reset the tissue to the initial state.
 * This function resets the time, all nodes and the event queue.
 * It does not change the parameters of the nodes.
 */
/*
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
    timer.fill(0.0f);
}
*/
/**
 * Initialize the tissue from Python. Calls Init with a vector of parameters.
 * @param cell_types_ Vector of cell types.
 * @param parameters_ Dictionary with the parameters.
 * @param fiber_orientation_ Vector of fiber orientations.
 *
 * @todo Why parameters_ can't be const?
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::InitPy(const vector<CellType> & cell_types_, std::map<std::string, std::vector<float> > & parameters_, const std::vector<vector<float> > & fiber_orientation_)
{
    vector<NodeParameters> parameters(tissue_nodes.size() );

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
 * Get the last DI (diastolic interval) of the tissue nodes.
 * @return Vector of last DI of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetLastDI() const
{
    vector<float> last_di(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        last_di[i] = tissue_nodes[i].apd_model.getLastDI();
    return last_di;
}

/**
 * Get the LAT (local activationtime) of the tissue nodes.
 * @return Vector of LAT of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetLAT() const
{
    vector<float> lat(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        lat[i] = tissue_nodes[i].apd_model.getActivationTime();
    return lat;
}

/**
 * Get the LT (life time) of the tissue nodes.
 * @return Vector of life times of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetLT() const
{
    vector<float> lt(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        lt[i] = GetTime() - tissue_nodes[i].apd_model.getActivationTime();
    return lt;
}

/**
 * Get the beat number that induced the last activation of the tissue nodes.
 * @return Vector of beat number of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<int> BasicTissue<APM,CVM>::GetBeat() const
{
    vector<int> beat(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        beat[i] = tissue_nodes[i].GetBeat();
    return beat;
}

/**
 * Get the variation in APD of the tissue nodes.
 * @return Vector of variation of APD of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetAPDVariation() const
{
    vector<float> delta_apd(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        delta_apd[i] = tissue_nodes[i].apd_model.getDeltaAPD();
    return delta_apd;
}

/**
 * Set a timer for the simulation.
 * @param t Period (time between events) in milliseconds.
*/
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::SetTimer(SystemEventType type, float t)
{
    // Setting timer before the simulation starts
    if(this->tissue_time == 0)
    {
        this->timer.at(int(type)) = t;
        // Insert the first system event
        event_queue.InsertSystemEvent(t, type);
        return;
    }

    // Simulation is running.
    // If the timer is already set, update it
    if(this->timer.at(int(type)) > 0)
    {
        this->timer.at(int(type)) = t;
    }
    else
    {
        LOG::Error(true, "Timer for type ", (int)type, " does not exist. Use SetTimer before the simulation starts.");
    }

    return;
}

/**
 * Set a system event for the simulation.
 * @param type Type of the system event.
 * @param t Time of the system event in milliseconds.
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::SetSystemEvent(SystemEventType type, float t)
{
    if(t < this->tissue_time)
    {
        LOG::Error(true, "System event time (", t, ") is before the current tissue time (", this->tissue_time, ").");
        return;
    }

    int priority = 1; // Default priority for system events
    if(type == SystemEventType::EXT_ACTIVATION)
        priority = 0; // Higher priority for external activations
    this->event_queue.InsertSystemEvent(t, type, priority);
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
void BasicTissue<APM,CVM>::SaveState(const std::string & filename) const
{
    std::ofstream state_file;
    state_file.open(filename, std::ios::binary);
    if(!state_file)
    {
        LOG::Error(true, "Could not open file " + filename + " for writing.");
        return;
    }

    // Save version
    state_file.write( (const char*) (&SAVE_VERSION), sizeof(SAVE_VERSION) );
    // Save tissue time
    state_file.write( (const char*) (&tissue_time), sizeof(tissue_time) );
    // Save timer
    state_file.write( (const char*) (timer.data()), sizeof(float) * int(SystemEventType::SIZE) );
    // Save number of live nodes
    state_file.write( (const char*) (&n_live_nodes), sizeof(n_live_nodes) );

    // Save geometry
    tissue_geometry.SaveState(state_file);
    // Save parameters pool
    parameters_pool.SaveState(state_file);
    // Save event queue
    event_queue.SaveState(state_file, tissue_nodes);

    // Save each node
    for(const auto & node : tissue_nodes)
    {
        node.SaveState(state_file, parameters_pool, event_queue);
    }

    state_file.close();
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
