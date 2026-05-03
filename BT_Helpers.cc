#include "BT_Helpers.hh"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <climits>


BT_SolutionManager::BT_SOlutionManager(const BT_Input& pin)
    : SolutionManager<BT_Input, BT_Output>(pin, "BT_SolutionManager") {}

/* Greedy Algorithm
 Input:  sorted list of tasks T (by priority desc), machines M, compatibility matrix V, Smax
 Output: period assignments PAt, machine assignments MAt
*/
void BT_SolutionManager::GreedyState(BT_Output& out)
{
    out.Reset();

    // Sort tasks by priority descending
    std::vector<unsigned> sorted_tasks(in.OrdersCount());
    std::iota(sorted_tasks.begin(), sorted_tasks.end(), 0);
    std::sort(sorted_tasks.begin(), sorted_tasks.end(), [&](unsigned a, unsigned b){
        return in.Order_Priority(a) > in.Order_Priority(b);
    });

    std::vector<unsigned> machine_load(in.ResourcesCount(), 0);
    unsigned current_period = 0;

    for(unsigned t : sorted_tasks)
    {
        unsigned S_max = in.OrderType_MaxGroupSize(in.Order_TypeId(t) - 1);
        unsigned s_t   = in.Order_Quantity(t);

        unsigned selected_machine = in.ResourcesCount(); 
        unsigned min_load = UINT_MAX;
        // Traduction of assigning task t to the compatible machine with the least load
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
            if(in.IsCompatible(t, m) && machine_load[m] < min_load)
                { min_load = machine_load[m]; selected_machine = m; }

        if(selected_machine == in.ResourcesCount())
        {
            std::cerr << "GreedyState: no compatible machine for task " << t << "\n";
            exit(1);
        }

        if(S_max - machine_load[selected_machine] < s_t && current_period + 1 < in.UpperBoundPeriods())
        {
            current_period++;

            std::fill(machine_load.begin(), machine_load.end(), 0);

            selected_machine = in.ResourcesCount();
            for(unsigned m = 0; m < in.ResourcesCount(); m++)
                if(in.IsCompatible(t, m)) { selected_machine = m; break; }
        }
        out.Assign(t, selected_machine, current_period);
        machine_load[selected_machine] += s_t;
    }
}

void BT_SolutionManager::RandomState(BT_Output& out)
{
    out.Reset();
    for(unsigned t = 0; t < in.OrdersCount(); t++)
    {
        unsigned p = rand() % in.UpperBoundPeriods();
        const auto& valid = in.Order_ValidResourceIds(t);
        unsigned m = valid[rand() % valid.size()] - 1;
        out.Assign(t, m, p);
    }
}

bool BT_SolutionManager::CheckConsistency(const BT_Output& out) const
{
    for(unsigned t = 0; t < in.OrdersCount(); t++)
    {
        if(!out.IsAssigned(t)) return false;
        if(!in.IsCompatible(t, out.AssignedResource(t))) return false;
    }
    return true;
}

// f1: sum_p sum_m (max_m'(MSp,m') - MSp,m)
long long BT_LoadDeviation::ComputeCost(const BT_Output& out) const
{
    long long cost = 0;
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
    {
        unsigned max_load = 0;
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
            max_load = std::max(max_load, out.Load(m, p));
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
            cost += max_load - out.Load(m, p);
    }
    return cost;
}

void BT_LoadDeviation::PrintViolations(const BT_Output& out, std::ostream& os) const
{
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
    {
        unsigned max_load = 0;
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
            max_load = std::max(max_load, out.Load(m, p));
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
        {
            unsigned dev = max_load - out.Load(m, p);
            if(dev > 0)
                os << "Period " << p << ", Machine " << m
                   << ": load deviation = " << dev << "\n";
        }
    }
}

// f2: sum_p sum_m |S_tar - MSp,m|
long long BT_TargetDeviation::ComputeCost(const BT_Output& out) const
{
    long long cost     = 0;
    long long S_target = in.OrderType_TargetGroupSize(in.Order_TypeId(0) - 1);
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
            cost += std::abs(S_target - static_cast<long long>(out.Load(m, p)));
    return cost;
}

