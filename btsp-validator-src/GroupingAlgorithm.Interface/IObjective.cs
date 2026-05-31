namespace GroupingAlgorithm.Interface
{
    public interface IObjective
    {
        /// <summary>
        /// The priority of the objective, which determines the lexicographic ordering of this objective in the minimization function
        /// (1 is the most important priority) 
        /// </summary>
        int Priority { get; }

        /// <summary>
        /// The linear weight of the objective
        /// </summary>
        int Weight { get; }

        /// <summary>
        /// The type of the objective
        /// </summary>
        ObjectiveType Type { get; }
    }
}
