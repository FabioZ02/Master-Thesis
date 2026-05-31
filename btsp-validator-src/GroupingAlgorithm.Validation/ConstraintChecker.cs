using GroupingAlgorithm.Interface;
using GroupingAlgorithm.Interface.Implementation;
using System;
using System.Collections.Generic;
using System.Linq;

namespace GroupingAlgorithm.Validation
{
    public static class ConstraintChecker
    {
        public static void CheckConstraints(IInput input, IOutput output)
        {
            IDictionary<int, IOrderType> orderTypeByOrderIdDictionary = GetOrderTypeByOrderId(input);
            IDictionary<int, IOrder> orderByIdDictionary = GetOrderById(input);

            CheckOrderOccurrences(input, output);
            CheckHomogeneityOfPlanningGroups(input, output, orderTypeByOrderIdDictionary);
            CheckValidResources(input, output, orderByIdDictionary);
            CheckGroupSizeConstraints(input, output, orderTypeByOrderIdDictionary, orderByIdDictionary);
        }

        /// <summary>
        /// Checks that each order is planned exactly once.
        /// </summary>
        /// <param name="input">The input to the algorithm.</param>
        /// <param name="output">The output of the algorithm.</param>
        private static void CheckOrderOccurrences(IInput input, IOutput output)
        {
            foreach (IOrder order in input.Orders)
            {
                int count = output.PlanningGroups.SelectMany(x => x.PlannedOrders).Count(x => x.OrderId == order.Id);

                if (count == 0)
                {
                    Console.WriteLine($"Order {order.Id} is not planned!");
                }
                else if (count > 1)
                {
                    Console.WriteLine($"Order {order.Id} is planned {count} times!");
                }
            }
        }

        /// <summary>
        /// Checks whether orders in a planning group are of the same type.
        /// </summary>
        /// <param name="input">The input to the algorithm.</param>
        /// <param name="output">The output of the algorithm.</param>
        /// <param name="orderTypeByOrderIdDictionary">A dictionary to get the type of an order by its ID.</param>
        private static void CheckHomogeneityOfPlanningGroups(IInput input, IOutput output, IDictionary<int, IOrderType> orderTypeByOrderIdDictionary)
        {
            foreach (IPlanningGroup planningGroup in output.PlanningGroups)
            {
                IOrderType planningGroupOrderType = orderTypeByOrderIdDictionary[planningGroup.PlannedOrders[0].OrderId];

                bool isHomogeneous = true;
                foreach (IPlannedOrder plannedOrder in planningGroup.PlannedOrders)
                {
                    IOrderType orderType = orderTypeByOrderIdDictionary[plannedOrder.OrderId];

                    if (orderType.Id != planningGroupOrderType.Id)
                    {
                        isHomogeneous = false;
                        break;
                    }
                }

                if (!isHomogeneous)
                {
                    Console.WriteLine($"Planning group with rank {planningGroup.Rank} contains orders of different types!");
                }
            }
        }

        private static void CheckValidResources(IInput input, IOutput output, IDictionary<int, IOrder> orderByOrderId)
        {
            foreach (IPlanningGroup planningGroup in output.PlanningGroups)
            {
                foreach (IPlannedOrder plannedOrder in planningGroup.PlannedOrders)
                {
                    IOrder order = orderByOrderId[plannedOrder.OrderId];

                    if (!order.ValidResourceIds.Contains(plannedOrder.ResourceId))
                    {
                        Console.WriteLine($"Order {order.Id} is planned on resource {plannedOrder.ResourceId} which is not contained in list of valid resources!");
                    }
                }
            }
        }

