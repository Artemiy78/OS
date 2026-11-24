#include "common.h"

int main(int argc, char **argv)
{
	if (argc < 2)
		die("usage: child2 <suffix>\n");
	const char *suffix = argv[1];

	Channel in, out;
	channel_open(&in, suffix, "2");
	channel_open(&out, suffix, "3");

	int prev_space = 0;

	int running = 1;
	while (running)
	{
		sem_wait(in.full);
		sem_wait(out.empty);

		uint32_t len = 0;
		for (uint32_t i = 0; i < in.msg->length; i++)
		{
			char c = in.msg->data[i];
			if (c == ' ' && prev_space)
			{
				continue;
			}
			out.msg->data[len] = c;
			len++;
			prev_space = (c == ' ');
		}
		out.msg->length = len;
		out.msg->last = in.msg->last;

		if (in.msg->last)
		{
			running = 0;
		}

		sem_post(in.empty);
		sem_post(out.full);
	}

	channel_close(&in);
	channel_close(&out);
	return 0;
}
