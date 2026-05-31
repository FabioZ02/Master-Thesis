namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// The type of an order.
    /// </summary>
    public interface IOrderType
    {
        /// <summary>
        /// The ID of the order type.
        /// </summary>
        int Id { get; }

        /// <summary>
        /// The targeted group size.
        /// </summary>
        int TargetGroupSize { get; }

        /// <summary>
        /// The minimal group size.
        /// </summary>
        int MinGroupSize { get; }

        /// <summary>
        /// The maximal group size.
        /// </summary>
        int MaxGroupSize { get; set; }
    }
}
