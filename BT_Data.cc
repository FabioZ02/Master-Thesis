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

    for(const auto& ot : j["OrderTypes"])
    {
        unsigned target_group_size = ot["TargetGroupSize"];
        unsigned min_group_size    = ot["MinGroupSize"];
        unsigned max_group_size    = ot["MaxGroupSize"];
        order_types.push_back({target_group_size, min_group_size, max_group_size});
    }

    for(const auto& o : j["Orders"])
    {
        unsigned type_id  = o["TypeId"];
        unsigned quantity = o["Quantity"];
        unsigned priority = o["Priority"];
        std::vector<unsigned> valid_resource_ids = o["ValidResourceIds"].get<std::vector<unsigned>>();
        orders.push_back({type_id, quantity, priority, valid_resource_ids});
    }

    for(const auto& r : j["Resources"])
        resource_ids.push_back(r["ResourceId"].get<unsigned>());

    for(const auto& obj : j["Objectives"])
    {
        std::string type = obj["Type"];
        double weight    = obj["Weight"];
        int priority     = obj["Priority"];
        objectives.push_back({type, weight, priority});
    }

    ranking_mode    = j["RankingMode"].get<std::string>();
    max_run_time_ms = j["Parameters"]["MaxRunTimeMS"];

    // Upper bound on |P|
    long long sum_size = 0;
    for(const auto& o : orders)
        sum_size += o.quantity;

    upper_bound_periods = sum_size / (order_types[0].min_group_size * resource_ids.size()) + 1;

    // BigM
    long long max1 = sum_size * (static_cast<long long>(resource_ids.size()) - 1);

    long long max2 = (long long)orders.size() * resource_ids.size() *
                     std::max(order_types[0].target_group_size,
                              order_types[0].max_group_size - order_types[0].target_group_size);

    unsigned min_priority = UINT_MAX;
    unsigned max_priority = 0;
    for(const Order& o : orders)
    {
        if(o.priority < min_priority) min_priority = o.priority;
        if(o.priority > max_priority) max_priority = o.priority;
    }

    long long max3 = (long long)orders.size() * (max_priority - min_priority);

    bigM = max1 + max2 + max3 + 1;

    // Compatibility matrix
    compatibility_matrix.assign(orders.size(), std::vector<bool>(resource_ids.size(), false));
    for(unsigned i = 0; i < orders.size(); i++)
        for(unsigned id : orders[i].valid_resource_ids)
            compatibility_matrix[i][id - 1] = true;
}

BT_Output::BT_Output(const BT_Input& my_in)
    : in(my_in),
      assigned_resource(in.OrdersCount(), -1),
      assigned_period(in.OrdersCount(), -1),
      load(in.ResourcesCount()),
      last_period(0)
{}

BT_Output& BT_Output::operator=(const BT_Output& out)
{
    assigned_resource = out.assigned_resource;
    assigned_period   = out.assigned_period;
    load              = out.load;
    last_period       = out.last_period;
    return *this;
}

void BT_Output::Assign(unsigned t, unsigned r, unsigned p)
{
    if(IsAssigned(t))
    {
        unsigned old_r = assigned_resource[t];
        unsigned old_p = assigned_period[t];
        load[old_r][old_p] -= in.Order_Quantity(t);
    }

    assigned_resource[t] = r;
    assigned_period[t]   = p;

    for(unsigned m = 0; m < in.ResourcesCount(); m++)
        if(load[m].size() <= p)
            load[m].resize(p + 1, 0);

    load[r][p] += in.Order_Quantity(t);

    if(p > last_period)
        last_period = p;
}

void BT_Output::RemovePeriod(unsigned p)
{
    // Shift left the indexes of the assigned periods greater than p
    for(unsigned t = 0; t < in.OrdersCount(); t++)
        if(assigned_period[t] > (int)p)
            assigned_period[t]--;

    // Remove column p from the load matrix
    for(unsigned m = 0; m < in.ResourcesCount(); m++)
        load[m].erase(load[m].begin() + p);

    if(last_period > 0)
        last_period--;
}

void BT_Output::Reset()
{
    for(unsigned t = 0; t < in.OrdersCount(); t++)
    {
        assigned_resource[t] = -1;
        assigned_period[t]   = -1;
    }
    for(unsigned r = 0; r < in.ResourcesCount(); r++)
        load[r].clear();

    last_period = 0;
}

void BT_Output::Dump(std::ostream& os) const
{
    // Global parameters
    unsigned type_idx = in.Order_TypeId(0) - 1;
    unsigned S_min = in.OrderType_MinGroupSize(type_idx);
    unsigned S_max = in.OrderType_MaxGroupSize(type_idx);
    unsigned S_tar = in.OrderType_TargetGroupSize(type_idx);

    os << "S_min: " << S_min << "\n";
    os << "S_target: " << S_tar << "\n";
    os << "S_max: " << S_max << "\n\n";

    os << "Load Matrix:\n";
    os << "M \\ P\t";
    for (unsigned p = 0; p <= last_period; p++)
    {
        os << "P" << p << "\t";
    }
    os << "\n";

    // Resources
    for (unsigned m = 0; m < in.ResourcesCount(); m++)
    {
        os << "M" << m << "\t";
        for (unsigned p = 0; p <= last_period; p++)
        {
            os << load[m][p] << "\t";
        }
        os << "\n";
    }
    os << "\n";

    // Tasks
    os << "Task Assignments:\n";
    for (unsigned t = 0; t < in.OrdersCount(); t++)
    {
        if (IsAssigned(t))
        {
            os << "Task " << t << "\t-> M" << assigned_resource[t] << "\tP" << assigned_period[t] << "\n";
        }
        else
        {
            os << "Task " << t << "\t-> Unassigned\n";
        }
    }
    os << "\n";
}

std::ostream& operator<<(std::ostream& os, const BT_Output& out)
{
    out.Dump(os);
    return os;
}

std::istream& operator>>(std::istream& is, BT_Output& out)
{
    out.Reset();
    unsigned t, r, p;
    while (is >> t >> r >> p)
    {
        out.Assign(t, r, p);
    }
    return is;
}

bool operator==(const BT_Output& out1, const BT_Output& out2)
{
    return out1.assigned_resource == out2.assigned_resource &&
           out1.assigned_period == out2.assigned_period;
}

std::ostream& operator<<(std::ostream& os, const BT_Input& in)
{
    os << "=== Parameters ===\n";
    os << "Orders : " << in.OrdersCount() << "\n";
    os << "Resources : " << in.ResourcesCount() << "\n";
    os << "Upper Bound Periods : " << in.UpperBoundPeriods() << "\n";


    return os;
}