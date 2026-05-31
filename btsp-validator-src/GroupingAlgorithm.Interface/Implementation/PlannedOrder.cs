namespace GroupingAlgorithm.Interface.Implementation
{
    /// <summary>
    /// An order which is planned on a resource.
    /// </summary>
    public class PlannedOrder : IPlannedOrder
    {
        /// <summary>
        /// The ID of the order which is planned on a resource.
        /// </summary>
        public int OrderId { get; set; }

        /// <summary>
        /// The ID of the resource on which the order was planned.
        /// </summary>
        public int ResourceId { get; }

        /// <summary>
        /// Create a new planned order.
        /// </summary>
        /// <param name="orderId">The ID of the order which is planned on a resource.</param>
        /// <param name="resourceId">The ID of the resource on which the order was planned.</param>
        public PlannedOrder(int orderId, int resourceId)
        {
            OrderId = orderId;
            ResourceId = resourceId;
        }
    }
}
