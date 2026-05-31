using GroupingAlgorithm.Interface;

namespace GroupingAlgorithm.Validation
{
    /// <summary>
    /// Data type to store validation results for an objective component
    /// </summary>
    public struct ObjComponentValue
    {
        /// <summary>
        /// The type of the corresponding objective.
        /// </summary>
        public readonly ObjectiveType ObjComponentType;

        /// <summary>
        /// The absolute cost value.
        /// </summary>
        public readonly long AbsoluteValue;

        /// <summary>
        /// The weighted cost value.
        /// </summary>
        public readonly long WeightedValue;

        public readonly double ExactAbsoluteValue;

        public readonly double ExactWeightedValue;

        /// <summary>
        /// Data type to store validation results for an objective component
        /// </summary>
        /// <param name="objComponentType">The type of the corresponding objective </param>
        /// <param name="absoluteValue">The absolute cost value</param>
        /// <param name="weightedValue">The weighted cost value</param>
        public ObjComponentValue(ObjectiveType objComponentType, long absoluteValue, long weightedValue, double exactAbsoluteValue, double exactWeightedValue)
        {
            ObjComponentType = objComponentType;
            AbsoluteValue = absoluteValue;
            WeightedValue = weightedValue;
            ExactAbsoluteValue = exactAbsoluteValue;
            ExactWeightedValue = exactWeightedValue;
        }
    }
}