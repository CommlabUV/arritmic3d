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
    friend class NodeT<APM,CVM>;

    BasicTissue(int size_x_, int size_y_, int size_z_, float dx_, float dy_, float dz_) :
        tissue_geometry(size_x_, size_y_, size_z_, dx_, dy_, dz_),
        grid_size(size_x_ * size_y_ * size_z_),
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
    //void Reset();
    void ChangeParameters(vector<NodeParameters> & parameters_);
    vector<int> GetStates() const;
    vector<float> GetAPD() const;
    vector<float> GetAP() const;
    vector<float> GetCV() const;
    vector<float> GetDI() const;
    vector<float> GetLastDI() const;
    vector<float> GetLAT() const;
    vector<float> GetLife() const;
    vector<int> GetBeat() const;
    vector<float> GetAPDVariation() const;

    vector<int> GetStatesIndexed() const;
    vector<float> GetAPDIndexed() const;
    vector<float> GetAPIndexed() const;
    vector<float> GetCVIndexed() const;
    vector<float> GetDIIndexed() const;
    vector<float> GetLastDIIndexed() const;
    vector<float> GetLATIndexed() const;
    vector<float> GetLifeIndexed() const;
    vector<int> GetBeatIndexed() const;
    vector<float> GetAPDVariationIndexed() const;
    /** Get the current time of the tissue */
    float GetTime() const { return tissue_time; }
    void SetBorder(vector<CellType> & cell_types_, CellType border_type);
    /** Get the id (index) of node with coordinates (x, y, z) */
    size_t GetIndex(int x, int y, int z) const
    {
        return tissue_geometry.GetGridIndex_from_Coords(x, y, z);
    }
    /** Get the number of nodes in the tissue */
    size_t size() const { return grid_size; }
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
    void SetTimer(SystemEventType type, float period, float initial_time = 0.0f);
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

    /**
     * @brief Set the initial APD for all nodes.
     * It should be called before Init.
     * @param apd Initial APD to set.
     */
    void SetInitialAPD(float apd)
    {
        initial_apd = apd;
    }

