#include <unistd.h>

int main() {
    char buf[4096];
    ssize_t bytes;
 
    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        for (int i = 0; i < bytes; ++i) {
            if (buf[i] >= 'a' && buf[i] <= 'z') {
                buf[i] = buf[i] - 'a' + 'A';
            }
        }
        write(STDOUT_FILENO, buf, bytes);
    }
 
    return 0;
}
