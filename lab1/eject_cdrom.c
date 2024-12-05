#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

int main() {
    const char *device = "/dev/cdrom"; // 替換為你的CD-ROM設備路徑

    // 開啟CD-ROM設備
   int fd = open(device, O_RDONLY);
    if (fd == -1) {
        perror("Error opening CD-ROM device");
        exit(EXIT_FAILURE);
    }

    // 解鎖CD-ROM門
    if (ioctl(fd, CDROM_LOCKDOOR, 0) == -1) {
        perror("Error unlocking CD-ROM door");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 彈出CD-ROM
    if (ioctl(fd, CDROMEJECT) == -1) {
        perror("Error ejecting CD-ROM");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("CD-ROM ejected successfully.\n");
    close(fd);
    return 0;
}
