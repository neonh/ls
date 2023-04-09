/*
"ls -l" utility realization
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

// #include <pwd.h>
// #include <grp.h>
// #include <time.h>

#define MIN_ARG_QTY                 (2)
#define FIRST_DIR_ARG_NUM           (MIN_ARG_QTY)

#define MODE_STR_LEN                (10U)

// Permissions
#define DENIED_CHAR                 ('-')
#define GROUP_QTY                   (3U)
#define PERMISSION_QTY              (3U)
const char PERMISSION[PERMISSION_QTY] = "rwx";

// File types
#define REG_FILE                    ('-')
#define DIR_CHAR                    ('d')
#define LINK_CHAR                   ('l')
#define BLOCK_CHAR                  ('b')
#define PIPE_CHAR                   ('p')
#define CHARACTER_CHAR              ('c')
#define SOCKET_CHAR                 ('s')


static void mode_to_str(const mode_t mode, char* const str);
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


static void mode_to_str(const mode_t mode, char* const str)
{
    // Number of char in output string
    uint8_t char_num = 0U;
    // Bit mask to read permissions
    mode_t mask = S_IRUSR;
    // File type
    char type;

    if (S_ISREG(mode))
    {
        type = REG_FILE;
    }
    else if (S_ISDIR(mode))
    {
        type = DIR_CHAR;
    }
    else if (S_ISLNK(mode))
    {
        type = LINK_CHAR;
    }
    else if (S_ISBLK(mode))
    {
        type = BLOCK_CHAR;
    }
    else if (S_ISFIFO(mode))
    {
        type = PIPE_CHAR;
    }
    else if (S_ISCHR(mode))
    {
        type = CHARACTER_CHAR;
    }
    // else if (S_ISSOCK(mode))
    // {
    //     type = SOCKET_CHAR;
    // }
    else
    {
        // Shouldn't be here
        type = ' ';
    }
    str[char_num++] = type;

    // Permissions
    for (uint8_t g = 0U; g < GROUP_QTY; g++)
    {
        for (uint8_t i = 0U; i < PERMISSION_QTY; i++)
        {
            // Set permission
            str[char_num++] = (mode & mask)? PERMISSION[i]: DENIED_CHAR;
            // Move to next bit
            mask >>= 1;
        }
    }
}

static void ls_l(const char* const dirname)
{
	struct dirent *d;
    struct stat st;
	DIR *dir = opendir(dirname);

	if (dir != NULL)
	{
        while ((d = readdir(dir)) != NULL)
        {
            // Ignore hidden
            if (d->d_name[0] != '.')
            {
                if (stat(d->d_name, &st) == 0)
                {
                    char mode_str[MODE_STR_LEN];
                    mode_to_str(st.st_mode, mode_str);
                    printf("%s ", mode_str);
                    printf(" %s\n", d->d_name);
                }
            }
        }
        closedir(dir);
    }
}