protected:

    // Geometry
    FiberOrientation    tissue_fiber_orientation;
    Geometry    tissue_geometry;
    vector<Node>        tissue_nodes;
    CellEventQueue<Node>  event_queue;
    size_t      grid_size;      ///< Total number of nodes in the tissue (including CORE nodes).
    int         n_live_nodes = 0;   ///< Number of nodes that are not CORE

    // Parameters
    ParametersPool      parameters_pool;

    // Simulation
    float       tissue_time;
    int         debug_level = 0;
    float       initial_apd = 100.0f;
    std::array<float, int(SystemEventType::SIZE)> timer; ///< Timer for each of the different system events. 0 unused.

    SensorDict<typename Node::NodeData> sensor_dict;  ///< Dictionary to store sensor data

    /**
     * @brief Get the position of a node in the tissue_nodes vector from its id.
     * @param id Node id. Corresponds with the grid index of the node.
     */
    Node* GetNodePtr(size_t id)
    {
        auto mem_index = tissue_geometry.GetMemIndex_from_GridIndex(id);
        assert(mem_index < static_cast<long int>(tissue_nodes.size()));
        if(mem_index == NO_INDEX)
            throw std::out_of_range("GetNodePtr: Node id " + std::to_string(id) + " is VOID.");
        std::cout << "GetNodePtr: id=" << id << " mem_index=" << mem_index << std::endl;
        return &tissue_nodes[mem_index];
    }

    /**
     * @brief Get the node at a certain index distance from another node in the grid.
     * @param node Pointer to the node from which to calculate the displacement.
     * @param grid_distance Index distance in the grid to the node to get. It can be positive or negative.
     */
    Node* NodeDisplace(Node* node, int grid_distance)
    {
        size_t grid_pos = node->ext_grid_pos + grid_distance;
        auto mem_index = tissue_geometry.GetMemIndex_from_ExtGridIndex(grid_pos);
        if(mem_index == NO_INDEX)
            return nullptr;
        return &tissue_nodes[mem_index];
    }

    /**
     * @brief Update the position of the nodes in the extended grid (ext_grid_pos attribute). It should be called after initializing the nodes.
     */
    void UpdateExtGridPos()
    {
        for(size_t i = 0; i < tissue_geometry.index.size(); i++)
        {
            auto mem_index = tissue_geometry.GetMemIndex_from_ExtGridIndex(i);
            if(mem_index != NO_INDEX)
                tissue_nodes.at(mem_index).ext_grid_pos = i;
        }
    }

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
    size_t n_nodes = grid_size;
    LOG::Error(cell_types_.size() != n_nodes, "Number of cell types (", cell_types_.size(), ") does not match number of nodes (", n_nodes, ").");
    assert(cell_types_.size() == n_nodes);

    // Reset basic variables
    n_live_nodes = 0;
    tissue_time = 0.0;

    sensor_dict.Init();

    // Reset the timer
    timer.fill(0.0f);

    // Fiber orientation
    if( fiber_orientation_.size() == n_nodes )
        this->tissue_fiber_orientation = FiberOrientation::HETEROGENEOUS;
    else
        if( fiber_orientation_.size() == 1 && fiber_orientation_.at(0).norm() > ALMOST_ZERO )
            this->tissue_fiber_orientation = FiberOrientation::HOMOGENEOUS;
        else
            this->tissue_fiber_orientation = FiberOrientation::ISOTROPIC;

    // --- Initialize nodes ---
    assert(parameters_.size() == n_nodes || parameters_.size() == 1);
    // First calculate number of live nodes
    n_live_nodes = 0;
    for(auto type : cell_types_)
        if(type != CELL_TYPE_VOID)
            n_live_nodes++;
    tissue_nodes.resize(n_live_nodes);

    LOG::Warning(n_live_nodes == 0, "Tissue has no live cells (all cells are VOID).");

    size_t grip_pos = 0;
    for(size_t i = 0; i < n_nodes; i++)
    {
        int extended_grid_pos = tissue_geometry.GetExtGridIndex_from_GridIndex(i);
        assert(extended_grid_pos != NO_INDEX);
        CellType type = cell_types_[i];
        if(type != CELL_TYPE_VOID)
        {
            tissue_nodes[grip_pos] = Node();  // Totally reset the node

            tissue_nodes[grip_pos].id = i;     // The id corresponds with the grid position.
            tissue_nodes[grip_pos].type = type;
            // Set the fiber orientation, default is isotropic
            if(this->tissue_fiber_orientation == FiberOrientation::HOMOGENEOUS)
            {
                tissue_nodes[grip_pos].orientation = fiber_orientation_.at(0);
            }
            else if(this->tissue_fiber_orientation == FiberOrientation::HETEROGENEOUS)
            {
                tissue_nodes[grip_pos].orientation = fiber_orientation_.at(i);
            }
            // Update the index of the node in the extended grid
            tissue_geometry.index[extended_grid_pos] = grip_pos;

            grip_pos++;
        }
        else
        {
            // VOID node. Index vector should be adjusted.
            tissue_geometry.index[extended_grid_pos] = NO_INDEX;
        }
    }
    UpdateExtGridPos();
    //tissue_geometry.WriteIndex();

    // Node parameters
    ChangeParameters(parameters_);

    // Initialize the event queue
    event_queue.Init(tissue_nodes, n_live_nodes);

    // Link each node with its events.
    for(size_t i = 0; i < tissue_nodes.size(); i++)
    {
        tissue_nodes[i].next_activation_event = event_queue.GetEvent(i,CellEventType::ACTIVATION);
        tissue_nodes[i].next_deactivation_event = event_queue.GetEvent(i,CellEventType::DEACTIVATION);

        // Init should only be called after the Node parameters are set.
        tissue_nodes[i].Init(tissue_time, initial_apd);
    }

    LOG::Info(debug_level > 1, "End of Init");
}

