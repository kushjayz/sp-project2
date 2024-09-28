#include<stdio.h>
#include<stdlib.h>
#define SIZE_PHYSICAL_MEMORY 16
#define SIZE_VIRTUAL_MEMORY 32
#define PROCESS_PAGE_SIZE 4
#define NUMBER_OF_PROCESSES 4
#define DISK_MODE 99


// Structure to represent each frame in memory
struct memory {
    int process_id;
    int page_num;
    int last_accessed;
};

struct page_frame {
    int locations[PROCESS_PAGE_SIZE];
};

// Structure to the processes introduced
struct process{
    // Process ID of the process. Will be the index in the array
    int process_id;
    // Has 4 pages. The array index key represents the page number (0 - 3)
    // Value represents the position in physical / virtual memory
    int page_table[PROCESS_PAGE_SIZE];
};


void report_error(const char *message);
void init(struct memory **RAM, struct memory **virtual_memory, struct process **processes);
void init_memory (struct memory **location, int memory_size);
void init_processes (struct process **processList);
void load_processes_to_virtual_memory(struct memory **virtual_memory,struct process **processes);

void print_memory (struct memory **location, int memory_size);
void print_processes (struct process **processList);

int main (int argc, char* argv[]) {
    if(argc != 3) {
        report_error("Error: Invalid Arguments!\n Usage : ./stimulation <input file.txt> <output file.txt>");
        return EXIT_FAILURE;
    }

    FILE* r_file = fopen(argv[1], "r");

    // Array of pointers for phyiscal RAM, Virtual Memory and Processes
    struct memory* RAM[SIZE_PHYSICAL_MEMORY];
    struct memory* virtual_memory[SIZE_VIRTUAL_MEMORY];
    struct process* processes[NUMBER_OF_PROCESSES];

    // Calling initalising function for the pointers
    init(RAM, virtual_memory, processes);

    load_processes_to_virtual_memory(virtual_memory, processes);

    print_memory (virtual_memory, SIZE_VIRTUAL_MEMORY);
    fclose(r_file);

}

// This functon loads a process into virtual memory
void load_processes_to_virtual_memory(struct memory **virtual_memory, struct process **processes) {
    int vir_mem_location = 0;
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        for(int z = 0; z < PROCESS_PAGE_SIZE; z++) {
            for(int y = 0; y < 2; y++) {
                virtual_memory[vir_mem_location]->process_id = processes[i]->process_id;
                virtual_memory[vir_mem_location]->page_num = z;
                virtual_memory[vir_mem_location]->last_accessed = 0;
                vir_mem_location++;
            }
        }
    }
}

// This function initalizes the entire virtual memory system
void init(struct memory **RAM, struct memory **virtual_memory, struct process **processes) {
    
    // Init Physical memory
    init_memory(RAM, SIZE_PHYSICAL_MEMORY);
    // Init Virtual memory
    init_memory(virtual_memory, SIZE_VIRTUAL_MEMORY);
    // Init and load all the processes to Virtual Memory
    init_processes(processes);
}

// This function initalizes the processes by loading them to virtual memory
void init_processes (struct process **processList) {
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        processList[i] = malloc(sizeof(struct process));
        if(processList[i] != NULL) {
            processList[i]->process_id = i;
            for (int y = 0; y < PROCESS_PAGE_SIZE; y++) {
                processList[i]->page_table[y] = DISK_MODE; // Disk mode means loading to virtual memory
            }
        }
    }
    // print_processes(processList);
}

// This function initialises memory on start up
void init_memory (struct memory **location, int memory_size) {
    for (int i = 0; i < memory_size; i++) {
        location[i] = malloc(sizeof(struct memory));
        if(location[i] != NULL) {
            location[i]->process_id = -1;
            location[i]->page_num = -1;
            location[i]->last_accessed = 0;
        }
    }
    // print_memory(location, memory_size);
}

void print_memory (struct memory **location, int memory_size) {
    for (int i = 0; i < memory_size; i++) {
        int id = location[i]->process_id;
        int num = location[i]->page_num;
        int l_access = location[i]->last_accessed;
        printf("Page No: %d -> process_id: %d, page_num: %d, last_accessed: %d\n", i, id, num, l_access);
    }
}


void print_processes (struct process **processList) {
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        printf("Process: %d\n\tEntries\n\t", processList[i]->process_id);
        for (int y = 0; y < PROCESS_PAGE_SIZE; y++) {
            printf("%d ", processList[i]->page_table[y]);
        }
        printf("\n");
    }
}

void report_error(const char *message) {
    fprintf(stderr, "! %s\n", message);
}