#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include <termbox.h>

// Compile-Time Constants
#define GAME_X 79
#define GAME_Y 39
#define MONSTER_Y 4
#define PLAYER_Y GAME_Y
#define LOG_FILENAME "/tmp/spacei.log"

// Global variables
static unsigned int player_x = GAME_X/2;
static bool bullets[GAME_Y][GAME_X];

// Global constant "prototypes"
static const struct tb_cell empty = {
	.ch = ' ',
	.fg = TB_DEFAULT,
	.bg = TB_DEFAULT,
};
static const struct tb_cell bullet = {
	.ch = '*',
	.fg = TB_DEFAULT,
	.bg = TB_YELLOW,
};
static const struct tb_cell player = {
	.ch = '^',
	.fg = TB_DEFAULT,
	.bg = TB_GREEN,
};
static const struct tb_cell monster = {
	.ch = 'X',
	.fg = TB_DEFAULT | TB_BOLD,
	.bg = TB_RED,
};

/**
 * Moves all bullets "one up"
 */
static void update_bullets(void)
{
	// handle y = 0 case: let bullets vanish
	{
		for (size_t x = 0; x < sizeof(bullets[0]); x++) {
			if (bullets[0][x]) {
				tb_put_cell(x, 0, &empty);
				bullets[0][x] = false;
			}
		}
	}
	// y > 0 case: bullets move up
	for (size_t y = 1; y < sizeof(bullets)/sizeof(bullets[0]); y++) {
		for (size_t x = 0; x < sizeof(bullets[0]); x++) {
			if (bullets[y][x]) {
				bullets[y][x] = false;
				tb_put_cell(x, y, &empty);
				bullets[y-1][x] = true;
				tb_put_cell(x, y-1, &bullet);
			}
		}
	}
}

/**
 * Handles cursor keys, space, and escape
 *
 * @returns 1 if Escape is pressed to signal end of game
 * @returns 0 otherwise
 */
static int handle_key(const uint16_t key)
{
	switch (key) {
	case TB_KEY_ARROW_LEFT:
		if (player_x > 0) {
			tb_put_cell(player_x, PLAYER_Y, &empty);
			player_x--;
			tb_put_cell(player_x, PLAYER_Y, &player);
		}
		break;
	case TB_KEY_ARROW_RIGHT:
		if (player_x < GAME_X) {
			tb_put_cell(player_x, PLAYER_Y, &empty);
			player_x++;
			tb_put_cell(player_x, PLAYER_Y, &player);
		}
		break;
	case TB_KEY_SPACE:
		bullets[PLAYER_Y-1][player_x] = true;
		break;
	case TB_KEY_ESC:
		return 1;
	default:
		break;
	}
	return 0;
}

int main(void)
{
	int status = EXIT_SUCCESS;

	// Open logfile
	FILE *const log = fopen(LOG_FILENAME, "w+");
	if (!log) {
		perror("fopen");
		status = EXIT_FAILURE;
		goto exit;
	}
	fprintf(stderr, "Logfile: %s\n", LOG_FILENAME);

	// Initialize Termbox
	{
		const int tbe = tb_init();
		if (tbe) {
			switch (tbe) {
			case TB_EUNSUPPORTED_TERMINAL:
			case TB_EFAILED_TO_OPEN_TTY:
			case TB_EPIPE_TRAP_ERROR:
				status = EXIT_FAILURE;
				fprintf(log, "Termbox error code: %d\n", tbe);
				break;
			default:
				status = EXIT_FAILURE;
				fprintf(log, "Unknown termbox error code: %d\n",
						tbe);
			}
			goto close_log;
		}
	}

	// Check terminal size
	{
		const int tbw = tb_width();
		const int tbh = tb_height();
		fprintf(log, "Dimensions: width %d, height %d\n", tbw, tbh);
		if (tbw < GAME_X || tbh < GAME_Y) {
			status = EXIT_FAILURE;
			fprintf(log, "Console too small\n");
			fprintf(log, "Needs: width: %d, height: %d\n",
					GAME_X, GAME_Y);
			goto shutdown_tb;
		}
	}
	// Initialize board
	//   Monster layout:
	//   (4+5) | X | 5 | X | 5 | ... | (5+4)    = 79 = GAME_X
	for (int i = 1; i <= 11; i++) {
		tb_put_cell(4+ i*5 +1, MONSTER_Y, &monster);
	}
	//   Player
	tb_put_cell(player_x, PLAYER_Y, &player);

	// Show output
	tb_present();

	// Game loop
	struct tb_event e;
	while (true) {
		// give user 100ms for input
		tb_peek_event(&e, 100);
		switch (e.type) {
		case TB_EVENT_KEY:
			if (handle_key(e.key) == 1) goto shutdown_tb;
			break;
		default:
			;
		}

		update_bullets();

		tb_present();
	}

shutdown_tb:
	fprintf(log, "exiting...\n");
	tb_shutdown();
close_log:
	fclose(log);
exit:
	exit(status);
}
