using CommandLine;
using GroupingAlgorithm.Interface;
using GroupingAlgorithm.Interface.Implementation;
using GroupingAlgorithm.CLI;
using GroupingAlgorithm.Validation;
using System.Text.Json;
using System.Text.Json.Serialization;
using GroupingAlgorithm.TabuSearchCLI;
using GroupingAlgorithm.Interface;
using GroupingAlgorithm.Interface.Implementation;
using GroupingAlgorithm.CLI;
using GroupingAlgorithm.Validation;

internal static class Program
{
    static void Main(string[] args)
    {
        // Parse and process command line arguments
        Parser.Default.ParseArguments<Options>(args)
            .WithParsed(RunProgram)
            .WithNotParsed(HandleParseError);
    }

    private static void HandleParseError(IEnumerable<Error> errs)
    {
    }

    private static void RunProgram(Options opts)
    {
        Logging.SetupLogger(1, true);

        IInput input = GetInputFromFile(opts.InstanceFile);
        IOutput output = DeserializeOutput(opts.SolutionFile);

        SerializeOutput(output);

        ConstraintChecker.CheckConstraints(input, output);
        SolutionAssessmentAndComparison.ValidateAndAssessSolution(input, output);

        ObjectiveCalculator objectiveCalculator = new ObjectiveCalculator(input, output);
        IList<ObjComponentValue> objectiveComponents = objectiveCalculator.CalculateObjectiveComponents();
        (double exactAggregatedObj, long aggregatedObj) = ObjectiveCalculator.CalculateAggregatedObjective(objectiveComponents);
        double normalizedObj = objectiveCalculator.CalculateNormalizedAggregatedObjective(aggregatedObj);
        double exactNormalizedObj = objectiveCalculator.CalculateNormalizedAggregatedObjective(exactAggregatedObj);

        Console.Write(aggregatedObj);
    }

    private static IInput GetInputFromFile(string filename)
    {
        Input jsonInput = JsonSerializer.Deserialize<Input>(File.ReadAllText(filename),
            new JsonSerializerOptions
            {
                Converters = { new JsonStringEnumConverter(JsonNamingPolicy.CamelCase) }
            }
        );
        return jsonInput;
    }

    private static void SerializeOutput(IOutput output)
    {
        JsonSerializerOptions options = new JsonSerializerOptions()
        {
            Converters = { new JsonStringEnumConverter() },
            WriteIndented = true
        };

        string jsonString = JsonSerializer.Serialize(output, options);

        Console.WriteLine(jsonString);
    }

    private static void SerializeInput(Input input, string fileName)
    {
        JsonSerializerOptions options = new JsonSerializerOptions()
        {
            Converters = { new JsonStringEnumConverter(JsonNamingPolicy.CamelCase) },
            WriteIndented = true
        };
        string jsonString = JsonSerializer.Serialize(input, options);

        File.WriteAllText(fileName, jsonString);
    }

    private static IOutput DeserializeOutput(string fileName)
    {
        string jsonString = File.ReadAllText(fileName);
        var output = JsonSerializer.Deserialize<Output>(jsonString);

        return output;
    }
}
