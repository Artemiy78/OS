#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define MAX_DATA 4088

typedef struct
{
	uint32_t length;
	uint32_t last;
	char data[MAX_DATA];
} Message;

typedef struct
{
	Message *msg;
	sem_t *empty;
	sem_t *full;
	char shm_name[64];
	char empty_name[64];
	char full_name[64];
} Channel;

static inline void die(const char *text)
{
	write(STDERR_FILENO, text, strlen(text));
	_exit(1);
}

static inline void number_to_string(int n, char *out)
{
	char tmp[16];
	int len = 0;

	if (n == 0)
	{
		tmp[len++] = '0';
	}
	while (n > 0)
	{
		tmp[len++] = '0' + n % 10;
		n = n / 10;
	}
	for (int i = 0; i < len; i++)
	{
		out[i] = tmp[len - 1 - i];
	}
	out[len] = '\0';
}

static inline void build_name(char *out, const char *suffix, const char *id, const char *tail)
{
	strcpy(out, "/lab3_");
	strcat(out, suffix);
	strcat(out, "_");
	strcat(out, id);
	strcat(out, tail);
}

static inline void fill_names(Channel *ch, const char *suffix, const char *id)
{
	build_name(ch->shm_name, suffix, id, "_shm");
	build_name(ch->empty_name, suffix, id, "_empty");
	build_name(ch->full_name, suffix, id, "_full");
}

static inline void channel_create(Channel *ch, const char *suffix, const char *id)
{
	fill_names(ch, suffix, id);

	shm_unlink(ch->shm_name);
	sem_unlink(ch->empty_name);
	sem_unlink(ch->full_name);

	int fd = shm_open(ch->shm_name, O_RDWR | O_CREAT, 0600);
	if (fd == -1)
		die("error: failed to create shared memory\n");

	if (ftruncate(fd, sizeof(Message)) == -1)
		die("error: failed to resize shared memory\n");

	ch->msg = mmap(NULL, sizeof(Message), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ch->msg == MAP_FAILED)
		die("error: failed to map shared memory\n");

	close(fd);

	ch->empty = sem_open(ch->empty_name, O_CREAT, 0600, 1);
	ch->full = sem_open(ch->full_name, O_CREAT, 0600, 0);
	if (ch->empty == SEM_FAILED || ch->full == SEM_FAILED)
		die("error: failed to create semaphore\n");
}

static inline void channel_open(Channel *ch, const char *suffix, const char *id)
{
	fill_names(ch, suffix, id);

	int fd = shm_open(ch->shm_name, O_RDWR, 0);
	if (fd == -1)
		die("error: failed to open shared memory\n");

	ch->msg = mmap(NULL, sizeof(Message), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ch->msg == MAP_FAILED)
		die("error: failed to map shared memory\n");

	close(fd);

	ch->empty = sem_open(ch->empty_name, 0);
	ch->full = sem_open(ch->full_name, 0);
	if (ch->empty == SEM_FAILED || ch->full == SEM_FAILED)
		die("error: failed to open semaphore\n");
}

static inline void channel_close(Channel *ch)
{
	sem_close(ch->empty);
	sem_close(ch->full);
	munmap(ch->msg, sizeof(Message));
}

static inline void channel_destroy(Channel *ch)
{
	channel_close(ch);
	sem_unlink(ch->empty_name);
	sem_unlink(ch->full_name);
	shm_unlink(ch->shm_name);
}

#endif