#include "BT_Helpers.hh"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <climits>


BT_SolutionManager::BT_SolutionManager(const BT_Input& pin)
    : SolutionManager<BT_Input, BT_Output, DefaultCostStructure<long long>>(pin, "BT_SolutionManager") {}

/* Greedy Algorithm
 Input:  sorted list of tasks T (by priority desc), machines M, compatibility matrix V, Smax
 Output: period assignments PAt, machine assignments MAt
*/
void BT_SolutionManager::GreedyState(BT_Output& out)
{
    out.Reset();

    // 1. Sort tasks by descending priority
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

        // 2. Find the compatible machine with the absolute MINIMUM load in the current period
        unsigned selected_machine = in.ResourcesCount();
        unsigned min_load = UINT_MAX;
        
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
        {
            if(in.IsCompatible(t, m) && machine_load[m] < min_load)
            {
                min_load = machine_load[m];
                selected_machine = m;
            }
        }

        // Safety check: if a task has no compatible machines defined at all
        if(selected_machine == in.ResourcesCount())
        {
            std::cerr << "GreedyState: no compatible machine for task " << t << "\n";
            exit(1);
        }

        // 3. If the machine with the minimum load does NOT have enough space, open a new period
        if(machine_load[selected_machine] + s_t > S_max)
        {
            current_period++;
            std::fill(machine_load.begin(), machine_load.end(), 0);
            
            // In the new period (all machines at 0 load), simply pick the first compatible machine
            selected_machine = in.ResourcesCount();
            for(unsigned m = 0; m < in.ResourcesCount(); m++)
            {
                if(in.IsCompatible(t, m))
                {
                    selected_machine = m;
                    break;
                }
            }
        }

        // 4. Assign the task and update the load
        out.Assign(t, selected_machine, current_period);
        machine_load[selected_machine] += s_t;
    }
}

void BT_SolutionManager::RandomState(BT_Output& out)
{
    // RandomState is the actual SA/HC entry point in EasyLocal.
    // Prefer the greedy construction (Algorithm 3.1): it packs to Smax and gives
    // a compact, ~minimal period count. On loose instances this is the right start.
    GreedyState(out);

    // On tight-compatibility instances the greedy can over-fragment, producing
    // MORE periods than the theoretical bound UpperBoundPeriods (= floor(sumQ /
    // (Smin*M)) + 1). Starting SA from such an over-fragmented, thin layout is
    // worse than a bounded random one (SA cannot cleanly collapse the extra
    // period). In that case fall back to a random layout that respects the bound.
    if (out.LastPeriod() + 1 > in.UpperBoundPeriods())
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
        long long sum_prio = 0;
        unsigned count    = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(out.IsAssigned(t) && out.AssignedPeriod(t) == p)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count == 0) continue;

        long long avg = ( static_cast<long long>(sum_prio) + count - 1) / count;
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
        long long sum_prio = 0;
        unsigned count    = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(out.IsAssigned(t) && out.AssignedPeriod(t) == p)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count == 0) continue;

        long long avg = ( static_cast<long long>(sum_prio) + count - 1) / count;
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

long long BT_MaxLoadPenalty::ComputeCost(const BT_Output& out) const
{
    long long penalty = 0;
    long long S_max   = in.OrderType_MaxGroupSize(in.Order_TypeId(0) - 1);
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
    {
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
            penalty += std::max(0LL, static_cast<long long>(out.Load(m, p)) - S_max);
    }
    return penalty;
}

void BT_MaxLoadPenalty::PrintViolations(const BT_Output& out, std::ostream& os) const
{
    long long S_max = in.OrderType_MaxGroupSize(in.Order_TypeId(0) - 1);
    for(unsigned p = 0; p <= out.LastPeriod(); p++)
    {
        for(unsigned m = 0; m < in.ResourcesCount(); m++)
        {
            long long viol = static_cast<long long>(out.Load(m, p)) - S_max;
            if(viol > 0)
                os << "Period " << p << ", Machine " << m
                   << ": Smax violation = " << viol << "\n";
        }
    }
}


/*****************************************************************************
  * BT_Shift Neighborhood Methods
*****************************************************************************/

