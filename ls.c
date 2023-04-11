/*
"ls -l" utility realization
*/

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
// Maximum allowed lengthes for filenames and usernames
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


static void mode_to_str(const mode_t mode, char* const str);
static int cmp_fn(const void* f1, const void* f2);
static void print_file_info(const f_info_t* const f);
static int ls_l(const char* const dir);


int main(int argc, const char *argv[])
{
    int ret = EXIT_FAILURE;

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

static int cmp_fn(const void* f1, const void* f2)
{
    return strcmp(((f_info_t*)f1)->fname, ((f_info_t*)f2)->fname);
}

static void print_file_info(const f_info_t* const f)
{
    struct tm* time;
    char time_str[TIME_STR_LEN + 1U];
    char mode_str[MODE_STR_LEN + 1U];

    // Mode
    mode_to_str(f->mode, mode_str);
    printf("%s ", mode_str);

    // Links quantity
    printf("%ld ", f->nlink);

    // User
    printf("%s ", f->user);

    // Group
    printf("%s ", f->group);

    // Size
    printf("%ld ", f->size);

    // Modification time
    time = localtime(&f->mtime);
    strftime(time_str, sizeof(time_str), "%b %e %H:%M", time);
    printf("%s ", time_str);

    // Name
    printf("%s\n", f->fname);
}

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

        // TODO
        // 1 find max len of size and links fields
        size_t max_fname_len = 0U;

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
                    strcpy(files[f_cnt].fname, d->d_name);

                    // TODO
                    len = strlen(d->d_name);
                    max_fname_len = (len > max_fname_len)? len: max_fname_len;

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
        for (size_t i = 0U; i < f_cnt; i++)
        {
            print_file_info(&files[i]);
        }
        free(files);

        ret = EXIT_SUCCESS;
    }

    return ret;
}
