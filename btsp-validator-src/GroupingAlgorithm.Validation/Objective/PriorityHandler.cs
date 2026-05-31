using System.Collections.Generic;
using System.Linq;

namespace GroupingAlgorithm.Validation.Objective
{
    /// <summary>
    /// Convert priority values to bigM factors for the evaluators
    /// </summary>
    public static class PriorityHandler
    {
        /// <summary>
        /// Return the bigM factors for a list of priorities and corresponding capacities
        /// </summary>
        /// <param name="priorityCapacityPairs">List of priority and capacity information in an input</param>
        /// <returns>The bigM factors for a list of priorities and corresponding capacities</returns>
        public static IList<long> GetDistinctBigMsForPriorities(IList<(int Priority, int Capacity)> priorityCapacityPairs)
        {
            (_, IList<long> bigMs) = GetUpperBoundAndBigMsSortedByPriority(priorityCapacityPairs);
            return bigMs;
        }

        /// <summary>
        /// Return a total upper bound for a list of priorities and corresponding capacities
        /// </summary>
        /// <param name="priorityCapacityPairs">List of priority and capacity information in an input</param>
        /// <returns>A total upper bound for a list of priorities and corresponding capacities</returns>
        public static long GetUpperBoundForPriorityObjective(IList<(int Priority, int Capacity)> priorityCapacityPairs)
        {
            (long upperBound, _) = GetUpperBoundAndBigMsSortedByPriority(priorityCapacityPairs);
            return upperBound;
        }

        /// <summary>
        /// Return a total upper bound and the bigM factors for a list of priorities and corresponding capacities
        /// </summary>
        /// <param name="priorityCapacityPairs">List of priority and capacity information in an input</param>
        /// <returns>A total upper bound and the bigM factors for a list of priorities and corresponding capacities</returns>
        public static (long UpperBound, IList<long> BigMs) GetUpperBoundAndBigMsByOrderOfPriorityInList(IList<(int Priority, int Capacity)> priorityCapacityPairs)
        {
            (long upperBound, IList<long> bigMsPerPrio) = GetUpperBoundAndBigMsSortedByPriority(priorityCapacityPairs);
            return (upperBound, priorityCapacityPairs.Select(x => bigMsPerPrio[x.Priority - 1]).ToList());
        }

        /// <summary>
        /// Return a total upper bound and the bigM values sorted by priority values
        /// </summary>
        /// <param name="priorityCapacityPairs">List of priority and capacity information in an input</param>
        /// <returns>A total upper bound and the bigM values sorted by priority values</returns>
        private static (long UpperBound, IList<long> BigMs) GetUpperBoundAndBigMsSortedByPriority(IList<(int Priority, int Capacity)> priorityCapacityPairs)
        {
            // We can take the max as all priorities from 1 to max will appear and no others
            int priorityCount = priorityCapacityPairs.Max(x => x.Priority);
            // 
            IList<long> upperBoundPerPriority = new long[priorityCount];
            foreach ((int priority, int capacity) in priorityCapacityPairs)
            {
                //TODO: improve bound; considering operation capacities? efficiency?
                upperBoundPerPriority[priority - 1] += capacity;
            }

            IList<long> bigMsByPriority = new long[priorityCount];
            long factor = 1;
            for (int i = priorityCount - 1; i >= 0; i--)
            {
                bigMsByPriority[i] = factor;
                factor *= upperBoundPerPriority[i] + 1;
            }
            // upperBound is one less than the next bigM would be if another priority existed
            long upperBound = factor - 1;
            return (upperBound, bigMsByPriority);
        }
    }
}