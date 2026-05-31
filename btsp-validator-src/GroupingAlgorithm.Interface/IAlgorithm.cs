using GroupingAlgorithm.Interface.Implementation;

namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// An algorithm for the grouping problem.
    /// </summary>
    public interface IAlgorithm
    {
        /// <summary>
        /// Solve the grouping problem.
        /// </summary>
        /// <param name="input">An input for the grouping problem.</param>
        /// <returns>The output of the algorithm.</returns>
        IOutput Solve(IInput input);
    }
}