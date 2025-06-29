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
NodeT<APD, CVM>::NodeT() : next_event(nullptr)
{

}

/**
 * Obtain the activation state at current time
*/
template <typename APD, typename CVM>
typename NodeT<APD, CVM>::CellActivationState NodeT<APD, CVM>::GetState(float current_time_) const
{
    assert(current_time_ >= start_time || start_time == INFINITY);  // In the current beat

    CellActivationState state = CellActivationState::INACTIVE;
    if(this->type != CellType::CORE and this->apd_model.IsActive(current_time_))
        state = CellActivationState::ACTIVE;
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
    this->apd_model.Init(type, this->parameters->tissue_region, this->parameters->initial_apd, current_time_,0.0);
    this->cv_model.Init(type, this->parameters->tissue_region);
    this->start_time = INFINITY;
    this->next_activation_time = INFINITY;
    this->next_deactivation_time = INFINITY;
    this->received_potential = 0.0;
    this->next_event->Reset();

    this->external_activation = false;

    this->conduction_vel = this->parameters->correction_factor_cv_border_zone*cv_model.getConductionVelocity();

    this->path_length = 0;

    this->waiting = false;
}



/**
 * From Node.pde: desactivar
*/
/*
template <typename APD, typename CVM>
void NodeT<APD, CVM>::Deactivate(float current_time_)
{
    this->start_time = INFINITY;
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
    // Action potential duration
    //float node_apd = this->apd_model.getAPD() * this->parameters->correction_factor_apd;

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

        if ( active_neighs > 0)
            avg_apd /= active_neighs;   // @todo Check: take into account *this ?
        else
            avg_apd = this->apd_model.getAPD() * this->parameters->correction_factor_apd;

        // @todo Do we need electrotonic effect for CV?
        this->cv_model.Activate(this->apd_model.getDI(current_time_));
        this->apd_model.Activate(current_time_, avg_apd, e_eff);
//        this->action_potential_duration = node_apd*(1.0 - e_eff) + avg_apd*e_eff;
    }
    else
    {
        // Conduction velocity, first. Otherwise, DI will be 0
        this->cv_model.Activate(this->apd_model.getDI(current_time_));
        this->apd_model.Activate(current_time_);
//        this->action_potential_duration = node_apd;
    }

    // Conduction Velocity
    this->conduction_vel = this->cv_model.getConductionVelocity( );
}

/**
 * Activation function for the iterative case with events.
 * From Node.pde: activar
*/
template <typename APD, typename CVM>
void NodeT<APD, CVM>::Activate(float current_time_, const Geometry &geometry)
{
    // @todo Original function has no parameter, current_time_ is "tiempo transcurrido" and is of type void. Check
    this->start_time = current_time_;  // Old next_activation_time

    this->ComputeActivation(current_time_, geometry);

    this->next_activation_time = INFINITY;
    this->next_deactivation_time = current_time_ + this->apd_model.getAPD();
    //this->life_time = this->action_potential_duration; ///< @todo Check life time.

    if (this->activation_parent != nullptr)
    {
        if (this->beat == this->activation_parent->beat)  // There is a reentry
        {
            // LOG::Warning(true, "Reentry detected in node ", this->id, " at time ", current_time_,
            //             " beat ", this->beat);
            // @todo Better reentry handling. Filter external activations?
        }

        this->beat = this->activation_parent->beat;

        ///< @todo Create the path search of the reentry ?
    }

}


/**
 * Prepare the node for activation at a given time.
 *
 * @param parent_ Node that activates this one.
 * @param activation_time_ Time of activation. If INFINITY, nothing is done.
 * @param path_length_ Length of the path from the origin of propagation to the node.
*/
template <typename APD, typename CVM>
typename NodeT<APD, CVM>::CellEvent* NodeT<APD, CVM>::ActivateAtTime(NodeT<APD, CVM>* parent_, float activation_time_, float path_length_)
{
    CellEvent *ev = nullptr;
    if( parent_->parameters->safety_factor < this->parameters->safety_factor)  // @todo Why?
        return ev;


    // Only if activation is earlier we modify anything
    if (activation_time_ < this->next_activation_time)
    {
        if (this->GetState(activation_time_) <= CellActivationState::WAITING_FOR_ACTIVATION)
        {
            this->next_activation_time = activation_time_;

            this->next_event->ChangeEvent(this->next_activation_time);
            ev = this->next_event;
            //this->beat = parent_->beat;
        }
        else{ // @todo CHECK! Not implemented.
            // The node is active. We check if Effective Refractory Period has ended.
            // If that is the case, we schedule the next activation time, but we do
            // not generate event, that will be generated on deactivation.
        }
        ///< @todo Check for reentry.

        this->activation_parent = parent_;
        this->path_length = path_length_;
    }

    return ev;
}

/**
 * Prepare the node for external activation at a given time.
 *
 * @param activation_time_ Time of activation.
 * @param beat_n_ Beat number of the activation.
*/
template <typename APD, typename CVM>
typename NodeT<APD, CVM>::CellEvent* NodeT<APD, CVM>::ActivateAtTimeExternal(float activation_time_, int beat_n_)
{
    CellEvent *ev = nullptr;

    // Only if activation is earlier we modify anything
    if (activation_time_ < this->next_activation_time)
    {
        this->next_activation_time = activation_time_;

        this->next_event->ChangeEvent(this->next_activation_time);
        ev = this->next_event;

        this->external_activation = true;
        this->activation_parent = nullptr;
        this->path_length = 0.0;
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

    if(this->type == CellType::BORDER_ZONE || this->parameters->isotropic_diffusion ||
        direction_.norm() < ALMOST_ZERO)
    {
        // BORDER_ZONE: isotropic conduction
        // The same if direction is not interpretable or
        // there is no fiber orientation
        cond_vel = this->conduction_vel;
    }
    else if(this->type == CellType::HEALTHY)
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




