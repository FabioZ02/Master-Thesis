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
    os << "MAt = [";
    for(unsigned t = 0; t < in.OrdersCount(); t++)
    {
        os << assigned_resource[t];
        if(t < in.OrdersCount() - 1) os << ", ";
    }
    os << "]" << std::endl;

    os << "PAt = [";
    for(unsigned t = 0; t < in.OrdersCount(); t++)
    {
        os << assigned_period[t];
        if(t < in.OrdersCount() - 1) os << ", ";
    }
    os << "]" << std::endl;

    os << "MSp,m = [|";
    for(unsigned p = 0; p <= last_period; p++)
    {
        for(unsigned r = 0; r < in.ResourcesCount(); r++)
        {
            os << load[r][p];
            if(r < in.ResourcesCount() - 1) os << ", ";
        }
        os << (IsRemainderPeriod(p) ? "| (R)" : "|") << std::endl;
    }
    os << "]" << std::endl;
}

std::ostream& operator<<(std::ostream& os, const BT_Output& out)
{
    os << "[";
    for(unsigned t = 0; t < out.in.OrdersCount(); t++)
    {
        os << out.assigned_resource[t];
        if(t < out.in.OrdersCount() - 1) os << ", ";
    }
    os << "]" << std::endl;

    os << "[";
    for(unsigned t = 0; t < out.in.OrdersCount(); t++)
    {
        os << out.assigned_period[t];
        if(t < out.in.OrdersCount() - 1) os << ", ";
    }
    os << "]" << std::endl;

    return os;
}

std::istream& operator>>(std::istream& is, BT_Output& out)
{
    unsigned r, p;
    char ch;

    out.Reset();

    is >> ch; // '['
    for(unsigned t = 0; t < out.in.OrdersCount(); t++)
    {
        is >> r >> ch;
        out.assigned_resource[t] = r;
    }

    is >> ch; // '['
    for(unsigned t = 0; t < out.in.OrdersCount(); t++)
    {
        is >> p >> ch;
        out.assigned_period[t] = p;
    }

    for(unsigned t = 0; t < out.in.OrdersCount(); t++)
        out.Assign(t, out.assigned_resource[t], out.assigned_period[t]);

    return is;
}

bool operator==(const BT_Output& out1, const BT_Output& out2)
{
    for(unsigned t = 0; t < out1.in.OrdersCount(); t++)
        if(out1.assigned_resource[t] != out2.assigned_resource[t] ||
           out1.assigned_period[t]   != out2.assigned_period[t])
            return false;
    return true;
}