// Kyle Savell & Antony Qin

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SIZE 64
#define MAX_PROC 4
#define MAX_PAGES 10

// Memory
unsigned char memory[SIZE];

// PID array
int pid_array[MAX_PROC];

// Free list
int free_list[MAX_PROC];

// Permissions
int write_list[MAX_PROC][MAX_PAGES];

// Existing Pages
int page_exists[MAX_PROC][MAX_PAGES];

// Disk
FILE *disk;

// Function Declarations
int find_page(int addr); // Returns a corresponding page based on an address
int find_address(int page); // Returns address of the start of a given page
int write_mem(int start, char* value); // Writes integer into memory, start is the physical address we want to write to
int read_mem(int start); // Reads integer from memory
int translate_ptable(int pid, int v_addr); // Translate page table, return physical address from virtual address
int create_ptable(int pid); // Allocates page table entry into virtual page
int map(int pid, int v_addr, int r_value); // Maps virtual page to physical page
int store(int pid, int v_addr, int value); // Stores value in physical memory
int load(int pid, int v_addr); // Loads value from physical memory
int swap(int page, int lineNum); // Swaps page from physical memory and disk
int putToDisk(char page[16]); // Puts page in disk
int getFromDisk(char (*pageHolder)[16], int lineNum); // Gets page from disk

// Returns a corresponding page based on an address
int find_page(int addr)
{
    return (addr/16);
}

// Returns address of the start of a given page
int find_address(int page)
{
    if (page == 0)
    {
        return 0;
    }
    else if (page == 1)
    {
        return 16;
    }
    else if (page == 2)
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
    int num_bytes = strlen(value);
    int remainder = start%16;
    if ((remainder + num_bytes) >= 15)
    {
        return -1; // Not enough room on the current page
    }

    int i;
    for(i = start; i < 16 + start; i++)
    {
        if(value[i - start] != 0)
            memory[i] = value[i - start];
        else
            break;
    }

    return (i - start); // Number of bytes written
}

// Reads integer from memory
int read_mem(int start)
{
    int read_val;
    char buffer[4];
    for(int i = 0; i < 4; i++)
    {
        if(memory[start + i] != '*' && i != 3)
            buffer[i] = memory[start + i];
        else
        {
            buffer[i] = '\0';
            break;
        }
    }
    if(buffer[0] == '\0') return -1;
    else
    {
        sscanf(buffer, "%d", &read_val);
        return read_val;
    }
}

// Translate page table, return physical address from virtual address
// ptable format is an array {0,',',1,1,',',2} -> single digit to immediate left of , is virtual page, single digit to immediate right of , is physical page the vitual page is mapped to.
// In example, virtual page 0 is mapped to physical page 1 & virtual page 1 is mapped to physical page 2
int translate_ptable(int pid, int v_addr)
{
    /*
        int pt_start = pid_array[pid]; // Get address of page table for that process
        char int_buf[10] = ""; // Holds the string that makes up an address index
        int cur_v_addr = 0;
        int phys_addr = -1;
        int diff = -1;

        //Get physical address
        int cur_addr = pt_start;
        for (int i = 0; i < 16; i++) // Only look up to end of page table virtual page
        {
            if (memory[cur_addr] == ',') // End of virtual address for PTE
            {
                cur_v_addr = atoi(int_buf);int_buf[0] = '\0';
                memset(&int_buf[0], 0, sizeof(int_buf));
            }
            else if (memory[cur_addr] == '.') // End of PTE
            {
                if(diff == -1) diff = atoi(int_buf) - cur_v_addr;
                if (cur_v_addr == v_addr)
                {
                    phys_addr = atoi(int_buf);
                }
                memset(&int_buf[0], 0, sizeof(int_buf));

            }
            else // Reading address
            {
    	    if(isdigit(memory[cur_addr]))
                    strncat(int_buf, &memory[cur_addr], 1);
            }
            cur_addr++;
        }
        if(phys_addr == -1) phys_addr = v_addr + diff;
    */
    int start = pid_array[pid];
    int phys_addr = -1;
    int cur_addr = start;
    for (int i = 0; i < 16; i++) // Only look up to end of page table virtual page
    {
        if (memory[cur_addr] == ',') // PTE Separator
        {
            if(memory[cur_addr - 1] - '0' == find_page(v_addr))
                return find_address(memory[cur_addr + 1] - '0') + v_addr;
        }
        cur_addr++;
    }
    return phys_addr; // Return physical address, -1 if address not found
}

