using GroupingAlgorithm.Interface;
using GroupingAlgorithm.Interface.Implementation;
using GroupingAlgorithm.Validation.Objective;
using GroupingAlgorithm.ValidationAndObjective;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;

namespace GroupingAlgorithm.Validation
{
    public class ObjectiveCalculator
    {
        private readonly IInput _input;
        private readonly IOutput _output;

        private List<ObjectiveDataWithWeight> _objectiveDataWithWeights;
        private long _totalUpperBound;

        public ObjectiveCalculator(IInput input, IOutput output)
        {
            _input = input ?? throw new NullReferenceException(nameof(input));
            _output = output ?? throw new NullReferenceException(nameof(output));

            InitializeObjectiveDataWithWeights(input);
        }

        private void InitializeObjectiveDataWithWeights(IInput input)
        {
            IList<ObjectiveData> objectiveData = new List<ObjectiveData>();
            foreach (IObjective objective in input.Objectives)
            {
                objectiveData.Add(new ObjectiveData(objective, input));
            }

            List<long> objectiveDistinctBigMs = new List<long>();
            _objectiveDataWithWeights = new List<ObjectiveDataWithWeight>();

            // objectiveDistinctBigMs is still empty and will be filled by CalculateNormalizedWeightsAndBigMs
            _objectiveDataWithWeights.AddRange(WeightHandler.CalculateNormalizedWeightsAndBigMs(objectiveData, objectiveDistinctBigMs));

            _totalUpperBound =
                _objectiveDataWithWeights.Sum(
                    x => x.BigM * x.NormalizedWeight * x.ObjectiveData.UpperBound);

            if (_totalUpperBound < 0)
            {
                throw new ArgumentOutOfRangeException("Total upper bound for objective function is negative due to arithmetical overflow. " +
                    "Solution cost cannot be calculated correctly for this instance.");
            }
        }

        /// <summary>
        /// TODO
        /// </summary>
        /// <returns></returns>
        public IList<ObjComponentValue> CalculateObjectiveComponents()
        {
            IList<ObjComponentValue> objectiveComponents
                = new List<ObjComponentValue>();

            IDictionary<ObjectiveType, ObjectiveDataWithWeight> objectiveTypeDictionary =
                _objectiveDataWithWeights.ToDictionary(x => x.ObjectiveType, x => x);

            int minPriority = int.MaxValue;

            IList<ObjectiveType> allObjectiveTypes = ((ObjectiveType[])Enum.GetValues(typeof(ObjectiveType))).ToList();
            foreach (ObjectiveType objectiveType in allObjectiveTypes)
            {
                if (objectiveType == ObjectiveType.MinGroupSizePenalty)
                {
                    continue;
                }

                // TODO: make more elegant
                long objectiveValue, weightedValue;
                double exactObjectiveValue, exactWeightedValue;
                if (objectiveTypeDictionary.ContainsKey(objectiveType))
                {
                    ObjectiveDataWithWeight objectiveDataWithWeight = objectiveTypeDictionary[objectiveType];
                    ObjectiveData objectiveData = objectiveDataWithWeight.ObjectiveData;
                    objectiveValue = CalculateObjectiveValue(objectiveData);
                    weightedValue = CalculateWeightedValue(objectiveValue, objectiveDataWithWeight);
                    minPriority = Math.Min(objectiveDataWithWeight.ObjectiveData.Objective.Priority, minPriority);

                    if (objectiveType == ObjectiveType.OrderPriority)
                    {
                        (exactObjectiveValue, _) = CalculateOrderPriorityObjective();
                        exactWeightedValue = CalculateWeightedValue(exactObjectiveValue, objectiveDataWithWeight);
                    }
                    else
                    {
                        exactObjectiveValue = objectiveValue;
                        exactWeightedValue = weightedValue;
                    }
                }
                else
                {
                    IObjective objective = new ObjectiveDummy(objectiveType);
                    ObjectiveData objectiveData = new ObjectiveData(objective, _input);
                    objectiveValue = CalculateObjectiveValue(objectiveData);
                    weightedValue = 0;

                    if (objectiveType == ObjectiveType.OrderPriority)
                    {
                        (exactObjectiveValue, _) = CalculateOrderPriorityObjective();
                        exactWeightedValue = 0;
                    }
                    else
                    {
                        exactObjectiveValue = objectiveValue;
                        exactWeightedValue = 0;
                    }

                }

                ObjComponentValue objComponentValue = new ObjComponentValue(objectiveType, objectiveValue, weightedValue, exactObjectiveValue, exactWeightedValue);
                objectiveComponents.Add(objComponentValue);
            }

            int hardConstraintPriority = minPriority - 1;

            IObjective minGroupSizeObjective = new Interface.Implementation.Objective(ObjectiveType.MinGroupSizePenalty, hardConstraintPriority);
            ObjectiveData minGroupSizeObjectiveData = new ObjectiveData(minGroupSizeObjective, _input);
            long minGroupSizeObjectiveValue = CalculateMinGroupSizeConstraintPenalty();
            ObjectiveDataWithWeight minGroupSizeObjectiveDataWithWeight = new ObjectiveDataWithWeight(minGroupSizeObjectiveData, 1, _totalUpperBound + 1);
            long minGroupSizeWeightedValue = CalculateWeightedValue(minGroupSizeObjectiveValue, minGroupSizeObjectiveDataWithWeight);

            ObjComponentValue minGroupSizeObjComponentValue = new ObjComponentValue(ObjectiveType.MinGroupSizePenalty, minGroupSizeObjectiveValue, minGroupSizeWeightedValue, minGroupSizeObjectiveValue, minGroupSizeWeightedValue);
            objectiveComponents.Add(minGroupSizeObjComponentValue);

            return objectiveComponents;
        }

