#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// global data
const int TIME_MAX = 100000;

int num_of_processes = 0;
int last_announcement = -1;

// Structs for processes, mem map, and queue

typedef struct
{
    int pid;
    int arrival_time;
    int life_time;
    int mem_reqs;
    int time_added_to_memory;
    int is_active;
    int time_finished;
} Process;

typedef struct
{
    int assigned;
    char location[40];
    int proc_assign;
    int page_num;
} Map;

typedef struct
{
    Map *frames;
    int number_of_frames;
    int page_size;
} Frame;

typedef struct
{
    int capacity;
    int size;
    int front;
    int back;
    Process **elements;
} Queue;

// creating functions to manage process data
Frame *create_frame(int number_of_frames, int page_size)
{
    Frame *f;

    f = malloc(sizeof(Frame));
    f->frames = (Map *)malloc(sizeof(Map) * number_of_frames);
    f->page_size = page_size;
    f->number_of_frames = number_of_frames;

    for (int i = 0; i < number_of_frames; i++)
    {
        f->frames[i].assigned = 0;
    }
    return f;
}

int process_into_mem(Frame *list, Process *proc)
{
    int free_frames = 0;
    for (int i = 0; i < list->number_of_frames; i++)
    {
        if (!list->frames[i].assigned)
        {
            free_frames += 1;
        }
    }
    return (free_frames * list->page_size) >= proc->mem_reqs;
}

void fit_process(Frame *list, Process *proc)
{
    int remaining_mem, current_page = 1;
    remaining_mem = proc->mem_reqs;

    for (int i = 0; i < list->number_of_frames; i += 1)
    {
        if (!list->frames[i].assigned)
        {
            list->frames[i].assigned = 1;
            list->frames[i].page_num = current_page;
            list->frames[i].proc_assign = proc->pid;
            current_page++;
            remaining_mem -= list->page_size;
        }
        if (remaining_mem <= 0)
        {
            break;
        }
    }
}

void print_frame(Frame *list)
{
    int free = 0;
    int start;
    int i;
    printf("\tMemory map:\n");

    for (i = 0; i < list->number_of_frames; i++)
    {
        if (!free && !list->frames[i].assigned)
        {
            free = 1;
            start = i;
        }
        else if (free && list->frames[i].assigned)
        {
            free = 0;
            printf("\t\t%d-%d: Free frame(s)\n", start * list->page_size, i * list->page_size - 1);
        }

        if (list->frames[i].assigned)
        {
            printf("\t\t%d-%d: Process %d, Page %d\n",
                   i * list->page_size, (i + 1) * list->page_size - 1,
                   list->frames[i].proc_assign, list->frames[i].page_num);
        }
    }

    if (free)
    {
        printf("\t\t%d-%d: Free frame(s)\n", start * list->page_size, i * list->page_size - 1);
    }
}

int empty_frame(Frame *list)
{
    for (int i = 0; i < list->number_of_frames; i++)
    {
        if (list->frames[i].assigned)
            return 0;
    }
    return 1;
}

void free_memory(Frame *list, int pid)
{
    Map *frame;

    for (int i = 0; i < list->number_of_frames; i++)
    {
        frame = &list->frames[i];

        if (frame->proc_assign == pid)
        {
            frame->proc_assign = 0;
            frame->page_num = 0;
            frame->assigned = 0;
        }
    }
}

Queue *create_queue(int length)
{
    Queue *q;
    q = malloc(sizeof(Queue));
    q->elements = malloc(sizeof(Process) * length);
    q->size = 0;
    q->capacity = length;
    q->front = 0;
    q->back = -1;

    return q;
}

void enqueue_proc(Queue *q, Process *p)
{
    if (q->size == q->capacity)
    {
        printf("ERROR: queue is at full capacity!\n");
        exit(2);
    }

    q->size++;
    q->back = q->back + 1;

    if (q->back == q->capacity)
    {
        q->back = 0;
    }

    q->elements[q->back] = p;
}

