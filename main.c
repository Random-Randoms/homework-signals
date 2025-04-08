#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>


// config
#define DEBUG


// constants
#define MAX_FILENAME 64
#define BUF_SIZE 1024

#define DEFAULT_LOG "default.log"
#define DEFAULT_TIME 1


// status
volatile sig_atomic_t term = 0;
volatile sig_atomic_t intr = 0;
volatile sig_atomic_t hup = 0;
volatile sig_atomic_t alrm = 0;
volatile sig_atomic_t usr1 = 0;

char status = 0;

#define DAEMONIZE       1 << 0
#define IS_DAEMON       1 << 1
#define WAS_FIFO_OPENED 1 << 2
#define VERBOSE         1 << 3

unsigned int time = 0;
char fifo_name[MAX_FILENAME] = "";
char log_name[MAX_FILENAME] = "";
int log_fd = 0;


// statistics
#define REASON_DAEMON 0
#define REASON_ALARM  1
#define REASON_USER   2

const char* stat_reason_text[] = {
    "daemon", "alarm ", "user  "
};

unsigned int opens = 0;
size_t bytes = 0;
unsigned int alarms = 0;


// exit codes
#define SUCCESS    0
#define ERR_ARGS   1
#define ERR_OPEN   2
#define ERR_READ   3
#define ERR_FORK   4
#define ERR_SSID   5
#define ERR_CHDIR  6
#define ERR_CREATE 7
#define ERR_DUP    8
#define ERR_HNDLR  9

void finish(int code) {
    if (status & WAS_FIFO_OPENED)
        remove(fifo_name);

    if (status & IS_DAEMON)
        close(log_fd);

    exit(code);
}

void error(const char* msg) {
    fprintf(stderr, "%s", msg);
}

void cannot_create_fifo() {
    error("cannot create fifo\n");
    finish(ERR_CREATE);
}

void cannot_open_fifo() {
    error("cannot open fifo\n");
    finish(ERR_OPEN);
}

void cannot_open_log() {
    error("cannot open log\n");
    finish(ERR_OPEN);
}

void read_error() {
    error("read from fifo failed\n");
    finish(ERR_READ);
}

void fork_error() {
    error("fork error\n");
    finish(ERR_FORK);
}

void set_sid_error() {
    error("set sid failed\n");
    finish(ERR_SSID);
}

void chdir_error() {
    error("chdir failed\n");
    finish(ERR_CHDIR);
}

void dup_error() {
    error("dup failed\n");
    finish(ERR_DUP);
}

void handler_reg_error() {
    error("cannot register signal handler\n");
    finish(ERR_HNDLR);
}

// actions
void print_stat(char reason) {
    printf("[%s] opens: %4d, bytes: %10ld, alarms: %4d\n",
        stat_reason_text[reason], opens, bytes, alarms);
    fflush(stdout);
}

void setup_alarm() {
    alarm(time);
}

void daemonize() {
    fflush(stdout);

    daemon(1, 1);
    
    if (status & VERBOSE)
        printf("daemon id: %d\n", getpid());
    else 
        printf("%d\n", getpid());
    fflush(stdout);

    if ((log_fd = open(log_name, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0) cannot_open_log();
    
    status |= IS_DAEMON;

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    if (dup2(log_fd, STDOUT_FILENO) < 0) dup_error();
    if (dup2(log_fd, STDERR_FILENO) < 0) dup_error();
    
    setup_alarm();
    printf("process was daemonized\n");
    print_stat(REASON_DAEMON);
}


// signals
void handler(int signum) {
    fflush(stdout);
    switch(signum) {
        case SIGTERM: term = 1; break;
        case SIGINT:  intr = 1; break;
        case SIGHUP:  hup  = 1; break;
        case SIGUSR1: usr1 = 1; break;
        case SIGALRM: {
            alrm = 1;
            alarm(time);
            break;
        }
    }
}

void link_handler() {
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, 0) < 0) handler_reg_error();
    if (sigaction(SIGTERM, &sa, 0) < 0) handler_reg_error();
    if (sigaction(SIGQUIT, &sa, 0) < 0) handler_reg_error();
    if (sigaction(SIGHUP, &sa, 0) < 0) handler_reg_error();
    if (sigaction(SIGUSR1, &sa, 0) < 0) handler_reg_error();
    if (sigaction(SIGALRM, &sa, 0) < 0) handler_reg_error();
}

void term_handler() {
    term = 0;

    finish(SUCCESS);
}

void intr_handler() {
    intr = 0;

    finish(SUCCESS);
}

void hup_handler() {
    hup = 0;
    daemonize();
}

void alarm_handler() {
    ++alarms;
    alrm = 0;
    print_stat(REASON_ALARM);
}

