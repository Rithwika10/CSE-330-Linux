Project 2
-------------------------------------------------------------------------------------------------------------------------------------
CSE 330
Group members: Rithwika Annamaneedu, Nithin Abburi, Sri Ram Reddy Lankireddy, Rishik Gannavarapu.

Overview
In this project, we have developed a Linux kernel module that addresses the classic producer-consumer problem, tailored to calculate and log the total elapsed time of all processes owned by a specified user. Our module utilizes kernel threads to create producers and consumers that work, with semaphores to ensure synchronization and mutual exclusion.

Objective
Our project's main objective was to increase our knowledge of Linux kernel inter-process communication (IPC), with a particular emphasis on how semaphores are used to synchronize threads. In order to get practical experience with kernel module development, we also sought to apply these ideas by keeping track of and computing the elapsed time of processes in a multi-threaded environment.

Key Features
Process Time Tracking: Calculates the total elapsed time for processes belonging to a specified UID, providing insights into resource usage.
Producer-Consumer Mechanism: Implements producers that identify and encapsulate process information into items and consumers that calculate and log the elapsed time for these processes.
Synchronization: Uses semaphores (mutex, empty, full) to regulate access to the shared buffer, ensuring that data integrity is maintained across producer and consumer operations.
Dynamic Configuration: Allows for runtime specification of buffer size, the number of producers/consumers, and the target UID through module parameters.

Implementation Details
Shared Buffer: A bounded buffer stores items produced by producer threads. Each item contains information about a process, including its PID and start time.
Producers: Producer threads iterate over the system's process list, selecting processes based on the specified UID. For each selected process, they generate an item and place it into the shared buffer.
Consumers: Consumer threads retrieve items from the buffer, calculate the elapsed time since the process's start time, and aggregate this to compute the total elapsed time for all selected processes.
Thread Synchronization: Semaphore-based synchronization prevents race conditions, ensuring that producers wait when the buffer is full and consumers wait when the buffer is empty.

Challenges and Learning Outcomes
One of the significant challenges we faced was managing race conditions and ensuring that access to the shared buffer was synchronized across multiple threads. Implementing semaphore logic to solve this issue was both challenging and rewarding, as it deepened our understanding of synchronization primitives in the kernel. This project also enhanced our skills in kernel module programming and debugging, providing us with invaluable experience in working with low-level system components.

Test case Screenshots