/**
 * Change the parameters of the tissue nodes. Simulation can continue normally.
 * @param parameters_ Vector of new parameters. Isotropic diffusion is set according to fiber orientation.
 *
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::ChangeParameters(vector<NodeParameters> & parameters_)
{
    assert(parameters_.size() == this->size() || parameters_.size() == 1);

    // Set isotropic diffusion, default is true
    // @todo if heterogeneous and locally isotropic, it is not set in parameters.
    bool isotropic = true;
    if(this->tissue_fiber_orientation == FiberOrientation::HOMOGENEOUS || this->tissue_fiber_orientation == FiberOrientation::HETEROGENEOUS)
        isotropic = false;
    for(size_t i = 0; i < parameters_.size(); i++)
    {
        parameters_.at(i).isotropic_diffusion = isotropic;
    }

    parameters_pool.Init(parameters_);
    LOG::Info(debug_level > 0, parameters_pool.Info());

    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(parameters_.size() == 1)
            tissue_nodes.at(index).parameters = parameters_pool.Find(parameters_[0]);
        else
            if(index != NO_INDEX)
                tissue_nodes.at(index).parameters = parameters_pool.Find(parameters_[i]);

        if(index != NO_INDEX && tissue_nodes.at(index).type != CELL_TYPE_VOID)
            tissue_nodes.at(index).ReApplyParam(tissue_time);
    }


    // Clear the finder map in the parameters pool to save memory
    parameters_pool.FinderClear();

    LOG::Info(debug_level > 1, "End of ChangeParameters");
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
    vector<NodeParameters> parameters(this->size() );

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
    vector<Eigen::Vector3f> fiber_orientation(this->size(), Eigen::Vector3f::Zero());
    if(fiber_orientation_.size() == 1)
    {
        // If only one fiber orientation is given, use it for all nodes
        for(size_t i = 0; i < this->size(); i++)
            fiber_orientation[i] = Eigen::Vector3f(fiber_orientation_[0].data());
    }
    else if(fiber_orientation_.size() == this->size())
    {
        // If fiber orientation is given for each node, use it
        for(size_t i = 0; i < this->size(); i++)
            fiber_orientation[i] = Eigen::Vector3f(fiber_orientation_[i].data());
    }
    else
    {
        LOG::Error(true, " Number of fiber orientations (", fiber_orientation_.size(), ") does not match number of nodes (", this->size(), " or 1).");
        return;
    }
    LOG::Info(debug_level > 1, "End of InitPy");

    Init(cell_types_, parameters, fiber_orientation);
}

/**
 * Get the states of the non-void tissue nodes.
 * @return Vector of states of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<int> BasicTissue<APM,CVM>::GetStatesIndexed() const
{
    vector<int> state(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        state[i] = int(tissue_nodes[i].GetState(tissue_time));
    return state;
}

/**
 * Get the states of the tissue nodes.
 * @return Vector of states of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<int> BasicTissue<APM,CVM>::GetStates() const
{
    vector<int> state(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            state[i] = int(tissue_nodes[index].GetState(tissue_time));
    }
    return state;
}

/**
 * Get the APD of the tissue nodes.
 * @return Vector of APD of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetAPDIndexed() const
{
    vector<float> apd(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        apd[i] = tissue_nodes[i].apd_model.getAPD();
    return apd;
}

/**
 * Get the APD of the tissue nodes.
 * @return Vector of APD of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetAPD() const
{
    vector<float> apd(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            apd[i] = tissue_nodes[index].apd_model.getAPD();
    }
    return apd;
}

/**
 * Get the AP of the tissue nodes.
 * @return Vector of AP of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetAPIndexed() const
{
    vector<float> ap(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        ap[i] = tissue_nodes[i].apd_model.getActionPotential(GetTime());
    return ap;
}

/**
 * Get the AP of the tissue nodes.
 * @return Vector of AP of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetAP() const
{
    vector<float> ap(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            ap[i] = tissue_nodes[index].apd_model.getActionPotential(GetTime());
    }
    return ap;
}

/**
 * Get the conduction velocity of the tissue nodes.
 * @return Vector of conduction velocity of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetCVIndexed() const
{
    vector<float> cv(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        cv[i] = tissue_nodes[i].conduction_vel;
    return cv;
}

/**
 * Get the conduction velocity of the tissue nodes.
 * @return Vector of conduction velocity of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetCV() const
{
    vector<float> cv(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            cv[i] = tissue_nodes[index].conduction_vel;
    }
    return cv;
}

/**
 * Get the DI (diastolic interval) of the tissue nodes.
 * @return Vector of DI of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetDIIndexed() const
{
    vector<float> di(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        di[i] = tissue_nodes[i].apd_model.getDI(GetTime());
    return di;
}

/**
 * Get the DI (diastolic interval) of the tissue nodes.
 * @return Vector of DI of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetDI() const
{
    vector<float> di(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            di[i] = tissue_nodes[index].apd_model.getDI(GetTime());
    }
    return di;
}

/**
 * Get the last DI (diastolic interval) of the tissue nodes.
 * @return Vector of last DI of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetLastDIIndexed() const
{
    vector<float> last_di(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        last_di[i] = tissue_nodes[i].apd_model.getLastDI();
    return last_di;
}

/**
 * Get the last DI (diastolic interval) of the tissue nodes.
 * @return Vector of last DI of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetLastDI() const
{
    vector<float> last_di(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            last_di[i] = tissue_nodes[index].apd_model.getLastDI();
    }
    return last_di;
}

/**
 * Get the LAT (local activationtime) of the tissue nodes.
 * @return Vector of LAT of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetLATIndexed() const
{
    vector<float> lat(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        lat[i] = tissue_nodes[i].apd_model.getActivationTime();
    return lat;
}

/**
 * Get the LAT (local activationtime) of the tissue nodes.
 * @return Vector of LAT of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetLAT() const
{
    vector<float> lat(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            lat[i] = tissue_nodes[index].apd_model.getActivationTime();
    }
    return lat;
}

/**
 * Get the Life (life time) of the tissue nodes.
 * Life is a value between 0 and 1 that indicates how long the cell
 * has been active, normalized by its APD.
 * It is 0 if the cell is inactive and 1 if the cell has been active for a time equal to its APD.
 * @return Vector of life values of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetLifeIndexed() const
{
    vector<float> lt(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        lt[i] = tissue_nodes[i].apd_model.getLife(GetTime());
    return lt;
}

/**
 * Get the Life (life time) of the tissue nodes.
 * Life is a value between 0 and 1 that indicates how long the cell
 * has been active, normalized by its APD.
 * It is 0 if the cell is inactive and 1 if the cell has been active for a time equal to its APD.
 * @return Vector of life values of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetLife() const
{
    vector<float> lt(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            lt[i] = tissue_nodes[index].apd_model.getLife(GetTime());
    }
    return lt;
}

/**
 * Get the beat number that induced the last activation of the tissue nodes.
 * @return Vector of beat number of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<int> BasicTissue<APM,CVM>::GetBeatIndexed() const
{
    vector<int> beat(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        beat[i] = tissue_nodes[i].GetBeat();
    return beat;
}

/**
 * Get the beat number that induced the last activation of the tissue nodes.
 * @return Vector of beat number of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<int> BasicTissue<APM,CVM>::GetBeat() const
{
    vector<int> beat(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            beat[i] = tissue_nodes[index].GetBeat();
    }
    return beat;
}

/**
 * Get the variation in APD of the tissue nodes.
 * @return Vector of variation of APD of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetAPDVariationIndexed() const
{
    vector<float> delta_apd(tissue_nodes.size());
    for(size_t i = 0; i < tissue_nodes.size(); i++)
        delta_apd[i] = tissue_nodes[i].apd_model.getDeltaAPD();
    return delta_apd;
}

/**
 * Get the variation in APD of the tissue nodes.
 * @return Vector of variation of APD of the tissue nodes.
 */
