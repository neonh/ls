/* ls utility realization */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>


#define MIN_ARG_QTY                 (2)
#define FIRST_DIR_ARG_NUM           (MIN_ARG_QTY)

#define HIDDEN_FILE_PREFIX          ('.')

// Permissions
#define DENIED_CHAR                 ('-')
#define GROUP_QTY                   (3U)
#define PERMISSION_QTY              (3U)
const char PERMISSION[PERMISSION_QTY] = "rwx";

// String lengths
#define MODE_STR_LEN                (1U + GROUP_QTY*PERMISSION_QTY)
#define TIME_STR_LEN                (15U)

// File types
#define REG_FILE                    ('-')
#define DIR_CHAR                    ('d')
#define LINK_CHAR                   ('l')
#define BLOCK_CHAR                  ('b')
#define PIPE_CHAR                   ('p')
#define CHARACTER_CHAR              ('c')
//#define SOCKET_CHAR                 ('s')

// Number for initial memory allocation
#define INIT_FILES_QTY              (64U)
// Maximum allowed lengths for filenames and usernames
#define FNAME_LEN_MAX               (255U)
#define USERNAME_LEN_MAX            (32U)

// File info structure
typedef struct f_info_st
{
    char fname[FNAME_LEN_MAX + 1U];
    char user[USERNAME_LEN_MAX + 1U];
    char group[USERNAME_LEN_MAX + 1U];
    nlink_t nlink;
    off_t size;
    time_t mtime;
    mode_t mode;

} f_info_t;


static int ls_l(const char* const dir);
static void mode_to_str(const mode_t mode, char* const str);
static int cmp_fn(const void* f1, const void* f2);
static void print_files_info(const f_info_t* const files, const size_t qty);


int main(int argc, const char *argv[])
{
    int ret = EXIT_FAILURE;

    // TODO getopt
    // Process only with -l option
	if ((argc >= MIN_ARG_QTY) &&
        (argv[1][0] == '-') &&
        (argv[1][1] == 'l') &&
        (argv[1][2] == '\0'))
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
        ret = ls_l(dirname);

        // For other directories
        for (int i = 1; i < dir_qty; i++)
        {
            dirname = argv[FIRST_DIR_ARG_NUM + i];
            printf("\n%s:\n", dirname);
            ret |= ls_l(dirname);
        }
    }
    else
    {
        fprintf(stderr, "Only -l option is supported\n");
    }

    return ret;
}


// Function for "ls -l" command - print long info about files in directory
static int ls_l(const char* const dirname)
{
    int ret = EXIT_FAILURE;
	DIR* dir = opendir(dirname);

	if (dir != NULL)
	{
	    struct dirent* d;

        f_info_t* files;
        size_t f_cnt = 0U;
        size_t f_alloc_qty = INIT_FILES_QTY;

        files = malloc(sizeof(f_info_t) * f_alloc_qty);

        // Read
        while ((d = readdir(dir)) != NULL)
        {
            // Ignore hidden
            if (d->d_name[0U] != HIDDEN_FILE_PREFIX)
            {
                struct stat st;
                struct passwd* user;
                struct group* group;

                if ((stat(d->d_name, &st) == 0) &&
                    ((user = getpwuid(st.st_uid)) != NULL) &&
                    ((group = getgrgid(st.st_gid)) != NULL))
                {
                    size_t len;

                    if (f_cnt == f_alloc_qty)
                    {
                        // Increase memory for files structures
                        f_alloc_qty *= 2U;
                        files = realloc(files, sizeof(f_info_t) * f_alloc_qty);
                    }

                    // Copy file info
                    files[f_cnt].mode = st.st_mode;
                    files[f_cnt].nlink = st.st_nlink;
                    strcpy(files[f_cnt].user, user->pw_name);
                    strcpy(files[f_cnt].group, group->gr_name);
                    files[f_cnt].size = st.st_size;
                    files[f_cnt].mtime = st.st_mtime;
                    strcpy(files[f_cnt].fname, d->d_name);

                    // Increase number of files
                    f_cnt++;
                }
            }
        }
        closedir(dir);

        // Sort
        if (f_cnt > 1U)
        {
            qsort(files, f_cnt, sizeof(f_info_t), &cmp_fn);
        }

        // Print
        // TODO print total blocks numbers

        print_files_info(files, f_cnt);
        free(files);

        ret = EXIT_SUCCESS;
    }

    return ret;
}


// Get file type and permissions string from mode value
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
    else
    {
        type = '?';
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
    // Terminate string
    str[char_num] = '\0';
}


// Function for comparing filenames
static int cmp_fn(const void* f1, const void* f2)
{
    return strcmp(((f_info_t*)f1)->fname, ((f_info_t*)f2)->fname);
}


// Print info from files struct
static void print_files_info(const f_info_t* const files, const size_t qty)
{
    off_t max_size = 10;
    size_t max_size_len = 1U;
    nlink_t max_nlink = 10U;
    size_t max_nlink_len = 1U;
    size_t max_user_len = 1U;
    size_t max_group_len = 1U;

    // Find max lengths of print fields
    for (size_t i = 0U; i < qty; i++)
    {
        const f_info_t* f = &files[i];
        size_t len;

        len = strlen(f->user);
        if (len > max_user_len) max_user_len = len;

        len = strlen(f->group);
        if (len > max_group_len) max_group_len = len;

        if (f->nlink >= max_nlink)
        {
            max_nlink *= 10U;
            max_nlink_len++;
        }

        if (f->size >= max_size)
        {
            max_size *= 10U;
            max_size_len++;
        }
    }

    // Print
    for (size_t i = 0U; i < qty; i++)
    {
        const f_info_t* f = &files[i];
        struct tm* time;
        char time_str[TIME_STR_LEN + 1U];
        char mode_str[MODE_STR_LEN + 1U];

        // Mode
        mode_to_str(f->mode, mode_str);
        printf("%s ", mode_str);

        // Links quantity
        printf("%*ld ", (int)max_nlink_len, f->nlink);

        // User
        printf("%*s ", (int)max_user_len, f->user);

        // Group
        printf("%*s ", (int)max_group_len, f->group);

        // Size
        printf("%*ld ", (int)max_size_len, f->size);

        // Modification time
        time = localtime(&f->mtime);
        strftime(time_str, sizeof(time_str), "%b %e %H:%M", time);
        printf("%s ", time_str);

        // Name
        printf("%s\n", f->fname);
    }
}
