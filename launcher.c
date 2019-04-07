#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <editline.h>

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

void do_touch(char *dst) {
  printf("touching %s\n", dst);
  int output;
  assert((output = creat(dst, mask)) + 1);
  close(output);
}

int do_mount(char *src, char *dst) {
  return mount(src, dst, "tmpfs", MS_BIND | MS_REC | MS_PRIVATE, NULL);
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
  assert(rmdir(dst) + 1);
  assert(symlink(src, dst) + 1);
}

void do_prepare(char *name, char *dataname) {
  if (access(dataname, F_OK) == -1) {
    if (access(name, F_OK) == 0)
      do_copy(name, dataname);
    else
      do_touch(dataname);
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

int start();

int main() {
  prepare("ops.json");
  prepare("permissions.json");
  prepare("whitelist.json");
  prepare("server.properties");
  prepare("Debug_Log.txt");
  prepare("valid_known_packs.json");
  prepare_dir("worlds");
  prepare_dir("world_templates");
  prepare_dir("premium_cache");
  prepare_dir("development_resource_packs");
  prepare_dir("development_behavior_packs");
  prepare_dir("treatments");
  chdir("/server");
  if (getenv("DISABLE_READLINE")) {
    execl("./bedrock_server", "Minecraft Dedicated Server", NULL);
    perror("execl");
    return 1;
  }
  return start();
}

static char *line_buffer;

void output(char *str) {
  printf("\33[s\33[2K\r%s%s%s\33[u", str, "➜ ", rl_line_buffer);
  fflush(stdout);
}

char *get_line() {
  if (line_buffer)
    free(line_buffer);
  return (line_buffer = readline("➜ "));
}

static int stop = 0;
static char child_stack[4096];
static char *buffer = NULL;

void *thread(void *userdata) {
  FILE *file = (FILE *)userdata;
  size_t size = 0;
  while (1) {
    int read = getline(&buffer, &size, file);
    if (read == -1) {
      stop = 1;
      return (void *)-1;
    }
    output(buffer);
  }
  return NULL;
}

void sigHandler(int s) {
  stop = 1;
  fclose(stdin);
}

int start() {
  int stdinfd[2];
  int stdoutfd[2];
  pid_t pid;
  pthread_t worker;
  FILE *rfile, *wfile;
  struct sigaction sigIntHandler;
  line_buffer = NULL;
  assert(pipe(stdinfd) == 0);
  assert(pipe(stdoutfd) == 0);
  switch (pid = fork()) {
  case -1:
    perror("fork");
    return -1;
  case 0:
    close(stdinfd[1]);
    close(stdoutfd[0]);
    dup2(stdinfd[0], STDIN_FILENO);
    dup2(stdoutfd[1], STDOUT_FILENO);
    close(stdinfd[0]);
    close(stdinfd[1]);
    signal(SIGINT, SIG_IGN);
    execl("./bedrock_server", "Minecraft Dedicated Server", NULL);
    perror("execl");
    return -2;
  default:
    rl_initialize();
    close(stdinfd[0]);
    close(stdoutfd[1]);
    rfile = fdopen(stdoutfd[0], "r");
    wfile = fdopen(stdinfd[1], "w");
    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    if (rfile == NULL)
      goto err;
    if (pthread_create(&worker, NULL, thread, rfile) != 0)
      goto err;
    while (!stop) {
      char *line = get_line();
      if (!line)
        break;
      if (strlen(line) == 0)
        continue;
      if (fprintf(wfile, "%s\n", line) < 0)
        break;
      fflush(wfile);
    }
    printf("stopped!\n");
    fprintf(wfile, "stop\n");
    fflush(wfile);
    waitpid(pid, NULL, 0);
    stop = 1;
    fclose(rfile);
    fclose(wfile);
    close(stdoutfd[0]);
    close(stdinfd[1]);
    pthread_join(worker, NULL);
  }
  return 0;
err:
  fprintf(wfile, "stop\n");
  perror("popen");
  pthread_join(worker, NULL);
  return -3;
}
