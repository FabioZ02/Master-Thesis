using System.Collections.Generic;

namespace GroupingAlgorithm.Interface
{
    public interface IInput
    {
        /// <summary>
        /// The order types.
        /// </summary>
        IList<IOrderType> OrderTypes { get; }

        /// <summary>
        /// The orders.
        /// </summary>
        IList<IOrder> Orders { get; }

        /// <summary>
        /// The resources.
        /// </summary>
        IList<IResource> Resources { get; }

        /// <summary>
        /// The objectives used for optimization.
        /// </summary>
        IList<IObjective> Objectives { get; }

        /// <summary>
        /// The mode used for the ranking of planning groups by order priorities.
        /// </summary>
        RankingMode RankingMode { get; }

        /// <summary>
        /// Parameters given as part of the input.
        /// </summary>
        Parameters Parameters { get; set; }

        IEnumerable<IInput> PartitionInput(bool partitionRuntime);
    }
}
