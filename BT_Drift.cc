// bt_drift.cc — diagnostica drift delta-cost vs ComputeCost
//
// Esercita SOLO il vicinato shift con mosse random feasible (accept-all),
// accumulando il delta-cost esattamente come fa SA, e ogni N mosse confronta
// l'accumulatore con un ricalcolo full di CostFunctionComponents.
//
// Con un delta corretto, drift == 0 IDENTICAMENTE per sempre, a prescindere
// da quanto peggiora la soluzione. Qualsiasi drift != 0 e' un bug del delta;
// se compare in sincrono con la colonna rmperiods e' RemovePeriod.
//
// Uso: ./bt_drift <instance.json> [num_moves=200000] [check_every=1000] [seed=1]

#include "BT_Helpers.hh"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>

using namespace std;

int main(int argc, const char* argv[])
{
    if(argc < 2)
    {
        cerr << "uso: " << argv[0]
             << " <instance.json> [num_moves] [check_every] [seed]\n";
        return 1;
    }
    string   instance    = argv[1];
    long     num_moves   = (argc > 2) ? atol(argv[2]) : 200000;
    long     check_every = (argc > 3) ? atol(argv[3]) : 1000;
    unsigned seed        = (argc > 4) ? (unsigned)atol(argv[4]) : 1u;

    Random::SetSeed(seed);
    BT_Input in(instance);

    // --- stesse componenti di BT_Main.cc ---
    BT_LoadDeviation     cc1(in, 1, false);
    BT_TargetDeviation   cc2(in, 1, false);
    BT_PriorityDeviation cc3(in, 1, false);
    BT_MinLoadPenalty    cc4(in, 1, true);
    BT_MaxLoadPenalty    cc5(in, 1, true);

    BT_ShiftDeltaLoadDeviation     dcc1(in, cc1);
    BT_ShiftDeltaTargetDeviation   dcc2(in, cc2);
    BT_ShiftDeltaPriorityDeviation dcc3(in, cc3);
    BT_ShiftDeltaMinLoadPenalty    dcc4(in, cc4);
    BT_ShiftDeltaMaxLoadPenalty    dcc5(in, cc5);

    BT_SolutionManager           sm(in);
    BT_ShiftNeighborhoodExplorer nhe(in, sm);

    sm.AddCostComponent(cc1); sm.AddCostComponent(cc2); sm.AddCostComponent(cc3);
    sm.AddCostComponent(cc4); sm.AddCostComponent(cc5);
    nhe.AddDeltaCostComponent(dcc1); nhe.AddDeltaCostComponent(dcc2);
    nhe.AddDeltaCostComponent(dcc3); nhe.AddDeltaCostComponent(dcc4);
    nhe.AddDeltaCostComponent(dcc5);

    // --- stato iniziale: greedy, come nel menu ---
    BT_Output st(in);
    sm.GreedyState(st);

    auto full0 = sm.CostFunctionComponents(st);
    long long acc_viol  = (long long)full0.violations;
    long long acc_obj   = (long long)full0.objective;
    long long acc_total = (long long)full0.total;

    cout << "instance=" << instance << "  seed=" << seed
         << "  moves=" << num_moves << "  check_every=" << check_every << "\n";
    cout << left
         << setw(10) << "move"
         << setw(16) << "acc_total"
         << setw(16) << "full_total"
         << setw(14) << "drift_tot"
         << setw(12) << "drift_viol"
         << setw(12) << "drift_obj"
         << setw(12) << "rmperiods"
         << setw(7)  << "lastP" << "\n";
    cout << string(99, '-') << "\n";

    auto report = [&](long m)
    {
        auto f = sm.CostFunctionComponents(st);
        cout << left
             << setw(10) << m
             << setw(16) << acc_total
             << setw(16) << (long long)f.total
             << setw(14) << (acc_total - (long long)f.total)
             << setw(12) << (acc_viol  - (long long)f.violations)
             << setw(12) << (acc_obj   - (long long)f.objective)
             << setw(12) << g_remove_period_calls
             << setw(7)  << st.LastPeriod() << "\n";
    };
    report(0);

    BT_Shift mv;
    long accepted = 0;
    for(long i = 1; i <= num_moves; i++)
    {
        int guard = 0;
        do { nhe.RandomMove(st, mv); }
        while(!nhe.FeasibleMove(st, mv) && ++guard < 2000);
        if(!nhe.FeasibleMove(st, mv)) continue;

        auto d = nhe.DeltaCostFunctionComponents(st, mv);
        nhe.MakeMove(st, mv);              // applica (puo' chiamare RemovePeriod)
        acc_viol  += (long long)d.violations;
        acc_obj   += (long long)d.objective;
        acc_total += (long long)d.total;
        accepted++;

        if(accepted % check_every == 0)
            report(accepted);
    }

    auto f = sm.CostFunctionComponents(st);
    cout << string(99, '-') << "\n";
    cout << "FINE: mosse accettate=" << accepted
         << "  RemovePeriod totali=" << g_remove_period_calls
         << "  drift_total finale=" << (acc_total - (long long)f.total) << "\n";
    return 0;
}