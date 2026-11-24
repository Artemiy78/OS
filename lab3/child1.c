#include "common.h"

int main(int argc, char **argv)
{
	if (argc < 2)
		die("usage: child1 <suffix>\n");
	const char *suffix = argv[1];

	Channel in, out;
	channel_open(&in, suffix, "1");
	channel_open(&out, suffix, "2");

	int running = 1;
	while (running)
	{
		sem_wait(in.full);
		sem_wait(out.empty);

		for (uint32_t i = 0; i < in.msg->length; i++)
		{
			char c = in.msg->data[i];
			if (c >= 'a' && c <= 'z')
			{
				c = c - 'a' + 'A';
			}
			out.msg->data[i] = c;
		}
		out.msg->length = in.msg->length;
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
