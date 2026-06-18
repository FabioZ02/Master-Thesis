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
    Parameter<string> method("method", "Solution method (empty for tester)", main_parameters);
    Parameter<string> output_file("output_file", "Write the output to a file (filename required)", main_parameters);
    Parameter<double> swap_rate("swap_rate", "Swap rate", main_parameters); // for the union neighborhood
    Parameter<bool> irace("irace", "IRace mode", main_parameters); // For the tuning with irace
    swap_rate = 0.5;
    irace = false;

    CommandLineParameters::Parse(argc, argv, false, true);

    if(!instance.IsSet())
    {
        cout << "Error: --main::instance filename option must always be set" << endl;
        return 1;
    }
    BT_Input in(instance);
    BT_Output out(in);

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

    // Union Neighbourhood
    SetUnionNeighborhoodExplorer<BT_Input, BT_Output, DefaultCostStructure<int>, decltype(BT_shift_nhe), decltype(BT_swap_nhe)> bt_bnhe(in, BT_sm, "ShiftOrSwap", BT_shift_nhe, BT_swap_nhe, {1 - swap_rate, swap_rate});

    // Simulated Annealing using union neighborhood
    SimulatedAnnealing<BT_Input, BT_Output, decltype(bt_bnhe)::MoveType> bt_bsa(in, BT_sm, bt_bnhe, "BSA");
    SimulatedAnnealingWithReheating<BT_Input, BT_Output, decltype(bt_bnhe)::MoveType> bt_bsawr(in, BT_sm, bt_bnhe, "BSAwr");

    // Tester
    Tester<BT_Input, BT_Output> tester(in, BT_sm);
    MoveTester<BT_Input, BT_Output, BT_Shift> shift_move_test(in, BT_sm, BT_shift_nhe, "BT_Shift move", tester);
    MoveTester<BT_Input, BT_Output, BT_Swap>  swap_move_test(in, BT_sm, BT_swap_nhe,  "BT_Swap move",  tester);

    //Tester for the union neighborhood
    MoveTester<BT_Input, BT_Output, decltype(bt_bnhe)::MoveType> bimodal_test(in, BT_sm, bt_bnhe, "ShiftOrSwap move", tester);

    SimpleLocalSearch<BT_Input, BT_Output> bt_solver(in, BT_sm, "BT SOlver");

    if(!CommandLineParameters::Parse(argc, argv, true, false))
        return 1;
 
    if (!method.IsSet())
        {
            if(init_state.IsSet())
            tester.RunMainMenu(init_state);
            else
            tester.RunMainMenu();
        }
    else
        {
            if (method == string("BSA"))
              bt_solver.SetRunner(bt_bsa);
            else if (method == string("BSAwr"))
              bt_solver.SetRunner(bt_bsawr);
            else
                {
                    cerr << "Error: method " << string(method) << " not recognized" << endl;
                    exit(1);
                }
            SolverResult<BT_Input, BT_Output> result = bt_solver.Solve();
            out = result.output;
            if (irace)
               cout << result.cost.total << endl;
            else if (output_file.IsSet())
               {
                  ofstream os(output_file);
                  os << out << endl;
                  os << "Cost: " << result.cost.total << endl;
                  os << "Time: " << result.running_time << endl;
                  os.close();
                  string instance_name = instance;
                  instance_name = instance_name.substr(instance_name.find_last_of("/") + 1);
                  instance_name = instance_name.substr(0, instance_name.find_last_of("."));
                  cout << instance_name << ": " << result.cost.total << " (time: " << result.running_time << ")" << endl;
               }
            else
               {
                   cout << out << endl;
                   cout << "Cost: " << result.cost.total << endl;
                   cout << "Time: " << result.running_time << endl;
               }
         }
    return 0;
}