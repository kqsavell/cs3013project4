// Kyle Savell & Antony Qin

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int pid = 0; // Process ID
    int inst_type = 0; // Instruction type
    int v_addr = 0; // Virtual address
    int input = 0; // Value

    // Read stdin
    printf("Instructions?: ");
    if (argc >= 2)
    {
        pid = *argv[1];
    }
    if (argc >= 3)
    {
        if (strncmp(argv[2], "load", sizeof(argv[2])))
        {
            inst_type = 1;
        }
    }
    if (argc >= 4)
    {
        v_addr = *argv[3];
    }
    if (argc >= 5)
    {
        input = *argv[4];
    }
    printf("%d, %d, %d, %d\n", pid, inst_type, v_addr, input);

    return 0;
}
