// TS2002 Project 2 2024
//  Student1:   24205163   Kushan Sanka Jayasekera
//  Student2:   24297797   Gayathri Kasunthika Kanakaratne
//  Platform:   Linux

// Based on this simulation, a page will be skipped if it already resides in memory
// But still the timestep will execute

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SIZE_PHYSICAL_MEMORY 16
#define MAX_LINE_SIZE 1024
#define SIZE_VIRTUAL_MEMORY 32
#define PROCESS_PAGE_SIZE 4
#define NUMBER_OF_PROCESSES 4
#define DISK_MODE 99
#define UNUTILIZED -1
#define ALL_PAGES_IN_DISK -999


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
void perform_stimulation(FILE ** r_file);
int get_stimulation_PID(int *array, char *line);
void load_proc_to_ram (int processID);
void alloc_proc_page_to_ram (int process_id, int page_num);
int get_process(int processID, struct process **process_to_load);
struct memory* get_ref_virt_mem_loc(int process_id, int page_num, int *index_virt_mem);
void update_last_acc_time();
void write_to_file(char* filename);

// Free allocated space
void free_processes(struct process **processList);
void free_memory(struct memory **location, int memory_size);


// Array of pointers for phyiscal RAM, Virtual Memory and Processes
struct memory* RAM[SIZE_PHYSICAL_MEMORY];
struct memory* virtual_memory[SIZE_VIRTUAL_MEMORY];
struct process* processes[NUMBER_OF_PROCESSES];

int main (int argc, char* argv[]) {
    if(argc != 3) {
        report_error("Error: Invalid Arguments!\n Usage : ./stimulation <input file.txt> <output file.txt>");
        return EXIT_FAILURE;
    }

    // Calling initalising function for the pointers
    init(RAM, virtual_memory, processes);
    load_processes_to_virtual_memory(virtual_memory, processes);

    FILE* r_file = fopen(argv[1], "r");
    if(r_file == NULL) {
        report_error("Error!: Unable to open the file!");
        exit(EXIT_FAILURE);
    }
    perform_stimulation(&r_file);
    fclose(r_file);
    write_to_file(argv[2]);
    
    // Free allocated memory
    free_processes(processes);
    free_memory(virtual_memory, SIZE_VIRTUAL_MEMORY);
}

// Frees the memory allocated for processes after execution
void free_processes(struct process **processList) {
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        if(processList[i] != NULL) {   // Check if pointer is not already freed
            free(processList[i]);
            processList[i] = NULL;     // Set to NULL after freeing
        }
    }
}


void free_memory(struct memory **location, int memory_size) {
    for (int i = 0; i < memory_size; i++) {
        if(location[i] != NULL) {
            free(location[i]); // Free each allocated struct
            location[i] = NULL; // Set pointer to NULL to avoid dangling pointer
        }
    }
}

// Function that performs the simulation
void perform_stimulation(FILE **r_file) {
    char line[MAX_LINE_SIZE];
    // For each line in the file
    while(fgets(line, sizeof line, (*r_file)) > 0) {
        int array[MAX_LINE_SIZE];

        int no_of_proc = get_stimulation_PID(array, line);
        for(int i = 0; i < no_of_proc; i++) {
            // Loading each page frame on to RAM
            load_proc_to_ram(array[i]);
        }
    }
}

// This function loads a specific process into RAM
void load_proc_to_ram (int processID) {
    int highest_last_access = -1; // Used to 
    int last_access_RAM_index = -1;
    int index_virt_mem; // This index can be used if needed to replace currently allocated RAM process onto Virtual Memory
    // Since not required skipping step.
    struct process *process_to_load;

    int page_num = get_process(processID, &process_to_load);
    if(page_num == ALL_PAGES_IN_DISK) {
        update_last_acc_time();
        return; // If all pages have already been in RAM then ignore.
    }
    struct memory *virt_mem_reference = get_ref_virt_mem_loc(processID, page_num, &index_virt_mem);
    int id = virt_mem_reference->process_id;
    int num = virt_mem_reference->page_num;
    int l_access = virt_mem_reference->last_accessed;
    for(int i = 0; i < SIZE_PHYSICAL_MEMORY; i+=2) {
        if(highest_last_access < RAM[i]->last_accessed) {
            highest_last_access = RAM[i]->last_accessed; 
            last_access_RAM_index = i; // Index of the RAM location to be replaced incase the memory is full
        }
        // If any untilized memory location allocate the process
        if(RAM[i]->process_id == UNUTILIZED) {
            update_last_acc_time();
            process_to_load->page_table[page_num] = (i/2); // Updating reference in page table
            RAM[i] = virt_mem_reference; // loading reference to RAM
            RAM[i+1] = virt_mem_reference++; // loading the contiguous reference to RAM
            return;
        }
    }

    // This section will run only if the RAM is full.
    // If the RAM is full have to update the reference to the new memory location based on LRU Algorithmn
    if(last_access_RAM_index != -1) {
        // Updating page_num in process page table of process moved to virtual memory
        int rm_proc_id = RAM[last_access_RAM_index]->process_id;
        int rm_proc_id_pg_num = RAM[last_access_RAM_index]->page_num;
        processes[rm_proc_id]->page_table[rm_proc_id_pg_num] = 99; 

        int process_id = virt_mem_reference->process_id;
        int page = virt_mem_reference->page_num;
        update_last_acc_time();
        process_to_load->page_table[page_num] = (last_access_RAM_index/2); // Updating page table 
        RAM[last_access_RAM_index]= virt_mem_reference;
        RAM[last_access_RAM_index+1] = virt_mem_reference++;
        return;
    }
}

