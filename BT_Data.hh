/*
This class class will containt, following the EasyLocal++ workflow Input & Output classe.
*/

#ifndef BT_DATA_HH
#define BT_DATA_HH
#include <iostream>
#include <vector>
#include <cstdlib>
#include <string>

using namespace std;

struct OrderTypes 
{
    unsigned id;
    unsigned target_group_size;
    unsigned min_group_size;
    unsigned max_group_size;
};

struct Task 
{
    unsigned id;
    unsigned load;
    unsigned priority;
    vector<unsigned> compatible_machines;
};

struct Machine
{
    unsigned id;
};

struct Objectives
{
    string type;
    double weight;
    int priority;
};

class BT_Input
{
        friend ostream& operator<<(ostream& os, const BT_Input& in);
public:
        BT_Input(string file_name);
        
        // This block defines the periods' filling constraints (ID, S_tar, S_min, S_max) 
        unsigned OrderTypeId()     const { return order_type_id;     }
        unsigned TargetGroupSize() const { return target_group_size; }
        unsigned MinGroupSize()    const { return min_group_size;    }
        unsigned MaxGroupSize()    const { return max_group_size;    }

        // This block defines the tasks
        unsigned Tasks()                                             const { return task.size();                 }
        unsigned Task_ID(unsigned i)                                 const { return task[i].id;                  }
        unsigned Task_Load(unsigned i)                               const { return task[i].load;                }
        unsigned Task_Priority(unsigned i)                           const { return task[i].priority;            }
        const vector<unsigned>& Task_Compatible_Machines(unsigned i) const { return task[i].compatible_machines; }

        // This block defines the machines
        unsigned Machines()              const { return machine.size(); }
        unsigned Machine_ID(unsigned i)  const { return machine[i].id;  }

        // Methods for Objectives (Access by index i)
        unsigned ObjectivesCount()       const { return objective.size();   }
        string Objective_Type(unsigned i) const { return objective[i].type;   }
        double Weight(unsigned i)         const { return objective[i].weight; }
        int Priority(unsigned i)       const { return objective[i].priority; }
    
private:
        unsigned order_type_id, target_group_size, min_group_size, max_group_size;
        vector<Task> task;
        vector<Machine> machine;
        vector<Objectives> objective;
};

#endif