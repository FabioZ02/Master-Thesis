#ifndef BT_INPUT_HH
#define BT_INPUT_HH

#include <iostream>
#include <vector>
#include <string>


struct OrderType
{
    unsigned target_group_size;
    unsigned min_group_size;
    unsigned max_group_size;
};

struct Order
{
    unsigned type_id;
    unsigned quantity;
    unsigned priority;
    std::vector<unsigned> valid_resource_ids;
};

struct Objective
{
    std::string type;
    double weight;
    int priority;
};

class BT_Input
{
    friend std::ostream& operator<<(std::ostream& os, const BT_Input& in);
public:
    explicit BT_Input(const std::string& file_name);

    // OrderTypes
    unsigned OrderTypesCount()                      const { return order_types.size();                    }
    unsigned OrderType_TargetGroupSize(unsigned i)  const { return order_types[i].target_group_size;      }
    unsigned OrderType_MinGroupSize(unsigned i)     const { return order_types[i].min_group_size;         }
    unsigned OrderType_MaxGroupSize(unsigned i)     const { return order_types[i].max_group_size;         }

    // Orders
    unsigned OrdersCount()                                          const { return orders.size();                         }
    unsigned Order_TypeId(unsigned i)                               const { return orders[i].type_id;                     }
    unsigned Order_Quantity(unsigned i)                             const { return orders[i].quantity;                    }
    unsigned Order_Priority(unsigned i)                             const { return orders[i].priority;                    }
    const std::vector<unsigned>& Order_ValidResourceIds(unsigned i) const { return orders[i].valid_resource_ids;          }

    // Resources
    unsigned ResourcesCount()             const { return resource_ids.size(); }
    unsigned Resource_ID(unsigned i)      const { return resource_ids[i];     }

    // Objectives
    unsigned ObjectivesCount()                 const { return objectives.size();        }
    std::string Objective_Type(unsigned i)     const { return objectives[i].type;       }
    double      Objective_Weight(unsigned i)   const { return objectives[i].weight;     }
    int         Objective_Priority(unsigned i) const { return objectives[i].priority;   }

    // Global parameters
    const std::string& RankingMode()  const { return ranking_mode;    }
    unsigned MaxRunTimeMS()           const { return max_run_time_ms; }

    // Methods and structures built in the input class to help the solver
    unsigned ComputeBigM()                                         const { return bigM;                                        }
    const std::vector<std::vector<bool>>& CompatibilityMatrix()    const { return compatibility_matrix;                        }
    bool IsCompatible(unsigned order_idx, unsigned resource_idx)   const { return compatibility_matrix[order_idx][resource_idx]; }

private:
    std::vector<OrderType>  order_types;
    std::vector<Order>      orders;
    std::vector<unsigned>   resource_ids;
    std::vector<Objective>  objectives;

    std::string ranking_mode;
    unsigned    max_run_time_ms = 0;

    unsigned bigM = 0;
    std::vector<std::vector<bool>> compatibility_matrix;
};

class BT_Output
{
    friend std::ostream& operator<<(std::ostream& os, const BT_Output& out);
    friend std::istream& operator>>(std::istream& is, BT_Output& out);
    friend bool operator==(const BT_Output& out1, const BT_Output& out2);
public:
    BT_Output(const BT_Input& i);
    BT_Output& operator=(const BT_Output& out);

    void Assign(unsigned t, unsigned r, unsigned period);
    bool IsAssigned(unsigned t)                const;
    unsigned AssignedResource(unsigned t)      const { return assigned_resource[t]; }
    unsigned AssignedPeriod(unsigned t)        const { return assigned_period[t];   }

    unsigned Load(unsigned r, unsigned period) const { return load[r][period]; }

    void Reset();
    void Dump(std::ostream& os) const;

private:
    const BT_Input& in;
    std::vector<int> assigned_resource; // assigned_resource[t] = r if t is assigned to resource r
    std::vector<int> assigned_period;   // assigned_period[t] = p if t is assigned to period p
    std::vector<std::vector<unsigned>> load; // load[r][p] = load of machine r in period p
};

#endif
