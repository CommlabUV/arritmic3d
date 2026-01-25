/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef NODE_H
#define NODE_H

#include <vector>
#include <limits>
#include <fstream>
#include <iostream>
#include <Eigen/Dense>

#include "definitions.h"
#include "node_parameters.h"
#include "cell_event_queue.h"
#include "geometry.h"

using std::vector;


template <typename ActionPotentialModel, typename ConductionVelocityModel>
class BasicTissue;
template <typename ActionPotentialModel, typename ConductionVelocityModel>
class CardiacTissue;

/**
 * @todo write docs
 */
template <typename ActionPotentialModel, typename ConductionVelocityModel>
class NodeT
{
public:
    using Vector3 = Eigen::Vector3f;
    using Vector2 = Eigen::Vector2f;

    using CellEvent = Event<NodeT>;

    friend class CardiacTissue<ActionPotentialModel,ConductionVelocityModel>;
    friend class BasicTissue<ActionPotentialModel,ConductionVelocityModel>;

    /// State of the cell.
    enum class CellActivationState : char { INACTIVE = 0, WAITING_FOR_ACTIVATION, ACTIVE };

    NodeT();
    void Reset(float current_time_);
    void ReApplyParam(float current_time_);
    float ComputeDirectionalConductionVelocity(const NodeT::Vector3 &direction_);
    CellEvent* ActivateAtTime( NodeT *origin_, float current_time_, float activation_time_);
    CellEvent* ActivateAtTimeExternal(float activation_time_, int beat_n_);

    unsigned int GetId() const { return id; }
    CellActivationState GetState(float current_time_) const;
    int GetBeat() const { return beat; }
    void SaveState(std::ofstream & f, const class ParametersPool & parameters_pool, const CellEventQueue<NodeT> & event_queue) const;

    // Data extraction ---
    using NodeData = std::tuple<float, int, int, int, float, int, float, float, float, float, float, float>;
    NodeData GetData(const CellEvent* ev, float current_time_) const
    {
        return NodeData(current_time_, int(ev->event_type), int(type), beat, local_activation_time, apd_model.IsActive(current_time_), apd_model.getAPD(), apd_model.getLastDI(),
                        conduction_vel, next_activation_time, next_deactivation_time, received_potential);
    }
    static const vector<std::string> GetDataNames()
    {
        static const vector<std::string> names = {"Time", "event_type", "type", "beat", "local_activation_time", "activated", "APD", "LastDI", "conduction_velocity",
                                                "next_activation_time", "next_deactivation_time",
                                                "received_potential"};
        return names;
    }
    //----------
    friend std::ostream & operator<<(std::ostream &os, const NodeT &node)
    {
        os << "Node id: " << node.id << " Type: " << (int)node.type << " Beat: " << node.beat;
        os << " conduction velocity: " << node.conduction_vel;
        os << " APD: " << node.apd_model.getAPD();
        os << " CV: " << node.cv_model.getConductionVelocity();
        os << " LAT: " << node.local_activation_time;
        os << " Next activation time: " << node.next_activation_time;
        os << " Next deactivation time: " << node.next_deactivation_time;
        os << " Received potential: " << node.received_potential;
        if(node.activation_parent != nullptr)
            os << " Activation parent: " << node.activation_parent->id;
        return os;
    }

private:
    constexpr static int SAVE_VERSION = 1;  ///< Version of the NodeT class for state saving/loading.
    NodeParameters*  parameters;         ///< @brief Parameters of the Node
    unsigned int    id;                 ///< @brief Unique Node id

    CellType        type = CELL_TYPE_VOID; ///< @brief Type of the Node
    bool            external_activation;
    bool            blocked;            ///< Node activation has been blocked
    int             beat;               ///< @brief Last beat  of activation

    float           conduction_vel;             ///< @brief Conduction velocity in the long. direction
    Vector3         orientation = Vector3::Zero();     ///< @brief Fiber orientation.
                                                    ///< A normalized vector indicating longitudinal direction.
                                                    ///< Default to (0,0,0) for isotropic diffusion.

    ActionPotentialModel apd_model;

    ConductionVelocityModel   cv_model;

    float           local_activation_time; ///< @brief Time of the last activation. A.k.a. LAT.

    float           kapd_v;

    // Activation
    float               received_potential;

    float               next_activation_time;  ///< @brief Time of the next activation
    float               next_deactivation_time; ///< @brief Time of the next deactivation

    CellEvent *         next_activation_event;  ///< @brief Event for the next activation of the node
    CellEvent *         next_deactivation_event;  ///< @brief Event for the next deactivation of the node

    NodeT *              activation_parent; ///< Node that activated this one


    //void Deactivate(float current_time_);
    bool Activate(float current_time_, const Geometry &geometry);
    bool ComputeActivation(float current_time_, const Geometry &geometry);

};

#include "node.cpp"

#endif // NODE_H
