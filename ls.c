/*
"ls -l" utility realization
*/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#define MIN_ARG_QTY                 (2)
#define FIRST_DIR_ARG_NUM           (MIN_ARG_QTY)


static void ls_l(const char* const dir);


int main(int argc, const char *argv[])
{
    int ret = EXIT_FAILURE;

    // Process only with -l option
	if ((argc >= MIN_ARG_QTY) &&
        (argv[1][0] == '-') &&
        (argv[1][1] == 'l') &&
        (argv[1][2] == 0U))
	{
        int dir_qty = argc - FIRST_DIR_ARG_NUM;
        // Default directory
        const char* dirname = ".";

        if (dir_qty > 0)
        {
            // If directory name specified
            dirname = argv[FIRST_DIR_ARG_NUM];
            if (dir_qty > 1)
            {
                // If more than one directory specified - print dirname before output
                printf("%s:\n", dirname);
            }
        }
        // Output for first directory
        ls_l(dirname);

        // For other directories
        for (int i = 1; i < dir_qty; i++)
        {
            dirname = argv[FIRST_DIR_ARG_NUM + i];
            printf("\n%s:\n", dirname);
            ls_l(dirname);
        }

        ret = EXIT_SUCCESS;
    }
    else
    {
        fprintf(stderr, "Only -l option is supported\n");
    }

    return ret;
}


static void ls_l(const char* const dirname)
{
	struct dirent *d;
	DIR *dir = opendir(dirname);

	if (dir != NULL)
	{
        while ((d = readdir(dir)) != NULL)
        {
            // Ignore hidden
            if (d->d_name[0] != '.')
            {
                printf("%s\n", d->d_name);
            }
        }
        closedir(dir);
    }
}
