namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// The objective types.
    /// </summary>
    public enum ObjectiveType
    {
        /// <summary>
        /// Optimization of the order priorities.
        /// </summary>
        OrderPriority = 0,

        /// <summary>
        /// Minimize the deviation from the target group sizes.
        /// </summary>
        TargetGroupSizeDeviation = 1,

        /// <summary>
        /// Minimize the deviation of capacity loads of resources within the single planning groups.
        /// </summary>
        BalancedCapacityLoad = 2,

        /// <summary>
        /// DO NOT USE IN INSTANCE!
        /// </summary>
        MinGroupSizePenalty = 3
    }
}