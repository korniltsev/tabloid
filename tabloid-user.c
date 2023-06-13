#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MY_MAGIC 'G'
#define CMD_SET_PID _IOW(MY_MAGIC, 0, int)

#define BUFFER_SIZE 0x1000

int main(int argc, char *argv[]) {
    int fd = open("/proc/foo", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        int pid = atoi(argv[1]);
        int res = ioctl(fd, CMD_SET_PID, &pid);
        if (res == -1) {
            perror("ioctl");
            close(fd);
            return EXIT_FAILURE;
        }
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [<pid>]\n", argv[0]);
        close(fd);
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        write(1, buffer, bytes_read);
    }

    if (bytes_read == -1) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}