#include "BT_Data.hh"
#include <fstream>
#include <algorithm>
#include <vector
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

BT_Input::BT_Input(std::string file_name)
{
    std::ifstream is(file_name);
    if(!is)
    {
        std::cerr <<"Cannot open input file " << file_name << std::endl;
        exit(1);
    }

    json j;
    is >> j;

    // Parse OrderTypes
    for(const OrderType& ot : j["OrderTypes"])
    {
        unsigned id = ot["Id"];
        unsigned target_group_size = ot["TargetGroupSize"];
        unsigned min_group_size = ot["MinGroupSize"];
        unsigned max_group_size = ot["MaxGroupSize"];
        order_types.push_back({id, target_group_size, min_group_size, max_group_size});
    }

    // Parse Orders (Tasks)
    for(const Order& o : j["Orders"])
    {
        unsigned id = o["Id"];
        unsigned type_id = o["TypeId"];
        unsigned quantity = o["Quantity"];
        unsigned priority = o["Priority"];
        vector<unsigned> valid_resource_ids = o["ValidResourceIds"].get<vector<unsigned>>();
        orders.push_back({id, type_id, quantity, priority, valid_resource_ids});
    }

    // Parse Resources (Machines)
    for(const Resource& r : j["Resources"])
    {
        unsigned id = r["Id"];
        resources.push_back({id});
    }

    // Parse Objectives
    for(const Objective& obj : j["Objectives"])
    {
      std::string type = obj["Type"];
      double weight = obj["Weight"];
      int priority = obj["Priority"];
      objectives.push_back({type, weight, priority});  
    }

    // Parse Ranking mode and max run time
    ranking_mode = j["RankingMode"];
    max_run_time_ms = j["MaxRunTimeMs"];

    // Compute bigM
    bigM = 0;
    int sum_size = 0;
    int max1 = 0;
    int max2 = 0;
    int max3 = 0;
    
    for(const Order& o : orders)
    {
        sum_size += o.quantity;
    }

    max1 = sum_size * (ResourcesCount() - 1);
    max2 = OrdersCount() * ResourcesCount() * std::max(target_group_size, (max_group_size-target_group_size));
    
    int min_priority = 8000;
    for(const Orders& o : orders)
    {
        if(o.priority < min_priority) 
        {
            min_priority = o.priority
        }
        return min_priority;
    }

    int max_priority = 0;
    for(const Orders& o : orders) {
        if(o.priority > max_priority)
        {
            max.priority = o.priority;
        }
        return max_priority;
    }

    max3 = OrdersCount() * (max_priority - min_priority);


    bigM = max1 + max2 + max3 + 1;

    // Build compatibility matrix
    



}