BT_Shift::BT_Shift()
{
    task =0;
    new_period = 0;
    new_machine = 0;
    old_period = 0;
    old_machine = 0;
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

/*****************************************************************************
  * BT_Shift Neighborhood Explorer
*****************************************************************************/
void BT_ShiftNeighborhoodExplorer::RandomMove(const BT_Output& st, BT_Shift& mv) const
{
    mv.task = Random::Uniform<unsigned>(0, in.OrdersCount() - 1);
    mv.old_period = st.AssignedPeriod(mv.task);
    mv.old_machine = st.AssignedResource(mv.task);

    // Nagler's shift adds at most ONE new period: clamp new_period to [0, LastPeriod+1]
    // (and never beyond the theoretical upper bound). This avoids creating gaps.
    unsigned max_period = st.LastPeriod() + 1;
    if (max_period > in.UpperBoundPeriods() - 1)
        max_period = in.UpperBoundPeriods() - 1;

    do{
        mv.new_period = Random::Uniform<unsigned>(0, max_period);
        const auto& valid = in.Order_ValidResourceIds(mv.task);
        unsigned id = Random::Uniform<unsigned>(0, valid.size() - 1);
        mv.new_machine = valid[id] - 1;
    } while(mv.new_period == mv.old_period && mv.new_machine == mv.old_machine); // a shift move may also only change the assigned machine (or period) without changing the period (or machine).
}

bool BT_ShiftNeighborhoodExplorer::FeasibleMove(const BT_Output& st, const BT_Shift& mv) const
{
    // 1. Valid indices
    if(mv.task >= (int)in.OrdersCount()) return false;
    if(mv.new_machine >= (int)in.ResourcesCount()) return false;
    if(mv.new_period >= (int)in.UpperBoundPeriods()) return false;

    // 2. Null move — neither machine nor period changes
    if(mv.new_machine == mv.old_machine && mv.new_period == mv.old_period)
        return false;

    // 3. Task-machine compatibility
    if(!in.IsCompatible(mv.task, mv.new_machine))
        return false;

    // 4. No gaps — new period cannot skip beyond LastPeriod + 1
    if(mv.new_period > (int)(st.LastPeriod() + 1))
        return false;

    return true;
}

void BT_ShiftNeighborhoodExplorer::MakeMove(BT_Output& st, const BT_Shift& mv) const
{
    unsigned old_period = mv.old_period;

    st.Assign(mv.task, mv.new_machine, mv.new_period);

    // Old period empty? Then remove it
    bool empty = true;
    for(unsigned m = 0; m < in.ResourcesCount(); m++)
        if(st.Load(m, old_period) > 0) { empty = false; break; }

    if(empty)
        st.RemovePeriod(old_period);
}

void BT_ShiftNeighborhoodExplorer::FirstMove(const BT_Output& st, BT_Shift& mv) const
{
    mv.task = 0;
    mv.old_period = st.AssignedPeriod(mv.task);
    mv.old_machine = st.AssignedResource(mv.task);

    mv.new_period = 0;
    const auto& valid = in.Order_ValidResourceIds(mv.task);
    mv.new_machine = valid[0] - 1;

    if(mv.new_period == mv.old_period && mv.new_machine == mv.old_machine)
    {
        NextMove(st,mv);
    }
}

bool BT_ShiftNeighborhoodExplorer::NextMove(const BT_Output& st, BT_Shift& mv) const
{
    do
      if(!AnyNextMove(st, mv))
        return false;
    while(!FeasibleMove(st, mv));
    return true;
}

bool BT_ShiftNeighborhoodExplorer::AnyNextMove(const BT_Output& st, BT_Shift& mv) const 
{
    mv.new_period++;
    
    if(mv.new_period >= in.UpperBoundPeriods())
    {
        mv.new_period = 0;
        mv.new_machine++;

        if(mv.new_machine >= in.ResourcesCount())
        {
            mv.new_machine = 0;
            mv.task++;

            if(mv.task >= in.OrdersCount())
                return false;

            mv.old_period = st.AssignedPeriod(mv.task);
            mv.old_machine = st.AssignedResource(mv.task);
        }
    }
    return true;
}

long long BT_ShiftDeltaLoadDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const 
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    unsigned old_period = mv.old_period;
    after.Assign(mv.task, mv.new_machine, mv.new_period);
    bool empty = true;
    for (unsigned m = 0; m < in.ResourcesCount(); m++)
        if (after.Load(m, old_period) > 0) { empty = false; break; }
    if (empty)
        after.RemovePeriod(old_period);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}

long long BT_ShiftDeltaTargetDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    unsigned old_period = mv.old_period;
    after.Assign(mv.task, mv.new_machine, mv.new_period);
    bool empty = true;
    for (unsigned m = 0; m < in.ResourcesCount(); m++)
        if (after.Load(m, old_period) > 0) { empty = false; break; }
    if (empty)
        after.RemovePeriod(old_period);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}


long long BT_ShiftDeltaPriorityDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    unsigned old_period = mv.old_period;
    after.Assign(mv.task, mv.new_machine, mv.new_period);
    bool empty = true;
    for (unsigned m = 0; m < in.ResourcesCount(); m++)
        if (after.Load(m, old_period) > 0) { empty = false; break; }
    if (empty)
        after.RemovePeriod(old_period);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}

long long BT_ShiftDeltaMinLoadPenalty::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    unsigned old_period = mv.old_period;
    after.Assign(mv.task, mv.new_machine, mv.new_period);
    bool empty = true;
    for (unsigned m = 0; m < in.ResourcesCount(); m++)
        if (after.Load(m, old_period) > 0) { empty = false; break; }
    if (empty)
        after.RemovePeriod(old_period);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}

long long BT_ShiftDeltaMaxLoadPenalty::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    unsigned old_period = mv.old_period;
    after.Assign(mv.task, mv.new_machine, mv.new_period);
    bool empty = true;
    for (unsigned m = 0; m < in.ResourcesCount(); m++)
        if (after.Load(m, old_period) > 0) { empty = false; break; }
    if (empty)
        after.RemovePeriod(old_period);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}
/*****************************************************************************
  * BT_Swap Neighborhood Methods
*****************************************************************************/

BT_Swap::BT_Swap()
{
    task1 = 0;
    task2 = 0;
    old_period1 = 0;
    old_machine1 = 0;
    old_period2 = 0;
    old_machine2 = 0;
}

bool operator==(const BT_Swap& mv1, const BT_Swap& mv2)
{
    return (mv1.task1 == mv2.task1 && mv1.task2 == mv2.task2)||(mv1.task1 == mv2.task2 && mv1.task2 == mv2.task1);
}

bool operator!=(const BT_Swap& mv1, const BT_Swap& mv2)
{
    return !(mv1 == mv2);
}

bool operator<(const BT_Swap& mv1, const BT_Swap& mv2)
{
    unsigned a1,a2;
    if(mv1.task1 < mv1.task2) { a1 = mv1.task1; a2 = mv1.task2; }
    else                      { a1 = mv1.task2; a2 = mv1.task1; }
    
    unsigned b1, b2; 
    if(mv2.task1 < mv2.task2) { b1 = mv2.task1; b2 = mv2.task2; }
    else                      { b1 = mv2.task2; b2 = mv2.task1; } 
    
    if (a1 != b1) return a1 < b1;
    return a2 < b2;
}

std::istream& operator>>(std::istream& is, BT_Swap& mv)
{
    char ch;
    is >> mv.task1 >> ch >> mv.task2;
    return is;
}

std::ostream& operator<<(std::ostream& os, const BT_Swap& mv)
{
    os << "Swap(t1="<< mv.task1 << ", t2=" << mv.task2 << ")";
    return os;
}

/*****************************************************************************
  * BT_Swap Neighborhood Explorer
*****************************************************************************/

void BT_SwapNeighborhoodExplorer::RandomMove(const BT_Output& st, BT_Swap& mv) const
{
    // NOTE: EasyLocal's SA applies RandomMove via MakeMove WITHOUT calling
    // FeasibleMove, so compatibility MUST be enforced here. A swap exchanges the
    // machines of task1 and task2, hence it must keep BOTH assignments compatible:
    //   - task1 compatible with task2's machine
    //   - task2 compatible with task1's machine
    const unsigned n = in.OrdersCount();

    // Try several random task1 until one has at least one compatible swap partner.
    for (unsigned outer = 0; outer < n; ++outer)
    {
        mv.task1        = Random::Uniform<unsigned>(0, n - 1);
        mv.old_period1  = st.AssignedPeriod(mv.task1);
        mv.old_machine1 = st.AssignedResource(mv.task1);

        std::vector<unsigned> cand;
        for (unsigned t2 = 0; t2 < n; ++t2)
        {
            if (t2 == (unsigned)mv.task1) continue;
            int m2 = st.AssignedResource(t2);
            int p2 = st.AssignedPeriod(t2);
            // skip null swaps (same machine AND same period)
            if (m2 == mv.old_machine1 && p2 == mv.old_period1) continue;
            // both directions must remain compatible after the swap
            if (in.IsCompatible(mv.task1, m2) && in.IsCompatible(t2, mv.old_machine1))
                cand.push_back(t2);
        }

        if (!cand.empty())
        {
            mv.task2        = cand[Random::Uniform<unsigned>(0, cand.size() - 1)];
            mv.old_period2  = st.AssignedPeriod(mv.task2);
            mv.old_machine2 = st.AssignedResource(mv.task2);
            return;
        }
    }

    // Degenerate fallback: no compatible swap exists anywhere -> emit a no-op
    // (task2 == task1 makes MakeMove idempotent; SA simply sees delta 0).
    mv.task2        = mv.task1;
    mv.old_period2  = mv.old_period1;
    mv.old_machine2 = mv.old_machine1;
}

