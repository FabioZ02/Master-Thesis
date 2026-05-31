namespace GroupingAlgorithm.Interface
{
    /// <summary>
    /// Possible types of solution status
    /// </summary>
    public enum SolutionStatus
    {
        /// <summary>
        /// solution was proven to be optimal by minizinc
        /// </summary>
        OptimalSolutionFound,

        /// <summary>
        /// solution found but optimality not proven
        /// </summary>
        SolutionFound,

        /// <summary>
        /// Instance is unsatifiable
        /// </summary>
        Unsatisfiable,

        /// <summary>
        /// No solution found within timelimit
        /// </summary>
        NoSolutionFound,

        /// <summary>
        /// Solution status is unknown
        /// </summary>
        Unknown
    }
}
