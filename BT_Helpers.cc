#include "BT_Helpers.hh"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <climits>


BT_SolutionManager::BT_SolutionManager(const BT_Input& pin)
    : SolutionManager<BT_Input, BT_Output>(pin, "BT_SolutionManager") {}

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
int BT_LoadDeviation::ComputeCost(const BT_Output& out) const
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
int BT_TargetDeviation::ComputeCost(const BT_Output& out) const
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
int BT_PriorityDeviation::ComputeCost(const BT_Output& out) const
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
int BT_MinLoadPenalty::ComputeCost(const BT_Output& out) const
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

int BT_MaxLoadPenalty::ComputeCost(const BT_Output& out) const
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

    do{
        mv.new_period = Random::Uniform<unsigned>(0, in.UpperBoundPeriods() - 1);
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

int BT_ShiftDeltaLoadDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const 
{
    long long delta = 0;
    unsigned s_t = in.Order_Quantity(mv.task);
    unsigned m_count = in.ResourcesCount();

    if (mv.old_period == mv.new_period) 
    {
        // Case 1: Shift within the same period
        unsigned p = mv.old_period;
        unsigned old_max = 0;
        unsigned new_max = 0;
        
        for (unsigned m = 0; m < m_count; m++) 
        {
            unsigned load = st.Load(m, p);
            old_max = std::max(old_max, load);
            
            if (m == mv.old_machine) load -= s_t;
            else if (m == mv.new_machine) load += s_t;
            
            new_max = std::max(new_max, load);
        }
        
        delta = static_cast<long long>(new_max - old_max) * m_count;
    } 
    else 
    {
        // Case: Shift between different periods
        // Old period
        unsigned p0 = mv.old_period;
        unsigned old_max0 = 0, new_max0 = 0;
        
        for (unsigned m = 0; m < m_count; m++) 
        {
            unsigned load = st.Load(m, p0);
            old_max0 = std::max(old_max0, load);
            
            if (m == mv.old_machine) load -= s_t;
            
            new_max0 = std::max(new_max0, load);
        }
        delta += static_cast<long long>(new_max0 - old_max0) * m_count + s_t;

        // --- New period ---
        unsigned p1 = mv.new_period;
        unsigned old_max1 = 0, new_max1 = 0;
        
        for (unsigned m = 0; m < m_count; m++) 
        {
            unsigned load = 0;
            if(p1 <= st.LastPeriod())
                load = st.Load(m, p1);
            old_max1 = std::max(old_max1, load);
            
            if (m == mv.new_machine) load += s_t;
            
            new_max1 = std::max(new_max1, load);
        }
        delta += static_cast<long long>(new_max1 - old_max1) * m_count - s_t;
    }
    
    return delta;
}

int BT_ShiftDeltaTargetDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    long long delta = 0;
    long long S_tar = in.OrderType_TargetGroupSize(in.Order_TypeId(mv.task) - 1);
    unsigned  qty   = in.Order_Quantity(mv.task);

    // Old period, Old machine
    long long old_load_old = st.Load(mv.old_machine, mv.old_period);
    delta -= std::abs(S_tar - old_load_old);
    delta += std::abs(S_tar - (old_load_old - qty));

    // New Period, new machine
    if(mv.new_period > (int)st.LastPeriod())
    {
        // Nuovo periodo: tutte le macchine partono da 0
        delta += std::abs(S_tar - (long long)qty);
        delta += (long long)(in.ResourcesCount() - 1) * S_tar;
    }
    else
    {
        long long old_load_new = st.Load(mv.new_machine, mv.new_period);
        delta -= std::abs(S_tar - old_load_new);
        delta += std::abs(S_tar - (old_load_new + qty));
    }

    return delta;
}


int BT_ShiftDeltaPriorityDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    long long delta = 0;
    if(mv.old_period == mv.new_period) return 0;

    // Cost before in old_period
    {
        long long sum_prio = 0;
        unsigned count = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(st.AssignedPeriod(t) == (unsigned)mv.old_period)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count > 0)
        {
            long long avg = (sum_prio + count - 1) / count;
            for(unsigned t = 0; t < in.OrdersCount(); t++)
                if(st.AssignedPeriod(t) == (unsigned)mv.old_period)
                    delta -= std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
        }
    }

    // Cost after in old_period (task removed)
    {
        long long sum_prio = 0;
        unsigned count = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(t != (unsigned)mv.task && st.AssignedPeriod(t) == (unsigned)mv.old_period)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count > 0)
        {
            long long avg = (sum_prio + count - 1) / count;
            for(unsigned t = 0; t < in.OrdersCount(); t++)
                if(t != (unsigned)mv.task && st.AssignedPeriod(t) == (unsigned)mv.old_period)
                    delta += std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
        }
    }

    // Cost before in new_period (only if period exists)
    if(mv.new_period <= (int)st.LastPeriod())
    {
        long long sum_prio = 0;
        unsigned count = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(st.AssignedPeriod(t) == (unsigned)mv.new_period)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count > 0)
        {
            long long avg = (sum_prio + count - 1) / count;
            for(unsigned t = 0; t < in.OrdersCount(); t++)
                if(st.AssignedPeriod(t) == (unsigned)mv.new_period)
                    delta -= std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
        }
    }

    // Cost after in new_period (task added)
    {
        long long sum_prio = in.Order_Priority(mv.task);
        unsigned count = 1;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(t != (unsigned)mv.task && st.AssignedPeriod(t) == (unsigned)mv.new_period)
                { sum_prio += in.Order_Priority(t); count++; }

        long long avg = (sum_prio + count - 1) / count;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(t == (unsigned)mv.task || st.AssignedPeriod(t) == (unsigned)mv.new_period)
                delta += std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
    }

    return delta;
}

int BT_ShiftDeltaMinLoadPenalty::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    long long delta = 0;
    long long S_min = in.OrderType_MinGroupSize(in.Order_TypeId(mv.task) - 1);
    unsigned  qty   = in.Order_Quantity(mv.task);

    // Old machine, old period — loses qty
    // Skipped if old_period is remainder (Smin not enforced)
    if(!st.IsRemainderPeriod(mv.old_period))
    {
        long long load_before = st.Load(mv.old_machine, mv.old_period);
        long long load_after  = load_before - qty;
        delta -= std::max(0LL, S_min - load_before);
        delta += std::max(0LL, S_min - load_after);
    }

    // New machine, new period — gains qty
    if(mv.new_period > (int)st.LastPeriod())
    {

        unsigned old_rem = st.LastPeriod();

        for(unsigned m = 0; m < in.ResourcesCount(); m++)
        {
            long long load = st.Load(m, old_rem);
            if(mv.old_period == (int)old_rem && m == (unsigned)mv.old_machine)
                load -= qty;
            delta += std::max(0LL, S_min - load);
        }
    }
    else if(!st.IsRemainderPeriod(mv.new_period))
    {
        long long load_before = st.Load(mv.new_machine, mv.new_period);
        long long load_after  = load_before + qty;
        delta -= std::max(0LL, S_min - load_before);
        delta += std::max(0LL, S_min - load_after);
    }

    return delta;
}

int BT_ShiftDeltaMaxLoadPenalty::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    long long delta = 0;
    long long S_max = in.OrderType_MaxGroupSize(in.Order_TypeId(mv.task) - 1);
    unsigned  qty   = in.Order_Quantity(mv.task);

    // Old machine, old period — loses qty
    // Skipped if old_period is remainder (Smin not enforced)
    {
        long long load_before = st.Load(mv.old_machine, mv.old_period);
        long long load_after  = load_before - qty;
        delta -= std::max(0LL, load_before - S_max);
        delta += std::max(0LL, load_after - S_max );
    }

    // New machine, new period — gains qty
    if(mv.new_period > (int)st.LastPeriod())
    { 
        delta += std::max(0LL, qty - S_max);
    }
    else 
    {
        long long load_before = st.Load(mv.new_machine, mv.new_period);
        long long load_after  = load_before + qty;
        delta -= std::max(0LL, load_before - S_max);
        delta += std::max(0LL, load_after - S_max);
    }

    return delta;
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
    mv.task1 = Random::Uniform<unsigned>(0, in.OrdersCount() - 1);
    mv.old_period1  = st.AssignedPeriod(mv.task1);
    mv.old_machine1 = st.AssignedResource(mv.task1);
    

    do{
        mv.task2 = Random::Uniform<unsigned>(0, in.OrdersCount() - 1);
        mv.old_period2  = st.AssignedPeriod(mv.task2);
        mv.old_machine2 = st.AssignedResource(mv.task2);
    } while(mv.task2 == mv.task1);
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


