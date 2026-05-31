using GroupingAlgorithm.Interface;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;

namespace GroupingAlgorithm.Validation.Objective
{
    /// <summary>
    /// Objective data including an IObjective and an upper bound
    /// </summary>
    public class ObjectiveData
    {
        /// <summary>
        /// The Objective
        /// </summary>
        public IObjective Objective { get; }

        /// <summary>
        /// The upper bound of the objective
        /// </summary>
        public long UpperBound { get; }

        /// <summary>
        /// Create objective data with upper bound
        /// </summary>
        /// <param name="objective">The given objective</param>
        /// <param name="upperBound">The upper bound of the objective</param>
        public ObjectiveData(IObjective objective, long upperBound)
        {
            Objective = objective;
            UpperBound = upperBound;
        }

        /// <summary>
        /// Create objective data with upper bound
        /// </summary>
        /// <param name="objective">The given objective</param>
        /// <param name="mathematicalInput">The given mathematical input</param>
        public ObjectiveData(IObjective objective, IInput input)
        {
            Objective = objective;
            UpperBound = GetUpperBoundForObjective(objective, input);
        }

        private static IList<(int Priority, int Quantity)> GetPriorityQuantityPairs(IInput input)
        {
            IList<(int Priority, int Quantity)> result = new List<(int Priority, int Quantity)>();

            foreach (IOrder order in input.Orders)
            {
                result.Add((order.Priority, order.Quantity));
            }

            return result;
        }

        private static long GetUpperBoundForOrderPriorityObjective(IInput input)
        {
            long upperBound = 0;

            int minPriority = input.Orders.Min(x => x.Priority);
            int maxPriority = input.Orders.Max(x => x.Priority);

            return (maxPriority - minPriority) * input.Orders.Count;
        }

        private static long GetUpperBoundForTargetGroupSizeDeviationObjective(IInput input)
        {
            int upperBound = 0;

            // Here we assume that in the worst case each planning group contains exactly one order
            foreach (IOrder order in input.Orders)
            {
                IOrderType orderType = input.OrderTypes.First(x => x.Id == order.TypeId);

                upperBound += Math.Max(orderType.TargetGroupSize, orderType.MaxGroupSize - orderType.TargetGroupSize) * input.Resources.Count;
            }

            return upperBound;
        }

        private static long GetUpperBoundForTargetGroupSizeDeviationObjectiveOld(IInput input)
        {
            int upperBound = 0;

            // Here we assume that in the worst case each planning group contains exactly one order
            foreach (IOrder order in input.Orders)
            {
                IOrderType orderType = input.OrderTypes.First(x => x.Id == order.TypeId);

                int maximalOrderQuantity = input.Orders.Where(o => o.TypeId == orderType.Id).Max(o => o.Quantity);

                int maximalFeasiblePlanningGroupSize = Math.Max(orderType.MaxGroupSize, maximalOrderQuantity);

                int maximalTargetSizeDeviation = Math.Max(orderType.TargetGroupSize, maximalFeasiblePlanningGroupSize - orderType.TargetGroupSize);

                upperBound += maximalTargetSizeDeviation * input.Resources.Count;
            }

            return upperBound;
        }

        private static long GetUpperBoundForBalancedCapacityLoadObjective(IInput input)
        {
            if (input.Resources.Count == 0)
            {
                return 0;
            }

            int upperBound = 0;

            // Here we assume that in the worst case each planning group contains exactly one order
            foreach (IOrder order in input.Orders)
            {
                IOrderType orderType = input.OrderTypes.First(x => x.Id == order.TypeId);

                upperBound += (input.Resources.Count - 1) * orderType.MaxGroupSize;
            }

            return upperBound;
        }

        /// <summary>
        /// Returns the upper bound for the given objective in the given input
        /// </summary>
        /// <param name="objective">The given objective</param>
        /// <param name="input">The given input</param>
        /// <returns></returns>
        /// <exception cref="InvalidEnumArgumentException"></exception>
        private static long GetUpperBoundForObjective(IObjective objective, IInput input)
        {
            switch (objective.Type)
            {
                case ObjectiveType.OrderPriority:
                    return GetUpperBoundForOrderPriorityObjective(input);
                case ObjectiveType.TargetGroupSizeDeviation:
                    return GetUpperBoundForTargetGroupSizeDeviationObjective(input);
                case ObjectiveType.BalancedCapacityLoad:
                    return GetUpperBoundForBalancedCapacityLoadObjective(input);
                case ObjectiveType.MinGroupSizePenalty:
                    return 1;
                default:
                    throw new InvalidEnumArgumentException(nameof(objective.Type));
            }
        }

    }
}