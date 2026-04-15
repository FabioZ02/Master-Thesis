#include "BT_Helpers.hh"
#include <iostream>
#include <vector>
#include <algorithm>

BT_SolutionManager::BT_SolutionManager(const BT_Input &input)
    : SolutionManager<BT_Input, BT_Output>(input, "BTSolutionManager") {}

void BT_SolutionManager::GreedyState(BT_Output& out)
{
    // Sorted tasks by priority
    std::vector<unsigned> sorted_task(in.OrdersCount());
    std::iota(sorted_task.begin(), sorted_task.end(), 0);
    std::sort(sorted_task.begin(), sorted_task.end(), [&](unsigned a, unsigned b) {
        return in.Order_Priority(a) > in.Order_Priority(b);
    });

    std::vector<unsigned> machine_load(in.ResourcesCount(), 0);
    unsigned current_period = 1;

    for (unsigned t : sorted_task)
    {
        unsigned min_load = UINT_MAX;
        unsigned selected_machine = in.ResourcesCount();
        for (unsigned m=0; m < in.ResourcesCount(); m++)
        {
            if (in.IsCompatible(t, m) && machine_load[m] < min_load)
            {
                min_load = machine_load[m];
                selected_machine = m;
            }
        }

        if(selected_machine == in.ResourcesCount())
        {
            std::cerr << "No compatible machine found for task " <<  std::endl; 
            exit(1);
        }

        unsigned type_id = in.OrderTypeId(t);
        unsigned S_max   = in.OrderType_MaxGroupSize(type_id);
        unsigned s_t     = in.Order_Quantity(t);

        // If the task doesn't fit in the current period
        if ((S_max-machine_load[selected_machine]) < s_t)
        {
            current_period++;
            std::fill(machine_load.begin(), machine_load.end(), 0);

            selected_machine = in.ResourcesCount();
            for(unsigned m = 0; m < in.ResourcesCount(); m++)
            {
                if(in.IsCompatible(t,m))
                {
                    selected_machine = m;
                    break;
                }
            }
        }
        out.Assign(t, selected_machine, current_period);
        machine_load[selected_machine] += s_t;
    }
}