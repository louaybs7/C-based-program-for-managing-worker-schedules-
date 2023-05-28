#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_WORKER_PER_DAY 10
#define MAX_NAME_LENGTH 100
#define MAX_WORKERS_PER_DAY 3
// Function to print the contents of the file with all workers and their days of work
void printFileContents(char *fileName)
{
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        printf("Failed to open file %s\n", fileName);
        return;
    }

    char currentLine[100];

    while (fgets(currentLine, 100, file) != NULL)
    {
        printf("%s", currentLine);
    }

    fclose(file);
}
// take a file name and a worker name to delete
void deleteLineFromFile(char *fileName, char *nameToDelete)
{
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        printf("Failed to open file %s\n", fileName);
        return;
    }

    // Create a temporary file to write the modified contents
    FILE *tempFile = fopen("temp.txt", "w");
    if (tempFile == NULL)
    {
        printf("Failed to create temporary file\n");
        fclose(file);
        return;
    }

    char currentLine[100];
    int lineFound = 0;

    while (fgets(currentLine, 100, file) != NULL)
    {
        if (strstr(currentLine, nameToDelete) == currentLine)
        {
            lineFound = 1;
        }
        else
        {
            fprintf(tempFile, "%s", currentLine);
        }
    }

    fclose(file);
    fclose(tempFile);

    if (lineFound)
    {
        remove(fileName);
        rename("temp.txt", fileName);
    }
    else
    {
        remove("temp.txt");
        printf("Line with name '%s' not found in file %s\n", nameToDelete, fileName);
    }
}
// add a worker to the file using name and day of work
void addWorkerToFile(char *fileName, char *workerName, char *day)
{
    FILE *file = fopen(fileName, "a");
    if (file == NULL)
    {
        printf("Failed to open file %s\n", fileName);
        return;
    }

    // Check if the name exist before
    char currentLine[100];
    while (fgets(currentLine, 100, file) != NULL)
    {
        if (strstr(currentLine, workerName) != NULL)
        {
            printf("Worker %s is already present in the file\n", workerName);
            fclose(file);
            return;
        }
    }

    // Add the new worker to the file
    fprintf(file, "%s %s\n", workerName, day);
    printf("Worker %s added to file %s\n", workerName, fileName);

    fclose(file);
}
// a function to count the number of workers in a given day
int countWorkers(char *fileName, char *word)
{
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        printf("Error: Unable to open file '%s' for reading.\n", fileName);
        return -1;
    }

    int count = 0;
    char line[100];

    while (fgets(line, sizeof(line), file))
    {
        char *p = line;
        while ((p = strstr(p, word)))
        {
            count++;
            p += strlen(word);
        }
    }

    fclose(file);
    return count;
}
// This function is used to check if the number of workers in a given day is greater than the maximum number of workers allowed
int addAndCheck(char *day, int max)
{
#define MAX_WORKERS_PER_DAY 3
    char input_copy[100];
    strcpy(input_copy, day);

    // Split the input string into words using strtok
    char *token = strtok(input_copy, " ");
    while (token != NULL)
    {
        // check number of workers for each day if its >= max
        int count = countWorkers("workers.txt", token);
        if (count >= max)
        {
            return 1;
        }
        token = strtok(NULL, " ");
    }

    return 0;
}
// bus manager functions 

struct Worker
{
    char name[MAX_NAME_LENGTH];
    char day[20];
};

struct Message
{
    long M_Type;
    int M_Number;
};

void sigusr1Handler(int signum)
{
    printf("Received SIGUSR1 signal.\n");
}

void sigusr2Handler(int signum)
{
    printf("Received SIGUSR2 signal.\n");
}

void GetWorker(struct Worker workers[], struct Worker workerperday[], int *cnt, char *day)
{
    int total_workers = 0;
    for (int i = 0; i < *cnt; i++)
    {
        if (strstr(workers[i].day, day) != NULL)
        {
            workerperday[total_workers] = workers[i];
            total_workers++;
        }
    }
    *cnt = total_workers;
}

int readWorkersFromFile(struct Worker *workers)
{
    FILE *file = fopen("workers.txt", "r");
    if (file == NULL)
    {
        printf("Failed to open the file.\n");
        return -1;
    }
    char line[MAX_NAME_LENGTH];
    int count = 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *name = strtok(line, " \t\n");
        char *days = strtok(NULL, "\n");
        if (name != NULL && days != NULL)
        {
            strcpy(workers[count].name, name);
            strcpy(workers[count].day, days);
            count++;
        }
    }
    fclose(file);
    return count;
}