        private long CalculateMinGroupSizeConstraintPenalty()
        {
            IDictionary<int, IList<int>> penaltiesByType = new Dictionary<int, IList<int>>();

            foreach (IOrderType orderType in _input.OrderTypes)
            {
                penaltiesByType[orderType.Id] = new List<int>();
            }

            foreach (IPlanningGroup planningGroup in _output.PlanningGroups)
            {
                int firstOrderId = planningGroup.PlannedOrders[0].OrderId;
                IOrder firstOrder = _input.Orders.First(x => x.Id == firstOrderId);
                IOrderType orderType = _input.OrderTypes.First(x => x.Id == firstOrder.TypeId);
                int minGroupSize = orderType.MinGroupSize;

                IDictionary<int, int> resourceSizes = CalculateResourceSizes(planningGroup);

                int penalty = 0;

                foreach (int resourceSize in resourceSizes.Values)
                {
                    penalty += Math.Max(0, minGroupSize - resourceSize);
                }

                if (penalty > 0)
                {
                    penaltiesByType[orderType.Id].Add(penalty);
                }
            }

            int minGroupSizeCost = 0;
            foreach (IOrderType orderType in _input.OrderTypes)
            {
                minGroupSizeCost += penaltiesByType[orderType.Id].OrderByDescending(x => x).Skip(1).Sum();
            }

            return minGroupSizeCost;
        }

        private long CalculateObjectiveValue(ObjectiveData objectiveData)
        {
            IObjective objective = objectiveData.Objective;

            switch (objective.Type)
            {
                case ObjectiveType.OrderPriority:
                    (double cost, long roundedCost) = CalculateOrderPriorityObjective();
                    return roundedCost;
                case ObjectiveType.TargetGroupSizeDeviation:
                    return CalculateTargetGroupSizeDeviationObjective();
                case ObjectiveType.BalancedCapacityLoad:
                    return CalculateBalancedCapacityLoadObjective();
                case ObjectiveType.MinGroupSizePenalty:
                    return CalculateMinGroupSizeConstraintPenalty();
            }

            throw new InvalidEnumArgumentException(nameof(objective.Type));
        }

        private static long CalculateWeightedValue(long objectiveValue, ObjectiveDataWithWeight objectiveDataWithWeight)
        {
            return objectiveDataWithWeight.BigM * objectiveDataWithWeight.NormalizedWeight * objectiveValue;
        }

        private static double CalculateWeightedValue(double objectiveValue, ObjectiveDataWithWeight objectiveDataWithWeight)
        {
            return objectiveDataWithWeight.BigM * objectiveDataWithWeight.NormalizedWeight * objectiveValue;
        }

