using CommandLine;

namespace GroupingAlgorithm.CLI
{
    internal class Options
    {
        [Option('i', HelpText = "Instance file", Required = true)]
        public required string InstanceFile { get; set; }

        [Option('s', HelpText = "Solution file", Required = true)]
        public required string SolutionFile { get; set; }
    }
}
