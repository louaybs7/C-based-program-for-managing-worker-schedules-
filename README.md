This project is a worker schedule management program implemented in C. It provides functionalities to add, delete, and modify worker information in a file. Additionally, it includes a bus manager system that distributes workers into buses based on their availability.

Technologies and concepts used in this project include:
- C programming language
- File handling (opening, reading, writing, and deleting files)
- String manipulation (string copying, concatenation, and comparison)
- Signal handling (handling SIGUSR1 and SIGUSR2 signals)
- Inter-process communication using message queues
- Process creation and management (forking processes)
- User input handling (reading input from the user)
- Conditional checks and loops (if-else statements, switch-case statements, and while loops)

The main functionalities of the program include:
- Adding a new worker to the file with their name and days of work
- Deleting a worker from the file based on their name
- Printing the contents of the file, showing all registered workers
- Clearing the file, deleting all workers' information
- Editing the list of workers by deleting and adding new information
- Running the bus manager system, which distributes workers into buses based on their availability on a given day

Please note that this project requires a "workers.txt" file to store and manage the worker information.
