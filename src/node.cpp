/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#include <iostream>
#include <limits>

#include "node.h"
#include "error.h"

using std::cerr;
using std::endl;


/**
 * @brief Default constructor.
 *
 */
template <typename APD, typename CVM>
NodeT<APD, CVM>::NodeT() :
    parameters(nullptr),
    next_activation_event(nullptr),
    next_deactivation_event(nullptr),
    activation_parent(nullptr)
{

}

/**
 * Obtain the activation state at current time
*/
template <typename APD, typename CVM>
typename NodeT<APD, CVM>::CellActivationState NodeT<APD, CVM>::GetState(float current_time_) const
{
    assert(current_time_ >= local_activation_time || local_activation_time == MAX_TIME);  // In the current beat

    CellActivationState state = CellActivationState::INACTIVE;
    if(this->type != CELL_TYPE_VOID && this->apd_model.IsActive(current_time_))
        state = CellActivationState::ACTIVE;
    else if( this->type != CELL_TYPE_VOID &&
            this->next_activation_time > current_time_ &&
            this->next_activation_time < MAX_TIME )
        state = CellActivationState::WAITING_FOR_ACTIVATION;
    else
        state = CellActivationState::INACTIVE;
    return state;
}

/**
 * @brief Reset the state of the Node to the initial state.
 * From Node.pde: reset
 */
template <typename APD, typename CVM>
void NodeT<APD, CVM>::Reset(float current_time_)
{
    this->beat = -1;

    // Activation data
    this->apd_model.Init(type,
        this->parameters->initial_apd,
        current_time_,
        0.0,
        this->parameters->correction_factor_apd
    );
    this->cv_model.Init(this->type,
        this->parameters->correction_factor_cv
    );
    this->local_activation_time = MAX_TIME;
    this->next_activation_time = MAX_TIME;
    this->next_deactivation_time = MAX_TIME;
    this->received_potential = 0.0;
    this->next_activation_event->Reset();
    this->next_deactivation_event->Reset();

    this->external_activation = false;

    this->conduction_vel = cv_model.getConductionVelocity();

    this->activation_parent = nullptr;
}


template <typename APD, typename CVM>
void NodeT<APD, CVM>::ReApplyParam(float current_time_)
{
    // @todo Check if correct !
    this->apd_model.Init(type,
        this->apd_model.getAPD(),
        current_time_,
        this->apd_model.getLastDI(),
        this->parameters->correction_factor_apd
    );
    this->cv_model.Init(this->type,
        this->parameters->correction_factor_cv
    );
}

/**
 * From Node.pde: desactivar
*/
/*
template <typename APD, typename CVM>
void NodeT<APD, CVM>::Deactivate(float current_time_)
{
    this->local_activation_time = MAX_TIME;
    this->received_potential = 0.0;
    //this->path_length = 0.0; // @todo Check if this is correct
}
*/

/**
 * Calculates the action potential duration and the conduction velocity after the activation of the node.
 * From Node.pde: calcularActivacion
*/
template <typename APD, typename CVM>
bool NodeT<APD, CVM>::ComputeActivation(float current_time_, const Geometry &geometry)
{
    // Conduction velocity. Has to be activated with the previous DI. Otherwise, DI will be 0
    // Thus, we activate before updating the APD.
    // @todo Do we need electrotonic effect for CV?
    // @todo Do we need CV memory?
    bool activated = false;

    float prev_di = this->apd_model.getDI(current_time_);
    float prev_apd = this->apd_model.getAPD();

    // Electrotonic effect
    float e_eff = this->parameters->electrotonic_effect;
    if (e_eff > 0.0)
    {
        float avg_apd = 0;
        unsigned int active_neighs = 0;
        for(int disp: geometry.displacement)    //(Node * neigh : this->neighbours)
        {
            // Danger! May go out of the array of Node
            NodeT * neigh = this + disp;
            if (neigh->GetState(current_time_) == CellActivationState::ACTIVE)
            {
                avg_apd += neigh->apd_model.getAPD();
                active_neighs += 1;
            }
        }

        if ( active_neighs > 0 )
        {
            avg_apd /= active_neighs;   // @todo Check: take into account *this ?
            activated = this->apd_model.Activate(current_time_, avg_apd, e_eff);
        }
        else
            activated = this->apd_model.Activate(current_time_);
    }
    else
        activated = this->apd_model.Activate(current_time_);

    if (activated)
    {
        // Conduction velocity
        this->cv_model.Activate(prev_di, prev_apd);
        this->conduction_vel = this->cv_model.getConductionVelocity( );
    }

    return activated;
}

/**
 * Activation function for the iterative case with events.
 * From Node.pde: activar
*/
template <typename APD, typename CVM>
bool NodeT<APD, CVM>::Activate(float current_time_, const Geometry &geometry)
{
    bool activated = false;

    if (this->GetState(current_time_) <= CellActivationState::WAITING_FOR_ACTIVATION)
    {
        if( ! this->ComputeActivation(current_time_, geometry))
            activated = false;
        else
        {
            this->local_activation_time = current_time_;

            this->next_deactivation_time = current_time_ + this->apd_model.getAPD();
            //this->life_time = this->action_potential_duration; ///< @todo Check life time.

            if (this->activation_parent != nullptr)
                this->beat = this->activation_parent->beat;

            activated = true;
        }
    }
    // both if it activated or not, we reset next_activation_time
    this->next_activation_time = MAX_TIME;
    return activated;
}


