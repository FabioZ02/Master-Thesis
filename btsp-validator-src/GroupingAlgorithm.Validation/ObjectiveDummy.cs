using GroupingAlgorithm.Interface;

namespace GroupingAlgorithm.ValidationAndObjective
{
    public class ObjectiveDummy : IObjective
    {
        /// <summary>
        /// The priority of the objective, which determines the lexicographic ordering of this objective in the minimization function
        /// (1 is the most important priority) 
        /// </summary>
        public int Priority { get; }

        /// <summary>
        /// The linear weight of the objective
        /// </summary>
        public int Weight { get; }

        /// <summary>
        /// The type of the objective
        /// </summary>
        public ObjectiveType Type { get; }

        public ObjectiveDummy(ObjectiveType type)
        {
            Type = type;
            Priority = 0;
            Weight = 0;
        }
    }
}