/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#include <iostream>

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
    assert(current_time_ >= local_activation_time || local_activation_time == INFINITY);  // In the current beat

    CellActivationState state = CellActivationState::INACTIVE;
    if(this->type != CELL_TYPE_VOID and this->apd_model.IsActive(current_time_))
        state = CellActivationState::ACTIVE;
    else if( this->type != CELL_TYPE_VOID and
            this->next_activation_time > current_time_ and
            this->next_activation_time < INFINITY )
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
    this->local_activation_time = INFINITY;
    this->next_activation_time = INFINITY;
    this->next_deactivation_time = INFINITY;
    this->received_potential = 0.0;
    this->next_activation_event->Reset();
    this->next_deactivation_event->Reset();

    this->external_activation = false;

    this->conduction_vel = cv_model.getConductionVelocity();

    this->activation_parent = nullptr;
}



/**
 * From Node.pde: desactivar
*/
/*
template <typename APD, typename CVM>
void NodeT<APD, CVM>::Deactivate(float current_time_)
{
    this->local_activation_time = INFINITY;
    this->received_potential = 0.0;
    //this->path_length = 0.0; // @todo Check if this is correct
}
*/

/**
 * Calculates the action potential duration and the conduction velocity after the activation of the node.
 * From Node.pde: calcularActivacion
*/
template <typename APD, typename CVM>
void NodeT<APD, CVM>::ComputeActivation(float current_time_, const Geometry &geometry)
{
    // Conduction velocity. Has to be activated with the previous DI. Otherwise, DI will be 0
    // Thus, we activate before updating the APD.
    // @todo WARNING! if apd does not activate later, CV will be wrong
    // @todo Do we need electrotonic effect for CV?
    // @todo Do we need CV memory?
    this->cv_model.Activate(this->apd_model.getDI(current_time_),this->apd_model.getAPD());
    this->conduction_vel = this->cv_model.getConductionVelocity( );

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
            this->apd_model.Activate(current_time_, avg_apd, e_eff);
        }
        else
            this->apd_model.Activate(current_time_);
    }
    else
        this->apd_model.Activate(current_time_);

}

/**
 * Activation function for the iterative case with events.
 * From Node.pde: activar
*/
template <typename APD, typename CVM>
bool NodeT<APD, CVM>::Activate(float current_time_, const Geometry &geometry)
{
    bool activated = false;
    // Refractory state is checked here.
    // If next_activation_time is before deactivation, no activation.
    if (this->GetState(current_time_) <= CellActivationState::WAITING_FOR_ACTIVATION)
    {
        this->local_activation_time = current_time_;

        this->ComputeActivation(current_time_, geometry);

        this->next_deactivation_time = current_time_ + this->apd_model.getAPD();
        //this->life_time = this->action_potential_duration; ///< @todo Check life time.

        if (this->activation_parent != nullptr)
            this->beat = this->activation_parent->beat;

        activated = true;
    }
    // both if it activated or not, we reset next_activation_time
    this->next_activation_time = INFINITY;
    return activated;
}


/**
 * Prepare the node for activation at a given time.
 *
 * @param parent_ Node that activates this one.
 * @param activation_time_ Time of activation. If INFINITY, nothing is done.
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
float NodeT<APD, CVM>::ComputeConductionVelocity(const NodeT::Vector3 &direction_)
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




