/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef TISSUE_H
#define TISSUE_H

#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>
#include <eigen3/Eigen/Dense>

#include "geometry.h"
#include "node.h"
#include "cell_event_queue.h"
#include "error.h"
#include "basic_tissue.h"

using std::vector;

/**
 * @brief Class to model cardiac tissue. Adds propagation functions to the basic tissue.
 *
 * It contains a vector of nodes, that represent cardiac cells, and
 * the functions to perform the simulation using the fast reaction
 * diffusion model.
 */
template <typename APM,typename CVM>
class CardiacTissue : public BasicTissue<APM,CVM>
{
public:
    using CellEvent = Event<NodeT<APM,CVM> >;
    using Node = NodeT<APM,CVM>;

    CardiacTissue(int size_x_, int size_y_, int size_z_, float dx_, float dy_, float dz_) :
                BasicTissue<APM,CVM>(size_x_, size_y_, size_z_, dx_, dy_, dz_) {}
    bool update(int debug = 0);
    void ExternalActivation(const vector<size_t> & nodes, float activation_time, int beat_n);
    void TriggerEvent(CellEvent* ev);

private:
};

/**
 * Update the tissue simulation processing an event.
 * @param debug Debug level
 * @return true if there is an event of the simulation.
*/
template <typename APM,typename CVM>
bool CardiacTissue<APM,CVM>::update(int debug)
{
    float t_next_event = INFINITY;  // t_next_event was attribute of AC.

    if(!this->event_queue.IsEmpty())
    {
        CellEvent * ev = this->event_queue.ExtractFirst();
        if(debug > 0)
            std::cout << "Event at t=" << ev->event_time << " for node " << ev->cell_node->id << std::endl;
        if(debug > 1)
            std::cout << "Before processing event: " << *(ev->cell_node) << std::endl;

        float new_t = ev->event_time;
        LOG::Warning(new_t < this->tissue_time, " t=", this->tissue_time, " older than   ev.t=", new_t);

        this->tissue_time = new_t;

        // System event
        if(ev->cell_node->id == 0)
        {
            // For now, system events are only timer events
            this->tissue_nodes[0].next_event->ChangeEvent(this->tissue_time + this->timer);
            this->event_queue.InsertEvent(this->tissue_nodes[0].next_event);
            return true;
        }

        TriggerEvent(ev);
        //n_cells_updated++;
        if(!this->event_queue.IsEmpty())
        {
            CellEvent * ev_next = this->event_queue.getFirst();
            t_next_event = ev_next->event_time;
            LOG::Error(t_next_event < this->tissue_time, " We skiped an event!");
        }
        if(debug > 1)
            std::cout << "After processing event: " << *(ev->cell_node) << std::endl;

        // Store info in case of sensor node
        if(ev->cell_node->parameters->sensor)
        {
            this->sensor_dict.AddData(ev->cell_node->id, ev->cell_node->GetData(this->tissue_time));
        }

    }
    else
        std::cout << " NO EVENT " << std::endl;

    return false;
}



/**
 * External activation of a set of nodes.
 * @param nodes List of nodes to activate.
 * @param activation_time Time of activation.
 *
 * @todo If the node is already active, generates a core-dump.
*/
template <typename APM,typename CVM>
void CardiacTissue<APM,CVM>::ExternalActivation(const vector<size_t> & nodes, float activation_time, int beat_n)
{
    for(size_t i = 0; i < nodes.size(); i++)
    {
        if(this->tissue_nodes.at(nodes[i]).type == CellType::CORE)
        {
            LOG::Warning(true, "ExternalActivation(): Node ", nodes[i], " is a CORE node. Activation ignored.");
            continue;
        }
        CellEvent * e = this->tissue_nodes.at(nodes[i]).ActivateAtTimeExternal(activation_time, beat_n);
        if(e)
            this->event_queue.InsertEvent(e);
    }
}

