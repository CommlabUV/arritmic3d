/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef CELLEVENTQUEUE_H
#define CELLEVENTQUEUE_H

#include <vector>
#include <iostream>
#include <fstream>
#include <queue>
#include <cassert>
#include "definitions.h"


enum class CellEventType : unsigned char
{
    ACTIVATION = 0,
    DEACTIVATION
};


/**
* @brief Cell event for node dynamics
*
* It stores the information of the next activation/deactivation of a cell.
*/
template<typename Node>
struct Event
{
    Event() = default;

    /**
    * @brief Class constructor
    *
    * @param n Node
    * @param t Time for the event
    */
    Event(CellEventType t, Node * n = nullptr)
        : cell_node(n), event_type(t)
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
        this->event_time = MAX_TIME;
    }

    /**
     * Change the event for next activation/deactivation.
     * From Node.pde: siguienteEvento
    */
    void ChangeEvent(float time_)
    {
        this->event_time = time_;
    }

    Node * cell_node;                       ///< Pointer to the node to which it affects.
    int position_in_tree;                   ///< Position of the event in the CellEventQueue tree
    float event_time;                       ///< Event time
    CellEventType event_type;               ///< Event type

};

enum class SystemEventType : unsigned char
{
    NODE_EVENT = 0,
    EXT_ACTIVATION,
    FILE_WRITE,
    OTHER,
    NO_EVENT,
    SIZE
};

/**
 * @brief System event structure
 *
 * This structure is used to store system events that are not related to a specific node.
 */
struct SystemEvent
{
    float event_time;               ///< Event time
    SystemEventType type;
    unsigned char priority;         ///< Priority of the event (0 - before normal events, 1 - after normal events)

    bool operator<(const SystemEvent & other) const
    {
        return this->event_time > other.event_time; // Note: Inverted comparison for priority queue (min-heap)
    }
};

template<typename T>
class SystemQueue : public std::priority_queue<T>
{
public:

    void clear()
    {
        this->c.clear();
    }

    auto Data()
    {
        return this->c.data();
    }

    auto Data() const
    {
        return this->c.data();
    }

    /**
     * @brief Resize the underlying container. Clears the queue.
     */
    void Resize(size_t n)
    {
        this->clear();
        this->c.resize(n);
    }
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

    CellEventQueue() = default;

    /**
     * @brief Initializes the event queue with the nodes of the tissue
     * @pre The nodes type should be assigned before calling this function.
     */
    void Init(std::vector<Node> & tissue_nodes_, size_t n_live_nodes)
    {
        // Clear previous data
        tree.clear();
        events.clear();
        system_events.clear();

        tree.reserve(2 * n_live_nodes);

        // Assuming 2 events per node. @todo Generalize for more events
        events.reserve(2 * tissue_nodes_.size());
        for(auto & n : tissue_nodes_)
        {
            // Activation nodes are in even positions, deactivation in odd
            assert(int(CellEventType::ACTIVATION) == 0 && int(CellEventType::DEACTIVATION) == 1);
            events.push_back(CellEvent(CellEventType::ACTIVATION, &n) );
            events.push_back(CellEvent(CellEventType::DEACTIVATION, &n) );
        }
    }

    /**
     * @brief Get the event of a node using its id
    */
    CellEvent* GetEvent(int node_id, CellEventType type)
    {
        return &events[2*node_id + int(type)];
    }

    size_t GetIndex(CellEvent * event) const
    {
        assert(event != nullptr);
        return event - &events[0];
    }

