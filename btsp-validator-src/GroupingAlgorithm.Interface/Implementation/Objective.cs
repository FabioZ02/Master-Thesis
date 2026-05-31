using System;
using System.Text.Json.Serialization;

namespace GroupingAlgorithm.Interface.Implementation
{
    public class Objective : IObjective
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

        /// <summary>
        /// Create an objective type object
        /// </summary>
        /// <param name="priority">The priority of the objective (default = 1)</param>
        /// <param name="weight">The weight of the objective (default = 1)</param>
        /// <param name="type">The type of the objective</param>
        [JsonConstructor]
        public Objective(ObjectiveType type, int priority = 1, int weight = 1)
        {
            if (priority < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(priority), $"{nameof(priority)} must be >= 0.");
            }

            if (weight <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(weight), $"{nameof(weight)} must be >= 1.");
            }

            Type = type;
            Priority = priority;
            Weight = weight;
        }
    }
}
