/*
"ls -l" utility realization
*/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#define MIN_ARG_QTY     (2)


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
        int dir_qty = argc - MIN_ARG_QTY;

        if (dir_qty == 0)
        {
            ls_l(".");
        }
        else
        {
            for (int i = 0; i < dir_qty; i++)
            {
                const char* dirname = argv[MIN_ARG_QTY + i];
                printf("%s:\n", dirname);
                ls_l(dirname);
                printf("\n");
            }
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
	DIR *dirs = opendir(dirname);
	if (dirs != NULL)
	{
        while ((d = readdir(dirs)) != NULL)
        {
            // Ignore hidden
            if (d->d_name[0] != '.')
            {
                printf("%s\n", d->d_name);
            }
        }
    }
}