        private static void CheckGroupSizeConstraints(IInput input, IOutput output, IDictionary<int, IOrderType> orderTypeByOrderId, IDictionary<int, IOrder> orderById)
        {
            IDictionary<int, int> planningGroupWithMinSizeViolationsByTypeId = new Dictionary<int, int>();
            foreach (IOrderType orderType in input.OrderTypes)
            {
                planningGroupWithMinSizeViolationsByTypeId.Add(orderType.Id, 0);
            }

            IDictionary<int, IList<(int resourceId, int resourceSize)>> minSizeViolationsByPlanningGroupRank = new Dictionary<int, IList<(int resourceId, int resourceSize)>>();
            foreach (IPlanningGroup planningGroup in output.PlanningGroups)
            {
                minSizeViolationsByPlanningGroupRank.Add(planningGroup.Rank, new List<(int resourceId, int resourceSize)>());
            }

            foreach (IPlanningGroup planningGroup in output.PlanningGroups)
            {
                bool hasPlanningGroupMinSizeViolation = false;

                IOrderType orderType = orderTypeByOrderId[planningGroup.PlannedOrders[0].OrderId];
                IDictionary<int, int> resourceSizesById = new Dictionary<int, int>();

                foreach (IResource resource in input.Resources)
                {
                    resourceSizesById.Add(resource.Id, 0);
                }

                foreach (IPlannedOrder plannedOrder in planningGroup.PlannedOrders)
                {
                    IOrder order = input.Orders.First(x => x.Id == plannedOrder.OrderId);

                    resourceSizesById[plannedOrder.ResourceId] += order.Quantity;
                }

                foreach (IResource resource in input.Resources)
                {
                    if (resourceSizesById[resource.Id] < orderType.MinGroupSize)
                    {
                        minSizeViolationsByPlanningGroupRank[planningGroup.Rank].Add((resource.Id, resourceSizesById[resource.Id]));
                        hasPlanningGroupMinSizeViolation = true;
                    }

                    if (resourceSizesById[resource.Id] > orderType.MaxGroupSize)
                    {
                        Console.WriteLine($"Resource {resource.Id} of planning group with rank {planningGroup.Rank} has size {resourceSizesById[resource.Id]} but maximal group size is {orderType.MaxGroupSize}!");
                    }
                }

                if (hasPlanningGroupMinSizeViolation)
                {
                    planningGroupWithMinSizeViolationsByTypeId[orderType.Id] += 1;
                }
            }

            foreach (IOrderType orderType in input.OrderTypes)
            {
                if (planningGroupWithMinSizeViolationsByTypeId[orderType.Id] >= 2)
                {
                    foreach (IPlanningGroup planningGroup in output.PlanningGroups)
                    {
                        IOrderType planningGroupOrderType = orderTypeByOrderId[planningGroup.PlannedOrders[0].OrderId];

                        if (orderType.Id == planningGroupOrderType.Id)
                        {
                            foreach ((int resourceId, int resourceSize) in minSizeViolationsByPlanningGroupRank[planningGroup.Rank])
                            {
                                Console.WriteLine($"Resource {resourceId} of planning group with rank {planningGroup.Rank} has size {resourceSize} but minimal group size is {orderType.MinGroupSize}!");
                            }
                        }
                    }
                }
            }
        }

        private static int CalculatePlanningGroupSize(IInput input, IPlanningGroup planningGroup, IDictionary<int, IOrder> orderById)
        {
            IDictionary<int, int> quantitiesByResourceId = new Dictionary<int, int>();

            foreach (IResource resource in input.Resources)
            {
                quantitiesByResourceId[resource.Id] = 0;
            }

            foreach (IPlannedOrder plannedOrder in planningGroup.PlannedOrders)
            {
                IOrder order = orderById[plannedOrder.OrderId];

                quantitiesByResourceId[plannedOrder.ResourceId] += order.Quantity;
            }

            return quantitiesByResourceId.Max(x => x.Value);
        }
        private static IDictionary<int, IOrderType> GetOrderTypeByOrderId(IInput input)
        {
            IDictionary<int, IOrderType> orderTypeByOrderId = new Dictionary<int, IOrderType>();

            foreach (IOrder order in input.Orders)
            {
                orderTypeByOrderId[order.Id] = input.OrderTypes.First(x => x.Id == order.TypeId);
            }

            return orderTypeByOrderId;
        }

        private static IDictionary<int, IOrder> GetOrderById(IInput input)
        {
            IDictionary<int, IOrder> orderById = new Dictionary<int, IOrder>();

            foreach (IOrder order in input.Orders)
            {
                orderById[order.Id] = order;
            }

            return orderById;
        }
    }
}