void BT_TargetDeviation::PrintViolations(const BT_Output& out, std::ostream& os) const
{
    long long S_target = in.OrderType_TargetGroupSize(in.Order_TypeId(0) - 1);
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
        {
            long long dev = std::abs(S_target - static_cast<long long>(out.Load(m, p)));
            if(dev > 0)
                os << "Period " << p << ", Machine " << m
                   << ": target deviation = " << dev << "\n";
        }
}


// f3: sum_p sum_t |ceil(avgPrio_p) - prio_t| * [PAt == p]
long long BT_PriorityDeviation::ComputeCost(const BT_Output& out) const
{
    long long cost = 0;
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
    {
        double   sum_prio = 0.0;
        unsigned count    = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(out.IsAssigned(t) && out.AssignedPeriod(t) == p)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count == 0) continue;

        long long avg = static_cast<long long>(std::ceil(sum_prio / count));
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(out.IsAssigned(t) && out.AssignedPeriod(t) == p)
                cost += std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
    }
    return cost;
}

void BT_PriorityDeviation::PrintViolations(const BT_Output& out, std::ostream& os) const
{
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
    {
        double   sum_prio = 0.0;
        unsigned count    = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(out.IsAssigned(t) && out.AssignedPeriod(t) == p)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count == 0) continue;

        long long avg = static_cast<long long>(std::ceil(sum_prio / count));
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(out.IsAssigned(t) && out.AssignedPeriod(t) == p)
            {
                long long dev = std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
                if(dev > 0)
                    os << "Period " << p << ", Task " << t
                       << ": priority deviation = " << dev << "\n";
            }
    }
}


// penalty: sum_p sum_m max(0, Smin - MSp,m)  for p != remainder
long long BT_MinLoadPenalty::ComputeCost(const BT_Output& out) const
{
    long long penalty = 0;
    long long S_min   = in.OrderType_MinGroupSize(in.Order_TypeId(0) - 1);
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
    {
        if(out.IsRemainderPeriod(p)) continue;
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
            penalty += std::max(0LL, S_min - static_cast<long long>(out.Load(m, p)));
    }
    return penalty;
}

void BT_MinLoadPenalty::PrintViolations(const BT_Output& out, std::ostream& os) const
{
    long long S_min = in.OrderType_MinGroupSize(in.Order_TypeId(0) - 1);
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
    {
        if(out.IsRemainderPeriod(p)) continue;
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
        {
            long long viol = S_min - static_cast<long long>(out.Load(m, p));
            if(viol > 0)
                os << "Period " << p << ", Machine " << m
                   << ": Smin violation = " << viol << "\n";
        }
    }
}

/*****************************************************************************
  * BT_Shift Neighborhood Methods
*****************************************************************************/

BT_Shift::BT_Shift()
{
    task =0;
    new_machine = 0;
    new_period = 0;
}

bool operator==(const BT_Shift& mv1, const BT_Shift& mv2)
{
    return mv1.task == mv2.task && mv1.new_machine == mv2.new_machine && mv1.new_period == mv2.new_period;
}

bool operator!=(const BT_Shift& mv1, const BT_Shift& mv2)
{
    return mv1.task != mv2.task || mv1.new_machine != mv2.new_machine || mv1.new_period != mv2.new_period;
}

bool operator<(const BT_Shift& mv1, const BT_Shift& mv2)
{
    if (mv1.task != mv2.task) return mv1.task < mv2.task;
    if (mv1.new_machine != mv2.new_machine) return mv1.new_machine < mv2.new_machine;
    return mv1.new_period < mv2.new_period;
}

std::istream& operator>>(std::istream& is, BT_Shift& mv)
{
    char ch;
    is >> mv.task >> ch >> mv.new_machine >> ch >> mv.new_period;
    return is;
}

std::ostream& operator<<(std::ostream& os, const BT_Shift& mv)
{
    os << "(" << mv.task << "->" << "M" << mv.new_machine << ", P" << mv.new_period << ")";
    return os; 
}