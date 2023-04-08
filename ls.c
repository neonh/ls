/*
"ls -l" utility realization
*/

#include <stdio.h>
#include <stdlib.h>
// #include <dirent.h>

#define ARG_QTY     (2)


int main(int argc, const char *argv[])
{
    int ret = EXIT_FAILURE;

	if ((argc == ARG_QTY) &&
        (argv[1][0] == '-') &&
        (argv[1][1] == 'l') &&
        (argv[1][2] == 0U))
	{
        printf("Done\n");

        ret = EXIT_SUCCESS;
    }

    return ret;
}
