#include "BT_Data.hh"
#include <iostream>
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const BT_Input& in)
{
    os << "=== BT_Input Summary ===\n";
    os << "OrderTypes: " << in.OrderTypesCount() << "\n";
    for(unsigned i = 0; i < in.OrderTypesCount(); i++)
        os << "  [" << i << "] target=" << in.OrderType_TargetGroupSize(i)
           << " min=" << in.OrderType_MinGroupSize(i)
           << " max=" << in.OrderType_MaxGroupSize(i) << "\n";

    os << "\nOrders: " << in.OrdersCount() << "\n";
    for(unsigned i = 0; i < in.OrdersCount(); i++)
        os << "  [" << std::setw(3) << i << "] typeId=" << in.Order_TypeId(i)
           << " qty=" << std::setw(3) << in.Order_Quantity(i)
           << " priority=" << in.Order_Priority(i) << "\n";

    os << "\nResources: " << in.ResourcesCount() << "\n";
    for(unsigned i = 0; i < in.ResourcesCount(); i++)
        os << "  [" << i << "] id=" << in.Resource_ID(i) << "\n";

    os << "\nObjectives: " << in.ObjectivesCount() << "\n";
    for(unsigned i = 0; i < in.ObjectivesCount(); i++)
        os << "  [" << i << "] type=" << in.Objective_Type(i)
           << " weight=" << in.Objective_Weight(i)
           << " priority=" << in.Objective_Priority(i) << "\n";

    os << "\nRankingMode:  " << in.RankingMode()   << "\n";
    os << "MaxRunTimeMS: " << in.MaxRunTimeMS()    << "\n";
    os << "BigM:         " << in.ComputeBigM()      << "\n";

    return os;
}

void PrintCompatibilityMatrix(const BT_Input& in)
{
    unsigned O = in.OrdersCount();
    unsigned R = in.ResourcesCount();

    std::cout << "\n=== Compatibility Matrix (" << O << " orders x " << R << " resources) ===\n";

    // Header
    std::cout << "      ";
    for(unsigned r = 0; r < R; r++)
        std::cout << std::setw(3) << ("R" + std::to_string(r));
    std::cout << "\n";

    // Rows
    for(unsigned o = 0; o < O; o++)
    {
        std::cout << "O" << std::setw(3) << o << "  ";
        for(unsigned r = 0; r < R; r++)
            std::cout << std::setw(3) << (in.IsCompatible(o, r) ? "1" : "0");
        std::cout << "\n";
    }
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cerr << "Usage: ./test_bt <instance_file>\n";
        return 1;
    }

    BT_Input in(argv[1]);
    std::cout << in;
    PrintCompatibilityMatrix(in);

    return 0;
}