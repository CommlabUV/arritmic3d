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
    SystemEventType update(int debug = 0);
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
SystemEventType CardiacTissue<APM,CVM>::update(int debug)
{

    if(!this->event_queue.IsEmpty())
    {
        auto [ev_time, ev_type] = this->event_queue.GetInfo();
        LOG::Info(debug > 0, "Event at t=", ev_time, " type=", (int)ev_type);

        float new_t = ev_time;
        LOG::Warning(new_t < this->tissue_time, " t=", this->tissue_time, " older than   ev.t=", new_t);

        this->tissue_time = new_t;

        // System event
        if(ev_type != SystemEventType::NODE_EVENT)
        {
            this->event_queue.ExtractFirstSystem();

            float inc_time = this->timer.at(int(ev_type));
            if(inc_time > 0)
            {
                int priority = 1;
                if(ev_type == SystemEventType::EXT_ACTIVATION)
                    priority = 0;
                float new_ev_time = this->tissue_time + inc_time;
                this->event_queue.InsertSystemEvent(new_ev_time, ev_type, priority);
            }

            return ev_type;
        }

        // Node event
        CellEvent * ev = this->event_queue.GetFirstCell();
        this->event_queue.ExtractFirstCell();
        LOG::Info(debug > 0, "Node Event for node ", ev->cell_node->id);
        LOG::Info(debug > 1, "Before processing event. Node value: ", *(ev->cell_node) );

        TriggerEvent(ev);
        //n_cells_updated++;

        // Check next event
        if(!this->event_queue.IsEmpty())
        {
            auto ev_info = this->event_queue.GetInfo();
            LOG::Error(std::get<0>(ev_info) < this->tissue_time, " We skiped an event!");
        }

        LOG::Info(debug > 1, "After processing event. Node value: ", *(ev->cell_node) );

        // Store info in case of sensor node
        if(ev->cell_node->parameters->sensor)
        {
            this->sensor_dict.AddData(ev->cell_node->id, ev->cell_node->GetData(this->tissue_time));
        }

        return SystemEventType::NODE_EVENT;
    }
    else
        return SystemEventType::NO_EVENT;
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
        if(e != nullptr)
            this->event_queue.InsertCellEvent(e);
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
        }
        else
        {
            /// @todo Missing reentry checks
            // The Node is activated.
            if (node_->Activate(this->tissue_time, this->tissue_geometry))
            {
                node_->next_deactivation_event->ChangeEvent(node_->next_deactivation_time);
                this->event_queue.InsertCellEvent(node_->next_deactivation_event); // @todo Check

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
                        float direct_vel = node_->ComputeConductionVelocity(activation_dir);
                        float direct_activation_time = node_->local_activation_time + distance/direct_vel;
                        CellEvent * ev_neigh = neigh->ActivateAtTime(node_, this->tissue_time, direct_activation_time);

                        // if ev_neigh is nullptr means it is active and rejected activation or it has an earlier activation time
                        if (ev_neigh != nullptr)
                        {
                            this->event_queue.InsertCellEvent(ev_neigh);
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

            }
            node_->next_activation_time = INFINITY;
        }
    }

    if( this->tissue_time == node_->next_deactivation_time)
    {
        // Deactivate node
        node_->received_potential = 0.0;
        node_->external_activation = false;

        if (node_->next_activation_time < INFINITY)
        {
            node_->next_activation_event->ChangeEvent(node_->next_activation_time);
            this->event_queue.InsertCellEvent(node_->next_activation_event);
        }

    }

}


#endif
