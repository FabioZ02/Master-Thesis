using GroupingAlgorithm.Interface;

namespace GroupingAlgorithm.Validation.Objective
{
    /// <summary>
    /// An Object containing ObjectiveData, a normalized weight and big M information for the objective data depending on all used objective data
    /// </summary>
    public class ObjectiveDataWithWeight
    {
        /// <summary>
        /// The objective data
        /// </summary>
        public ObjectiveData ObjectiveData { get; }

        /// <summary>
        /// The type of the objective
        /// </summary>
        public ObjectiveType ObjectiveType => ObjectiveData.Objective.Type;

        /// <summary>
        /// The normalized weight compared with other objective data with same priority
        /// </summary>
        public long NormalizedWeight { get; }

        /// <summary>
        /// The big M used when comparing with other objective data with different priority
        /// </summary>
        public long BigM { get; }

        /// <summary>
        /// Create an ObjectiveDataWithWeight object
        /// </summary>
        /// <param name="objectiveData">The given objective data</param>
        /// <param name="normalizedWeight">The calculated normalized weight</param>
        /// <param name="bigM">The calculated big M value</param>
        public ObjectiveDataWithWeight(
            ObjectiveData objectiveData,
            long normalizedWeight,
            long bigM)
        {
            ObjectiveData = objectiveData;
            NormalizedWeight = normalizedWeight;
            BigM = bigM;
        }
    }
}