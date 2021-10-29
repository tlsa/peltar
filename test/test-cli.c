
#include <stdio.h>
#include <stdlib.h>

#include "../src/lib/cli.h"

static struct peltar_options {
	bool vsync;
	bool fullscreen;
	uint64_t width;
	uint64_t height;
	const char *path;
	const char *outfile;
	const char *logfile;
} opt;

static const struct cli_table_entry cli_entries[] = {
	{ .l = "vsync",       .s = 'v',  .t = CLI_BOOL,   .v.b = &opt.vsync,
	  .d = "Start up in fullscreen mode." },
	{ .l = "fullscreen",  .s = 'f',  .t = CLI_BOOL,   .v.b = &opt.fullscreen,
	  .d = "Start up in fullscreen mode." },
	{ .l = "width",       .s = 'w',  .t = CLI_UINT,   .v.u = &opt.width,
	  .d = "Window width in pixels." },
	{ .l = "height",      .s = 'h',  .t = CLI_UINT,   .v.u = &opt.height,
	  .d = "Window height in pixels." },
	{ .l = "path",        .s = 'p',  .t = CLI_STRING, .v.s = &opt.path,
	  .d = "Path to a texture to use to render on planets." },
	{ .l = "OUTFILE",     .p = true, .t = CLI_STRING, .v.s = &opt.outfile,
	  .d = "This is an output file.  Provide the path to the output file here. This argument must be supplied." },
	{ .l = "LOGFILE",     .p = true, .t = CLI_STRING, .v.s = &opt.logfile,
	  .d = "Optional log file. This is a path to where you want the log to go." },
};

const struct cli_table cli = {
	.entries = cli_entries,
	.count = CLI_ARRAY_LEN(cli_entries),
	.min_positional = 1,
};

struct cli_test {
	bool expected;
	char **argv;
};

#define T(_e, ...) { .expected = _e, .argv = (char *[]){__VA_ARGS__, NULL}, }

struct cli_test tests[] = {
	T(true , "./peltar", "-w500", "-h500", "out.png" ),
	T(true , "./peltar", "-w500", "-h500", "-f", "out.png" ),
	T(true , "./peltar", "-w500", "-h500", "-fv", "out.png" ),
	T(true , "./peltar", "-w500", "-h500", "-fv", "out.png", "log.txt" ),
	T(true , "./peltar", "-w500", "-h500", "-vf", "out.png" ),
	T(false, "./peltar", "-w500", "-h500" ),
	T(false, "./peltar", "-w500", "-h500", "-f" ),
	T(false, "./peltar", "-w500", "-h500", "-fv" ),
	T(false, "./peltar", "-w500", "-h500", "-vf" ),
	T(false, "./peltar", "-w500", "-h500", "-fz", "out.png" ),
	T(false, "./peltar", "-w500", "-h500", "-zf", "out.png" ),
	T(true , "./peltar", "-w500", "-h500", "-ff", "out.png" ),
	T(true , "./peltar", "-fw500", "-h500", "-ff", "out.png" ),
	T(true , "./peltar", "-ffv", "-w500", "-h500", "out.png" ),
	T(false, "./peltar", "-w 500", "-h 500", "out.png" ),
	T(true , "./peltar", "-w", "500", "-h", "500", "out.png" ),
	T(false, "./peltar", "-wh", "500", "out.png" ),
	T(true , "./peltar", "-fh", "500", "out.png" ),
	T(true , "./peltar", "-fh", "500", "out.png", "log.txt" ),
	T(false, "./peltar", "-fh", "500", "out.png", "log.txt", "foo" ),
	T(true , "./peltar", "-h", "500", "-h", "500", "out.png" ),
	T(false, "./peltar", "-h", "500", "-hf", "500", "out.png" ),
	T(true , "./peltar", "-p", "500", "out.png" ),
	T(true , "./peltar", "-p", "/directory/dir", "out.png" ),
	T(true , "./peltar", "--path", "/directory/dir", "out.png" ),
	T(false, "./peltar", "--path /directory/dir", "out.png" ),
	T(true , "./peltar", "--path=/directory/dir", "out.png" ),
	T(false, "./peltar", "-p /directory/dir", "out.png" ),
	T(false, "./peltar", "-p/directory/dir", "out.png" ),
	T(true , "./peltar", "--width=500", "out.png" ),
	T(false, "./peltar", "-p/directory/dir" ),
	T(false, "./peltar", "--width=500" ),
	T(false, "./peltar", "--width 500", "out.png" ),
	T(true , "./peltar", "--width", "500", "out.png" ),
	T(true , "./peltar", "--width", "500", "out.png", "log.txt" ),
	T(true , "./peltar", "out.png", "log.txt", "-fh", "500" ),
	T(true , "./peltar", "out.png", "-fh", "500", "log.txt" ),
	T(false, "./peltar", "-pf", "out.png" ),
};

static inline int get_argc(char **argv)
{
	int ret = 0;

	while (*argv++ != NULL) {
		ret++;
	}

	return ret;
}

int main(void)
{
	int ret = EXIT_SUCCESS;

	cli_help(&cli, "./peltar");

	for (unsigned i = 0; i < CLI_ARRAY_LEN(tests); i++) {
		struct cli_test *t = &tests[i];
		int argc = get_argc(t->argv);

		if (cli_parse(&cli, argc, t->argv) != t->expected) {
			ret = EXIT_FAILURE;
			fprintf(stderr, "%u FAIL\n", i);
			goto out;
		}

		fprintf(stderr, "%u PASS\n", i);
	}

out:
	fprintf(stderr, "######\n");
	fprintf(stderr, " %s\n", ret == EXIT_SUCCESS ? "PASS" : "FAIL");
	fprintf(stderr, "######\n");
	return ret;
}
