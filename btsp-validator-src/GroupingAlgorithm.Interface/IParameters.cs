namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// The parameters for solving the grouping problem.
    /// </summary>
    public interface IParameters
    {
        /// <summary>
        /// The maximum runtime in milliseconds.
        /// </summary>
        long MaxRunTimeMS { get; }
    }
}