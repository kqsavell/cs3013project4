// Kyle Savell & Antony Qin

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SIZE 64
#define MAX_PROC 4
#define MAX_PAGES 4

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

// Page Location on Disk
int on_disk[MAX_PROC][MAX_PAGES + 1];

// Disk
FILE *disk;

// Round Robin Eviction
int last_evict = 0;

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
int evict(int pid); // Returns physical page that is to be evicted
int remap(int pid, int v_page, int p_page); //Remaps virtual page if pulled from disk
int replace_page(int pid, int v_page); // Handles page replacements
int swap(int page, int lineNum); // Swaps page from physical memory and disk, returns lineNum page was put in disk
int putToDisk(char page[16]); // Puts page in disk
int getFromDisk(char (*pageHolder)[16], int lineNum); // Gets page from disk

void logMem(); // DEBUGGING ONLY; DISPLAYS PHYSICAL MEMORY
void logMem()
{
    for(int i = 0; i < 64; i++)
    {
        printf("INDEX %d IN MEMORY IS: [%c]\n", i, memory[i]);
    }
}

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
    int start = pid_array[pid];
    int v_page = find_page(v_addr);
    int offset = v_page * 16;
    int phys_addr = -1;
    int cur_addr = start;
    for (int i = 0; i < 16; i++) // Only look up to end of page table virtual page
    {
        if (memory[cur_addr] == ',') // PTE Separator
        {
            if(memory[cur_addr - 1] - '0' == find_page(v_addr))
                return find_address(memory[cur_addr + 1] - '0') + v_addr - offset;
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
	 int to_evict = evict(pid);
        swap(to_evict, -1);
        //replace_page(pid, -1);
        int p_page = last_evict;
        pid_array[pid] = find_address(p_page);
        printf("Put page table for PID %d into physical frame %d\n", pid, p_page);
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
        if (write_list[pid][v_page] == r_value) printf("ERROR: virtual page %d is already mapped with rw_bit=%d\n", v_page, r_value);
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
		int to_evict = evict(pid);
        swap(to_evict, -1);
            //replace_page(pid, -1);
            write_list[pid][v_page] = r_value; // Set permissions
            page_exists[pid][v_page] = 1; // Set existence of page
            been_allocated = 1;
            p_page = last_evict;

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
        }
    }

    return 0; // Success
}

