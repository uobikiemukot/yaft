/* See LICENSE for licence details. */
#include "../yaft.h"
#include "../conf.h"
#include "../util.h"
#include "pseudo.h"
#include "../terminal.h"
#include "../function.h"
#include "../osc.h"
#include "../dcs.h"
#include "../parse.h"
#include "gifsave89.h"

enum {
	TERM_WIDTH      = 640,
	TERM_HEIGHT     = 384,
	INPUT_WAIT      = 0,
	INPUT_BUF       = 1,
	GIF_DELAY       = 10,
	OUTPUT_BUF      = 1024,
	NO_OUTPUT_LIMIT = 16,
};

enum cmap_bitfield {
	RED_SHIFT   = 5,
	GREEN_SHIFT = 2,
	BLUE_SHIFT  = 0,
	RED_MASK	= 3,
	GREEN_MASK  = 3,
	BLUE_MASK   = 2
};

void fork_and_exec(int *master, int lines, int cols)
{
	pid_t pid;
	struct winsize ws = {.ws_row = lines, .ws_col = cols,
		.ws_xpixel = 0, .ws_ypixel = 0};

	pid = eforkpty(master, NULL, NULL, &ws);

	if (pid == 0) { /* child */
		esetenv("TERM", term_name, 1);
		eexecvp(shell_cmd, (const char *[]){shell_cmd, NULL});
	}
}

void check_fds(fd_set *fds, struct timeval *tv, int fd)
{
	FD_ZERO(fds);
	FD_SET(fd, fds);
	tv->tv_sec  = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	eselect(fd + 1, fds, NULL, NULL, tv);
}

void pb_init(struct pseudobuffer *pb)
{
	pb->width  = TERM_WIDTH;
	pb->height = TERM_HEIGHT;
	pb->bytes_per_pixel = BYTES_PER_PIXEL;
	pb->line_length = pb->width * pb->bytes_per_pixel;
	pb->buf = ecalloc(pb->width * pb->height, pb->bytes_per_pixel);
}

void pb_die(struct pseudobuffer *pb)
{
	free(pb->buf);
}

void set_colormap(int colormap[COLORS * BYTES_PER_PIXEL + 1])
{
	int i, ci, r, g, b;
	uint8_t index;

	/* colormap: terminal 256color
	for (i = 0; i < COLORS; i++) {
		ci = i * BYTES_PER_PIXEL;

		r = (color_list[i] >> 16) & bit_mask[8];
		g = (color_list[i] >> 8)  & bit_mask[8];
		b = (color_list[i] >> 0)  & bit_mask[8];

		colormap[ci + 0] = r;
		colormap[ci + 1] = g;
		colormap[ci + 2] = b;
	}
	*/

	/* colormap: red/green: 3bit blue: 2bit
	*/
	for (i = 0; i < COLORS; i++) {
		index = (uint8_t) i;
		ci = i * BYTES_PER_PIXEL;

		r = (index >> RED_SHIFT)   & bit_mask[RED_MASK];
		g = (index >> GREEN_SHIFT) & bit_mask[GREEN_MASK];
		b = (index >> BLUE_SHIFT)  & bit_mask[BLUE_MASK];

		colormap[ci + 0] = r * bit_mask[BITS_PER_BYTE] / bit_mask[RED_MASK];
		colormap[ci + 1] = g * bit_mask[BITS_PER_BYTE] / bit_mask[GREEN_MASK];
		colormap[ci + 2] = b * bit_mask[BITS_PER_BYTE] / bit_mask[BLUE_MASK];
	}
	colormap[COLORS * BYTES_PER_PIXEL] = -1;
}

uint32_t pixel2index(uint32_t pixel)
{
	/* pixel is always 24bpp */
    uint32_t r, g, b;

    /* split r, g, b bits */
    r = (pixel >> 16) & bit_mask[8];
    g = (pixel >> 8)  & bit_mask[8];
    b = (pixel >> 0)  & bit_mask[8];

	/* colormap: terminal 256color
	if (r == g && r == b) { // 24 gray scale
		r = 24 * r / COLORS;
		return 232 + r;
	}                       // 6x6x6 color cube

	r = 6 * r / COLORS;
	g = 6 * g / COLORS;
	b = 6 * b / COLORS;

	return 16 + (r * 36) + (g * 6) + b;
	*/

	/* colormap: red/green: 3bit blue: 2bit
	*/
    // get MSB ..._MASK bits
    r = (r >> (8 - RED_MASK))   & bit_mask[RED_MASK];
    g = (g >> (8 - GREEN_MASK)) & bit_mask[GREEN_MASK];
    b = (b >> (8 - BLUE_MASK))  & bit_mask[BLUE_MASK];

    return (r << RED_SHIFT) | (g << GREEN_SHIFT) | (b << BLUE_SHIFT);
}

