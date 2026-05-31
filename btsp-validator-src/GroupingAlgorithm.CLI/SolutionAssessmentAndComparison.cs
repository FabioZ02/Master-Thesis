using GroupingAlgorithm.Interface;
using GroupingAlgorithm.Interface.Implementation;
using GroupingAlgorithm.Validation;
using System;
using System.Collections.Generic;
using System.Text;

namespace GroupingAlgorithm.TabuSearchCLI
{
    public static class SolutionAssessmentAndComparison
    {
        /// <summary>
        /// Validate solution (=determine constraint violations) and calculate solution costs
        /// </summary>
        /// <param name="input"></param>
        /// <param name="output"></param>
        /// <returns></returns>
        public static string ValidateAndAssessSolution(IInput input, IOutput output)
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendLine("---------------------");
            sb.AppendLine("Calculating values of objective components:");
            ObjectiveCalculator objectiveCalculator = new ObjectiveCalculator(input, output);
            IList<ObjComponentValue> objectiveComponents = objectiveCalculator.CalculateObjectiveComponents();
            foreach (ObjComponentValue component in objectiveComponents)
            {
                sb.AppendLine($"The absolute (weighted) value of the {component.ObjComponentType} objective is: {component.AbsoluteValue} ({component.WeightedValue})");
                sb.AppendLine($"The exact absolute (weighted) value of the {component.ObjComponentType} objective is: {component.ExactAbsoluteValue} ({component.ExactWeightedValue})");
            }

            (double exactAggregatedObj, long aggregatedObj) = ObjectiveCalculator.CalculateAggregatedObjective(objectiveComponents);
            double normalizedObj = objectiveCalculator.CalculateNormalizedAggregatedObjective(aggregatedObj);
            double exactNormalizedObj = objectiveCalculator.CalculateNormalizedAggregatedObjective(exactAggregatedObj);

            sb.AppendLine($"The integer (normalized) value of the aggregated objective is: {aggregatedObj} ({normalizedObj})");
            sb.AppendLine($"The double (normalized) value of the aggregated objective is: {exactAggregatedObj} ({exactNormalizedObj})");

            Console.WriteLine(sb.ToString());

            return sb.ToString();
        }
    }
}