// Stores value in physical memory
int store(int pid, int v_addr, int value)
{
    if (on_disk[pid][0] != -1)
    {
        int to_evict = evict(pid);
        swap(to_evict, on_disk[pid][0]);
    }
    int phys_addr = translate_ptable(pid, v_addr);
    int v_page= find_page(v_addr);
    char buffer[10] = "";

    if (write_list[pid][v_page] == 1)
    {
        if (page_exists[pid][v_page] == 1)
        {
            if (on_disk[pid][v_page + 1] != -1)
            {
		    int to_evict = evict(pid);
        swap(to_evict, on_disk[pid][v_page+1]);
                //replace_page(pid, v_page);
            }
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
        printf("ERROR: Writes are not allowed to this page\n");
    }

    return 0; // Success
}

// Loads value from physical memory
int load(int pid, int v_addr)
{
    int v_page = find_page(v_addr);
    printf("On disk for pid %d: %d\n", pid, on_disk[pid][0]);
    if (on_disk[pid][0] != -1)
    {
        int to_evict = evict(pid);
        swap(to_evict, on_disk[pid][0]);
    }
    printf("On disk for v_page %d: %d\n", v_page, on_disk[pid][v_page + 1]);
    if (on_disk[pid][v_page + 1] != -1)
    {
        int to_evict = evict(pid);
        swap(to_evict, on_disk[pid][v_page+1]);
	    //replace_page(pid, v_page);
    }
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

// Chooses physical page to evict from memory
// Current algorithm is round robin, will skip page if it is the process's page table
int evict(int pid)
{
    int ptable = find_page(pid_array[pid]); // Physical page where pid's ptable is

    int cur_evict = last_evict + 1;
    if (cur_evict >= 5)
    {
        cur_evict = 0;
    }
    if (cur_evict == ptable)
    {
        cur_evict++;
        if (cur_evict >= 5)
        {
            cur_evict = 0;
        }
    }

    last_evict = cur_evict;
    return cur_evict;
}

// Changes mapping of virtual page in a page table when swapping in from disk
int remap(int pid, int v_page, int p_page)
{
    // Change physical address of page and overwrite entry in page table
    char full_str[16] = "";
    char buffer[10];
    int been_allocated = -1;

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
    

    return 0; //Success
}

// Swaps page, handles array data for disk location
int replace_page(int pid, int v_page)
{
	/*
    int to_evict = evict(pid);
    int disk_loc = -1;
    if (v_page != -1)
    {
        disk_loc = on_disk[pid][v_page + 1];
        if (disk_loc == -1) disk_loc = 0;
    }
    int new_line = -1;*/

    // Find the process and page we are removing from memory
    /*int r_pid = -1;
    int r_vpage = -1;
    for (int i = 0; i < MAX_PROC; i++)
    {
        if (pid_array[i] != -1)
        {
            int cur_addr = pid_array[i];
            for (int j = 0; j < 16; j++) // Only look up to end of page table virtual page
            {
                if (memory[cur_addr] == ',') // PTE Separator
                {
                    if(memory[cur_addr + 1] - '0' == to_evict)
                    {
                        r_pid = i;
                        r_vpage = memory[cur_addr - 1] - '0';
                        printf("test1\n");
                    }
                }
                cur_addr++;
            }
        }
    }

    // Page table we need is on disk
    int cur_ptable = -1;
    if (r_pid == -1)
    {
        for (int i = 0; i < MAX_PROC; i++)
        {
            cur_ptable = on_disk[i][0];
            printf("test3: On disk for pid %d: is %d, table address is %d\n", i, on_disk[i][0], pid_array[i]);
            if (on_disk[i][0] != -1)
            {
                swap(to_evict, cur_ptable);
                pid_array[i] = find_address(to_evict);
                int cur_addr = pid_array[i];
                for (int j = 0; j < 16; j++) // Only look up to end of page table virtual page
                {
                    if (memory[cur_addr] == ',') // PTE Separator
                    {
                        if(memory[cur_addr + 1] - '0' == to_evict)
                        {
                            r_pid = j;
                            r_vpage = memory[cur_addr - 1] - '0';
                            printf("test2\n");
                        }
                    }
                    cur_addr++;
                }
            }
            if (r_pid != -1) break;
        }
    }*/
/*
    new_line = swap(to_evict, disk_loc); // Swap pages
    free_list[to_evict] = -1; // Deallocated physical page
    if (v_page != -1)
    {
        remap(pid, v_page); // Remaps swapped in page to a physical page
        on_disk[pid][v_page + 1] = -1; // Update page that was swapped from disk
    }
    printf("Replace pid: %d, Replace v_page: %d\n", r_pid, r_vpage);
    on_disk[r_pid][r_vpage + 1] = new_line; // Update page that was swapped to disk
*/
    return 0; // Success
}

// Swaps page from physical memory and disk, returns lineNum page was put in disk
int swap(int page, int lineNum)
{
    int start = find_address(page);
    char putTemp[16];
    char getTemp[16];
    int replaceMem = -1;
    int putLine = -1;
    int ptable_flag = -1;

    // If page to swap is a page table, erase address in pid_array
    for(int i = 0; i < MAX_PROC; i++)
    {
        if (start == pid_array[i])
        {
            pid_array[i] = -1;
            ptable_flag = i;
        }
    }

    for(int i = 0; i < 16; i++)
    {
        putTemp[i] = memory[start + i];
    }

    if(lineNum != -1) // If lineNum is -1, don't try to get something from disk
    	replaceMem = getFromDisk(&getTemp, lineNum);
    putLine = putToDisk(putTemp);

    if(putLine == -1)
    {
        printf("ERROR: Could not put page to disk.\n");
        return -1;
    }
    else if(replaceMem != -1)
    {
        for(int i = 0; i < 16; i++)
        {
            memory[start + i] = getTemp[i];
        }
    }
    else // Cannot swap in new memory after putting old in disk, replace memory with empty page
    {
        for(int i = 0; i < 16; i++)
        {
            memory[start + i] = '*';
        }
    }

		// Find the process & page we are putting in memory
	if(lineNum != -1)
	{
		for(int i = 0; i < MAX_PROC; i++)
		{
			for(int j = 0; j < MAX_PROC + 1; j++)
			{
				if(on_disk[i][j] == lineNum)
				{
					on_disk[i][j] = -1;
					if(j != 0)
						remap(i, j - 1, page);
					else if(j == 0 && ptable_flag == -1)
						pid_array[i] = start;
					break;
				}
			}
		}
	}
	
    // Find the process and page we are removing from memory
if(ptable_flag != -1) // Swapping out page instead of page table
{
    int r_pid = -1;
    int r_vpage = -1;
	// Page table in physical memory
    for (int i = 0; i < MAX_PROC; i++)
    {
        if (pid_array[i] != -1) // Find page table
        {
            int cur_addr = pid_array[i];
		printf("cc page table of %d is ", i);
            for (int j = 0; j < 16; j++) // Only look up to end of page table virtual page
            {
		    printf("%c", memory[cur_addr]);
                if (memory[cur_addr] == ',') // PTE Separator
                {
                    if(memory[cur_addr + 1] - '0' == page)
                    {
                        r_pid = i;
                        r_vpage = memory[cur_addr - 1] - '0';
                    }
                }
                cur_addr++;
            }
		printf(" cc\n");
        }
    }
	// Page table in disk
	int cur_ptable = -1;
    if (r_pid == -1)
    {
        for (int i = 0; i < MAX_PROC; i++)
        {
            cur_ptable = on_disk[i][0]; // ptable location on disk
            if (cur_ptable != -1)
            {
                replaceMem = getFromDisk(&getTemp, cur_ptable);
                for (int j = 0; j < 16; j++) // Only look up to end of page table virtual page
                {
                    if (getTemp[j] == ',') // PTE Separator
                    {
                        if(getTemp[j + 1] - '0' == page)
                        {
                            r_pid = j;
                            r_vpage = getTemp[j - 1] - '0';
                        }
                    }
                }
            }
            if (r_pid != -1) break;
        }
    }
	if(r_pid != -1 && r_vpage != -1)
	{
		on_disk[r_pid][r_vpage + 1] = putLine;
	}
	else
	{
		printf("ERROR: Page being swapped out doesn't exist in a process?\n");
	}
}
	else
	{
	on_disk[ptable_flag][0] = putLine;	
	}
	
	
    //logMem();
    printf("Swapped frame %d to disk at swap slot %d\n", page, putLine);
    if (lineNum != -1)
    {
        printf("Swapped disk slot %d into frame %d\n", lineNum, page);
    }
    if (ptable_flag != -1)
    {
        printf("Put page table for PID %d into swap slot %d\n", ptable_flag, putLine);
    }
    return putLine;
}

// Puts page in disk
int putToDisk(char page[16])
{
    int line_placement = -1; // Where line is on disk
    char currChar;
    int pageCounter = 0; // Counts each character of a page

    disk = fopen("disk.txt", "r+");
    if(disk == NULL)
    {
        printf("ERROR: Cannot open disk in putToDisk.\n");
        return -1;
    }

    do
    {
        currChar = fgetc(disk);
        pageCounter++;

        if(feof(disk)) // Empty file, put page in
        {
            for(int i = 0; i < 16; i++)
            {
                fputc(page[i], disk);
            }
            fputc('\n', disk);
            line_placement = 0;
            break;
        }
        else // Look for a free space
        {
            if(pageCounter == 16)
            {
                line_placement++;
            }
            if(currChar == '!' && pageCounter == 16) // This line in disk is free, all '!'
            {
                fseek(disk, -16, SEEK_CUR);
                for(int i = 0; i < 16; i++)
                {
                    fputc(page[i], disk);
                }
                break;
            }
            else if(pageCounter > 16)
            {
                pageCounter = 0;
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
    char currChar;
    int pageCounter = 0; // Counts each character of a page

    disk = fopen("disk.txt", "r+");
    if(disk == NULL)
    {
	printf("ERROR: Cannot open disk in getFromDisk.\n");
        return -1;
    }

    do
    {
        currChar = fgetc(disk);
        pageCounter++;

        if(feof(disk) && line_placement == -1)
        {
            printf("ERROR: Cannot get page from empty disk.\n");

            fclose(disk);
            return -1;
        }
        else
        {
            if(pageCounter == 16)
            {
                line_placement++;
            }
            if(line_placement == lineNum && pageCounter == 16) // Get this line from disk
            {
                fseek(disk, -16, SEEK_CUR);
                for(int i = 0; i < 16; i++)
                {
                    currChar = fgetc(disk);
                    (*pageHolder)[i] = currChar;
                    fseek(disk, -1, SEEK_CUR);
                    fputc('!', disk); // Replace this line with a free line, all '!'
                }
                fputc('\n', disk);

                fclose(disk);
                return 0;
            }
            else if(pageCounter > 16)
            {
                pageCounter = 0;
            }
        }
    }
    while(currChar != EOF);

    fclose(disk);
    return -1;
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

    // Clean disk
    disk = fopen("disk.txt", "w");
    if(disk == NULL)
    {
	printf("ERROR: Cannot open disk in main.");
	return -1;
    }
    else
    	fclose(disk);

    // Initialize ptable, free and write lists
    for (int i = 0; i < MAX_PROC; i++)
    {
        pid_array[i] = -1;
        free_list[i] = -1;
        for (int j = 0; j < MAX_PAGES + 1; j++)
        {
            if (j < 4)
            {
                write_list[i][j] = 0;
                page_exists[i][j] = 0;
            }
            on_disk[i][j] = -1;
        }
    }

    // Initialize physical memory
    for (int i = 0; i < SIZE; i++)
    {
        memory[i] = '*';
    }

    while (is_end != 1)
    {
        printf("Instruction?: ");
        // Receive stdin
        if (argc <= 1)
        {
            // Read sequence from file
            if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            {
                is_end = 1;
                printf("End of File. Exiting\n");
                break;
            }
            buffer[strcspn(buffer, "\n")] = 0; // Remove newline
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


        if (pid >= 4 || pid < 0)
        {
            printf("ERROR: Process ID %d is invalid! Only ID's 0 to 3 are allowed\n", pid);
        }
        else if (v_addr >= 64 || v_addr < 0)
        {
            printf("ERROR: Virtual address %d is invalid! Only virtual addresses 0 to 63 are allowed\n", v_addr);
        }
        else if (inst_type == 1)
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

    for (int i = 0; i < MAX_PROC; i++)
    {
        printf("%d\n", pid_array[i]);
    }
    return 0;
}