void apply_colormap(struct pseudobuffer *pb, unsigned char *img)
{
    int w, h;
    uint32_t pixel = 0;

    for (h = 0; h < pb->height; h++) {
        for (w = 0; w < pb->width; w++) {
            memcpy(&pixel, pb->buf + h * pb->line_length
                + w * pb->bytes_per_pixel, pb->bytes_per_pixel);
            *(img + h * pb->width + w) = pixel2index(pixel) & bit_mask[BITS_PER_BYTE];
        }
    }
}

size_t write_gif(unsigned char *gifimage, int size)
{
    size_t wsize = 0;

    wsize = fwrite(gifimage, sizeof(unsigned char), size, stdout);
    return wsize;
}
void sig_handler(int signo)
{
    if (signo == SIGINT) {
        if (DEBUG)
            fprintf(stderr, "caught signal(no: %d)! exiting...\n", signo);
        tty.loop_flag = false;
    }
}

void sig_set()
{
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(struct sigaction));
    sigact.sa_handler = sig_handler;
    sigact.sa_flags   = SA_RESTART;
    sigaction(SIGINT, &sigact, NULL);
}

void sig_reset()
{
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(struct sigaction));
    sigact.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sigact, NULL);
}

int main(int argc, char *argv[])
{
	uint8_t ibuf[INPUT_BUF], obuf[OUTPUT_BUF];
	int fd, no_output_count = 0;
	ssize_t rsize, osize;
	fd_set fds;
	struct timeval tv;
	struct terminal term;
	struct pseudobuffer pb;

	void *gsdata;
	unsigned char *gifimage = NULL;
	int gifsize, colormap[COLORS * BYTES_PER_PIXEL + 1];
	unsigned char *img;

	fd = (argc < 2) ? STDIN_FILENO: eopen(argv[1], O_RDONLY);

	/* init */
	setlocale(LC_ALL, "");
	pb_init(&pb);
	term_init(&term, pb.width, pb.height);
	sig_set();

	/* fork and exec shell */
	fork_and_exec(&term.fd, term.lines, term.cols);

	/* init gif */
	img = (unsigned char *) ecalloc(pb.width * pb.height, 1);
	set_colormap(colormap);
	if (!(gsdata = newgif((void **) &gifimage, pb.width, pb.height, colormap, 0)))
		return EXIT_FAILURE;

	animategif(gsdata, /* repetitions */ 0, GIF_DELAY,
		/* transparent background */  -1, /* disposal */ 2);

	/* main loop */
	while (no_output_count < NO_OUTPUT_LIMIT) {
		usleep(INPUT_WAIT);
		rsize = read(fd, ibuf, INPUT_BUF);
		//fprintf(stderr, "rsize:%d\n", rsize);
		if (rsize > 0)
			ewrite(term.fd, ibuf, rsize);

		check_fds(&fds, &tv, term.fd);
		if (FD_ISSET(term.fd, &fds)) {
			//fprintf(stderr, "osize:%d\n", osize);
			osize = read(term.fd, obuf, OUTPUT_BUF);
			if (osize > 0) {
				parse(&term, obuf, osize);
				refresh(&pb, &term);

				/* take screenshot */
				apply_colormap(&pb, img);
				putgif(gsdata, img);
			}
			no_output_count = 0;
		} else {
			no_output_count++;
		}
	}

	/* output gif */
	gifsize = endgif(gsdata);
	if (gifsize > 0) {
		write_gif(gifimage, gifsize);
		free(gifimage);
	}
	free(img);

	/* normal exit */
	sig_reset();
	term_die(&term);
	eclose(fd);

	return EXIT_SUCCESS;
}