        /// <summary>
        /// Given a list objective components, calculate the aggregated objective function 
        /// = sum of all weighted objective components
        /// </summary>
        /// <param name="objectiveComponents">A list of objective components</param>
        /// <returns>the aggregated objective function value</returns>
        public static (double, long) CalculateAggregatedObjective(IList<ObjComponentValue> objectiveComponents)
        {
            return (objectiveComponents.Sum(x => x.ExactWeightedValue), objectiveComponents.Sum(x => x.WeightedValue));
        }

        /// <summary>
        /// Calculate a normalized aggregated objective value between 0 and 1 
        /// </summary>
        /// <param name="aggregatedObjectiveValue">The not normalized value</param>
        /// <returns></returns>
        public double CalculateNormalizedAggregatedObjective(long aggregatedObjectiveValue)
        {
            return (double)aggregatedObjectiveValue / _totalUpperBound;
        }

        /// <summary>
        /// Calculate a normalized aggregated objective value between 0 and 1 
        /// </summary>
        /// <param name="aggregatedObjectiveValue">The not normalized value</param>
        /// <returns></returns>
        public double CalculateNormalizedAggregatedObjective(double aggregatedObjectiveValue)
        {
            return aggregatedObjectiveValue / _totalUpperBound;
        }

        private (double, long) CalculateOrderPriorityObjective()
        {
            double cost = 0;
            long roundedCost = 0;

            foreach (IPlanningGroup planningGroup in _output.PlanningGroups)
            {
                long prioritySum = 0;
                foreach (IPlannedOrder plannedOrder in planningGroup.PlannedOrders)
                {
                    IOrder order = _input.Orders.First(x => x.Id == plannedOrder.OrderId);

                    prioritySum += order.Priority;
                }

                double averagePriority = (double)prioritySum / planningGroup.PlannedOrders.Count;

                long roundedPriorityDeviation = 0;
                double priorityDeviation = 0;
                foreach (IPlannedOrder plannedOrder in planningGroup.PlannedOrders)
                {
                    IOrder order = _input.Orders.First(x => x.Id == plannedOrder.OrderId);

                    priorityDeviation += Math.Abs(averagePriority - order.Priority);
                    roundedPriorityDeviation += Math.Abs((long)Math.Ceiling(averagePriority) - order.Priority);
                }

                cost += priorityDeviation;
                roundedCost += roundedPriorityDeviation;
            }

            return (cost, roundedCost);
        }

        private long CalculateTargetGroupSizeDeviationObjective()
        {
            long cost = 0;

            foreach (IPlanningGroup planningGroup in _output.PlanningGroups)
            {
                int firstOrderId = planningGroup.PlannedOrders[0].OrderId;
                IOrder firstOrder = _input.Orders.First(x => x.Id == firstOrderId);
                IOrderType orderType = _input.OrderTypes.First(x => x.Id == firstOrder.TypeId);
                int targetGroupSize = orderType.TargetGroupSize;

                IDictionary<int, int> resourceSizes = CalculateResourceSizes(planningGroup);

                foreach (int resourceSize in resourceSizes.Values)
                {
                    cost += Math.Abs(targetGroupSize - resourceSize);
                }
            }

            return cost;
        }

        private IDictionary<int, int> CalculateResourceSizes(IPlanningGroup planningGroup)
        {
            IDictionary<int, int> resourceSizes = new Dictionary<int, int>();

            foreach (IResource resource in _input.Resources)
            {
                resourceSizes.Add(resource.Id, 0);
            }

            foreach (IPlannedOrder plannedOrder in planningGroup.PlannedOrders)
            {
                resourceSizes[plannedOrder.ResourceId] += _input.Orders.First(x => x.Id == plannedOrder.OrderId).Quantity;
            }

            return resourceSizes;
        }

        private long CalculateBalancedCapacityLoadObjective()
        {
            long cost = 0;

            foreach (IPlanningGroup planningGroup in _output.PlanningGroups)
            {
                IDictionary<int, int> resourceSizes = CalculateResourceSizes(planningGroup);
                int planningGroupSize = resourceSizes.Values.Max();
                int planningGroupCost = 0;

                foreach (int resourceSize in resourceSizes.Values)
                {
                    planningGroupCost += planningGroupSize - resourceSize;
                }

                cost += planningGroupCost;
            }

            return cost;
        }
    }
}
