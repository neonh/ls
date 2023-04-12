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
#include <unistd.h>


#define MIN_ARG_QTY                 (2)
#define FIRST_DIR_ARG_NUM           (MIN_ARG_QTY)
#define BYTES_IN_KB                 (1024U)
#define DECIMAL_BASE                (10U)
#define HIDDEN_FILE_PREFIX          ('.')
const char CUR_DIR[] = ".";

// Permissions
#define DENIED_CHAR                 ('-')
#define GROUP_QTY                   (3U)
#define PERMISSION_QTY              (3U)
const char PERMISSION[PERMISSION_QTY] = "rwx";

// String lengths
#define MODE_STR_LEN                (1U + GROUP_QTY*PERMISSION_QTY)
#define TIME_STR_LEN                (15U)

// Date formats
const char DATE_TIME_FORMAT[] = "%b %e %H:%M";
const char DATE_YEAR_FORMAT[] = "%b %e  %Y";

// File types
#define REG_FILE                    ('-')
#define DIR_CHAR                    ('d')
#define LINK_CHAR                   ('l')
#define BLOCK_CHAR                  ('b')
#define PIPE_CHAR                   ('p')
#define CHARACTER_CHAR              ('c')
#define SOCKET_CHAR                 ('s')

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


static int  ls_l(const char* const path);
static void mode_to_str(const mode_t mode, char* const str);
static int  cmp_fn(const void* f1, const void* f2);
static void print_info(const char* const dirname, const f_info_t* const files, const size_t qty);
static void get_abs_name(char* abs_name, const char* const dirname, const char* const fname);


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
        const char* path = CUR_DIR;

        if (dir_qty > 0)
        {
            // If directory name specified
            path = argv[FIRST_DIR_ARG_NUM];
            if (dir_qty > 1)
            {
                // If more than one directory specified - print dirname before output
                printf("%s:\n", path);
            }
        }
        // Output for first directory
        ret = ls_l(path);

        // For other directories
        for (int i = 1; i < dir_qty; i++)
        {
            path = argv[FIRST_DIR_ARG_NUM + i];
            printf("\n%s:\n", path);
            ret |= ls_l(path);
        }
    }
    else
    {
        fprintf(stderr, "Only -l option is supported\n");
    }

    return ret;
}


