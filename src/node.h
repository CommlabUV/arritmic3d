/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef NODE_H
#define NODE_H

#include <vector>
#include <eigen3/Eigen/Dense>

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

    /**
     * State of the cell.
     * From node.pde: -1, 0, 2
     */
    enum class CellActivationState : char { INACTIVE, WAITING_FOR_ACTIVATION, ACTIVE };

    NodeT();
    void Reset(float current_time_);
    float ComputeConductionVelocity(const NodeT::Vector3 &direction_);
    CellEvent* ActivateAtTime( NodeT *origin_, float activation_time_, float path_length_);
    CellEvent* ActivateAtTimeExternal(float activation_time_, int beat_n_);

    unsigned int GetId() const { return id; }
    CellActivationState GetState(float current_time_) const;

    // Data extraction ---
    using NodeData = std::tuple<float, int, int, float, float, float, float, float, float, float>;
    NodeData GetData(float current_time_) const
    {
        return NodeData(current_time_, int(type), beat, start_time, apd_model.getAPD(), apd_model.getDI(current_time_),
                        conduction_vel, next_activation_time, next_deactivation_time, received_potential);
    }
    static const vector<std::string> GetDataNames()
    {
        static const vector<std::string> names = {"Time", "type", "beat", "start_time", "apd", "di", "conduction_velocity",
                                                "next_activation_time", "next_deactivation_time",
                                                "received_potential"};
        return names;
    }
    //----------

private:
    NodeParameters*  parameters;         ///< @brief Parameters of the Node
    unsigned int    id;                 ///< @brief Unique Node id

    CellType        type = CellType::HEALTHY; ///< @brief Type of the Node
    bool            external_activation;
    bool            waiting;
    int             beat;               ///< @brief Last beat  of activation

    float           conduction_vel;             ///< @brief Conduction velocity in the long. direction
    Vector3         orientation = Vector3::Zero();     ///< @brief Fiber orientation.
                                                    ///< A normalized vector indicating longitudinal direction.
                                                    ///< Default to (0,0,0) for isotropic diffusion.

    ActionPotentialModel apd_model;

    ConductionVelocityModel   cv_model;

    float           start_time; ///< @brief Start time of the last activation.

    //float           diastolic_interval; ///< @brief Diastolic Interval of the last activation,
                                        ///< in milliseconds.
    float           kapd_v;
    float           path_length;         ///< @brief Length of the activation path until Node


    // Activation
    float               received_potential;

    float               next_activation_time;  ///< @brief Time of the next activation
    float               next_deactivation_time; ///< @brief Time of the next deactivation

    CellEvent *         next_event;  // Can be substituted by GetEvent(i) !!!

    NodeT *              activation_parent; ///< Node that activated this one
    //float               last_start_time;
    //float               activation_period;


    //void Deactivate(float current_time_);
    void Activate(float current_time_, const Geometry &geometry);
    void ComputeActivation(float current_time_, const Geometry &geometry);

    friend std::ostream & operator<<(std::ostream &os, const NodeT &node)
    {
        os << "Node id: " << node.id << " Type: " << (int)node.type << " Beat: " << node.beat;
        os << " conduction velocity: " << node.conduction_vel;
        os << " APD: " << node.apd_model.getAPD();
        os << " CV: " << node.cv_model.getConductionVelocity();
        os << " Start time: " << node.start_time;
        os << " Next activation time: " << node.next_activation_time;
        os << " Next deactivation time: " << node.next_deactivation_time;
        os << " Received potential: " << node.received_potential;
        if(node.activation_parent != nullptr)
            os << " Activation parent: " << node.activation_parent->id;
        return os;
    }
};

#include "node.cpp"

#endif // NODE_H
