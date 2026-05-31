using System.Collections.Generic;
using System.Linq;
using System.Text.Json.Serialization;

namespace GroupingAlgorithm.Interface.Implementation
{
    /// <summary>
    /// A planning group.
    /// </summary>
    public class PlanningGroup : IPlanningGroup
    {
        /// <summary>
        /// The rank which is assigned to the planning group.
        /// </summary>
        public int Rank { get; set; }

        /// <summary>
        /// The orders which were planned for the planning group.
        /// </summary>
        [JsonIgnore]
        public IList<IPlannedOrder> PlannedOrders { get; }

        /// <summary>
        /// The orders which were planned for the planning group.
        /// </summary>
        [JsonPropertyName("PlannedOrders")]
        public List<PlannedOrder> JsonPlannedOrders { get; }

        /// <summary>
        /// Create a new planning group.
        /// </summary>
        /// <param name="rank">The rank which is assigned to the planning group.</param>
        /// <param name="orderIds">The order IDs which were assigned to the planning group.</param>
        public PlanningGroup(int rank, IList<IPlannedOrder> plannedOrders)
        {
            Rank = rank;
            PlannedOrders = plannedOrders;
            JsonPlannedOrders = plannedOrders.Select(x => new PlannedOrder(x.OrderId, x.ResourceId)).ToList();
        }

        [JsonConstructor]
        public PlanningGroup(int rank, List<PlannedOrder> jsonPlannedOrders)
        {
            Rank = rank;
            JsonPlannedOrders = jsonPlannedOrders;
            PlannedOrders = jsonPlannedOrders.Select(x => (IPlannedOrder)x).ToList();
        }
    }
}
