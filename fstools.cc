
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>

//adapted from SilkJS

char* readFile2(const char* path) {


    printf("readFile, path: %s \n", path);
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("could not open init file: %s\n", path);
        return NULL;
    }
    flock(fd, LOCK_SH);
    lseek(fd, 0, 0);
    std::string s;
   
    char buf[1024];
    ssize_t count;
    while ((count = read(fd, buf, 1024))) {
        s = s.append(buf, count);
        //cout << s << std::endl;
    }
    flock(fd, LOCK_UN);
    close(fd);


    char* ret = strdup(s.c_str()); 
    return ret;
}

