#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define TRUE 1
#define FALSE 0

char*
fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
    return buf;
}

void
find(char *dir_path, char *target_file_name)
{
    char buf[512], *p;
    int fd;
    struct stat st;
    struct dirent de;

    if((fd = open(dir_path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", dir_path);
        exit(1);
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", dir_path);
        close(fd);
        exit(1);
    }

    if(strlen(dir_path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find: path too long\n");
        exit(0);
    }
    strcpy(buf, dir_path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        printf("checking for %s\n", de.name);
        if(de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0){
            printf("find: cannot stat %s\n", buf);
            continue;
        }

        switch (st.type) {
            case T_FILE:
                if(strcmp(de.name, target_file_name) == 0) {
                    printf("%s\n", fmtname(buf));
                }
                break;
            case T_DIR:
                find(buf, target_file_name);
                break;
        }
    }
    close(fd);
}

int
is_target_file_type(char *dir_path, int target_type)
{
    int fd;
    int is_dir;
    struct stat st;

    if((fd = open(dir_path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", dir_path);
        exit(1);
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", dir_path);
        close(fd);
        exit(1);
    }

    is_dir = st.type == target_type;

    close(fd);

    return is_dir;
}

int
main(int argc, char *argv[])
{
    if(argc != 3) {
        fprintf(2, "usage: find [dir] [file_name]\n");
        exit(1);
    }
    printf("check args sucess\n");
    if(is_target_file_type(argv[1], T_DIR) == FALSE) {
        fprintf(2, "usage: find [dir] [file_name]\n");
        exit(1);
    }
    printf("check argv[1] is directory success\n");
    find(argv[1], argv[2]);
    exit(0);
}