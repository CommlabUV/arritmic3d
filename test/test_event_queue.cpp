/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */
#include <iostream>
#include <string>
#include <tuple>
#include "../src/node.h"
#include "../src/cell_event_queue.h"
#include "../src/tissue.h"

class APM {};
class CVM {};

int main(int argc, char **argv)
{
    using Node = NodeT<APM, CVM>;

    // Test of the CellEventQueue
    std::vector<Node> nodes(10);
    CellEventQueue<Node> qq;
    qq.Init(nodes, nodes.size());

    Node::CellEvent *ev;

    ev = qq.GetEvent(0, CellEventType::ACTIVATION);
    ev->event_time = 6.5;
    qq.InsertCellEvent(ev);

    ev = qq.GetEvent(1, CellEventType::ACTIVATION);
    ev->event_time = 3.0;
    qq.InsertCellEvent(ev);

    ev = qq.GetEvent(1, CellEventType::DEACTIVATION);
    ev->event_time = 2.0;
    qq.InsertCellEvent(ev);

    ev = qq.GetEvent(2, CellEventType::ACTIVATION);
    ev->event_time = 7.0;
    qq.InsertCellEvent(ev);

    ev = qq.GetEvent(3, CellEventType::ACTIVATION);
    ev->event_time = 4.0;
    qq.InsertCellEvent(ev);

    ev = qq.GetEvent(4, CellEventType::ACTIVATION);
    ev->event_time = 4.5;
    qq.InsertCellEvent(ev);

    qq.InsertSystemEvent(4.0, SystemEventType::FILE_WRITE, 1);

    auto [time1, type1] = qq.GetInfo();
    std::cout << "Highest priority before update: " << time1 << "  : " << int(type1) << std::endl;

    ev->event_time = 1.0;
    qq.Update(ev);

    auto [time2, type2] = qq.GetInfo();
    std::cout << "Highest priority after update: " << time2 << "  : " << int(type2) << std::endl;

    qq.InsertSystemEvent(1.0, SystemEventType::EXT_ACTIVATION, 0);

    // Save state
    std::ofstream f("event_queue_state.bin", std::ios::binary);
    qq.SaveState(f, nodes);
    f.close();

    while(not qq.IsEmpty())
    {
        auto [time, type] = qq.GetInfo();
        std::cout << "Time: " << time << "  : Type: " << int(type) ;

        if(type == SystemEventType::NODE_EVENT)
        {
            auto ev = qq.GetFirstCell();
            std::cout << " (Act 0 / Deact 1): " << int(ev->event_type);
            qq.ExtractFirstCell();
        }
        else
        {
            qq.ExtractFirstSystem();
        }
        std::cout << std::endl;
    }

    // Load state
    std::ifstream f_in("event_queue_state.bin",  std::ios::binary);
    qq.LoadState(f_in, nodes);
    f_in.close();

    std::cout << "After loading state:" << std::endl;
    while(not qq.IsEmpty())
    {
        auto [time, type] = qq.GetInfo();
        std::cout << "Time: " << time << "  : Type: " << int(type) ;

        if(type == SystemEventType::NODE_EVENT)
        {
            auto ev = qq.GetFirstCell();
            std::cout << " (Act 0 / Deact 1): " << int(ev->event_type);
            qq.ExtractFirstCell();
        }
        else
        {
            qq.ExtractFirstSystem();
        }
        std::cout << std::endl;
    }

    return 0;
}