// Allocates page table entry into virtual page
int create_ptable(int pid)
{
    int been_allocated = -1;
    for(int i = 0; i < 4; i++)
    {
        if (free_list[i] == -1)
        {
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

// Maps virtual page to physical page
int map(int pid, int v_addr, int r_value)
{
    int page_table = pid_array[pid];
    int been_allocated = -1;
    char full_str[16] = "";
    char buffer[10];
    char new_entry[10];

    int v_page = find_page(v_addr);
    int p_page;

    // Create page table for process if one does not exist
    if (page_table == -1)
    {
        create_ptable(pid);
    }

    // Check if entry already exists and update it
    if (page_exists[pid][v_page] == 1)
    {
        write_list[pid][v_page] = r_value;
    }

    // Create new entry
    // memset(&buffer[0], 0, sizeof(buffer));
    else
    {
        for(int i = 0; i < 4; i++)
        {
            if (free_list[i] == -1)
            {
                free_list[i] = 0;
                write_list[pid][v_page] = r_value; // Set permissions
                page_exists[pid][v_page] = 1; // Set existence of page
                been_allocated = 1;
                p_page = i;

                int write_addr = pid_array[pid];
                for(int j = 0; j < 16; j++)
                {
                    if (memory[write_addr] == '*') // Write entry to ptable
                    {
                        sprintf(buffer, "%d", v_page);
                        strcat(full_str, buffer);
                        strcat(full_str, ",");
                        sprintf(buffer, "%d", p_page);
                        strcat(full_str, buffer);
                        write_addr += write_mem(write_addr, full_str);
                        break;
                    }
                    write_addr++;
                }

                printf("Mapped virtual address %d (page %d) into physical frame %d\n", v_addr, v_page, p_page);
                break;
            }
        }
        if (been_allocated == -1)
        {
            printf("ERROR: No free space, Memory is full!\n");
        }
    }

    return 0; // Success
}

// Stores value in physical memory
int store(int pid, int v_addr, int value)
{
    int phys_addr = translate_ptable(pid, v_addr);
    int v_page= find_page(v_addr);
    char buffer[10] = "";

    if (write_list[pid][v_page] == 1)
    {
        if (page_exists[pid][v_page] == 1)
        {
            sprintf(buffer, "%d", value);
            int num_bytes = write_mem(phys_addr, buffer);
            if (num_bytes == -1)
            {
                printf("ERROR: Write goes over end of page! Value not stored\n");
            }
            else
            {
                printf("Stored value %d at virtual address %d (physical address %d)\n", value, v_addr, phys_addr);
            }
        }
        else
        {
            printf("ERROR: Virtual page %d has not been allocated for process %d!\n", v_page, pid);
        }
    }
    else
    {
        printf("ERROR: writes are not allowed to this page\n");
    }

    return 0; // Success
}

// Loads value from physical memory
int load(int pid, int v_addr)
{
    int phys_addr = translate_ptable(pid, v_addr);
    int value = read_mem(phys_addr);
    if  (value == -1)
    {
        printf("ERROR: No value stored at virtual address %d (physical address %d)\n", v_addr, phys_addr);
    }
    else
    {
        printf("The value %d is virtual address %d (physical address %d)\n", value, v_addr, phys_addr);
    }

    return 0; // Success
}

// Changes mapping of virtual page in a page table when swapping in from disk
int remap(int v_page)
{
    // Change physical address of page and overwrite entry in page table
    for(int i = 0; i < 4; i++)
        {
            if (free_list[i] == -1)
            {
                free_list[i] = 0;
                been_allocated = 1;
                p_page = i;

                int write_addr = pid_array[pid];
                int v_flag = 1; // Whether specific entry is virtual page or not
                int p_flag = 0; // Whether specific entry is a physical page or not
                int correct_p = 0; // Flag for correct page to overwrite
                for(int j = 0; j < 16; j++)
                {
                    if (p_flag == 1) // Pointer on physical page position
                    {
                        if (correct_p == 1) // If correct page to overwrite, do that
                        {
                            sprintf(buffer, "%d", p_page);
                            strcat(full_str, buffer);
                            write_addr += write_mem(write_addr, full_str);
                            break;
                        }
                        p_flag = 0;
                        v_flag = 0;
                    }
                    else if (memory[write_addr] == ',') // Pointer on in-between position
                    {
                        v_flag = 0;
                        p_flag = 1;
                    }

                    else if ((memory[write_addr] == v_page + '0') && (v_flag == 1)) // If correct virtual page, prepare overwrite
                    {
                        correct_p = 1;
                    }
                    write_addr++;
                }

                printf("Remapped virtual page %d into physical frame %d\n", v_page, p_page);
                break;
            }
        }
        if (been_allocated == -1)
        {
            printf("ERROR: No free space, Memory is full!\n");
        }
    }
}

// Swaps page from physical memory and disk
int swap(int page, int lineNum)
{
    int start = find_address(page);
    char putTemp[16];
    char getTemp[16];

    for(int i = 0; i < 16; i++)
    {
        putTemp[i] = memory[start + i];
    }

    if(putToDisk(putTemp) == -1)
    {
        printf("ERROR: Could not put page to disk.\n");
        return -1;
    }

    else
        if(getFromDisk(&getTemp, lineNum) == -1)
        {
            printf("ERROR: Could not get page from disk.\n");
            return -1;
        }

    return 0;
}

// Puts page in disk
int putToDisk(char page[16])
{
    int line_placement = -1; // Where line is on disk
    int newLine = -1;
    char currChar;

    disk = fopen("disk.txt", "r+");
    if(disk == NULL)
    {
        printf("ERROR: Cannot open disk.\n");
    }

    do
    {
        currChar = fgetc(disk);
        if(feof(disk) && line_placement == -1) // Empty file, put page in
        {
            for(int i = 0; i < 16; i++)
            {
                fputc(page[i], disk);
            }
            fputc('\n', disk);
            line_placement = 0;
            break;
        }
        else
        {
            if(currChar == '\n')
            {
                newLine = 1;
                line_placement++;
            }
            else if(currChar == '*' && newLine != -1) // Last character was a new line
            {
                fseek(disk, -1, SEEK_CUR);
                for(int i = 0; i < 16; i++)
                {
                    fputc(page[i], disk);
                }
                fputc('\n', disk);
                newLine = -1;
            }
            else
            {
                newLine = -1;
            }
        }
    }
    while(currChar != EOF);


    fclose(disk);

    return line_placement;
}

// Gets page from disk
int getFromDisk(char (*pageHolder)[16], int lineNum)
{
    int line_placement = -1; // Where line is on disk
    int newLine = -1;
    char currChar;

    disk = fopen("disk.txt", "r+");
    if(disk == NULL)
    {
        printf("ERROR: Cannot open disk.\n");
        return -1;
    }

    do
    {
        currChar = fgetc(disk);
        if(feof(disk) && line_placement == -1)
        {
            printf("ERROR: Cannot get page from empty disk.\n");
            fclose(disk);
            return -1;
        }
        else
        {
            if(currChar == '\n')
            {
                newLine = 1;
                line_placement++;
            }
            else if(currChar != '*' && newLine != -1 && line_placement == lineNum) // Last character was a new line
            {
                fseek(disk, -1, SEEK_CUR);
                for(int i = 0; i < 16; i++)
                {
                    currChar = fgetc(disk);
                    (*pageHolder)[i] = currChar;
                    fseek(disk, -1, SEEK_CUR);
                    fputc('*', disk);
                }
                fputc('\n', disk);
                newLine = -1;
                break;
            }
            else
            {
                newLine = -1;
            }
        }
    }
    while(currChar != EOF);

    fclose(disk);

    return 0;
}

// Main
int main(int argc, char *argv[])
{
    int pid = 0; // Process ID
    int inst_type = 0; // Instruction type
    int v_addr = 0; // Virtual address
    int input = 0; // Value
    int is_end = 0; // Boolean for ending simulation

    char buffer[20]; // Holds stdin buffer
    char cmd_seq[20]; // The command sequence read from stdin
    char* cmd_array[4]; // Holds the commands read from file
    char* token;

    // Initialize ptable, free and write lists
    for (int i = 0; i < 4; i++)
    {
        pid_array[i] = -1;
        free_list[i] = -1;
        for (int j = 0; j < 10; j++)
        {
            write_list[i][j] = 0;
            page_exists[i][j] = 0;
        }
    }

    //Initialiaze physical memory
    for (int i = 0; i < SIZE; i++)
    {
        memory[i] = '*';
    }

    while (is_end != 1)
    {
        printf("Instruction?: ");
        // Recieve stdin
        if (argc <= 1)
        {
            // Read sequence from file
            if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            {
                is_end = 1;
                printf("End of File. Exiting\n");
                break;
            }
            buffer[strcspn(buffer, "\n")] = 0; //Remove newline
            strncpy(cmd_seq, &buffer[0], sizeof(cmd_seq));
            printf("%s\n", cmd_seq);

            // Parse sequence
            token = strtok(cmd_seq, " ");
            int i = 0;
            while (token != NULL)
            {
                if (i >= 4)
                {
                    printf("ERROR: Too many input arguments!\n");
                    break;
                }
                cmd_array[i] = token;
                i++;
                token = strtok(NULL, " ");
            }

            // Put sequence into variables
            pid = atoi(cmd_array[0]);

            if (strncmp(cmd_array[1], "map", sizeof(cmd_array[1])) == 0)
            {
                inst_type = 1;
            }
            else if (strncmp(cmd_array[1], "store", sizeof(cmd_array[1])) == 0)
            {
                inst_type = 2;
            }
            else if (strncmp(cmd_array[1], "load", sizeof(cmd_array[1])) == 0)
            {
                inst_type = 3;
            }
            v_addr = atoi(cmd_array[2]);
            input = atoi(cmd_array[3]);
        }

        // Read argv
        else
        {
            if (argc >= 2)
            {
                pid = atoi(argv[1]);
            }
            if (argc >= 3)
            {
                if (strncmp(argv[2], "map", sizeof(argv[2])) == 0)
                {
                    inst_type = 1;
                }
                else if (strncmp(argv[2], "store", sizeof(argv[2])) == 0)
                {
                    inst_type = 2;
                }
                else if (strncmp(argv[2], "load", sizeof(argv[2])) == 0)
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
        }

        if (is_end == 1) break; // Break if EOF

        if (inst_type == 1)
        {
            map(pid, v_addr, input);
        }
        else if (inst_type == 2)
        {
            store(pid, v_addr, input);
        }
        else if (inst_type == 3)
        {
            load(pid, v_addr);
        }
    }
    return 0;
}
