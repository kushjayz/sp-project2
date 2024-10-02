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
    // Value represents the page in physical / virtual memory
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
struct memory* get_ref_virt_mem_loc(int process_id, int page_num);
void update_last_acc_time();
void write_to_output_file(char *filename);


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
        return EXIT_FAILURE;
    }
    perform_stimulation(&r_file);
    fclose(r_file);

    write_to_output_file(argv[2]);
}
void write_to_output_file(char *filename) {
    FILE* w_file = fopen(filename, "w");

    // Writing all the processes with page numbers
    for(int i = 0; i < NUMBER_OF_PROCESSES; i++) {
        fprintf(w_file, "Process %d : ", processes[i]->process_id);
        for (int y = 0; y < PROCESS_PAGE_SIZE; y++) {
            fprintf(w_file, "%d ", processes[i]->page_table[y]);
        }
        fprintf(w_file, "\n");
    }

    // Writing all the contents in the RAM
    for (int i = 0; i < SIZE_PHYSICAL_MEMORY; i++) {
        int process_id = RAM[i]->process_id;
        int page_num = RAM[i]->page_num;
        int last_accessed = RAM[i]->last_accessed;
        fprintf(w_file, "%d,%d,%d; ", process_id, page_num, last_accessed);
    }

    fclose(w_file);
}

// Function which reads the file and run overall stimulation based on pages in the text file
void perform_stimulation(FILE **r_file) {
    char line[MAX_LINE_SIZE];
    while(fgets(line, sizeof line, (*r_file)) > 0) {
        int array[MAX_LINE_SIZE];
        int no_of_proc = get_stimulation_PID(array, line);
        for(int i = 0; i < no_of_proc; i++) {
            load_proc_to_ram(array[i]);
        }
    }
}

// This is the main function which has the functionality to load a process from virtual memory to RAM
void load_proc_to_ram (int processID) {
    // Finding the process and the next page to load into RAM
    struct process *process_to_load;
    int page_num = get_process(processID, &process_to_load);
    struct memory *virt_mem_reference = get_ref_virt_mem_loc(processID, page_num);
    int highest_last_access = -1;
    // struct memory *last_accessed_frame;
    int last_access_RAM_index = -1;
    for(int i = 0; i < SIZE_PHYSICAL_MEMORY; i+=2) {
        if(highest_last_access < RAM[i]->last_accessed) {
            highest_last_access = RAM[i]->last_accessed;
            last_access_RAM_index = i;
        }
        // If any untilized memory location allocate the process directly
        if(RAM[i]->process_id == UNUTILIZED) {
            update_last_acc_time();
            process_to_load->page_table[page_num] = (i/2)+1; // Updating reference in page table with the page number
            RAM[i] = virt_mem_reference; // loading reference to RAM
            RAM[i+1] = virt_mem_reference + 1; // loading the contiguous reference to RAM
            return;
        }
    }

    // if the RAM is full have to update the reference to the new memory location based on LRU Algorithmn
    if(last_access_RAM_index != -1) {
        update_last_acc_time();
        process_to_load->page_table[page_num] = (last_access_RAM_index/2)+1;
        RAM[last_access_RAM_index]= virt_mem_reference;
        RAM[last_access_RAM_index+1] = virt_mem_reference++;
        return;
    }
}

// Function which simulates the time step for each time a process is loaded into memory
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
                if(processes[i]->page_table[y] == DISK_MODE) {
                    *process_to_load = processes[i];
                    return y;
                }
            }
        }
    }
}

// This function get the pointer for a page in virtual memory for a given process
struct memory* get_ref_virt_mem_loc(int process_id, int page_num) {
        for (int i = 0; i < SIZE_VIRTUAL_MEMORY; i+=2) {
            if(virtual_memory[i]->process_id == process_id && virtual_memory[i]->page_num == page_num) {
                return virtual_memory[i];
            }
        }
}

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

// This function loads all processes into virtual memory
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
        } else {
            report_error("Error! Memory allocation failed for Process");
            exit(EXIT_FAILURE);
        }
    }
}

// This function initialises memory on start up
void init_memory (struct memory **location, int memory_size) {
    for (int i = 0; i < memory_size; i++) {
        location[i] = malloc(sizeof(struct memory));
        if(location[i] != NULL) {
            location[i]->process_id = UNUTILIZED;
            location[i]->page_num = UNUTILIZED;
            location[i]->last_accessed = UNUTILIZED;
        } else {
            report_error("Error! Memory allocation failed for Memory");
            exit(EXIT_FAILURE);
        }
    }
}

void report_error(const char *message) {
    fprintf(stderr, "! %s\n", message);
}