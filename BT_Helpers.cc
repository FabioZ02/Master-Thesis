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

    // 4. Smax not violated in the new cell
    unsigned S_max    = in.OrderType_MaxGroupSize(in.Order_TypeId(mv.task) - 1);
    unsigned qty      = in.Order_Quantity(mv.task);
    unsigned cur_load = 0;

    // If the new period already exists, get the current load
    if(mv.new_period <= (int)st.LastPeriod())
        cur_load = st.Load(mv.new_machine, mv.new_period);

    if(cur_load + qty > S_max)
        return false;

    return true;
}

void BT_ShiftNeighborhoodExplorer::MakeMove(BT_Output& st, const BT_Shift& mv) const
{
    st.Assign(mv.task, mv.new_machine, mv.new_period);
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
        // Case: SHift between different periods
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
            unsigned load = st.Load(m, p1);
            old_max1 = std::max(old_max1, load);
            
            if (m == mv.new_machine) load += s_t;
            
            new_max1 = std::max(new_max1, load);
        }
        delta += static_cast<long long>(new_max1 - old_max1) * m_count - s_t;
    }
    
    return delta;
}

long long BT_ShiftDeltaTargetDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    long long delta = 0;
    long long S_tar = in.OrderType_TargetGroupSize(in.Order_TypeId(mv.task) - 1);
    unsigned  qty   = in.Order_Quantity(mv.task);

    // ── Vecchia macchina, vecchio periodo — perde qty ──
    long long old_load_old = st.Load(mv.old_machine, mv.old_period);
    delta -= std::abs(S_tar - old_load_old);           // contributo prima
    delta += std::abs(S_tar - (old_load_old - qty));   // contributo dopo

    // ── Nuova macchina, nuovo periodo — guadagna qty ──
    long long old_load_new = st.Load(mv.new_machine, mv.new_period);
    delta -= std::abs(S_tar - old_load_new);           // contributo prima
    delta += std::abs(S_tar - (old_load_new + qty));   // contributo dopo

    return delta;
}

long long BT_ShiftDeltaPriorityDeviation::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    long long delta = 0;

    // ── f3 PRIMA nel old_period (mv.task è ancora qui) ──
    {
        double   sum_prio = 0.0;
        unsigned count    = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(st.AssignedPeriod(t) == (unsigned)mv.old_period)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count > 0)
        {
            long long avg = static_cast<long long>(std::ceil(sum_prio / count));
            for(unsigned t = 0; t < in.OrdersCount(); t++)
                if(st.AssignedPeriod(t) == (unsigned)mv.old_period)
                    delta -= std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
        }
    }

    // ── f3 DOPO nel old_period (mv.task rimosso) ──
    {
        double   sum_prio = 0.0;
        unsigned count    = 0;
        for(unsigned t = 0; t < in.OrdersCount(); t++)
            if(t != (unsigned)mv.task && st.AssignedPeriod(t) == (unsigned)mv.old_period)
                { sum_prio += in.Order_Priority(t); count++; }

        if(count > 0)
        {
            long long avg = static_cast<long long>(std::ceil(sum_prio / count));
            for(unsigned t = 0; t < in.OrdersCount(); t++)
                if(t != (unsigned)mv.task && st.AssignedPeriod(t) == (unsigned)mv.old_period)
                    delta += std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
        }
    }

    // ── Periodo nuovo solo se diverso dal vecchio ──
    if(mv.old_period != mv.new_period)
    {
        // f3 PRIMA nel new_period (mv.task non ancora qui)
        {
            double   sum_prio = 0.0;
            unsigned count    = 0;
            for(unsigned t = 0; t < in.OrdersCount(); t++)
                if(st.AssignedPeriod(t) == (unsigned)mv.new_period)
                    { sum_prio += in.Order_Priority(t); count++; }

            if(count > 0)
            {
                long long avg = static_cast<long long>(std::ceil(sum_prio / count));
                for(unsigned t = 0; t < in.OrdersCount(); t++)
                    if(st.AssignedPeriod(t) == (unsigned)mv.new_period)
                        delta -= std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
            }
        }

        // f3 DOPO nel new_period (mv.task aggiunto)
        {
            double   sum_prio = 0.0;
            unsigned count    = 0;
            for(unsigned t = 0; t < in.OrdersCount(); t++)
                if(t == (unsigned)mv.task || st.AssignedPeriod(t) == (unsigned)mv.new_period)
                    { sum_prio += in.Order_Priority(t); count++; }

            if(count > 0)
            {
                long long avg = static_cast<long long>(std::ceil(sum_prio / count));
                for(unsigned t = 0; t < in.OrdersCount(); t++)
                    if(t == (unsigned)mv.task || st.AssignedPeriod(t) == (unsigned)mv.new_period)
                        delta += std::abs(avg - static_cast<long long>(in.Order_Priority(t)));
            }
        }
    }

    return delta;
}

long long BT_ShiftDeltaMinLoadPenalty::ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const
{
    long long delta = 0;
    long long S_min = in.OrderType_MinGroupSize(in.Order_TypeId(mv.task) - 1);
    unsigned  qty   = in.Order_Quantity(mv.task);

    // Old machine, old period — loses qty
    // Smin not enforced on remainder period
    if(!st.IsRemainderPeriod(mv.old_period))
    {
        long long load_before = st.Load(mv.old_machine, mv.old_period);
        long long load_after  = load_before - qty;
        delta -= std::max(0LL, S_min - load_before); // penalty before
        delta += std::max(0LL, S_min - load_after);  // penalty after
    }

    // New machine, new period — gains qty
    // Smin not enforced on remainder period
    if(!st.IsRemainderPeriod(mv.new_period))
    {
        long long load_before = (mv.new_period <= (int)st.LastPeriod())
                                ? st.Load(mv.new_machine, mv.new_period) : 0;
        long long load_after  = load_before + qty;
        delta -= std::max(0LL, S_min - load_before); // penalty before
        delta += std::max(0LL, S_min - load_after);  // penalty after
    }

    return delta;
}