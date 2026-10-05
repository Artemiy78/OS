#include <unistd.h>
#include <sys/wait.h>
 
int main() {
    int pipe1[2];
    int pipe_mid[2];
    int pipe2[2];
 
    if (pipe(pipe1) == -1 || pipe(pipe_mid) == -1 || pipe(pipe2) == -1) {
        const char msg[] = "error: failed to create pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }
 
    pid_t pid1 = fork();
 
    switch (pid1) {
    case -1: {
        const char msg[] = "error: failed to fork child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    } break;
 
    case 0: {
        close(pipe1[1]);
        close(pipe_mid[0]);
        close(pipe2[0]);
        close(pipe2[1]);
 
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);
        dup2(pipe_mid[1], STDOUT_FILENO);
        close(pipe_mid[1]);
 
        execl("./child1", "child1", NULL);
 
        const char msg[] = "error: failed to exec child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(1);
    } break;
 
    default:
        break;
    }
 
    pid_t pid2 = fork();
 
    switch (pid2) {
    case -1: {
        const char msg[] = "error: failed to fork child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    } break;
 
    case 0: {
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe_mid[1]);
        close(pipe2[0]);
 
        dup2(pipe_mid[0], STDIN_FILENO);
        close(pipe_mid[0]);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe2[1]);
 
        execl("./child2", "child2", NULL);
 
        const char msg[] = "error: failed to exec child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(1);
    } break;
 
    default:
        break;
    }
 
    close(pipe1[0]);
    close(pipe_mid[0]);
    close(pipe_mid[1]);
    close(pipe2[1]);
 
    char buf[4096];
    char out[4096];
    ssize_t bytes;
    ssize_t got;
 
    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        write(pipe1[1], buf, bytes);
 
        if (buf[bytes - 1] == '\n') {
            do {
                got = read(pipe2[0], out, sizeof(out));
                if (got <= 0) {
                    break;
                }
                write(STDOUT_FILENO, out, got);
            } while (out[got - 1] != '\n');
        }
    }
 
    close(pipe1[1]);
 
    while ((got = read(pipe2[0], out, sizeof(out))) > 0) {
        write(STDOUT_FILENO, out, got);
    }
    close(pipe2[0]);
 
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
 
    return 0;
}