bool BT_SwapNeighborhoodExplorer::FeasibleMove(const BT_Output& st, const BT_Swap& mv) const
{
    // 1. Task must be different
    if(mv.task1 == mv.task2) return false;

    // 2. Not assigned to same machine and same period
    if(mv.old_machine1 == mv.old_machine2 && mv.old_period1 == mv.old_period2)
        return false;

    // 3. Task-machine compatibility
    if(!in.IsCompatible(mv.task1, mv.old_machine2)) return false;
    if(!in.IsCompatible(mv.task2, mv.old_machine1)) return false;

    return true;
}

void BT_SwapNeighborhoodExplorer::MakeMove(BT_Output& st, const BT_Swap& mv) const
{
    st.Assign(mv.task1, mv.old_machine2, mv.old_period2);
    st.Assign(mv.task2, mv.old_machine1, mv.old_period1);
}

void BT_SwapNeighborhoodExplorer::FirstMove(const BT_Output& st, BT_Swap& mv) const
{
    mv.task1 = 0;
    mv.old_period1  = st.AssignedPeriod(mv.task1);
    mv.old_machine1 = st.AssignedResource(mv.task1);

    mv.task2 = 0;

    while(mv.task1 == mv.task2)
    {
        mv.task2++;
    }

    mv.old_period2  = st.AssignedPeriod(mv.task2);
    mv.old_machine2 = st.AssignedResource(mv.task2);

    if(mv.task2 >= in.OrdersCount())
    {
        NextMove(st,mv);
    }
}

bool BT_SwapNeighborhoodExplorer::NextMove(const BT_Output& st, BT_Swap& mv) const
{
    do
      if(!AnyNextMove(st,mv))
        return false;
    while(!FeasibleMove(st,mv));
    return true;
}

bool BT_SwapNeighborhoodExplorer::AnyNextMove(const BT_Output& st, BT_Swap& mv) const
{
    mv.task2++;

    if(mv.task2 == mv.task1)
      mv.task2++;

    if(mv.task2 >= in.OrdersCount())
    {
        mv.task1++;

        if(mv.task1 >= in.OrdersCount())
            return false;

        mv.old_period1 = st.AssignedPeriod(mv.task1);
        mv.old_machine1 = st.AssignedResource(mv.task1);

        mv.task2 = 0;
        mv.old_period2 = st.AssignedPeriod(mv.task2);
        mv.old_machine2 = st.AssignedResource(mv.task2);

        if(mv.task1 == mv.task2)
            mv.task2++;

        if(mv.task2 >= in.OrdersCount())
            return false;
    }
    mv.old_period2 = st.AssignedPeriod(mv.task2);
    mv.old_machine2 = st.AssignedResource(mv.task2);
    
    return true;
}


long long BT_SwapDeltaLoadDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    after.Assign(mv.task1, mv.old_machine2, mv.old_period2);
    after.Assign(mv.task2, mv.old_machine1, mv.old_period1);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}

long long BT_SwapDeltaTargetDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    after.Assign(mv.task1, mv.old_machine2, mv.old_period2);
    after.Assign(mv.task2, mv.old_machine1, mv.old_period1);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}

long long BT_SwapDeltaPriorityDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    after.Assign(mv.task1, mv.old_machine2, mv.old_period2);
    after.Assign(mv.task2, mv.old_machine1, mv.old_period1);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}

long long BT_SwapDeltaMinLoadPenalty::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    after.Assign(mv.task1, mv.old_machine2, mv.old_period2);
    after.Assign(mv.task2, mv.old_machine1, mv.old_period1);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}

long long BT_SwapDeltaMaxLoadPenalty::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    // EXACT delta (drift-free): replica MakeMove su una copia e differenzia il costo pieno.
    BT_Output after = st;
    after.Assign(mv.task1, mv.old_machine2, mv.old_period2);
    after.Assign(mv.task2, mv.old_machine1, mv.old_period1);
    return cc.ComputeCost(after) - cc.ComputeCost(st);
}