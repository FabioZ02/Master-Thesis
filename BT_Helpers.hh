#ifndef BT_HELPERS_HH
#define BT_HELPERS_HH

#include "BT_Data.hh"

class BT_SolutionManager
{
public:
    BT_SolutionManager(const BT_Input& in) : in(in) {}
    void GreedyState(BT_Output& out);
    void RandomState(BT_Output& out);
    bool CheckConsistency(const BT_Output& out) const;
private:
    const BT_Input& in;
};

class BT_LoadDeviation
{
public:
    BT_LoadDeviation(const BT_Input& in) : in(in) {}
    long long ComputeCost(const BT_Output& out) const;
    void      PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const;
private:
    const BT_Input& in;
};

class BT_TargetDeviation
{
public:
    BT_TargetDeviation(const BT_Input& in) : in(in) {}
    long long ComputeCost(const BT_Output& out) const;
    void      PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const;
private:
    const BT_Input& in;
};

class BT_PriorityDeviation
{
public:
    BT_PriorityDeviation(const BT_Input& in) : in(in) {}
    long long ComputeCost(const BT_Output& out) const;
    void      PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const;
private:
    const BT_Input& in;
};

class BT_MinLoadPenalty
{
public:
    BT_MinLoadPenalty(const BT_Input& in) : in(in) {}
    long long ComputeCost(const BT_Output& out) const;
    void      PrintViolations(const BT_Output& out, std::ostream& os = std::cout) const;
private:
    const BT_Input& in;
};

#endif