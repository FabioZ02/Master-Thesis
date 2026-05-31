using System.Collections.Generic;

namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// An order.
    /// </summary>
    public interface IOrder
    {
        /// <summary>
        /// The ID of the order.
        /// </summary>
        int Id { get; }

        /// <summary>
        /// The ID of the order type.
        /// </summary>
        int TypeId { get; }

        /// <summary>
        /// The quantity of the order.
        /// </summary>
        int Quantity { get; }

        /// <summary>
        /// The priority of the order, where 1 is the most important one.
        /// </summary>
        int Priority { get; }

        /// <summary>
        /// The IDs of the resources on which the order can be planned.
        /// </summary>
        List<int> ValidResourceIds { get; }
    }
}