/**
 * Process the event in the node.
 * From Node.pde: dispara_evento
*/
template <typename APM,typename CVM>
void CardiacTissue<APM,CVM>::TriggerEvent(CellEvent* ev)
{
    Node * node_ = ev->cell_node;

    // Time must match
    LOG::Warning(ev->event_time != this->tissue_time, "TriggerEvent(): In node ", node_->id,
            "Event time mismatch. Event time is ", ev->event_time, " while current time is ", this->tissue_time );

    // Activation
    if ( this->tissue_time == node_->next_activation_time )
    {
        // Safety factor
        if ( not node_->external_activation and node_->received_potential < node_->parameters->min_potential*node_->parameters->safety_factor)
        {
            // No activation. Potential too low
            LOG::Info(true, "TriggerEvent(): Safety factor acting. Node ", node_->id, " NOT activated with total potential ", node_->received_potential,
                 " does not reach the minimum: ", node_->parameters->min_potential);

            // Deactivate the node. There are situations where this is not correct as more potentials sent to the node may activate it, but this is the behaviour of pde version.
            node_->received_potential = 0.0;    // @todo Check
            node_->next_activation_time = INFINITY;
            //event_queue.InsertEvent( node_->Deactivate(tissue_time) );  // @todo Check if it is necessary something more of Deactivate/deactivation event
        }
        else
        {
            /// @todo Missing reentry checks
            // The Node is activated.
            node_->Activate(this->tissue_time, this->tissue_geometry);
            node_->next_event->ChangeEvent(node_->next_deactivation_time);
            this->event_queue.InsertEvent(node_->next_event); // @todo Check

            // The potential is sent to inactive neighbours.
            vector<Node*> inactive_neighs;
            for (unsigned int i = 0; i < this->tissue_geometry.num_neighbours; ++i )
            {
                // Danger! May go out of the array of Node
                Node* neigh = node_ + this->tissue_geometry.displacement[i];  // @todo Look for a better way to do this
                assert(neigh >= this->tissue_nodes.data() && neigh < this->tissue_nodes.data() + this->tissue_nodes.size());
                // We skip core nodes
                if ( neigh->type != CellType::CORE )
                {
                    float distance = this->tissue_geometry.distance_to_neighbour[i];
                    Vector3 activation_dir = - this->tissue_geometry.relative_position[i]; // @todo Maybe we should define the opposite direction in geometry

                    // We compute the direct diffusion, through the graph.
                    // This is the case for this->parameters.diffusion_mode == MINIMUM_TIME
                    // It is computed anyway, to keep the fastest.

                    float direct_vel = node_->ComputeConductionVelocity(activation_dir);
                    float direct_activation_time = node_->start_time + distance/direct_vel;
                    CellEvent * ev = neigh->ActivateAtTime(node_, direct_activation_time, node_->path_length + distance);

                    // if ev is nullptr means it is active and rejected activation or it has an earlier activation time
                    if (ev != nullptr)
                    {
                        this->event_queue.InsertEvent(ev);
                        inactive_neighs.push_back(neigh); // @todo Maybe it should include nodes with earlier activation time
                    }
                }
            }

            if (inactive_neighs.size() > 0)
            {
                float potential_to_send = node_->received_potential/inactive_neighs.size()*node_->parameters->safety_factor;
                for (auto neigh : inactive_neighs)
                    neigh->received_potential += potential_to_send;
            }

            node_->external_activation = false;
        }
    }

    if( this->tissue_time == node_->next_deactivation_time)
    {
        // Deactivate node
        node_->received_potential = 0.0;

        if (node_->next_activation_time < INFINITY)
        {
            node_->next_event->ChangeEvent(node_->next_activation_time);
            this->event_queue.InsertEvent(node_->next_event);
        }

    }

}

/**
 * Propagates activation from a neighboring node.
 * From Node.pde: propaga_activacion
*/
/*
CellEvent* CardiacTissue::PropagateActivation(Node* origin_, Node* dest_, float current_time_, float distance, const Vector3 & activation_direction)
{
    CellEvent *ev = nullptr;

    // We compute the direct diffusion, through the graph.
    // This is the case for this->parameters.diffusion_mode == MINIMUM_TIME
    // It is computed anyway, to keep the fastest.

    float direct_vel = origin_->ComputeConductionVelocity(activation_direction);
    float direct_activation_time = origin_->start_time + distance/direct_vel;

    if(direct_activation_time < dest_->next_activation_time)
        ev = dest_->ActivateAtTime(origin_, direct_activation_time, current_time_, origin_->path_length + distance,  origin_->beat);

    return ev;
}
*/


#endif