int next_queue(Queue *q)
{
    return q->size == 0 ? 0 : 1;
}

Process *peek_queue(Queue *q)
{

    if (!next_queue(q))
    {
        printf("ERROR: Queue is empty, next element unavailable.\n");
        exit(2);
    }
    return q->elements[q->front];
}

Process *peek_queue_index(Queue *q, int index)
{
    return q->elements[index];
}

void dequeue_proc(Queue *q)
{
    if (!next_queue(q))
    {
        printf("ERROR: Queue is empty, nothing to dequeue.\n");
        exit(2);
    }

    q->size--;
    q->front++;

    if (q->front == q->capacity)
    {
        q->front = 0;
    }
}

int iterate_queue(Queue *q, int index)
{
    return q->front + index;
}

int dequeue_proc_index(Queue *q, int index)
{
    int previous = 0;
    for (int i = 0; i < q->size; i++)
    {
        if (i > index)
        {
            q->elements[previous] = q->elements[i];
        }

        previous = i;
    }

    q->size--;
    q->back--;
}

void print_queue(Queue *q)
{
    Process *p;
    printf("\tInput queue: [ ");
    for (int i = 0; i < q->size; i++)
    {
        p = peek_queue_index(q, iterate_queue(q, i));
        printf("%d ", p->pid);
    }
    printf("]\n");
}

Process *process_list;
Queue *queue;
Frame *frame;

char *get_announcement(int current_time);

void enqueue_new_process(int current_time)
{
    Process *p;

    for (int i = 0; i < num_of_processes; i++)
    {
        p = &process_list[i];
        if (p->arrival_time == current_time)
        {
            printf("%sProcess %d arrives\n", get_announcement(current_time), p->pid);
            enqueue_proc(queue, p);
            print_queue(queue);
            print_frame(frame);
        }
    }
}

void terminate_process(int current_time)
{
    int time_spent;
    Process *p;

    for (int i = 0; i < num_of_processes; i++)
    {
        p = &process_list[i];
        time_spent = current_time - p->time_added_to_memory;

        if (p->is_active && (time_spent >= p->life_time))
        {
            printf("%sProcess %d completes\n", get_announcement(current_time), p->pid);

            p->is_active = 0;
            p->time_finished = current_time;
            free_memory(frame, p->pid);
            print_frame(frame);
        }
    }
}

void assign_memory(int current_time)
{
    int index, limit;
    Process *p;
    limit = queue->size;

    for (int i = 0; i < limit; i++)
    {
        index = iterate_queue(queue, i);
        p = queue->elements[index];

        if (process_into_mem(frame, p))
        {
            printf("%sMM moves Process %d to memory\n", get_announcement(current_time), p->pid);
            fit_process(frame, p);

            p->is_active = 1;
            p->time_added_to_memory = current_time;

            dequeue_proc_index(queue, i);
            print_queue(queue);
            print_frame(frame);
        }
    }
}

char *get_announcement(int current_time)
{
    char *result;

    result = malloc(20 * sizeof(char));

    if (last_announcement == current_time)
    {
        sprintf(result, "\t");
    }
    else
    {
        sprintf(result, "t = %d: ", current_time);
    }

    last_announcement = current_time;
    return result;
}

void print_turnaround_time()
{
    int i;
    float total = 0;

    for (i = 0; i < num_of_processes; i++)
    {
        total += process_list[i].time_finished - process_list[i].arrival_time;
    }

    printf("Average Turnaround Time: %2.2f\n", total / num_of_processes);
}

int multiple_of_hundred(int m)
{
    return (m % 100) == 0 ? 1 : 0;
}

int is_which(int m)
{
    return (m >= 1 && m <= 3) ? 1 : 0;
}

void clear_stdin(char *c)
{
    if (c[strlen(c) - 1] != '\n')
    {
        int chr;
        while (((chr = getchar()) != '\n') && (chr != EOF))
            ;
    }
}

