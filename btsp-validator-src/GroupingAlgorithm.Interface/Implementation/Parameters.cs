namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// The parameters for solving the grouping problem.
    /// </summary>
    public class Parameters : IParameters
    {
        /// <summary>
        /// The maximum runtime in milliseconds.
        /// </summary>
        public long MaxRunTimeMS { get; set; }

        public Parameters(long maxRunTimeMS)
        {
            MaxRunTimeMS = maxRunTimeMS;
        }
    }
}