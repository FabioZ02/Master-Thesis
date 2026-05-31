using System.Collections.Generic;

namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// A planning group.
    /// </summary>
    public interface IPlanningGroup
    {
        /// <summary>
        /// The rank which is assigned to the planning group.
        /// </summary>
        int Rank { get; set; }

        /// <summary>
        /// The orders which were planned for the planning group.
        /// </summary>
        IList<IPlannedOrder> PlannedOrders { get; }
    }
}
