#include <sys/wait.h>
#include "common.h"

int main(void)
{
	char suffix[16];
	number_to_string((int)getpid(), suffix);

	Channel ch1, ch2, ch3;
	channel_create(&ch1, suffix, "1");
	channel_create(&ch2, suffix, "2");
	channel_create(&ch3, suffix, "3");

	pid_t pid1 = fork();
	if (pid1 == -1)
		die("error: failed to fork child1\n");
	if (pid1 == 0)
	{
		execl("./child1", "child1", suffix, NULL);
		die("error: failed to exec child1\n");
	}

	pid_t pid2 = fork();
	if (pid2 == -1)
		die("error: failed to fork child2\n");
	if (pid2 == 0)
	{
		execl("./child2", "child2", suffix, NULL);
		die("error: failed to exec child2\n");
	}

	int running = 1;
	while (running)
	{
		sem_wait(ch1.empty);
		ssize_t bytes = read(STDIN_FILENO, ch1.msg->data, MAX_DATA);
		if (bytes < 0)
		{
			const char text[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, text, sizeof(text) - 1);
			bytes = 0;
		}

		ch1.msg->length = bytes;
		ch1.msg->last = (bytes == 0);
		sem_post(ch1.full);

		sem_wait(ch3.full);
		write(STDOUT_FILENO, ch3.msg->data, ch3.msg->length);
		if (ch3.msg->last)
		{
			running = 0;
		}
		sem_post(ch3.empty);
	}

	waitpid(pid1, NULL, 0);
	waitpid(pid2, NULL, 0);

	channel_destroy(&ch1);
	channel_destroy(&ch2);
	channel_destroy(&ch3);
	return 0;
}
