#include "BT_Data.hh"
#include <fstream>
#include <algorithm>
#include <climits>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

BT_Input::BT_Input(const std::string& file_name)
{
    std::ifstream is(file_name);
    if(!is)
    {
        std::cerr << "Cannot open input file " << file_name << std::endl;
        exit(1);
    }

    json j;
    is >> j;

    // Parse OrderTypes
    for(const auto& ot : j["OrderTypes"])
    {
        unsigned target_group_size = ot["TargetGroupSize"];
        unsigned min_group_size    = ot["MinGroupSize"];
        unsigned max_group_size    = ot["MaxGroupSize"];
        order_types.push_back({target_group_size, min_group_size, max_group_size});
    }

    // Parse Orders (Tasks)
    for(const auto& o : j["Orders"])
    {
        unsigned type_id  = o["TypeId"];
        unsigned quantity = o["Quantity"];
        unsigned priority = o["Priority"];
        std::vector<unsigned> valid_resource_ids = o["ValidResourceIds"].get<std::vector<unsigned>>();
        orders.push_back({type_id, quantity, priority, valid_resource_ids});
    }

    // Parse Resources (Machines)
    for(const auto& r : j["Resources"])
        resource_ids.push_back(r["ResourceId"].get<unsigned>());

    // Parse Objectives
    for(const auto& obj : j["Objectives"])
    {
        std::string type = obj["Type"];
        double weight    = obj["Weight"];
        int priority     = obj["Priority"];
        objectives.push_back({type, weight, priority});
    }

    // Parse ranking mode and max run time
    ranking_mode    = j["RankingMode"].get<std::string>();
    max_run_time_ms = j["Parameters"]["MaxRunTimeMS"];

    // Compute bigM
    int sum_size = 0;
    for(const auto& o : orders)
        sum_size += o.quantity;

    int max1 = sum_size * (static_cast<int>(resource_ids.size()) - 1);

    int max2 = orders.size() * resource_ids.size() *
               std::max(order_types[0].target_group_size,
                        order_types[0].max_group_size - order_types[0].target_group_size);

    unsigned min_priority = UINT_MAX;
    for(const Order& o : orders)
        if(o.priority < min_priority)
            min_priority = o.priority;

    unsigned max_priority = 0;
    for(const Order& o : orders)
        if(o.priority > max_priority)
            max_priority = o.priority;

    int max3 = orders.size() * (max_priority - min_priority);

    bigM = max1 + max2 + max3 + 1;

    // Build compatibility matrix
    compatibility_matrix.assign(orders.size(),
                                std::vector<bool>(resource_ids.size(), false));
    for(unsigned i = 0; i < orders.size(); i++)
        for(unsigned id : orders[i].valid_resource_ids)
            compatibility_matrix[i][id - 1] = true;
}

BT_Output::BT_Output(const BT_Input& my_in)
  : in(my_in), assigned_resource(in.OrdersCount(), -1), assigned_period(in.OrdersCount(), -1), load(in.ResourcesCount())
{}

BT_Output& BT_Output::operator=(const BT_Output& out)
{
    assigned_resource = out.assigned_resource;
    assigned_period   = out.assigned_period;
    load              = out.load;
    return *this;
}

void BT_Output::Assign(unsigned t, unsigned r, unsigned p)
{
    // Task t was already assigned
    if(IsAssigned(t))
    {
        unsigned old_r = assigned_resource[t];
        unsigned old_p = assigned_period[t];

        load[old_r][old_p] -= in.Order_Quantity(t);
    }

    // Otherwise
    assigned_resource[t] = r;
    assigned_period[t]   = p;

    load[r][p] += in.Order_Quantity(t);

}

void BT_Output::Reset()
{
    unsigned t, r, p;

    for(t =0; t < assigned_resource.size(); t++)
    {
        assigned_resource[t] = -1;
    }

     for(t =0; t < assigned_period.size(); t++)
    {
        assigned_period[t] = -1;
    }

    for(r = 0; r < in.ResourcesCount(); r++)
    {
        for(p = 0; p < load[r].size(); p++)
        {
            load[r][p] = 0;
        }   
    }
}

void BT_Output::Dump(std::ostream& os) const
{
    os << "=== Task Assignment ===" << std::endl;
    for (unsigned t = 0; t < assigned_resource.size(); t++)
    {
        os << "Task " << t << " -> ";
        if (IsAssigned(t)) {
            os << "Machine " << assigned_resource[t] 
               << ", Period " << assigned_period[t] << std::endl;
        } else {
            os << "Not Assigned" << std::endl;
        }
    }

    os << "\n=== Load ===" << std::endl;
    for (unsigned r = 0; r < load.size(); r++)
    {
        for (unsigned p = 0; p < load[r].size(); p++)
        {
            if (load[r][p] > 0)
            {
                os << "Period " << p << " - Machine " << r 
                   << " : Load = " << load[r][p] << std::endl;
            }
        }
    }
}

std::ostream& operator<<(std::ostream& os, const BT_Output& out)
{
    out.Dump(os);
    return os;
}

