#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>

static void get_permissions(mode_t mode, char *perm_str) {
    if (S_ISDIR(mode)) {
        perm_str[0] = 'd';
    } else if (S_ISREG(mode)) {
        perm_str[0] = '-';
    } else {
        perm_str[0] = '?';
    }

    perm_str[1] = (mode & S_IRUSR) ? 'r' : '-';
    perm_str[2] = (mode & S_IWUSR) ? 'w' : '-';
    perm_str[3] = (mode & S_IXUSR) ? 'x' : '-';

    perm_str[4] = (mode & S_IRGRP) ? 'r' : '-';
    perm_str[5] = (mode & S_IWGRP) ? 'w' : '-';
    perm_str[6] = (mode & S_IXGRP) ? 'x' : '-';

    perm_str[7] = (mode & S_IROTH) ? 'r' : '-';
    perm_str[8] = (mode & S_IWOTH) ? 'w' : '-';
    perm_str[9] = (mode & S_IXOTH) ? 'x' : '-';

    perm_str[10] = '\0';
}

static void format_time(time_t mtime, char *time_buf, size_t buf_size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&mtime);
    if (!tm_info) {
        snprintf(time_buf, buf_size, "??? ?? ??:??");
        return;
    }

    time_t six_months = 180 * 24 * 60 * 60;
    if ((now - mtime > six_months) || (mtime > now)) {
        strftime(time_buf, buf_size, "%b %e  %Y", tm_info);
    } else {
        strftime(time_buf, buf_size, "%b %e %H:%M", tm_info);
    }
}

static void print_file_info(const char *path) {
    struct stat st;

    if (lstat(path, &st) == -1) {
        perror(path);
        return;
    }

    char perm[11];
    get_permissions(st.st_mode, perm);

    char user_str[32];
    struct passwd *pw = getpwuid(st.st_uid);
    if (pw != NULL) {
        snprintf(user_str, sizeof(user_str), "%s", pw->pw_name);
    } else {
        snprintf(user_str, sizeof(user_str), "%u", (unsigned int)st.st_uid);
    }

    char grp_str[32];
    struct group *gr = getgrgid(st.st_gid);
    if (gr != NULL) {
        snprintf(grp_str, sizeof(grp_str), "%s", gr->gr_name);
    } else {
        snprintf(grp_str, sizeof(grp_str), "%u", (unsigned int)st.st_gid);
    }

    /* РАЗМЕР: теперь выводим для обычных файлов И для директорий */
    char size_str[16] = "";
    if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) {
        snprintf(size_str, sizeof(size_str), "%lld", (long long)st.st_size);
    }

    char time_str[32];
    format_time(st.st_mtime, time_str, sizeof(time_str));

    char path_copy[1024];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    char *file_name = basename(path_copy);

    printf("%s %3lu %-8s %-8s %8s %12s %s\n",
           perm,
           (unsigned long)st.st_nlink,
           user_str,
           grp_str,
           size_str,
           time_str,
           file_name);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_file_info(".");
    } else {
        for (int i = 1; i < argc; i++) {
            print_file_info(argv[i]);
        }
    }
    return EXIT_SUCCESS;
}
