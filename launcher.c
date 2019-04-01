#define _GNU_SOURCE

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>

#define mask 0766

void do_copy(char *src, char *dst) {
    printf("copying %s to %s\n", src, dst);
    int input, output;
    assert((input = open(src, O_RDONLY)) + 1);
    assert((output = creat(dst, mask)) + 1);
    off_t bytesCopied = 0;
    struct stat fileinfo = {0};
    fstat(input, &fileinfo);
    assert(sendfile(output, input, &bytesCopied, fileinfo.st_size) + 1);
    close(input);
    close(output);
}

int do_mount(char *src, char *dst) {
    return mount(src, dst, "tmpfs", MS_BIND | MS_REC, NULL);
}

void do_link(char *src, char *dst) {
    if (do_mount(src, dst) == 0)
        return;
    assert(unlink(dst) + 1);
    assert(symlink(src, dst) + 1);
}

void do_link_dir(char *src, char *dst) {
    if (do_mount(src, dst) == 0)
        return;
    assert(symlink(src, dst) + 1);
}

void do_prepare(char *name, char *dataname) {
    if (access(dataname, F_OK) == -1) {
        do_copy(name, dataname);
    }
    do_link(dataname, name);
}

void do_prepare_dir(char *name, char *dataname) {
    if (access(dataname, F_OK) == -1) {
        mkdir(dataname, mask);
    }
    do_link_dir(dataname, name);
}

#define prepare(name) do_prepare("/server/" name, "/data/" name)
#define prepare_dir(name) do_prepare_dir("/server/" name, "/data/" name)

int main() {
    prepare("ops.json");
    prepare("permissions.json");
    prepare("whitelist.json");
    prepare("server.properties");
    prepare_dir("worlds");
    execl("./bedrock_server", NULL);
}
