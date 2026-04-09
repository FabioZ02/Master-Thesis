/*
The greedy algorithm takes in input the tasks T, the machinse M and of course the compatibility matrix V, it also takes the maximum load of a machine S_max.
In order to obtain a better first solution, tasks are given as a sorted list. Tasks are sorted by their priorities, in this way tasks with similar priorities are assigned to the same period.
First of all the algorithm initializes as 0 the machine_load of all the machines, then it set the current_period as 1.
Then, it starts a for cycle on tasks, for each one it selects the current_machine as the machine with the least load, moreover to mantain the feasibility of the plan, V[current_task][current_machine] = 1.
Then the algorithm compute the new free_space, a new period is created. Finally the first machine compatible with the current task is assigned to t.
Lastly, the algorithm updates PA_t, MA_t and the machine_load of the selected_machine. if it is lower than s_t, the output consists of current_period, PA_t, and MA_t for each task t.
*/