/**
 * Prepare the node for activation at a given time.
 *
 * @param parent_ Node that activates this one.
 * @param activation_time_ Time of activation.
 * @param path_length_ Length of the path from the origin of propagation to the node.
*/
template <typename APD, typename CVM>
typename NodeT<APD, CVM>::CellEvent* NodeT<APD, CVM>::ActivateAtTime(NodeT<APD, CVM>* parent_, float current_time_, float activation_time_)
{
    CellEvent *ev = nullptr;
    if( parent_->parameters->safety_factor < this->parameters->safety_factor)  // @todo Why? Also managed in Tissue
        return ev;

    // If the cell is active, only activation attempts that are
    // by final 95% of the ERP are considered.
    // @todo Convert to a parameter
    LOG::Info(false, "APD: ", this->apd_model.getAPD(), " ERP: ", this->apd_model.getERP() );
    if (this->GetState(activation_time_) == CellActivationState::ACTIVE)
        if (activation_time_ < this->local_activation_time + 0.95 * this->apd_model.getERP())
            return ev;

    // Only if activation is earlier we modify anything
    if (activation_time_ < this->next_activation_time)
    {
        // We update activation time
        this->next_activation_time = activation_time_;
        this->activation_parent = parent_;
        this->beat = parent_->beat;

        this->next_activation_event->ChangeEvent(this->next_activation_time);
        ev = this->next_activation_event;

    }

    return ev;
}

/**
 * Prepare the node for external activation at a given time.
 *
 * @todo Integrate, if possible, with ActivateAtTime
 *
 * @param activation_time_ Time of activation.
 * @param beat_n_ Beat number of the activation.
*/
template <typename APD, typename CVM>
typename NodeT<APD, CVM>::CellEvent* NodeT<APD, CVM>::ActivateAtTimeExternal(float activation_time_, int beat_n_)
{
    CellEvent *ev = nullptr;

    // Only if cell is inactive
    if (this->GetState(activation_time_) <= CellActivationState::WAITING_FOR_ACTIVATION)
        // Only if activation is earlier we modify anything
        if (activation_time_ < this->next_activation_time)
        {
            this->next_activation_time = activation_time_;

            this->next_activation_event->ChangeEvent(this->next_activation_time);
            ev = this->next_activation_event;

            this->external_activation = true;
            this->activation_parent = nullptr;
            this->beat = beat_n_;
            // It is an external activation, so we set the potential by hand.
            this->received_potential = 1.0;
        }

    return ev;
}



template <typename APD, typename CVM>
float NodeT<APD, CVM>::ComputeDirectionalConductionVelocity(const NodeT::Vector3 &direction_)
{
    float cond_vel;

    if(this->parameters->isotropic_diffusion || direction_.norm() < ALMOST_ZERO)
    {
        // BORDER_ZONE: isotropic conduction
        // The same if direction is not interpretable or
        // there is no fiber orientation
        cond_vel = this->conduction_vel;
    }
    else if(this->type != CELL_TYPE_VOID)
    {
        // HEALTHY: anisotropic conduction
        float cond_vel_long = abs(this->orientation.dot(direction_))/direction_.norm();
        float cond_vel_transv = sqrt(1.0 - cond_vel_long*cond_vel_long);

        // We scale the relative position in space, according to fiber orientation.
        // Being slower in the transversal direction, space expands in that
        // direction and points are further away.
        Vector2 p;
        p[0] = cond_vel_long;
        p[1] = cond_vel_transv/this->parameters->cond_veloc_transversal_reduction;

        cond_vel = this->conduction_vel/p.norm();
    }
    else
    {
        // otherwise, CORE -> no conduction at all.
        cond_vel = 0.0;
    }

    return cond_vel;
}

template <typename APD, typename CVM>
void NodeT<APD, CVM>::SaveState(std::ofstream & f, const class ParametersPool & parameters_pool, const CellEventQueue<NodeT> & event_queue) const
{
    f.write( (char *) &SAVE_VERSION, sizeof(int) );

    // Save id
    f.write( (char *) &id, sizeof(id) );
    // Save type
    f.write( (char *) &type, sizeof(CellType) );
    // Save external activation flag
    f.write( (char *) &external_activation, sizeof(external_activation) );

    // Save parameters index in the pool
    size_t param_index = std::numeric_limits<size_t>::max();
    if(parameters != nullptr)
    {
        param_index = parameters - & (parameters_pool.pool[0]);
    }
    f.write( (char *) &param_index, sizeof(size_t) );

    // Save activation data
    f.write( (char *) &beat, sizeof(beat) );
    f.write( (char *) &conduction_vel, sizeof(conduction_vel) );

    f.write( (char *) &local_activation_time, sizeof(local_activation_time) );
    f.write( (char *) &kapd_v, sizeof(kapd_v) );
    f.write( (char *) &received_potential, sizeof(received_potential) );
    f.write( (char *) &next_activation_time, sizeof(next_activation_time) );
    f.write( (char *) &next_deactivation_time, sizeof(next_deactivation_time) );

    // Save APD model state
    apd_model.SaveState(f);

    // Save CV model state
    cv_model.SaveState(f);

    // Save events. It is possible to know the index directly, but this way is safer and prepared for possible changes.
    // Save next activation event state
    size_t next_act_index = std::numeric_limits<size_t>::max();
    if(next_activation_event != nullptr)
        next_act_index = next_activation_event - & (event_queue.events[0]);
    f.write( (char *) &next_act_index, sizeof(size_t) );

    // Save next deactivation event state
    size_t next_deact_index = std::numeric_limits<size_t>::max();
    if(next_deactivation_event != nullptr)
        next_deact_index = next_deactivation_event - & (event_queue.events[0]);
    f.write( (char *) &next_deact_index, sizeof(size_t) );
}