void usr_handler() {
    usr1 = 0;
    print_stat(REASON_USER);
}

void external_handler() {
    if (term) term_handler();
    if (alrm) alarm_handler();
    if (hup)  hup_handler();
    if (usr1) usr_handler();
}

// invalid arguments reasons
#define UNKNOWN_ARG     1
#define MULTIPLE_N      2
#define MULTIPLE_H      3
#define MULTIPLE_SRC    4
#define MULTIPLE_LOG    5
#define MULTIPLE_DAEMON 6
#define TOO_LONG_NAME   7
#define BAD_N_VALUE     8
#define NO_NAME         9


// argument parsing
void invalid_arguments(int reason) {
    switch(reason) {
        case UNKNOWN_ARG:     error("unknown argument\n"); break;
        case MULTIPLE_N:      error("multiple -N flags\n"); break;
        case MULTIPLE_H:      error("multiple -H flags\n"); break;
        case MULTIPLE_SRC :   error("multiple --src flags\n"); break;
        case MULTIPLE_LOG :   error("multiple --log flags\n"); break;
        case MULTIPLE_DAEMON: error("multiple daemon flags\n"); break;
        case TOO_LONG_NAME:   error("too long name\n"); break;
        case BAD_N_VALUE:     error("bad N value\n"); break;
        case NO_NAME:         error("no --name provided\n"); break;
        default:              error("argument error\n"); break;
    }
    
    finish(ERR_ARGS);
}

void parse_args(int argc, char** argv) {
    char parse_n = 0;
    char parse_name = 0;
    char parse_log = 0;

    for (int i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "--daemon")) {
            if (status & DAEMONIZE) invalid_arguments(MULTIPLE_DAEMON);

            status |= DAEMONIZE;
            continue;
        }

        if (!strcmp(argv[i], "-N")) {
            if (time || parse_n) invalid_arguments(MULTIPLE_N);

            parse_n = 1;
            continue;
        }

        if (!strcmp(argv[i], "-H")) {
            if (status & VERBOSE) invalid_arguments(MULTIPLE_H);

            status |= VERBOSE;
            continue;
        }

        if (!strcmp(argv[i], "--src")) {
            if (fifo_name[0] || parse_name) invalid_arguments(MULTIPLE_SRC);

            parse_name = 1;
            continue;
        }

        if (!strcmp(argv[i], "--log")) {
            if (log_name[0] || parse_log) invalid_arguments(MULTIPLE_LOG);

            parse_log = 1;
            continue;
        }

        if (parse_n) {
            time = atoi(argv[i]);
            if (time) {
                parse_n = 0;
                continue;
            }
            
            invalid_arguments(BAD_N_VALUE);
        }

        if (parse_name) {
            if (strlen(argv[i]) > MAX_FILENAME) invalid_arguments(TOO_LONG_NAME);

            strcpy(fifo_name, argv[i]);
            parse_name = 0;
            continue;
        }

        if (parse_log) {
            if (strlen(argv[i]) > MAX_FILENAME) invalid_arguments(TOO_LONG_NAME);

            strcpy(log_name, argv[i]);
            parse_log = 0;
            continue;
        }
        
        if (i) invalid_arguments(UNKNOWN_ARG);
    }

    if (fifo_name[0] == 0) invalid_arguments(NO_NAME);
    if (log_name[0] == 0) strcpy(log_name, DEFAULT_LOG);
    if (time == 0) time = DEFAULT_TIME;
}


// main
int main(int argc, char** argv) {
    parse_args(argc, argv);
    link_handler();
    setup_alarm();

    if (mkfifo(fifo_name, 0600)) {
        if (errno != EEXIST) cannot_create_fifo();
        
        struct stat stt;

        stat(fifo_name, &stt);

        if ((stt.st_mode & S_IFIFO) == 0)
            cannot_create_fifo();
    } else status |= WAS_FIFO_OPENED;

    if ((status & DAEMONIZE) && !(status & IS_DAEMON)) daemonize();

    while (1) {
        char buf[BUF_SIZE];
        char last = 0;
        int pipe = open(fifo_name, O_RDONLY);

        if (pipe < 0) {
            if (errno != EINTR) cannot_open_fifo();

            if (intr) intr_handler();
            external_handler();
            continue;
        }
        ++opens;

        while (1) {
            int num = read(pipe, buf, BUF_SIZE);

            if (num < 0) {
                if (errno != EINTR) read_error();

                external_handler();
                continue;
            }
            if (num == 0) break;
            bytes += num;
            last = buf[num - 1];

            write(STDOUT_FILENO, buf, num);
        }

        if (last != '\n') write(STDOUT_FILENO, "\n", 1);

        close(pipe);

        if (intr) intr_handler();
        
    }

    finish(SUCCESS);
}