int process_user_input(const char *output, int (*func)(int))
{
    char c[10];
    int success = 0;
    int result = 0;
    while (!success)
    {
        printf("%s: ", output);
        if (fgets(c, 10, stdin) == NULL)
        {
            clear_stdin(c);
            printf("ERROR: No data entered.\n");
            continue;
        }

        if (sscanf(c, "%d", &result) <= 0)
        {
            clear_stdin(c);
            printf("ERROR: Number required.\n");
            continue;
        }

        if (!(success = (*func)(result)))
        {
            clear_stdin(c);
            printf("ERROR: Number not a valid choice.\n");
        }
    }

    return result;
}

void prompt_filename(char *result)
{
    char c[100];
    FILE *f;

    while (1)
    {
        printf("Input file: ");

        if (fgets(c, 100, stdin) == NULL)
        {
            clear_stdin(c);
            printf("ERROR: No date entered.\n");

            continue;
        }

        if (sscanf(c, "%s", result) <= 0)
        {
            clear_stdin(c);
            printf("ERROR: You didn't enter a string!\n");

            continue;
        }

        if (!(f = fopen(result, "r")))
        {
            perror("ERROR: Could not open file!\n");
        }
        else
        {
            break;
        }
    }
}

void get_user_input(int *memory, int *page, char *file_path)
{
    while (1)
    {
        *memory = process_user_input("Memory size", multiple_of_hundred);

        *page = process_user_input(
            "Page size (1: 100, 2: 200, 3: 400)", is_which);

        switch (*page)
        {
        case 1:
            *page = 100;
            break;
        case 2:
            *page = 200;
            break;
        case 3:
            *page = 400;
            break;
        }

        if ((*memory) % (*page) == 0)
        {
            break;
        }

        printf("ERROR: Memory size must be a multiple of the page.");
        printf(" %d is not a multiple of %d, try again.\n", *memory, *page);
    }

    prompt_filename(file_path);
}

int get_processes_from_file(FILE *file)
{
    int num = 0;
    fscanf(file, "%d", &num);
    return num;
}

Process *assign_process_list(const char *file_path)
{
    int space;
    int temp;
    int counter = 0;
    int totalSpace = 0;
    FILE *file = fopen(file_path, "r");

    num_of_processes = get_processes_from_file(file);
    Process *list = malloc(num_of_processes * sizeof(Process));

    if (!file)
    {
        printf("ERROR: Failed to open file %s", file_path);
        exit(1);
    }

    while (!feof(file) && counter < num_of_processes)
    {
        // store values for processes
        fscanf(file, "%d %d %d %d",
               &(list[counter].pid), &(list[counter].arrival_time),
               &(list[counter].life_time), &space);

        // get total memory requirements for process
        totalSpace = 0;
        for (int i = 0; i < space; i++)
        {
            fscanf(file, "%d", &temp);
            totalSpace += temp;
        }
        list[counter].mem_reqs = totalSpace;

        list[counter].is_active = 0;
        list[counter].time_added_to_memory = -1;
        list[counter].time_finished = -1;

        counter++;
    }

    fclose(file);

    return list;
}

void loop()
{
    long current_time = 0;

    while (1)
    {
        enqueue_new_process(current_time);
        terminate_process(current_time);
        assign_memory(current_time);
        current_time++;

        if (current_time > TIME_MAX)
        {
            printf("DEADLOCK: Max time reached.\n");
            break;
        }

        if (queue->size == 0 && empty_frame(frame))
        {
            break;
        }
    }
    print_turnaround_time();
}

int main()
{
    int page_size = 0;
    int memory_size = 0;
    char *file_path = malloc(100 * sizeof(char));

    get_user_input(&memory_size, &page_size, file_path);

    process_list = assign_process_list(file_path);
    queue = create_queue(num_of_processes);
    frame = create_frame(memory_size / page_size, page_size);
    loop();

    return 0;
}
