/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */
#include <iostream>
#include <string>
#include "../src/node.h"
#include "../src/spline.h"
#include "../src/cell_event_queue.h"
#include "../src/tissue.h"
#include "../src/action_potential.h"

int main(int argc, char **argv)
{
    using Node = NodeT<ActionPotential>;
    Node MyNode;
    std::cout << "Hello, world!" << std::endl;

    //Spline r = Spline();

    // Test of the CellEventQueue
    std::vector<Node> nodes(10);
    CellEventQueue<Node> qq(nodes);
    Node::CellEvent *ev;
    ev = qq.GetEvent(0);
    ev->event_time = 6.0;
    qq.InsertEvent(ev);

    ev = qq.GetEvent(1);
    ev->event_time = 3.0;
    qq.InsertEvent(ev);

    ev = qq.GetEvent(2);
    ev->event_time = 7.0;
    qq.InsertEvent(ev);

    ev = qq.GetEvent(3);
    ev->event_time = 4.0;
    qq.InsertEvent(ev);

    ev = qq.GetEvent(4);
    ev->event_time = 4.5;
    qq.InsertEvent(ev);

    std::cout << "Highest priority before update: " << qq.getFirst()->event_time << std::endl;

    ev->event_time = 1.0;
    qq.Update(ev);

    std::cout << "Highest priority after update: " << qq.getFirst()->event_time << std::endl;

    while(not qq.IsEmpty())
    {
        ev = qq.ExtractFirst();
        std::cout << "  : " <<  ev->event_time <<  std::endl;
    }


    return 0;
}
