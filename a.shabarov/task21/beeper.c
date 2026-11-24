#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <poll.h>

static volatile sig_atomic_t beep_count = 0;
static volatile sig_atomic_t quit_flag = 0;
static struct termios orig_termios;

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

static void enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    /* Отключаем эхо и канонический ввод, но сохраняем ISIG (для Ctrl+C и Ctrl+\) */
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

/* Обработчик SIGINT (Ctrl+C): 1 гудок */
static void sigint_handler(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

/* Обработчик SIGQUIT (Ctrl+\): выставляем флаг выхода */
static void sigquit_handler(int sig) {
    (void)sig;
    quit_flag = 1;
}

int main(void) {
    struct sigaction sa_int;
    struct sigaction sa_quit;

    /* Настройка SIGINT */
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART; /* перезапуск read при Ctrl+C */
    sigaction(SIGINT, &sa_int, NULL);

    /* Настройка SIGQUIT */
    memset(&sa_quit, 0, sizeof(sa_quit));
    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0; /* прерывает read с EINTR */
    sigaction(SIGQUIT, &sa_quit, NULL);

    enable_raw_mode();

    printf("Пищалка запущена!\n");
    printf("  Ctrl+C        — 1 сигнал (SIGINT)\n");
    printf("  Стрелка ВВЕРХ — 2 сигнала\n");
    printf("  Стрелка ВНИЗ  — 3 сигнала\n");
    printf("  Ctrl+\\        — завершение и подсчет\n");
    fflush(stdout);

    while (!quit_flag) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n <= 0) {
            if (quit_flag) break;
            continue;
        }

        /* Обработка Escape-последовательностей (стрелочки) */
        if (c == 27) { /* ESC */
            struct pollfd pfd;
            pfd.fd = STDIN_FILENO;
            pfd.events = POLLIN;
            pfd.revents = 0;

            if (poll(&pfd, 1, 50) > 0 && (pfd.revents & POLLIN)) {
                char c2;
                if (read(STDIN_FILENO, &c2, 1) > 0 && (c2 == '[' || c2 == 'O')) {
                    if (poll(&pfd, 1, 50) > 0 && (pfd.revents & POLLIN)) {
                        char c3;
                        if (read(STDIN_FILENO, &c3, 1) > 0) {
                            if (c3 == 'A') {
                                /* Стрелка ВВЕРХ: 2 сигнала */
                                write(STDOUT_FILENO, "\a\a", 2);
                                beep_count += 2;
                            } else if (c3 == 'B') {
                                /* Стрелка ВНИЗ: 3 сигнала */
                                write(STDOUT_FILENO, "\a\a\a", 3);
                                beep_count += 3;
                            }
                        }
                    }
                }
            }
        }
    }

    printf("\nВсего прозвучало сигналов: %d\n", (int)beep_count);
    printf("Программа завершена по SIGQUIT.\n");

    return EXIT_SUCCESS;
}
