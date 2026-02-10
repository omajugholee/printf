#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define GRID_WIDTH 30
#define GRID_HEIGHT 12
#define MAX_ENEMIES 6
#define FRAME_USEC 120000

static struct termios orig_termios;

static void disable_raw_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enable_raw_mode(void)
{
	struct termios raw;

	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
	{
		perror("tcgetattr");
		exit(1);
	}
	atexit(disable_raw_mode);

	raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
	{
		perror("tcsetattr");
		exit(1);
	}
}

static int read_key(void)
{
	fd_set readfds;
	struct timeval timeout;
	char c;

	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0)
	{
		if (read(STDIN_FILENO, &c, 1) == 1)
			return (int)c;
	}
	return -1;
}

static void clear_screen(void)
{
	write(STDOUT_FILENO, "\033[2J\033[H", 7);
}

static void draw_frame(int naruto_x, int enemies[MAX_ENEMIES][2], int score, int lives)
{
	char line[GRID_WIDTH + 3];
	int x, y, i;

	clear_screen();
	printf("Naruto Arcade - Dodge the Akatsuki!  Score: %d  Lives: %d\n", score, lives);
	for (y = 0; y < GRID_HEIGHT; y++)
	{
		memset(line, ' ', sizeof(line));
		line[0] = '|';
		line[GRID_WIDTH + 1] = '|';
		line[GRID_WIDTH + 2] = '\0';

		for (i = 0; i < MAX_ENEMIES; i++)
		{
			if (enemies[i][1] == y)
			{
				x = enemies[i][0];
				if (x >= 0 && x < GRID_WIDTH)
					line[x + 1] = 'A';
			}
		}

		if (y == GRID_HEIGHT - 1)
			line[naruto_x + 1] = 'N';

		printf("%s\n", line);
	}
	printf("Controls: A/D to move, Q to quit.\n");
}

static void spawn_enemy(int enemies[MAX_ENEMIES][2])
{
	int i;

	for (i = 0; i < MAX_ENEMIES; i++)	{
		if (enemies[i][1] < 0)
		{
			enemies[i][0] = rand() % GRID_WIDTH;
			enemies[i][1] = 0;
			return;
		}
	}
}

static void update_enemies(int enemies[MAX_ENEMIES][2])
{
	int i;

	for (i = 0; i < MAX_ENEMIES; i++)	{
		if (enemies[i][1] >= 0)
			enemies[i][1] += 1;
	}
}

static int check_collision(int naruto_x, int enemies[MAX_ENEMIES][2])
{
	int i;

	for (i = 0; i < MAX_ENEMIES; i++)	{
		if (enemies[i][1] == GRID_HEIGHT - 1 && enemies[i][0] == naruto_x)
			return 1;
	}
	return 0;
}

static void cleanup_enemies(int enemies[MAX_ENEMIES][2], int *score)
{
	int i;

	for (i = 0; i < MAX_ENEMIES; i++)	{
		if (enemies[i][1] >= GRID_HEIGHT)
		{
			enemies[i][1] = -1;
			*score += 1;
		}
	}
}

int main(void)
{
	int naruto_x = GRID_WIDTH / 2;
	int enemies[MAX_ENEMIES][2];
	int i, score = 0, lives = 3;
	int frame = 0;
	int key = 0;

	srand((unsigned int)time(NULL));
	for (i = 0; i < MAX_ENEMIES; i++)	{
		enemies[i][0] = 0;
		enemies[i][1] = -1;
	}

	enable_raw_mode();
	clear_screen();

	while (lives > 0)
	{
		key = read_key();
		if (key == 'q' || key == 'Q')
			break;
		if (key == 'a' || key == 'A')
		{
			if (naruto_x > 0)
				naruto_x--;
		}
		else if (key == 'd' || key == 'D')
		{
			if (naruto_x < GRID_WIDTH - 1)
				naruto_x++;
		}

		if (frame % 6 == 0)
			spawn_enemy(enemies);

		update_enemies(enemies);

		if (check_collision(naruto_x, enemies))
		{
			lives--;
			for (i = 0; i < MAX_ENEMIES; i++)
				enemies[i][1] = -1;
		}

		cleanup_enemies(enemies, &score);
		draw_frame(naruto_x, enemies, score, lives);

		usleep(FRAME_USEC);
		frame++;
	}

	clear_screen();
	printf("Game Over! Final Score: %d\n", score);
	return 0;
}
