#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

enum {
	NORMAL = 0,
	CLOCKWISE = 90,
	UPSIDE_DOWN = 180,
	COUNTER_CLOCKWISE = 270,
};

struct disp_t {
	int x, y;
	int bpp, line_length;
};


int get_rotated_pos(struct disp_t disp, int x, int y, int rotate)
{
    if (rotate == CLOCKWISE)
        return y * disp.bpp + (disp.x - 1 - x) * disp.y * disp.bpp;
    else if (rotate == UPSIDE_DOWN)
        return (disp.x - 1 - x) * disp.bpp + (disp.y - 1 - y) * disp.line_length;
    else if (rotate == COUNTER_CLOCKWISE)
        return (disp.y - 1 - y) * disp.bpp + x * disp.y * disp.bpp;
    else /* rotate: NORMAL */
        return x * disp.bpp + y * disp.line_length;
}

int main()
{
	int i, j, pos;
	struct disp_t disp = {.x = 1280, .y = 1024, .bpp = 4, .line_length = 5120};

	for (i = 0; i < disp.x; i++) {
		for (j = 0; j < disp.y; j++) {
			pos = get_rotated_pos(disp, i, j, COUNTER_CLOCKWISE);
			if (!(0 <= pos && pos < 5242880)) {
				printf("i:%d j:%d pos:%d\n", i, j, pos);
				exit(EXIT_FAILURE);
			}
			else
				printf("%d\n", pos);
		}
	}
}
