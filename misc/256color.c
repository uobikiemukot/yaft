#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <term.h>

enum {
	ANSI = 8,
	ANSI_BOLD = 16,
	COLORCUBE = 232,
	MAX_COLOR = 256,
};

void fatal(char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

void print_color_block(int color)
{
	char *str;

	str = tparm(set_a_background, color);
	putp(str);
	printf("  ");
}

void reset_color()
{
	putp(exit_attribute_mode);
}

int main()
{
	int i, j, k, status, color;

	setupterm(NULL, STDOUT_FILENO, &status);
	if (status != 1)
		fatal("Error in setupterm");

	if (set_a_foreground == NULL
		|| exit_attribute_mode == NULL)
		return 1;

	printf("System colors:\n");
	for (i = 0; i < ANSI; i++)
		print_color_block(i);
	reset_color();
	printf("\n");

	for (i = ANSI; i < ANSI_BOLD; i++)
		print_color_block(i);
	reset_color();
	printf("\n\n");

	printf("Color cube, 6x6x6:\n");
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 6; j++) {
			for (k = 0; k < 6; k++) {
				color = 16 + (j * 36) + (i * 6) + k;
				print_color_block(color);
			}
			putp(exit_attribute_mode);
			printf(" ");
		}
		printf("\n");
	}
	reset_color();
	printf("\n");

	printf("Grayscale ramp:\n");
	for (i = COLORCUBE; i < MAX_COLOR; i++)
		print_color_block(i);
	reset_color();
	printf("\n");

	return 0;
}
