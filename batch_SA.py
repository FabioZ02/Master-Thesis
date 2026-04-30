#!/usr/bin/env python3
import os
from datetime import date
import random
import math

# list of names of instances
instances = ["wlp1.dzn", "wlp2.dzn", "wlp3.dzn", "wlp4.dzn", "wlp5.dzn",
             "wlp6.dzn", "wlp7.dzn", "wlp8.dzn", "wlp9.dzn", "wlp10.dzn",
             "wlp12.dzn", "wlp15.dzn", "wlp20.dzn", "wlp30.dzn", "wlp50.dzn",
              "wlp100.dzn"]

# number of repetitions
repetitions = 1

# directory of the instances
instance_directory = "./Instances"

# directory of the solutions
solution_directory = "Solutions"
solution_extension = "txt"

# total number of iterations
total_iterations = 500000

# ALGORITHM PARAMETERS
start_temp_array = [5.0, 7.5, 10.0]  
min_temp_array = [0.02, 0.05, 0.1]
cooling_rate_array = [0.98, 0.99, 0.995] 

today = date.today()
log_file = "Log_" + today.isoformat() + ".log"
f = open(log_file,"a")
f.write("Trial Instance Cost Seed Time Start_Temp Min_Temp Cooling_Rate Status\n")
f.close()

def parse_solution_file(sol_file):
    """Parse solution file safely"""
    try:
        with open(sol_file, 'r') as f:
            lines = f.readlines()
        
        if len(lines) < 3:
            print(f"WARNING: File {sol_file} has only {len(lines)} lines")
            return "ERROR", "ERROR"
        
        # Try to find Cost and Time lines
        cost = "ERROR"
        time = "ERROR"
        
        for line in lines:
            line = line.strip()
            if line.startswith("Cost:"):
                cost_parts = line.split()
                if len(cost_parts) >= 2:
                    cost = cost_parts[1]
            elif line.startswith("Time:"):
                time_parts = line.split()
                if len(time_parts) >= 2:
                    time = time_parts[1].rstrip('s')  # Remove 's' if present
        
        return cost, time
        
    except FileNotFoundError:
        print(f"ERROR: File {sol_file} not found!")
        return "FILE_NOT_FOUND", "FILE_NOT_FOUND"
    except Exception as e:
        print(f"ERROR reading file {sol_file}: {e}")
        return "READ_ERROR", "READ_ERROR"

for i in range(repetitions):
    for instance in instances:
        in_file_path = instance_directory + "/" + instance
        if os.path.isfile(in_file_path):
            for st in range(len(start_temp_array)):
                for mt in range(len(min_temp_array)):
                    for cr in range(len(cooling_rate_array)):
                        seed = random.randint(0,100000000)
                        start_temp = start_temp_array[st]
                        min_temp = min_temp_array[mt]
                        cooling_rate = cooling_rate_array[cr]
                        number_of_temperatures = math.log(min_temp/start_temp)/math.log(cooling_rate)              
                        neighbors_sampled = int(round(total_iterations/number_of_temperatures))
                        
                        if not os.path.isdir(solution_directory):
                            os.mkdir(solution_directory) 
                        
                        # the solution file
                        sol_file = solution_directory + "/sol-" + instance + \
                             "-" + str(st) + "-" + str(mt) + "-" + str(cr)  + "_" + \
                             str(i) + "." + solution_extension
                         
                        # command line
                        command = "./wlp" + \
                        " --main::instance " +  in_file_path  + \
                        " --main::method SA" + \
                        " --SA::max_evaluations " + str(total_iterations) + \
                        " --SA::compute_start_temperature-disable" + \
                        " --SA::start_temperature " + str(start_temp) + \
                        " --SA::min_temperature " + str(min_temp) + \
                        " --SA::cooling_rate " + str(cooling_rate) + \
                        " --main::seed " + str(seed) + \
                        " --main::output_file " + sol_file
                        
                        print(command + "\n")
                        
                        # Execute command and check return code
                        return_code = os.system(command)
                        
                        if return_code != 0:
                            print(f"WARNING: Command failed with return code {return_code}")
                            status = "EXEC_ERROR"
                            cost, time = "ERROR", "ERROR"
                        else:
                            print("Processing solution file: " + sol_file)
                            cost, time = parse_solution_file(sol_file)
                            status = "OK" if cost != "ERROR" and time != "ERROR" else "PARSE_ERROR"
                        
                        # Write to log
                        with open(log_file, "a") as f:
                            f.write(f"{i} {instance} {cost} {seed} {time} {start_temp} {min_temp} {cooling_rate} {status}\n")
                        
                        print(f"Result: Cost={cost}, Time={time}, Status={status}\n")
            
            # Write to log
            with open(log_file, "a") as f:
                f.write("\n")
        else:
            print(f"WARNING: Instance file {in_file_path} not found!")
        


print("Batch execution completed. Check " + log_file + " for results.")