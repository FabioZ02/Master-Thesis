```cpp
#ifndef BT_HELPERS_HH
#define BT_HELPERS_HH

#include "BT_Data.hh"
#include <easylocal.hh>

using namespace EasyLocal::Core;

class BT_Shift 
{
    friend bool operator==(const BT_Shift& m1, const BT_Shift& m2);
    friend bool operator!=(const BT_Shift& m1, const BT_Shift& m2);
    friend bool operator<(const BT_Shift& m1, const BT_Shift& m2);
    friend std::ostream& operator<<(std::ostream& os, const BT_Shift& c);
    friend std::istream& operator>>(std::istream& is, BT_Shift& c);
   public:
    int task, new_period, new_machine;
    BT_Shift();
};

class BT_Swap 
{
    friend bool operator==(const BT_Swap& m1, const BT_Swap& m2);
    friend bool operator!=(const BT_Swap& m1, const BT_Swap& m2);
    friend bool operator<(const BT_Swap& m1, const BT_Swap& m2);
    friend std::ostream& operator<<(std::ostream& os, const BT_Swap& c);
    friend std::istream& operator>>(std::istream& is, BT_Swap& c);
   public:
    int task1, task2;
    BT_Swap();
};

/***************************************************************************
 * State Manager 
***************************************************************************/

class BT_SolutionManager : public SolutionManager<BT_Input, BT_Output>
{
public:
    BT_SolutionManager(const BT_Input& in)
        : SolutionManager<BT_Input, BT_Output>(in, "BT_SolutionManager") {}
    void GreedyState(BT_Output& out);
    void RandomState(BT_Output& out) override;
    bool CheckConsistency(const BT_Output& out) const override;
};

class BT_LoadDeviation : public CostComponent<BT_Input, BT_Output>
{
public:
    BT_LoadDeviation(const BT_Input& in, int w, bool soft)
        : CostComponent<BT_Input, BT_Output>(in, w, soft, "BT_LoadDeviation") {}
    long long ComputeCost(const BT_Output& out) const override;
    void PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const override;
};

class BT_TargetDeviation : public CostComponent<BT_Input, BT_Output>
{
public:
    BT_TargetDeviation(const BT_Input& in, int w, bool hard)
        : CostComponent<BT_Input, BT_Output>(in, w, hard, "BT_TargetDeviation") {}
    long long ComputeCost(const BT_Output& out) const override;
    void  PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const override;
};

class BT_PriorityDeviation : public CostComponent<BT_Input, BT_Output>
{
public:
    BT_PriorityDeviation(const BT_Input& in, int w, bool hard)
        : CostComponent<BT_Input, BT_Output>(in, w, hard, "BT_PriorityDeviation") {}
    long long ComputeCost(const BT_Output& out) const override;
    void      PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const override;
};

class BT_MinLoadPenalty : public CostComponent<BT_Input, BT_Output>
{
public:
    BT_MinLoadPenalty(const BT_Input& in, int w, bool hard)
        : CostComponent<BT_Input, BT_Output>(in, w, hard, "BT_MinLoadPenalty") {}
    long long ComputeCost(const BT_Output& out) const override;
    void      PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const override;
};

/***************************************************************************
 * BT_ShiftNeighborhood Explorer:
***************************************************************************/

class BT_ShiftDeltaLoadDeviation
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Shift>
{
public: 
    BT_ShiftDeltaLoadDeviation(const BT_Input& in, BT_LoadDeviation& cc)
      : DeltaCostComponent<BT_Input, BT_Output, BT_Shift>(in,cc,"BT_ShiftDeltaLoadDeviation") {}
    long long ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const override;
};

class BT_ShiftDeltaTargetDeviation
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Shift>
{
public:
    BT_ShiftDeltaTargetDeviation(const BT_Input& in, BT_TargetDeviation& cc)
      : DeltaCostComponent<BT_Input, BT_Output, BT_Shift>(in,cc,"BT_ShiftDeltaTargetDeviation") {}
    long long ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const override;
};

class BT_ShiftDeltaPriorityDeviation
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Shift>
{
public:
    BT_ShiftDeltaPriorityDeviation(const BT_Input& in, BT_PriorityDeviation& cc)
       : DeltaCostComponent<BT_Input, BT_Output, BT_Shift>(in, cc, "BT_ShiftDeltaPriorityDeviation") {}
    long long ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const override;
};

class BT_ShiftDeltaMinLoadPenalty
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Shift>
{
public:
    BT_ShiftDeltaMinLoadPenalty(const BT_Input& in, BT_MinLoadPenalty& cc)
        : DeltaCostComponent<BT_Input, BT_Output, BT_Shift>(in, cc, "BT_ShiftDeltaMinLoadPenalty") {}
    long long ComputeDeltaCost(const BT_Output& st, const BT_Shift& mv) const override;
};

class BT_ShiftNeighborhoodExplorer
  : public NeighborhoodExplorer<BT_Input, BT_Output, BT_Shift>
{
public:
  BT_ShiftNeighborhoodExplorer(const BT_Input& pin, SolutionManager<BT_Input, BT_Output>& psm)
    : NeighborhoodExplorer<BT_Input, BT_Output, BT_Shift>(pin, psm, "BT_ShiftNeighborhoodExplorer") {}
  void RandomMove(const BT_Output&, BT_Shift&) const override;
  bool FeasibleMove(const BT_Output&, const BT_Shift&) const override;
  void MakeMove(BT_Output&, const BT_Shift&) const override;
  void FirstMove(const BT_Output&, BT_Shift&) const override;
  bool NextMove(const BT_Output&, BT_Shift&) const override;
protected:
  bool AnyNextMove(const BT_Output&, BT_Shift&) const;
};

/***************************************************************************
 * BT_SwapNeighborhood Explorer:
***************************************************************************/

class BT_SwapDeltaLoadDeviation
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Swap>
{
public: 
    BT_SwapDeltaLoadDeviation(const BT_Input& in, BT_LoadDeviation& cc)
      : DeltaCostComponent<BT_Input, BT_Output, BT_Swap>(in,cc,"BT_SwapDeltaLoadDeviation") {}
    long long ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const override;
};

class BT_SwapDeltaTargetDeviation
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Swap>
{
public:
    BT_SwapDeltaTargetDeviation(const BT_Input& in, BT_TargetDeviation& cc)
      : DeltaCostComponent<BT_Input, BT_Output, BT_Swap>(in,cc,"BT_SwapDeltaTargetDeviation") {}
    long long ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const override;
};

class BT_SwapDeltaPriorityDeviation
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Swap>
{
public:
    BT_SwapDeltaPriorityDeviation(const BT_Input& in, BT_PriorityDeviation& cc)
       : DeltaCostComponent<BT_Input, BT_Output, BT_Swap>(in, cc, "BT_SwapDeltaPriorityDeviation") {}
    long long ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const override;
};

class BT_SwapDeltaMinLoadPenalty
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Swap>
{
public:
    BT_SwapDeltaMinLoadPenalty(const BT_Input& in, BT_MinLoadPenalty& cc)
        : DeltaCostComponent<BT_Input, BT_Output, BT_Swap>(in, cc, "BT_SwapDeltaMinLoadPenalty") {}
    long long ComputeDeltaCost(const BT_Output& st, const BT_Swap& mv) const override;
};

class BT_SwapNeighborhoodExplorer
  : public NeighborhoodExplorer<BT_Input, BT_Output, BT_Swap>
{
public:
  BT_SwapNeighborhoodExplorer(const BT_Input& pin, SolutionManager<BT_Input, BT_Output>& psm)
    : NeighborhoodExplorer<BT_Input, BT_Output, BT_Swap>(pin, psm, "BT_SwapNeighborhoodExplorer") {}
  void RandomMove(const BT_Output&, BT_Swap&) const override;
  bool FeasibleMove(const BT_Output&, const BT_Swap&) const override;
  void MakeMove(BT_Output&, const BT_Swap&) const override;
  void FirstMove(const BT_Output&, BT_Swap&) const override;
  bool NextMove(const BT_Output&, BT_Swap&) const override;
protected:
  bool AnyNextMove(const BT_Output&, BT_Swap&) const;
};
#endif