template <typename APM,typename CVM>
vector<float> BasicTissue<APM,CVM>::GetAPDVariation() const
{
    vector<float> delta_apd(this->size(), 0);
    for(size_t i = 0; i < this->size(); i++)
    {
        auto index = tissue_geometry.GetMemIndex_from_GridIndex(i);
        if(index != NO_INDEX)
            delta_apd[i] = tissue_nodes[index].apd_model.getDeltaAPD();
    }
    return delta_apd;
}

/**
 * Set a timer for the simulation. There can be one timer for each type of system event.
 * @param period Period (time between events) in milliseconds.
 * @param initial_time Initial time for the timer in milliseconds.
 */
template <typename APM,typename CVM>
void BasicTissue<APM,CVM>::SetTimer(SystemEventType type, float period, float initial_time)
{
    // Setting timer before the simulation starts
    if(this->tissue_time == 0)
    {
        this->timer.at(int(type)) = period;
        // Insert the first system event
        event_queue.InsertSystemEvent(initial_time, type);
        return;
    }

    // Simulation is running.
    // If the timer is already set, update it
    if(this->timer.at(int(type)) > 0)
    {
        this->timer.at(int(type)) = period;
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
                cell_types_[tissue_geometry.GetGridIndex_from_Coords(x, y, k)] = border_type;
                cell_types_[tissue_geometry.GetGridIndex_from_Coords(x, y, tissue_geometry.size_z-1-k)] = border_type;
            }

    for(int x = 0; x < tissue_geometry.size_x; x++)
        for(int z = 0; z < tissue_geometry.size_z; z++)
            for(int k = 0; k < dist; k++)
            {
                cell_types_[tissue_geometry.GetGridIndex_from_Coords(x, k, z)] = border_type;
                cell_types_[tissue_geometry.GetGridIndex_from_Coords(x, tissue_geometry.size_y-1-k, z)] = border_type;
            }

    for(int y = 0; y < tissue_geometry.size_y; y++)
        for(int z = 0; z < tissue_geometry.size_z; z++)
            for(int k = 0; k < dist; k++)
            {
                cell_types_[tissue_geometry.GetGridIndex_from_Coords(k, y, z)] = border_type;
                cell_types_[tissue_geometry.GetGridIndex_from_Coords(tissue_geometry.size_x-1-k, y, z)] = border_type;
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
void BasicTissue<APM,CVM>::LoadState(const std::string & filename)
{
    std::ifstream state_file;
    state_file.open(filename, std::ios::binary);
    if(!state_file)
    {
        LOG::Error(true, "Could not open file " + filename + " for reading.");
        return;
    }

    // Load version
    int version;
    state_file.read( (char*) (&version), sizeof(version) );
    if(version != SAVE_VERSION)
    {
        LOG::Error(true, "Save version (", version, ") does not match current version (", SAVE_VERSION, ").");
        return;
    }

    // Load tissue time
    state_file.read( (char*) (&tissue_time), sizeof(tissue_time) );
    // Load timer
    state_file.read( (char*) (timer.data()), sizeof(float) * int(SystemEventType::SIZE) );
    // Load number of live nodes
    state_file.read( (char*) (&n_live_nodes), sizeof(n_live_nodes) );

    // Load geometry
    tissue_geometry.LoadState(state_file);
    // Load parameters pool
    parameters_pool.LoadState(state_file);
    LOG::Info(debug_level > 0, parameters_pool.Info());
    // Load event queue
    event_queue.LoadState(state_file, tissue_nodes);

    // Load each node
    for(auto & node : tissue_nodes)
    {
        node.LoadState(state_file, parameters_pool, event_queue, *this);
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

#endif
