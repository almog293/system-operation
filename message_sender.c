#include "message_slot.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
    int fd;
    int write_message_len;
    char *message_data;
    unsigned int ch_id;
    if(argc != 4)
    {
        perror("Error - Invalid umber of arguments\n");
        exit(1);
    }
    if((fd = open(argv[1], O_RDWR)) == -1)
    {
        perror("Error - Failed opening file\n");
        exit(1);
    }
    ch_id = atoi(argv[2]);
    write_message_len = strlen(argv[3]);
    message_data = argv[3];
    if(ioctl(fd,MSG_SLOT_CHANNEL,ch_id)<0)
    {
        perror("Error - ioctl\n");
        close(fd);
        exit(1);
    }
    if(write(fd,argv[3],write_message_len)!= write_message_len)
    {
        perror("Error - writing to fd\n");
        close(fd);
        exit(1);
    }
    close(fd);
    exit(0);
}