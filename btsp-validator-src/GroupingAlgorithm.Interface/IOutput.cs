using System.Collections.Generic;

namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// An output.
    /// </summary>
    public interface IOutput
    {
        /// <summary>
        /// The planning groups.
        /// </summary>
        IList<IPlanningGroup> PlanningGroups { get; }
    }
}
