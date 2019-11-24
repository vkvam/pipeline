#include "config.h"
#include <getopt.h>
#include <ncurses.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <term.h>
#include <unistd.h>

#define PROGRAM_NAME "pipeline"
char *program_name = PROGRAM_NAME;

int abort_ltz(int res) {
    if (res < 0) {
        perror(NULL);
        exit(res);
    }
    return res;
}

int abort_nz(int res) {
    if (res != 0) {
        perror(NULL);
        exit(res);
    }
    return res;
}

#define abort_null(ptr)                                                        \
    ({                                                                         \
        typeof(ptr) _ptr = ptr;                                                \
        if (!_ptr) {                                                           \
            perror(NULL);                                                      \
            exit(-1);                                                          \
        }                                                                      \
        _ptr;                                                                  \
    })

char cmdbuf[100];
// valid terminfo commands are available at `man 5 terminfo`
// http://man7.org/linux/man-pages/man5/terminfo.5.html
void termput(char *cmd) { putp(tgetstr(cmd, (char **)&cmdbuf)); }

void read_show_output(FILE *s, size_t *shown, size_t *total) {
    termput("cd");
    for (;;) {
        char *line = NULL;
        size_t len = 0;
        ssize_t read = getline(&line, &len, s);
        if (read < 0)
            break;
        if (*shown < LINES - 2) {
            // sometimes newline, sometimes not, so chomp off any newline and
            // we'll add it ourselves for consistency.
            if (line[read - 1] == '\n')
                read -= 1;
            // truncate lines that are wider than the screen.
            if (read > COLS)
                read = COLS;
            printf("%.*s\n", (int)read, line);
            *shown += 1;
        }
        *total += 1;
    }
}

int read_command(const char *command, size_t *shown, size_t *total) {
    int child_stdout[2];
    int child_stderr[2];
    abort_ltz(pipe(child_stdout));
    abort_ltz(pipe(child_stderr));

    int pid = abort_ltz(fork());
    if (pid == 0) {
        // child
        close(child_stdout[0]);
        close(child_stderr[0]);
        close(1);
        abort_ltz(dup(child_stdout[1]));
        close(2);
        abort_ltz(dup(child_stderr[1]));
        execl("/bin/sh", "sh", "-c", command, NULL);
    }

    // parent
    close(child_stdout[1]);
    close(child_stderr[1]);
    FILE *c_stdout = abort_null(fdopen(child_stdout[0], "r"));
    FILE *c_stderr = abort_null(fdopen(child_stderr[0], "r"));
    *shown = *total = 0;

    read_show_output(c_stdout, shown, total);

    fclose(c_stdout);
    int status = 0;
    waitpid(pid, &status, 0);
    if (status != 0) {
        // show the stderr instead
        for (int i = 0; i < *shown; ++i)
            termput("up");
        *shown = *total = 0;
        read_show_output(c_stderr, shown, total);
    }
    fclose(c_stderr);

    return WEXITSTATUS(status);
}

int last_status = -1;

int show_preview(const char *a, int b) {
    printf("\n");
    size_t shown = 0;
    size_t total = 0;
    last_status = read_command(rl_line_buffer, &shown, &total);
    termput("mr");
    if (last_status == 0) {
        printf(" %zu lines, showing %zu ", total, shown);
    } else {
        printf(" error in command: %i ", last_status);
    }
    termput("me");
    for (int i = 0; i < (shown + 1); ++i) {
        termput("up");
    }
    // moving the cursor left, necessary for libreadline but not libedit
    printf("\e[9999D");
    rl_forced_update_display();
    return 0;
}

int setup() {
    // libedit wants '\n' but libreadline wants '\r'.
    //
    // this ifdef is a hacky way to detect whether we're using readline or
    // libedit, I'd love to find something cleaner.
    //
    // also note : rl_bind_key seems to be totally broken with libedit, at least
    // on MacOS. I have to use rl_add_defun to see any effect in that
    // environment.
#ifdef RL_STATE_NONE
    abort_nz(rl_add_defun("pipeline-preview", (rl_command_func_t *)show_preview, '\r'));
#else
    abort_nz(rl_add_defun("pipeline-preview", (Function *)show_preview, '\n'));
#endif
    return 0;
}

static struct option const long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}};

void usage(int status) {
    printf("Usage: %s\n", program_name);
    exit(status);
}

void version() {
    printf("%s\n", PACKAGE_STRING);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *const *argv) {
    if (argc)
        program_name = argv[0];
    int c;
    while ((c = getopt_long(argc, argv, "hv", long_options, NULL)) != -1) {
        switch (c) {
        case 'h':
            usage(EXIT_SUCCESS);
            break;
        case 'v':
            version();
            break;
        default:
            usage(EXIT_FAILURE);
        }
    }

    setupterm(NULL, 1, NULL);
    rl_startup_hook = setup;
    char *line = readline("pipeline> ");
    if (line)
        free(line);
    return 0;
}
