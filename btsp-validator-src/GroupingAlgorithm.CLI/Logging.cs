using NLog;
using NLog.Config;
using NLog.Layouts;
using NLog.Targets;
using System;

namespace GroupingAlgorithm.TabuSearchCLI
{
    public static class Logging
    {
        /// <summary>
        /// Set up the logger.
        /// </summary>
        public static void SetupLogger(int verbosity, bool showTime)
        {
            // convert the verbosity level to minimum log level:
            // 1   = Debug
            // 2   = Trace
            // ... = Info

            LogLevel minLogLevel;

            switch (verbosity)
            {
                case 1:
                    minLogLevel = LogLevel.Debug;
                    break;
                case 2:
                    minLogLevel = LogLevel.Trace;
                    break;
                default:
                    minLogLevel = LogLevel.Info;
                    break;

            }

            // create three targets: Logs >= ERROR should be redirected to STDERR
            //                        Logs >= ERROR should be redirected to logFile
            //                          Logs < ERROR to STDOUT
            // and set up the layout to optionally display time
            string layout = showTime ? "[${shortdate} ${time}]\t${message}" : "${message}";
            //write to file as well
            FileTarget logFile = new FileTarget("Logfile")
            {
                //TODO adapt to current instance?
                //logger adds stuff at the end of localsearchLogfile.txt (if file exists already)
                FileName = @"C:\Projekte\EmployeeAssignmentAlgorithm\Logfiles\Logfile" + DateTime.Now + ".txt",
                Layout = new SimpleLayout(layout)
            };
            ConsoleTarget stdTarget = new ConsoleTarget("out")
            {
                Error = false,
                Layout = new SimpleLayout(layout)
            };
            ConsoleTarget errTarget = new ConsoleTarget("error")
            {
                Error = true,
                Layout = new SimpleLayout(layout)
            };

            LoggingConfiguration cfg = new LoggingConfiguration();
            cfg.AddRule(minLogLevel, LogLevel.Warn, logFile);
            cfg.AddRule(minLogLevel, LogLevel.Warn, stdTarget);
            cfg.AddRule(LogLevel.Error, LogLevel.Fatal, errTarget);

            LogManager.Configuration = cfg;
        }
    }
}