// Function to update the last access time of each process, each time a new process is to be allocated
void update_last_acc_time() {
    for(int i = 0; i < SIZE_PHYSICAL_MEMORY; i+=2) {
        int last_access = RAM[i]->last_accessed;
        if(last_access != UNUTILIZED) {
            RAM[i]->last_accessed = ++last_access;
        }
    }
}

// Function to get the next page (index) from process page table
int get_process(int processID, struct process **process_to_load) {
    for(int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        if(processes[i]->process_id == processID) {
            for (int y = 0; y < PROCESS_PAGE_SIZE; y++) {
                if(processes[i]->page_table[y] == 99) {
                    *process_to_load = processes[i];
                    return y;
                }
            }
            return ALL_PAGES_IN_DISK; // Returns a flag to indicate the pages have already been loaded onto RAM
        }
    }
}

// This function retrieves the page's reference from virtual memory
struct memory* get_ref_virt_mem_loc(int process_id, int page_num, int *index_virt_mem) {
        for (int i = 0; i < SIZE_VIRTUAL_MEMORY; i+=2) {
            if(virtual_memory[i]->process_id == process_id && virtual_memory[i]->page_num == page_num) {
                // Returning location in virtual memory 
                *index_virt_mem = i;
                return virtual_memory[i];
            }
        }

        return NULL;
}

// This function accepts a line, with a pointer to an array.
// The array pointer will have all the process ids in the specific line.
// The return value is the number of process ids in that line.
int get_stimulation_PID(int *array, char *line) {
    int count = 0;
    while(*line != '\0') {
        if(isdigit(*line)) {
            // Converting ASCI to integer
            int pid = (int)*line - '0';
            *array = pid;
            array++;
            count++;
        }
        line++;
    }
    return count;
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

// Function to write all the processes and RAM onto file
void write_to_file(char* filename) {
    FILE* w_file = fopen(filename, "w");
    if(w_file == NULL) {
        report_error("Error!: Unable to open the file!");
        exit(EXIT_FAILURE);
    }
    // Writing all the processes with page numbers
    for(int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        fprintf(w_file, "Process %d : ", processes[i]->process_id);
        for (int y = 0; y < PROCESS_PAGE_SIZE; y++) {
            fprintf(w_file, "%d ", processes[i]->page_table[y]);
        }
        fprintf(w_file, "\n");
    }

    for (int i = 0; i < SIZE_PHYSICAL_MEMORY; i++) {
        int process_page_no = RAM[i]->page_num;
        int process_no = RAM[i]->process_id;
        int timestep = RAM[i]->last_accessed;
        fprintf(w_file, "%d,%d,%d; ", process_page_no, process_no, timestep);
    }

    fclose(w_file);
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
void init_processes(struct process **processList) {
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        processList[i] = malloc(sizeof(struct process));
        if (processList[i] == NULL) {
            report_error("Memory allocation failed for processes.");
            exit(EXIT_FAILURE);
        }
        processList[i]->process_id = i;
        for (int y = 0; y < PROCESS_PAGE_SIZE; y++) {
            processList[i]->page_table[y] = DISK_MODE; // Disk mode means loading to virtual memory
        }
    }
}


// This function initialises memory on start up
void init_memory(struct memory **location, int memory_size) {
    for (int i = 0; i < memory_size; i++) {
        location[i] = NULL;
    }
    for (int i = 0; i < memory_size; i++) {
        location[i] = malloc(sizeof(struct memory));
        if (location[i] == NULL) {
            report_error("Memory allocation failed for memory locations.");
            exit(EXIT_FAILURE);
        }
        location[i]->process_id = UNUTILIZED;
        location[i]->page_num = UNUTILIZED;
        location[i]->last_accessed = UNUTILIZED;
    }
}
void report_error(const char *message) {
    fprintf(stderr, "! %s\n", message);
}