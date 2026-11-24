#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/* Глобальный счетчик сигналов */
static volatile sig_atomic_t beep_count = 0;

/* Обработчик SIGINT (Ctrl+C): издает звук '\a' */
static void sigint_handler(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

/* Безопасный перевод int в строку без использования snprintf/printf */
static int int_to_str(char *buf, int val) {
    char temp[16];
    int len = 0;
    
    if (val == 0) {
        buf[0] = '0';
        return 1;
    }
    while (val > 0) {
        temp[len++] = (char)('0' + (val % 10));
        val /= 10;
    }
    for (int i = 0; i < len; i++) {
        buf[i] = temp[len - 1 - i];
    }
    return len;
}

/* Обработчик SIGQUIT (Ctrl+\): печать статистики и выход */
static void sigquit_handler(int sig) {
    (void)sig;
    const char msg1[] = "\nВсего прозвучало сигналов: ";
    const char msg2[] = "\nПрограмма завершена по SIGQUIT.\n";
    char num_buf[16];

    write(STDOUT_FILENO, msg1, sizeof(msg1) - 1);
    int num_len = int_to_str(num_buf, (int)beep_count);
    write(STDOUT_FILENO, num_buf, (size_t)num_len);
    write(STDOUT_FILENO, msg2, sizeof(msg2) - 1);

    _exit(EXIT_SUCCESS);
}

int main(void) {
    struct sigaction sa_int;
    struct sigaction sa_quit;

    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        return EXIT_FAILURE;
    }

    memset(&sa_quit, 0, sizeof(sa_quit));
    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;

    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        perror("sigaction SIGQUIT");
        return EXIT_FAILURE;
    }

    printf("Пищалка запущена!\n");
    printf("Нажмите Ctrl+C — будет звуковой сигнал\n");
    printf("Нажмите Ctrl+\\ — завершение работы и подсчет сигналов\n");
    printf("Ожидаю сигналов...\n");
    fflush(stdout);

    while (1) {
        pause();
    }

    return EXIT_SUCCESS;
}
