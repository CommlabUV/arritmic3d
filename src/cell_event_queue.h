/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef CELLEVENTQUEUE_H
#define CELLEVENTQUEUE_H

#include <vector>
#include <iostream>


/**
* @brief Cell event for node dynamics
*
* It stores the information of the next activation/deactivation of a cell.
*/
template<typename Node>
struct Event
{
    /**
    * @brief Class constructor
    *
    * @param n Node
    * @param t Time for the event
    */
    Event(Node * n = nullptr) : cell_node(n)
    {
        this->Reset();
    }

    /**
    * @brief Comparison operator
    *
    * @param other_ev Reference to the other event
    * @return bool Returns true if this->time is less than other
    */
    bool operator<(const Event & other_ev) const
    {
        return this->event_time < other_ev.event_time;
    }

    void Reset()
    {
        this->position_in_tree = -1;
        this->event_time = INFINITY;
    }

    /**
     * Change the event for next activation/deactivation.
     * From Node.pde: siguienteEvento
    */
    void ChangeEvent(float time_)
    {
        this->event_time = time_;
    }

    int position_in_tree;                   ///< Position of the event in the CellEventQueue tree
    float event_time;                       ///< Event time
    Node * cell_node;                 ///< Pointer to the node to which it affects.

};


/**
 * @brief Priority queue of cell events
 *
 * Priority queue of cell events to store the events of the activation process.
 * The queue allows modification of the priority of a node.
 */
template<typename Node>
class CellEventQueue
{
public:

    using CellEvent = Event<Node>;

    CellEventQueue(std::vector<Node> & tissue_nodes_)
    {
        tree.reserve(tissue_nodes_.size());

        // Assuming 1 event per node. @to Generalize for more events
        events.reserve(tissue_nodes_.size());
        for(auto & n : tissue_nodes_)
        {
            events.push_back(CellEvent(&n) );
        }
    }

    /**
     * @brief Get the event of a node using its id
    */
    CellEvent* GetEvent(int node_id)
    {
        return &events[node_id];
    }

    /**
     * @brief Update the priority of an event in the queue
     *
     * @param event Event to update
     *
     * This function reorders the underlying tree to place the event given as
     * a parameter in the right location according to its event time.
     * This function does not modify the event time. It restores the consistency
     * of the location of the given event that, supposedly, has changed its
     * event time.
     *
     * If the event time has not changed or if the location is already correct it does
     * not change the queue. This function does not check the integrity of the complete
     * heap.
     *
     * If the event to update is not in the queue, then it is inserted.
     */
    void Update(CellEvent * event)
    {
        size_t pos_in_tree = event->position_in_tree;

        if(pos_in_tree >= this->tree.size() or event != this->tree[pos_in_tree])
        {
            std::cerr << "QUEUE WARNING: the event is not where it was suposed to be. Insert." << std::endl;
            InsertEvent(event);
        }
        else
        {
            BubbleUp(event->position_in_tree);
            BubbleDown(event->position_in_tree);
        }
    }


    /**
     * @brief Indicates if the queue is empty
     *
     * @return bool Returns true if the queue is empty, false otherwise.
     */
    bool IsEmpty() const
    {
        return tree.empty();
    }


    /**
     * @brief Inserts an event in the queue
     *
     * @param event The event to insert.
     */
    void InsertEvent(CellEvent * event)
    {
        int pos = event->position_in_tree;
        // If the event is in the queue
        if( pos >= 0 and pos < int(tree.size()) and event == tree[pos])
            this->Update(event);
        else
        {
            tree.push_back(event);
            event->position_in_tree = tree.size() - 1;
            BubbleUp(event->position_in_tree);
        }

    };


    /**
     * @brief Returns the first element in the queue.
     *
     * @return CellEvent* Pointer to the CellEvent with least event time in the queue.
     */
    CellEvent * getFirst() const
    {
        if( IsEmpty() )
            return nullptr;
        return tree[0];
    }

    /**
     * @brief Returns the first element and removes it from the queue.
     *
     * @return CellEvent* Pointer to the CellEvent with least event time in the queue.
     */
    CellEvent * ExtractFirst()
    {
        CellEvent * ev = getFirst();
        if( ev != nullptr )
        {
            tree[0] = tree.back();
            tree[0]->position_in_tree = 0;
            tree.pop_back();
            BubbleDown(0);

            ev->position_in_tree = -1;
        }

        return ev;
    }


private:


    /**
     * @brief Returns the index of the left descendant of the position given as argument.
     *
     * @param i Position in the tree to which get the left descendant.
     * @return size_t The position of the left descendant.
     */
    size_t GetLeftDescdentant(size_t i) const
    {
        return 2*i+1;
    }

    /**
     * @brief Returns the index of the right descendant of the position given as argument.
     *
     * @param i Position in the tree to which get the right descendant.
     * @return size_t The position of the right descendant.
     */
    size_t GetRightDescdentant(size_t i) const
    {
        return 2*i+2;
    }


    /**
     * @brief Returns the index of the predecessor of the position given as argument.
     *
     * @param i Position in the tree to which get the predecessor.
     * @return size_t The position of the predecessor.
     */
    size_t GetPredecessor(size_t i) const
    {
        return (i-1)/2;
    }


    /**
     * @brief Swaps two events in the tree.
     *
     * @param i First event to swap.
     * @param j Second event to swap.
     */
    void SwapEvents(size_t i, size_t j)
    {
        CellEvent * ev = tree[i];
        tree[i] = tree[j];
        tree[i]->position_in_tree = i;
        tree[j] = ev;
        tree[j]->position_in_tree = j;
    }


    /**
     * @brief Moves the element at the given index to its right position moving it up if necessary.
     *
     * @param index The index to promote.
     */
    void BubbleUp(size_t index);

    /**
     * @brief Moves the element at the given index to its right position moving it down if necessary.
     *
     * @param index The index to demote.
     */
    void BubbleDown(size_t index);

    // Data
    std::vector<CellEvent *> tree; ///< Vector to store the tree for the heap
    std::vector<CellEvent> events; ///< Vector to store the events


};

template<typename Node>
void CellEventQueue<Node>::BubbleUp(size_t index)
{
    int i = index;
    size_t j = GetPredecessor(i);

    while( i > 0 and *tree[i] < *tree[j] )
    {
        SwapEvents(i,j);
        i = j;
        j = GetPredecessor(i);
    }
}

template<typename Node>
void CellEventQueue<Node>::BubbleDown(size_t index)
{
    size_t min = index;
    size_t i;
    do
    {
        i = min;
        size_t left_desc = GetLeftDescdentant(i);

        if ( left_desc < tree.size() and *tree[left_desc] < *tree[min] )
            min = left_desc;

        size_t right_desc = GetRightDescdentant(i);

        if ( right_desc < tree.size() and *tree[right_desc] < *tree[min] )
            min = right_desc;

        if (min != i)
            SwapEvents(min,i);

    }while(min != i);
}

#endif // CELLEVENT_H
