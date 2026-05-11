#include "BT_Helpers.hh"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;
using namespace EasyLocal::Debug;

int main(int argc, const char* argv[])
{
    ParameterBox main_parameters("main", "Main Program options");
    Parameter<string> instance("instance", "Input instance", main_parameters);
    Parameter<int> seed("seed", "Random seed", main_parameters);
    Parameter<string> init_state("init_state", "Initial state (to be read from file)", main_parameters);

    CommandLineParameters::Parse(argc, argv, false, true);

    if(!instance.IsSet())
    {
        cout << "Error: --main::instance filename option must always be set" << endl;
        return 1;
    }
    BT_Input in(instance);

    if(seed.IsSet())
        Random::SetSeed(seed);

    // Cost components
    BT_LoadDeviation     cc1(in, 1, false);
    BT_TargetDeviation   cc2(in, 1, false);
    BT_PriorityDeviation cc3(in, 1, false);
    BT_MinLoadPenalty    cc4(in, 1, true);
    BT_MaxLoadPenalty    cc5(in, 1, true);

    // Delta cost components — Shift
    BT_ShiftDeltaLoadDeviation     dcc1_shift(in, cc1);
    BT_ShiftDeltaTargetDeviation   dcc2_shift(in, cc2);
    BT_ShiftDeltaPriorityDeviation dcc3_shift(in, cc3);
    BT_ShiftDeltaMinLoadPenalty    dcc4_shift(in, cc4);
    BT_ShiftDeltaMaxLoadPenalty    dcc5_shift(in, cc5);
    // Delta cost components — Swap
    BT_SwapDeltaLoadDeviation     dcc1_swap(in, cc1);
    BT_SwapDeltaTargetDeviation   dcc2_swap(in, cc2);
    BT_SwapDeltaPriorityDeviation dcc3_swap(in, cc3);
    BT_SwapDeltaMinLoadPenalty    dcc4_swap(in, cc4);
    BT_SwapDeltaMaxLoadPenalty    dcc5_swap(in, cc5);

    // Helpers
    BT_SolutionManager           BT_sm(in);
    BT_ShiftNeighborhoodExplorer BT_shift_nhe(in, BT_sm);
    BT_SwapNeighborhoodExplorer  BT_swap_nhe(in, BT_sm);

    // Add cost components to solution manager
    BT_sm.AddCostComponent(cc1);
    BT_sm.AddCostComponent(cc2);
    BT_sm.AddCostComponent(cc3);
    BT_sm.AddCostComponent(cc4);
    BT_sm.AddCostComponent(cc5);
    // Add delta cost components to neighborhood explorers
    BT_shift_nhe.AddDeltaCostComponent(dcc1_shift);
    BT_shift_nhe.AddDeltaCostComponent(dcc2_shift);
    BT_shift_nhe.AddDeltaCostComponent(dcc3_shift);
    BT_shift_nhe.AddDeltaCostComponent(dcc4_shift);
    BT_shift_nhe.AddDeltaCostComponent(dcc5_shift);

    BT_swap_nhe.AddDeltaCostComponent(dcc1_swap);
    BT_swap_nhe.AddDeltaCostComponent(dcc2_swap);
    BT_swap_nhe.AddDeltaCostComponent(dcc3_swap);
    BT_swap_nhe.AddDeltaCostComponent(dcc4_swap);
    BT_swap_nhe.AddDeltaCostComponent(dcc5_swap);

    // Tester
    Tester<BT_Input, BT_Output> tester(in, BT_sm);
    MoveTester<BT_Input, BT_Output, BT_Shift> shift_move_test(in, BT_sm, BT_shift_nhe, "BT_Shift move", tester);
    MoveTester<BT_Input, BT_Output, BT_Swap>  swap_move_test(in, BT_sm, BT_swap_nhe,  "BT_Swap move",  tester);

    if(!CommandLineParameters::Parse(argc, argv, true, false))
        return 1;

    // Enter tester
    if(init_state.IsSet())
        tester.RunMainMenu(init_state);
    else
        tester.RunMainMenu();

    return 0;
}