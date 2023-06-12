#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

int main() {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    // Open the directory
    dir = opendir("path/to/directory");
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    // Read directory entries and print regular file names
    while ((entry = readdir(dir)) != NULL) {
        // Construct the file path
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/%s", "path/to/directory", entry->d_name);

        // Retrieve the file information
        if (stat(file_path, &file_stat) == 0) {
            if (S_ISREG(file_stat.st_mode)) {
                printf("%s\n", entry->d_name);
            }
        }
    }

    // Close the directory
    closedir(dir);

    return 0;
}