int BT_SwapDeltaLoadDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    long long delta  = 0;
    unsigned  qty1   = in.Order_Quantity(mv.task1);
    unsigned  qty2   = in.Order_Quantity(mv.task2);
    unsigned  m_count = in.ResourcesCount();

    if(mv.old_period1 == mv.old_period2)
    {
        // Case 1: Swap within the same period
        unsigned p       = mv.old_period1;
        unsigned old_max = 0;
        unsigned new_max = 0;

        for(unsigned m = 0; m < m_count; m++)
        {
            unsigned load = st.Load(m, p);
            old_max = std::max(old_max, load);

            if     (m == (unsigned)mv.old_machine1) load = load - qty1 + qty2;
            else if(m == (unsigned)mv.old_machine2) load = load - qty2 + qty1;

            new_max = std::max(new_max, load);
        }

        delta = static_cast<long long>(new_max - old_max) * m_count;
    }
    else
    {
        // Case 2: Swap between different periods
        // Period 1: old_machine1 loses qty1, gains qty2
        unsigned old_max0 = 0, new_max0 = 0;

        for(unsigned m = 0; m < m_count; m++)
        {
            unsigned load = st.Load(m, mv.old_period1);
            old_max0 = std::max(old_max0, load);

            if(m == (unsigned)mv.old_machine1) load = load - qty1 + qty2;

            new_max0 = std::max(new_max0, load);
        }
        // delta load in period1 qty2 - qty1
        delta += static_cast<long long>(new_max0 - old_max0) * m_count - (long long)(qty2 - qty1);

        // Period 2: old_machine2 loses qty2, gains qty1
        unsigned old_max1 = 0, new_max1 = 0;

        for(unsigned m = 0; m < m_count; m++)
        {
            unsigned load = st.Load(m, mv.old_period2);
            old_max1 = std::max(old_max1, load);

            if(m == (unsigned)mv.old_machine2) load = load - qty2 + qty1;

            new_max1 = std::max(new_max1, load);
        }
        // delta load in period2 = qty1 - qty2
        delta += static_cast<long long>(new_max1 - old_max1) * m_count - (long long)(qty1 - qty2);
    }

    return delta;
}

int BT_SwapDeltaTargetDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    long long delta = 0;
    long long S_tar = in.OrderType_TargetGroupSize(in.Order_TypeId(mv.task1) - 1);
    unsigned  qty1  = in.Order_Quantity(mv.task1);
    unsigned  qty2  = in.Order_Quantity(mv.task2);

    // (old_machine1, old_period1) — loses qty1, gains qty2
    long long load1 = st.Load(mv.old_machine1, mv.old_period1);
    delta -= std::abs(S_tar - load1);
    delta += std::abs(S_tar - (load1 - qty1 + qty2));

    // (old_machine2, old_period2) — loses qty2, gains qty1
    long long load2 = st.Load(mv.old_machine2, mv.old_period2);
    delta -= std::abs(S_tar - load2);
    delta += std::abs(S_tar - (load2 - qty2 + qty1));

    return delta;
}

