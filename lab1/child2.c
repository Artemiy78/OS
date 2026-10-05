#include <unistd.h>

int main() {
    char in[4096];
    char out[4096];
    ssize_t bytes;

    int prev_space = 0;

    while ((bytes = read(STDIN_FILENO, in, sizeof(in))) > 0) {
        int len = 0;

        for (int i = 0; i < bytes; ++i) {
            if (in[i] == ' ' && prev_space) {
                continue;
            }
            out[len] = in[i];
            len++;
            prev_space = (in[i] == ' ');
        }

        if (len > 0) {
            write(STDOUT_FILENO, out, len);
        }
    }

    return 0;
}
