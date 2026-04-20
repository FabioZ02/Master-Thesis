#include "BT_Data.hh"
#include "BT_Helpers.hh"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cerr << "Usage: ./bt_main <instance_file>\n";
        return 1;
    }

    BT_Input  in(argv[1]);
    BT_Output out(in);

    BT_SolutionManager sm(in);
    sm.GreedyState(out);

    BT_LoadDeviation     cc1(in);
    BT_TargetDeviation   cc2(in);
    BT_PriorityDeviation cc3(in);
    BT_MinLoadPenalty    cc4(in);

    long long f1      = cc1.ComputeCost(out);
    long long f2      = cc2.ComputeCost(out);
    long long f3      = cc3.ComputeCost(out);
    long long penalty = cc4.ComputeCost(out);
    long long total   = f1 + f2 + f3 + in.ComputeBigM() * penalty;

    std::cout << "Tasks:    " << in.OrdersCount()   << "\n";
    std::cout << "Machines: " << in.ResourcesCount() << "\n\n";

    std::cout << "MAt (machine assignment) =\n[";
    for(unsigned t = 0; t < in.OrdersCount(); t++)
    {
        std::cout << out.AssignedResource(t);
        if(t < in.OrdersCount() - 1) std::cout << ", ";
    }
    std::cout << "]\n\n";

    std::cout << "PAt (period assignment) =\n[";
    for(unsigned t = 0; t < in.OrdersCount(); t++)
    {
        std::cout << out.AssignedPeriod(t);
        if(t < in.OrdersCount() - 1) std::cout << ", ";
    }
    std::cout << "]\n\n";

    std::cout << "MSp,m (load per period/machine) =\n";
    std::cout << std::setw(7) << " ";
    for(unsigned r = 0; r < in.ResourcesCount(); r++)
        std::cout << std::setw(8) << ("M" + std::to_string(r));
    std::cout << "\n";

    for(unsigned p = 0; p <= out.LastPeriod(); p++)
    {
        std::string label = "P" + std::to_string(p) + (out.IsRemainderPeriod(p) ? "[R]" : "   ");
        std::cout << std::setw(7) << label;
        for(unsigned r = 0; r < in.ResourcesCount(); r++)
            std::cout << std::setw(8) << out.Load(r, p);
        std::cout << "\n";
    }
    std::cout << "\n";

    std::cout << "Periods used:            " << out.LastPeriod() + 1 << "\n";
    std::cout << "Upper bound periods:     " << in.UpperBoundPeriods() << "\n";
    std::cout << "f1 (load deviation):     " << f1      << "\n";
    std::cout << "f2 (target deviation):   " << f2      << "\n";
    std::cout << "f3 (priority deviation): " << f3      << "\n";
    std::cout << "penalty (Smin viol.):    " << penalty << "\n";
    std::cout << "Cost: "                    << total   << "\n";

    return 0;
}