int BT_SwapDeltaPriorityDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    long long delta = 0;
    if(mv.old_period1 == mv.old_period2) return 0;

    // Cost function before in old_period1
    {
        long long sum_prio = 0;
        unsigned count     = 0;
        for(unsigned t     = 0; t < in.OrdersCount(); t++)
            if(st.AssignedPeriod(t) == (unsigned)mv.old_period1)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count > 0)
        {
            long long avg = ( static_cast<long long>(sum_prio) + count - 1) / count;
            for(unsigned t = 0; t < in.OrdersCount(); t++)
                if(st.AssignedPeriod(t) == (unsigned)mv.old_period1)
                    delta -= std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
        }
    }

    // Cost function after in old_period1 (task1 removed, task2 added)
    {
        long long sum_prio = 0;
        unsigned count     = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
        {
            if(t == (unsigned)mv.task1) continue;
            if(t == (unsigned)mv.task2) continue;
            if(st.AssignedPeriod(t) == (unsigned)mv.old_period1)
                { sum_prio += in.Order_Priority(t); count++; }
        }
        sum_prio += in.Order_Priority(mv.task2);
        count++;

        if(count > 0)
        {
            long long avg  = ( static_cast<long long>(sum_prio) + count - 1) / count;
            for(unsigned t = 0; t < in.OrdersCount(); t++)
            {
                if(t == (unsigned)mv.task1) continue;
                if(t == (unsigned)mv.task2) continue;
                if(st.AssignedPeriod(t) == (unsigned)mv.old_period1)
                    delta += std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
            }
            delta += std::abs(avg - static_cast<long long>(in.Order_Priority(mv.task2)));
        }
    }

    if(mv.old_period1 != mv.old_period2)
    {
        // Cost function before in old_period2
        {
            long long sum_prio = 0;
            unsigned count    = 0;
            for(unsigned t     = 0; t < in.OrdersCount(); t++)
                if(st.AssignedPeriod(t) == (unsigned)mv.old_period2)
                    { sum_prio += in.Order_Priority(t); count++; }

            if(count > 0)
            {
                long long avg = ( static_cast<long long>(sum_prio) + count - 1) / count;
                for(unsigned t = 0; t < in.OrdersCount(); t++)
                    if(st.AssignedPeriod(t) == (unsigned)mv.old_period2)
                        delta -= std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
            }
        }

        // Cost function after in old_period2 (task2 removed, task1 added)
        {
            long long sum_prio = 0;
            unsigned count     = 0;
            for(unsigned t     = 0; t < in.OrdersCount(); t++)
            {
                if(t == (unsigned)mv.task1) continue;
                if(t == (unsigned)mv.task2) continue;
                if(st.AssignedPeriod(t) == (unsigned)mv.old_period2)
                    { sum_prio += in.Order_Priority(t); count++; }
            }
            sum_prio += in.Order_Priority(mv.task1);
            count++;

            if(count > 0)
            {
                long long avg = ( static_cast<long long>(sum_prio) + count - 1) / count;
                for(unsigned t = 0; t < in.OrdersCount(); t++)
                {
                    if(t == (unsigned)mv.task1) continue;
                    if(t == (unsigned)mv.task2) continue;
                    if(st.AssignedPeriod(t) == (unsigned)mv.old_period2)
                        delta += std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
                }
                delta += std::abs(avg - static_cast<long long>(in.Order_Priority(mv.task1)));
            }
        }
    }

    return delta;
}

int BT_SwapDeltaMinLoadPenalty::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    long long delta = 0;
    long long S_min = in.OrderType_MinGroupSize(in.Order_TypeId(mv.task1) - 1);
    unsigned  qty1  = in.Order_Quantity(mv.task1);
    unsigned  qty2  = in.Order_Quantity(mv.task2);

    // (old_machine1, old_period1) — loses qty1, gains qty2
    if(!st.IsRemainderPeriod(mv.old_period1))
    {
        long long load_before = st.Load(mv.old_machine1, mv.old_period1);
        long long load_after  = load_before - qty1 + qty2;
        delta -= std::max(0LL, S_min - load_before);
        delta += std::max(0LL, S_min - load_after);
    }

    // (old_machine2, old_period2) — loses qty2, gains qty1
    if(!st.IsRemainderPeriod(mv.old_period2))
    {
        long long load_before = st.Load(mv.old_machine2, mv.old_period2);
        long long load_after  = load_before - qty2 + qty1;
        delta -= std::max(0LL, S_min - load_before);
        delta += std::max(0LL, S_min - load_after);
    }

    return delta;
}

int BT_SwapDeltaMaxLoadPenalty::ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const
{
    long long delta = 0;
    long long S_max = in.OrderType_MaxGroupSize(in.Order_TypeId(mv.task1) - 1); // S_max, non S_min
    unsigned  qty1  = in.Order_Quantity(mv.task1);
    unsigned  qty2  = in.Order_Quantity(mv.task2);

    // (old_machine1, old_period1) — loses qty1, gains qty2
    // Nessun skip per remainder: Smax vale per tutti i periodi
    {
        long long load_before = st.Load(mv.old_machine1, mv.old_period1);
        long long load_after  = load_before - qty1 + qty2;
        delta -= std::max(0LL, load_before - S_max); // load - S_max, non S_max - load
        delta += std::max(0LL, load_after  - S_max);
    }

    // (old_machine2, old_period2) — loses qty2, gains qty1
    // Nessun skip per remainder: Smax vale per tutti i periodi
    {
        long long load_before = st.Load(mv.old_machine2, mv.old_period2);
        long long load_after  = load_before - qty2 + qty1;
        delta -= std::max(0LL, load_before - S_max); // load - S_max, non S_max - load
        delta += std::max(0LL, load_after  - S_max);
    }

    return delta;
}