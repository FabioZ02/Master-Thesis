using System.Collections.Generic;
using System.Linq;

namespace GroupingAlgorithm.Validation.Objective
{
    /// <summary>
    /// A class for handling weight recalculations (normalizations)
    /// </summary>
    public static class WeightHandler
    {
        ///// <summary>
        ///// Calculate normalized weights for the given objectives
        ///// </summary>
        ///// <param name="objectiveData">The given objective data</param>
        ///// <param name="distinctBigMs">An empty list to be filled with the distinct bigM values</param>
        ///// <returns>A list of objective-weight-tuples</returns>
        //public static IList<ObjectiveDataWithWeight> CalculateNormalizedWeightsAndBigMs(IList<ObjectiveData> objectiveData, List<long> distinctBigMs)
        //{
        //    IList<ObjectiveDataWithWeight> objectiveDataWithWeights = new List <ObjectiveDataWithWeight>();

        //    distinctBigMs.Add(1);

        //    foreach (ObjectiveData objectiveDataEntry in objectiveData)
        //    {
        //        objectiveDataWithWeights.Add(new ObjectiveDataWithWeight(objectiveDataEntry, objectiveDataEntry.Objective.Weight, 1));
        //    }

        //    return objectiveDataWithWeights;
        //}

        /// <summary>
        /// Calculate normalized weights for the given objectives
        /// </summary>
        /// <param name="objectiveData">The given objective data</param>
        /// <param name="distinctBigMs">An empty list to be filled with the distinct bigM values</param>
        /// <returns>A list of objective-weight-tuples</returns>
        public static IList<ObjectiveDataWithWeight> CalculateNormalizedWeightsAndBigMs(IList<ObjectiveData> objectiveData, List<long> distinctBigMs)
        {
            // Group ObjectiveData by priorities
            IEnumerable<IGrouping<int, ObjectiveData>> groupedObjectiveData =
                objectiveData.GroupBy(x => x.Objective.Priority);
            // Order groups by descending priority
            IOrderedEnumerable<IGrouping<int, ObjectiveData>> orderedGroupedObjectiveData =
                groupedObjectiveData.OrderByDescending(x => x.Key);
            // Cast groups to simple lists of objectiveData
            IEnumerable<IList<ObjectiveData>> objectivesByDescPriority =
                orderedGroupedObjectiveData.Select(x => x.ToList());
            long bigMFactor = 1;
            IList<ObjectiveDataWithWeight> objectiveDataWithWeights = new List<ObjectiveDataWithWeight>();
            foreach (IList<ObjectiveData> objectiveDataForCertainPriority in objectivesByDescPriority)
            {
                distinctBigMs.Insert(0, bigMFactor);
                foreach (ObjectiveData objectiveDataEntry in objectiveDataForCertainPriority)
                {
                    objectiveDataWithWeights.Add(new ObjectiveDataWithWeight(objectiveDataEntry, objectiveDataEntry.Objective.Weight, bigMFactor));
                }
                bigMFactor *= objectiveDataForCertainPriority.Sum(x => x.Objective.Weight);
            }
            return objectiveDataWithWeights;
        }

    }
}

