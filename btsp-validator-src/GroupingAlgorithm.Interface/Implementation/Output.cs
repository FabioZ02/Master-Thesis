using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text.Json.Serialization;

namespace GroupingAlgorithm.Interface.Implementation
{
    /// <summary>
    /// An output.
    /// </summary>
    public class Output : IOutput
    {
        [JsonPropertyName("PlanningGroups")]
        public List<PlanningGroup> JsonPlanningGroups { get; private set; }

        /// <summary>
        /// The planning groups.
        /// </summary>
        [JsonIgnore]
        public IList<IPlanningGroup> PlanningGroups { get; private set; }

        /// <summary>
        /// Create an output.
        /// </summary>
        /// <param name="planningGroups">The list of planning groups.</param>
        public Output(List<IPlanningGroup> planningGroups)
        {
            if (planningGroups == null)
            {
                throw new ArgumentNullException(nameof(planningGroups));
            }

            PlanningGroups = planningGroups;
            JsonPlanningGroups = planningGroups.Select(x => new PlanningGroup(x.Rank, x.PlannedOrders)).ToList();
        }

        /// <summary>
        /// Create an output.
        /// </summary>
        /// <param name="planningGroups">The list of planning groups.</param>
        [JsonConstructor]
        public Output(List<PlanningGroup> jsonPlanningGroups)
        {
            if (jsonPlanningGroups == null)
            {
                throw new ArgumentNullException(nameof(jsonPlanningGroups));
            }

            JsonPlanningGroups = jsonPlanningGroups;
            PlanningGroups = jsonPlanningGroups.Select(x => (IPlanningGroup)x).ToList();
        }

        public void RankPlanningGroups(IInput input)
        {
            IDictionary<int, int> orderPrioritiesByOrderId = BuildOrderPriorityDictionary(input);

            // Ranking of planning groups
            switch (input.RankingMode)
            {
                case RankingMode.Minimum:
                    RankPlanningGroupsByMinimalPriority(orderPrioritiesByOrderId);
                    break;
                case RankingMode.Average:
                    RankPlanningGroupsByAveragePriority(orderPrioritiesByOrderId);
                    break;
                default:
                    throw new InvalidEnumArgumentException(nameof(input.RankingMode));
            }

            int planningGroupRank = 0;
            foreach (IPlanningGroup planningGroup in PlanningGroups)
            {
                planningGroup.Rank = planningGroupRank;
                planningGroupRank++;
            }
        }

        private IDictionary<int, int> BuildOrderPriorityDictionary(IInput input)
        {
            IDictionary<int, int> orderPrioritiesByOrderId = new Dictionary<int, int>();

            foreach (IOrder order in input.Orders)
            {
                orderPrioritiesByOrderId.Add(order.Id, order.Priority);
            }

            return orderPrioritiesByOrderId;
        }

        /// <summary>
        /// Rank the planning groups by the minimal order priority.
        /// </summary>
        /// <param name="planningGroups">The planning groups which shall be ranked.</param>
        /// <param name="orderPrioritiesByOrderId">A dictionary to retrieve the priorities by the ID of the order.</param>
        private void RankPlanningGroupsByMinimalPriority(IDictionary<int, int> orderPrioritiesByOrderId)
        {
            IDictionary<int, int> minimalPriorityByPlanningGroupRank = new Dictionary<int, int>();

            foreach (IPlanningGroup planningGroup in PlanningGroups)
            {
                minimalPriorityByPlanningGroupRank[planningGroup.Rank] = planningGroup.PlannedOrders.Min(x => orderPrioritiesByOrderId[x.OrderId]);
            }

            JsonPlanningGroups.Sort((x, y) => minimalPriorityByPlanningGroupRank[x.Rank].CompareTo(minimalPriorityByPlanningGroupRank[y.Rank]));
            PlanningGroups = JsonPlanningGroups.Select(x => (IPlanningGroup)x).ToList();
        }

        /// <summary>
        /// Rank the planning groups by the average order priority.
        /// </summary>
        /// <param name="planningGroups">The planning groups which shall be ranked.</param>
        /// <param name="orderPrioritiesByOrderId">A dictionary to retrieve the priorities by the ID of the order.</param>
        private void RankPlanningGroupsByAveragePriority(IDictionary<int, int> orderPrioritiesByOrderId)
        {
            IDictionary<int, double> minimalPriorityByPlanningGroupRank = new Dictionary<int, double>();

            foreach (IPlanningGroup planningGroup in PlanningGroups)
            {
                minimalPriorityByPlanningGroupRank[planningGroup.Rank] = planningGroup.PlannedOrders.Average(x => orderPrioritiesByOrderId[x.OrderId]);
            }

            JsonPlanningGroups.Sort((x, y) => minimalPriorityByPlanningGroupRank[x.Rank].CompareTo(minimalPriorityByPlanningGroupRank[y.Rank]));
            PlanningGroups = JsonPlanningGroups.Select(x => (IPlanningGroup)x).ToList();
        }
    }
}