int Send(int msg_queue, int num)
{
    const struct Message m = {5, num};
    if (msgsnd(msg_queue, &m, sizeof(int), 0) < 0)
    {
        perror("msgsnd ERROR !");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int Receive(int msg_q)
{
    struct Message m;
    if (msgrcv(msg_q, &m, sizeof(int), 5, 0) < 0)
    {
        perror("msgsnd ERROR !");
        exit(EXIT_FAILURE);
    }
    return m.M_Number;
}

int NewDay(struct Worker *workers, int cnt, char *workday)
{
    struct Worker workerperday[10];
    GetWorker(workers, workerperday, &cnt, workday);

    key_t key = ftok("./", 1);
    int Message = msgget(key, 0777 | IPC_CREAT);
    if (Message < 0)
    {
        perror("Message ERROR\n");
        exit(EXIT_FAILURE);
    }
    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("pipe ERROR");
        exit(EXIT_FAILURE);
    }
    char names[MAX_NAME_LENGTH * MAX_WORKER_PER_DAY] = "";
    pid_t bus1;
    bus1 = fork();
    pid_t bus2;
    int b1_capacity, b2_capacity = 0;
    if (bus1 < 0)
    {
        perror("Fork ERROR");
        exit(EXIT_FAILURE);
    }
    else if (bus1 == 0)
    {
        close(fd[1]);
        kill(getppid(), SIGUSR1);
        read(fd[0], names, MAX_NAME_LENGTH * MAX_WORKER_PER_DAY);
        close(fd[0]);
        printf("bus1 : %s\n", names);
        int workers;
        if (cnt > 5)
        {
            workers = 5;
        }
        else
        {
            workers = cnt;
        }
        Send(Message, workers);
        exit(EXIT_FAILURE);
    }
    else
    {
        pause();
        for (int i = 0; (i < 5 && i < cnt); i++)
        {
            strcat(names, workerperday[i].name);
            strcat(names, " ");
        }
        write(fd[1], names, strlen(names) + 1);
        b1_capacity = Receive(Message);
        printf("number of workers in bus1 : %d\n", b1_capacity);
        wait(NULL);
        if (cnt > 5)
        {
            memset(names, '\0', sizeof(names));
            bus2 = fork();
            if (bus2 == -1)
            {
                perror("fork ERROR");
                exit(EXIT_FAILURE);
            }
            if (bus2 == 0)
            {
                close(fd[1]);
                kill(getppid(), SIGUSR2);
                sleep(1);
                read(fd[0], names, MAX_NAME_LENGTH * MAX_WORKER_PER_DAY);
                close(fd[0]);
                printf("bus2 : %s\n", names);
                Send(Message, cnt - 5);
                exit(EXIT_SUCCESS);
            }
            else
            {
                pause();
                for (int i = 5; i < cnt; i++)
                {
                    strcat(names, workerperday[i].name);
                    strcat(names, " ");
                }
                write(fd[1], names, strlen(names) + 1);
                b2_capacity = Receive(Message);
                printf("Number of workers in bus2 : %d\n", b2_capacity);
                waitpid(bus2, NULL, 0);
            }
        }
        close(fd[0]);
        close(fd[1]);
        if (msgctl(Message, IPC_RMID, NULL) < 0)
        {
            perror("MSGCTL ERROR\n");
            exit(EXIT_FAILURE);
        }
        printf("Numbers of workers successfully brought: %d\n", (b1_capacity + b2_capacity));
    }
    return 0;
}

// Main function
int main()
{
    char fileName[] = "workers.txt";
    int choice;
    char name[100];
    char day[100];
    char lineToDelete[100];

    while (1)
    {

        printf("Enter your choice:\n");
        printf("----------------------------------\n");

        printf("0- Add new worker\n1- Delete worker \n2- Print all workers\n3- Exit program\n4- Delete all workers\n5-Change worker days\n6-Run buses manager system\n ");
        printf("\n");
        printf("----------------------------------\n");

        scanf("%d", &choice);
        getchar(); // consume the newline character left in the input buffer

        switch (choice)
        {
        case 0:

            printf("Enter worker name: ");
            fgets(name, 100, stdin);
            name[strcspn(name, "\n")] = 0; // remove the newline character after the name  at the end
            printf("Enter the days of the week separated by spaces (e.g. Monday Wednesday Thursday): ");
            fgets(day, 100, stdin);
            day[strcspn(day, "\n")] = 0; // remove the newline character after the day  at the end
            if (addAndCheck(day, MAX_WORKERS_PER_DAY) == 1)
            {
                printf("================================\n");
                printf("Sorry we can not accept you \n");
                printf("================================\n ");
                printf("\n");
                break;
            }
            else
            {
                addWorkerToFile(fileName, name, day);
            }
            break;

        case 1:
            printf("Enter worker name to delete: ");
            fgets(name, 100, stdin);
            name[strcspn(name, "\n")] = 0; // remove the newline character at the end of the taken name from input
            deleteLineFromFile(fileName, name);

            break;
        case 2:
            printf("=====================Workers Registered===============\n");
            printFileContents(fileName);
            printf("======================================================\n");
            break;
        case 3:
            exit(0);
            break;
        case 4:
            // Clear the contents of the file
            fclose(fopen(fileName, "w"));
            printf("All workers deleted from file %s\n", fileName);
            break;
        case 5:
            // edit the list throw delete then add
            printf("Enter worker name to Edit: ");
            fgets(name, 100, stdin);
            name[strcspn(name, "\n")] = 0;
            deleteLineFromFile(fileName, name);
            printf("Enter the days of the week separated by spaces(e.g. Monday Wednesday Thursday):");
            fgets(day, 100, stdin);
            day[strcspn(day, "\n")] = 0;
            if (addAndCheck(day, MAX_WORKERS_PER_DAY) == 1)
            {
                printf("================================\n");
                printf("Sorry we can not accept you \n");
                printf("================================\n ");
                printf("\n");
                break;
            }
            else
            {
                addWorkerToFile(fileName, name, day);
            }

            break;

        case 6:
            // hw2 manager

            signal(SIGUSR1, sigusr1Handler);
            signal(SIGUSR2, sigusr2Handler);

            struct Worker workers[100];
            int cnt = readWorkersFromFile(workers);
            const char *days[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
            printf("Applicants for %s\n", days[1]);
            NewDay(workers, cnt, "Monday");
            break;

        default:
            printf("Invalid choice. Please try again.\n");
            break;
        }
    }

    return 0;
}