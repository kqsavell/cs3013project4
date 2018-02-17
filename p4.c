// Kyle Savell & Antony Qin

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 64

char* itoa(int value, char* str, int base);

// Memory
unsigned char memory[SIZE];

// PID array
int pid_array[4];

// Free list
int free_list[4];

// Permissions
int write_list[4];

// Map virtual page to virtual address
int find_page(int v_addr)
{
    if (v_addr < 16)
    {
        return 0;
    }
    else if (v_addr < 32)
    {
        return 1;
    }
    else if (v_addr < 48)
    {
        return 2;
    }
    else
    {
        return 3;
    }
}

// Map virtual page to virtual address
int find_address(int pnum)
{
    if (pnum == 0)
    {
        return 0;
    }
    else if (pnum == 1)
    {
        return 16;
    }
    else if (pnum == 2)
    {
        return 32;
    }
    else
    {
        return 48;
    }
}

// Writes integer into memory, start is the physical address we want to write to
int write_mem(int start, char* value)
{

    return 0;
}

// Reads integer from memory
int read_mem(int start)
{
    int read_val = 0;
    return read_val;
}

// Translate page table, return physical address
int translate_ptable(int pid, int v_addr)
{
    return 0;
}

// Allocates page table entry into virtual page
int create_ptable(int pid)
{
    int been_allocated = -1;
    for(int i = 0; i < 4; i++)
    {
        if (free_list[i] == -1) {
            free_list[i] = 0;
            pid_array[pid] = find_address(i); // Put physical address into pid register
            been_allocated = 1;
            printf("Put page table for PID %d into physical frame %d\n", pid, i);
            break;
        }
    }

    if (been_allocated == -1)
    {
        printf("ERROR: No free space, Memory is full!\n");
    }
}

// Allocates physical page
int map(int pid, int v_addr, int value)
{
    int page_table = pid_array[pid];
    int been_allocated = -1;
    char full_str[16];
    char buffer[10];

    if (page_table == -1)
    {
        create_ptable(pid);
    }

    for(int i = 0; i < 4; i++)
    {
        if (free_list[i] == -1) {
            free_list[i] = 0;
            write_list[i] = value; // Set permissions
            been_allocated = 1;

            int write_addr = pid_array[pid];
            for(int i = 0; i < 16; i++)
            {
                if (memory[write_addr] == '*') // Write entry to ptable
                {
                    sprintf(buffer, "%d", v_addr);
                    strncat(full_str, buffer, sizeof(buffer));
                    strncat(full_str, ",", sizeof(","));
                    sprintf(buffer, "%d", find_address(i) + v_addr);
                    strncat(full_str, buffer, sizeof(buffer));
                    strncat(full_str, ",", sizeof("."));
                    write_addr += write_mem(write_addr, full_str);
                    break;
                }
                write_addr++;
            }
            printf("Mapped virtual address %d (page %d) into physical frame %d\n", v_addr, page_table, i);
            break;
        }
    }

    if (been_allocated == -1)
    {
        printf("ERROR: No free space, Memory is full!\n");
    }

    return 0; // Success
}

int store(int pid, int v_addr, int value)
{
    int phys_addr = translate_ptable(pid, v_addr);
    char buffer[10];

    if (write_list[find_page(phys_addr)] == 1)
    {
        sprintf(buffer, "%d", value);
        write_mem(phys_addr, buffer);
        printf("Stored value %d at virtual address %d (physical address %d)\n", value, v_addr, phys_addr);
    }
    else{
        printf("ERROR; writes are not allowed to this page\n");
    }

    return 0; // Success
}

int load(int pid, int v_addr)
{
    int phys_addr = translate_ptable(pid, v_addr);
    printf("The value %d is virtual address %d (physical address %d)\n", read_mem(phys_addr), v_addr, phys_addr);

    return 0; // Success
}

// Main
int main(int argc, char *argv[])
{
    int pid = 0; // Process ID
    int inst_type = 0; // Instruction type
    int v_addr = 0; // Virtual address
    int input = 0; // Value
    int is_end = 0;

    // Initialize ptable, free and write lists
    for (int i = 0; i < 4; i++)
    {
        pid_array[i] == -1;
        free_list[i] == -1;
        write_list[i] == 1;
    }

    //Initialiaze physical memory
    for (int i = 0; i < SIZE; i++)
    {
        memory[i] = '*';
    }

    while (is_end != 1)
    {
        // Read stdin
        printf("Instructions?: ");
        if (argc >= 2)
        {
            pid = atoi(argv[1]);
        }
        if (argc >= 3)
        {
            if (strncmp(argv[2], "map", sizeof(argv[2])))
            {
                inst_type = 1;
            }
            if (strncmp(argv[2], "store", sizeof(argv[2])))
            {
                inst_type = 2;
            }
            if (strncmp(argv[2], "load", sizeof(argv[2])))
            {
                inst_type = 3;
            }
        }
        if (argc >= 4)
        {
            v_addr = atoi(argv[3]);
        }
        if (argc >= 5)
        {
            input = atoi(argv[4]);
        }
        printf("%d, %s, %d, %d\n", pid, argv[2], v_addr, input);

        if (inst_type == 1)
        {
            map(pid, v_addr, input);
        }
        if (inst_type == 2)
        {
            store(pid, v_addr, input);
        }
        if (inst_type == 3)
        {
            load(pid, v_addr);
        }
    }

    return 0;
}
