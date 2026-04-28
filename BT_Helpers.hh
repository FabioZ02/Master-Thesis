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
    friend std::istream& operator>>(std::istream& is, BT_Shift& c):
   public:
    int task, old_machine, new_machine;
    BT_Shift();
}

class BT_Swap 
{
    friend bool operator==(const BT_Swap& m1, const BT_Swap& m2);
    friend bool operator!=(const BT_Swap& m1, const BT_Swap& m2);
    friend bool operator<(const BT_Swap& m1, const BT_Swap& m2);
    friend std::ostream& operator<<(std::ostream& os, const BT_Swap& c);
    friend std::istream& operator>>(std::istream& is, BT_Swap& c):
   public:
    int task1, task2;
    BT_Swap();
}

/***************************************************************************
 * State Manager 
***************************************************************************/

class BT_SolutionManager
{
public:
    BT_SolutionManager(const BT_Input& in) : in(in) {}
    void GreedyState(BT_Output& out);
    void RandomState(BT_Output& out);
    bool CheckConsistency(const BT_Output& out) const;
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
    long long ComputeCost(const BT_Output& out) const;
    void  PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const override;
};

class BT_PriorityDeviation : public CostComponent<BT_Input, BT_Output>
{
public:
    BT_PriorityDeviation(const BT_Input& in, int w, bool hard)
        : CostComponent<BT_Input, BT_Output>(in, w, hard, "BT_PriorityDeviation") {}
    long long ComputeCost(const BT_Output& out) const;
    void      PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const override;
};

class BT_MinLoadPenalty
{
public:
    BT_MinLoadPenalty(const BT_Input& in, int w, bool soft)
        : CostComponent<BT_Input, BT_Output>(in, w, hard, "BT_MinLoadPenalty") {}
    long long ComputeCost(const BT_Output& out) const;
    void      PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const override;
private:
    const BT_Input& in;
};

/***************************************************************************
 * BT_ShiftNeighborhood Explorer:
***************************************************************************/

class BT_DeltaLoadDeviation
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Change>
{
public: 
    BT_DeltaLoadDeviation(const BT_Input& in, BT_LoadDeviation& cc)
      : DeltaCostComponent<BT_Input, BT_Output, BT_Change>(in,cc,"BT_LoadDeviation") {}
    int ComputeDeltaCost(const BT_Output& st, const BT_Change& mv) const override;
};

class BT_DeltaTargetDeviation
    : public DeltaCostComponent<BT_Input, BT_Output, BT_Change>
{
public:
    BT_DeltaLoadDeviation(const BT_Input& in, BT_TargetDeviation& cc)
      : DeltaCostComponent<>()
}


#endif