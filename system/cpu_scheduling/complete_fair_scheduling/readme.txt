Here is how the CFS schedules

The CPU creates a Red-Black tree with the processes virtual runtime (runtime / nice_value) and sleeper fairness (if the process is waiting on something give it the CPU when it is done waiting).
(Nice values are the kernel's way of giving priority to certain processes, the lower nice value the higher priority)
The kernel chooses the lowest one based on this metric and schedules that process to run next, taking it off the queue. Since the red-black tree is self balancing this operation is guaranteed $O(log(n))$ (selecting the min process is the same runtime)