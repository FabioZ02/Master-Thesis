namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// An order which is planned on a resource.
    /// </summary>
    public interface IPlannedOrder
    {
        /// <summary>
        /// The ID of the order which is planned on a resource.
        /// </summary>
        int OrderId { get; }

        /// <summary>
        /// The ID of the resource on which the order was planned.
        /// </summary>
        int ResourceId { get; }
    }
}