    CellEvent* GetEventPtr(size_t index)
    {
        if(index >= events.size())
            return nullptr;
        return &events[index];
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

        if(pos_in_tree >= this->tree.size() || event != this->tree[pos_in_tree])
        {
            std::cerr << "QUEUE WARNING: the event is not where it was suposed to be. Insert." << std::endl;
            InsertCellEvent(event);
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
        return tree.empty() && system_events.empty();
    }


    /**
     * @brief Inserts an event in the queue
     *
     * @param event The event to insert.
     */
    void InsertCellEvent(CellEvent * event)
    {
        int pos = event->position_in_tree;
        // If the event is in the queue
        if( pos >= 0 && pos < int(tree.size()) && event == tree[pos])
            this->Update(event);
        else
        {
            tree.push_back(event);
            event->position_in_tree = tree.size() - 1;
            BubbleUp(event->position_in_tree);
        }

    };

    /**
     * @brief Inserts a system event in the queue
     *
     * @param event The system event to insert.
     */
    void InsertSystemEvent(float time, SystemEventType type, unsigned char priority = 1)
    {
        SystemEvent ev{time, type, priority};
        system_events.push(ev);
    }

    /**
     * @brief Gets information about the next event in the queue.
     *
     * @return A tuple containing the event time and type.
     */
    std::tuple<float, SystemEventType> GetInfo() const
    {
        // If cell events queue is empty, return the first system event
        if(tree.empty() )
        {
            const SystemEvent& ev = system_events.top();
            return std::make_tuple(ev.event_time, ev.type);
        }

        // If the system events queue is empty, return the first cell event
        if(system_events.empty() )
        {
            CellEvent ev = *GetFirstCell();
            return std::make_tuple(ev.event_time, SystemEventType::NODE_EVENT);
        }

        // Both queues have elements, compare the first element of each queue
        const SystemEvent& ev1 = system_events.top();
        CellEvent ev2 = *GetFirstCell();
        if (ev1.event_time < ev2.event_time || (ev1.event_time == ev2.event_time && ev1.priority == 0))
            return std::make_tuple(ev1.event_time, ev1.type);
        else
            return std::make_tuple(ev2.event_time, SystemEventType::NODE_EVENT);
    }

    /**
     * @brief Returns the first element in the queue.
     *
     * @return CellEvent* Pointer to the CellEvent with least event time in the queue.
     */
    CellEvent * GetFirstCell() const
    {
        assert(!IsEmpty() && "CellEventQueue::ExtractFirstCell: Queue is empty!");

        return tree[0];
    }

    /**
     * @brief Extracts the first element from the cell queue.
     *
     */
    void ExtractFirstCell()
    {
        assert(!IsEmpty() && "CellEventQueue::ExtractFirstCell: Queue is empty!");

        // Node event out of the queue
        tree[0]->position_in_tree = -1;

        // Remove the first element from the queue
        tree[0] = tree.back();
        tree[0]->position_in_tree = 0;
        tree.pop_back();
        BubbleDown(0);
    }

    void ExtractFirstSystem()
    {
        // Throws an exception if the queue is empty

        system_events.pop();
    }

    /**
     * Save the state of the event queue in binary format.
     * @param f Output file stream to save the state.
     * @param tissue_nodes Vector of nodes to which the events refer.
     */
    void SaveState(std::ofstream & f, const std::vector<Node> & tissue_nodes) const;

    /**
     * Load the state of the event queue from binary format.
     * @param f Input file stream to load the state.
     * @param tissue_nodes Vector of nodes to which the events refer.
     */
    void LoadState(std::ifstream & f, const std::vector<Node> & tissue_nodes);

private:

    /**
     * @brief Returns the index of the left descendant of the position given as argument.
     *
     * @param i Position in the tree to which get the left descendant.
     * @return size_t The position of the left descendant.
     */
    size_t GetLeftDescendant(size_t i) const
    {
        return 2*i+1;
    }

    /**
     * @brief Returns the index of the right descendant of the position given as argument.
     *
     * @param i Position in the tree to which get the right descendant.
     * @return size_t The position of the right descendant.
     */
    size_t GetRightDescendant(size_t i) const
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
    /*
    void SwapEvents(size_t i, size_t j)
    {
        CellEvent * ev = tree[i];
        tree[i] = tree[j];
        tree[i]->position_in_tree = i;
        tree[j] = ev;
        tree[j]->position_in_tree = j;
    }
    */

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

    // Data -------------------
    std::vector<CellEvent *> tree; ///< Vector to store the tree for the heap
    std::vector<CellEvent> events; ///< Vector to store the events

    SystemQueue<SystemEvent> system_events; ///< Priority queue for system events
};

template<typename Node>
void CellEventQueue<Node>::BubbleUp(size_t index)
{
    int i = index;  // Position of element going up
    size_t j = GetPredecessor(i);

    while( i > 0 && *tree[i] < *tree[j] )
    {
        // @todo Use std::swap(tree[i], tree[j]);  Update position_in_tree at the end
        //SwapEvents(i,j);
        std::swap(tree[i], tree[j]);
        // Now the element is in position j
        tree[i]->position_in_tree = i;
        //tree[j]->position_in_tree = j;  // To be updated at the end
        i = j;
        j = GetPredecessor(i);
    }
    tree[i]->position_in_tree = i;
}

template<typename Node>
void CellEventQueue<Node>::BubbleDown(size_t index)
{
    size_t min = index;
    size_t i;
    do
    {
        i = min;  // Position of element going down
        size_t left_desc = GetLeftDescendant(i);

        if ( left_desc < tree.size() && *tree[left_desc] < *tree[min] )
            min = left_desc;

        size_t right_desc = GetRightDescendant(i);

        if ( right_desc < tree.size() && *tree[right_desc] < *tree[min] )
            min = right_desc;

        if (min != i)
        {
            //SwapEvents(min,i);
            std::swap(tree[min], tree[i]);
            // Now the element is in position min
            tree[i]->position_in_tree = i;
            //tree[min]->position_in_tree = min; // To be updated at the end
        }

    } while(min != i);
    tree[min]->position_in_tree = min;
}

template<typename Node>
void CellEventQueue<Node>::SaveState(std::ofstream & f, const std::vector<Node> & tissue_nodes) const
{
    const int version = 1;

    f.write( (char *) &version, sizeof(int) );

    // Save number of events in the events vector
    size_t n_events = events.size();
    f.write( (char *) &n_events, sizeof(size_t) );
    // Save the events
    for(const auto & ev : events)
    {
        // Save the index of the node
        size_t node_index = ev.cell_node - &tissue_nodes[0];
        f.write( (char *) &node_index, sizeof(size_t) );
        // Save the rest of the event data
        f.write( (char *) &ev.position_in_tree, sizeof(int) );
        f.write( (char *) &ev.event_time, sizeof(float) );
        unsigned char ev_type = static_cast<unsigned char>(ev.event_type);
        f.write( (char *) &ev_type, sizeof(unsigned char) );
    }

    // Save number of events in the tree
    size_t n_tree = tree.size();
    f.write( (char *) &n_tree, sizeof(size_t) );
    // Save the pointers in the tree
    for(const auto ev_ptr : tree)
    {
        size_t ev_index = ev_ptr - &events[0];
        f.write( (char *) &ev_index, sizeof(size_t) );
    }

    // Save number of system events
    size_t n_system_events = system_events.size();
    f.write( (char *) &n_system_events, sizeof(size_t) );
    // Save the system events
    auto system_events_data = system_events.Data();
    f.write( (char *) system_events_data, n_system_events * sizeof(SystemEvent) );
}

template<typename Node>
void CellEventQueue<Node>::LoadState(std::ifstream & f, const std::vector<Node> & tissue_nodes)
{
    const int version = 1;

    int file_version;
    f.read( (char *) &file_version, sizeof(int) );
    if(file_version != version)
        throw std::runtime_error("CellEventQueue::LoadState: Wrong file version.");

    // Load events
    size_t n_events;
    f.read( (char *) &n_events, sizeof(size_t) );
    events.resize(n_events);
    for(size_t i = 0; i < n_events; ++i)
    {
        size_t node_index;
        f.read( (char *) &node_index, sizeof(size_t) );
        events[i].cell_node = const_cast<Node *>(&tissue_nodes[node_index]);
        f.read( (char *) &events[i].position_in_tree, sizeof(int) );
        f.read( (char *) &events[i].event_time, sizeof(float) );
        unsigned char ev_type;
        f.read( (char *) &ev_type, sizeof(unsigned char) );
        events[i].event_type = static_cast<CellEventType>(ev_type);
    }

    // Load events in the tree
    size_t n_tree;
    f.read( (char *) &n_tree, sizeof(size_t) );
    tree.resize(n_tree);
    for(size_t i = 0; i < n_tree; ++i)
    {
        size_t ev_index;
        f.read( (char *) &ev_index, sizeof(size_t) );
        tree[i] = &events[ev_index];
    }

    // Load system events
    size_t n_system_events;
    f.read( (char *) &n_system_events, sizeof(size_t) );
    system_events.Resize(n_system_events);
    auto system_events_data = system_events.Data();
    f.read( (char *) system_events_data, n_system_events * sizeof(SystemEvent) );
}

#endif // CELLEVENT_H