// Function for "ls -l" command - print long info about files in directory
static int ls_l(const char* const path)
{
    int ret = EXIT_FAILURE;
    struct stat st;

    if (lstat(path, &st) == 0)
    {
        // Check if it is directory or single file
        if (S_ISDIR(st.st_mode))
        {
            // Directory
            DIR* dir = opendir(path);

            if (dir != NULL)
            {
                struct dirent* d;

                f_info_t* files;
                size_t f_cnt = 0U;
                size_t f_alloc_qty = INIT_FILES_QTY;
                off_t total_blocks = 0;
                // Get block size
                off_t block_size = st.st_blksize;

                files = malloc(sizeof(f_info_t) * f_alloc_qty);

                // Read
                while ((d = readdir(dir)) != NULL)
                {
                    // Ignore hidden
                    if (d->d_name[0U] != HIDDEN_FILE_PREFIX)
                    {
                        struct passwd* user;
                        struct group* group;
                        char abs_name[FILENAME_MAX + 1U];

                        get_abs_name(abs_name, path, d->d_name);

                        if ((lstat(abs_name, &st) == 0) &&
                            ((user = getpwuid(st.st_uid)) != NULL) &&
                            ((group = getgrgid(st.st_gid)) != NULL))
                        {
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

                            // Count I/O blocks if any st_blocks allocated
                            // (symlinks could have st_size > 0 but st_blocks == 0)
                            if (st.st_blocks > 0)
                            {
                                total_blocks += ((st.st_size - 1) / block_size) + 1;
                            }

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

                // Print total size in 1KB blocks
                printf("total %ld\n", (total_blocks * block_size) / BYTES_IN_KB);
                // Print files list
                print_info(path, files, f_cnt);
                free(files);

                ret = EXIT_SUCCESS;
            }
        }
        else
        {
            // Single file
            f_info_t file;
            struct passwd* user;
            struct group* group;
            char abs_name[FILENAME_MAX + 1U];

            get_abs_name(abs_name, CUR_DIR, path);

            if ((lstat(abs_name, &st) == 0) &&
                ((user = getpwuid(st.st_uid)) != NULL) &&
                ((group = getgrgid(st.st_gid)) != NULL))
            {
                // Copy file info
                file.mode = st.st_mode;
                file.nlink = st.st_nlink;
                strcpy(file.user, user->pw_name);
                strcpy(file.group, group->gr_name);
                file.size = st.st_size;
                file.mtime = st.st_mtime;
                strcpy(file.fname, path);
                // Print file info
                print_info(CUR_DIR, &file, 1U);
            }
        }
    }
    else
    {
        // TODO error
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
    else if (S_ISSOCK(mode))
    {
        type = SOCKET_CHAR;
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
static void print_info(const char* const dirname, const f_info_t* const files, const size_t qty)
{
    off_t max_size = 0;
    nlink_t max_nlink = 0U;
    size_t max_size_len = 0U;
    size_t max_nlink_len = 0U;
    size_t max_user_len = 0U;
    size_t max_group_len = 0U;

    // Find max lengths of print fields and max numeric values
    for (size_t i = 0U; i < qty; i++)
    {
        const f_info_t* f = &files[i];
        size_t len;

        len = strlen(f->user);
        if (len > max_user_len) max_user_len = len;

        len = strlen(f->group);
        if (len > max_group_len) max_group_len = len;

        if (f->nlink > max_nlink) max_nlink = f->nlink;

        if (f->size > max_size) max_size = f->size;
    }
    // Calculate max length of numeric fields
    while (max_size > 0)
    {
        max_size /= DECIMAL_BASE;
        max_size_len++;
    }
    while (max_nlink > 0U)
    {
        max_nlink /= DECIMAL_BASE;
        max_nlink_len++;
    }

    // Print
    for (size_t i = 0U; i < qty; i++)
    {
        const f_info_t* f = &files[i];
        // File type and permissions
        char mode_str[MODE_STR_LEN + 1U];
        // Modification time
        char mtime_str[TIME_STR_LEN + 1U];
        struct tm* mtime;
        const char* time_fmt;
        // Current time
        time_t ct = time(NULL);
        int cur_year = localtime(&ct)->tm_year;

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
        mtime = localtime(&f->mtime);
        if (mtime->tm_year == cur_year)
        {
            time_fmt = DATE_TIME_FORMAT;
        }
        else
        {
            time_fmt = DATE_YEAR_FORMAT;
        }
        strftime(mtime_str, sizeof(mtime_str), time_fmt, mtime);
        printf("%s ", mtime_str);

        // Name
        printf("%s", f->fname);

        // If link - print real path
        if (S_ISLNK(f->mode))
        {
            char abs_name[FILENAME_MAX + 1U];
            char link_path[FILENAME_MAX + 1U];

            get_abs_name(abs_name, dirname, f->fname);
            memset(link_path, '\0', FILENAME_MAX);
            readlink(abs_name, link_path, FILENAME_MAX);

            printf(" -> %s", link_path);
        }
        printf("\n");
    }
}


// Get absolute filename
static void get_abs_name(char* abs_name, const char* const dirname, const char* const fname)
{
    if (fname[0U] == '/')
    {
        abs_name = (char*)fname;
    }
    else
    {
        size_t len = strlen(dirname);
        strcpy(abs_name, dirname);
        // Check if dirname ends with '/'
        if (dirname[len-1U] != '/')
        {
            abs_name[len] = '/';
            abs_name[len+1U] = '\0';

        }
        // Concatenate with filename
        strcat(abs_name, fname);
    }
}
