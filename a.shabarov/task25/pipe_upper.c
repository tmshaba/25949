#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

int main(void) {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return EXIT_FAILURE;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* ================= ДОЧЕРНИЙ ПРОЦЕСС ================= */
        close(pipefd[1]);

        printf("[CHILD] Запущен. Ожидаю данные от родителя...\n");
        fflush(stdout);

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        size_t total_changed = 0;

        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            size_t line_changed = 0;

            for (ssize_t i = 0; i < bytes_read; i++) {
                unsigned char orig = (unsigned char)buffer[i];
                unsigned char upper = (unsigned char)toupper(orig);

                /* Если символ изменился — увеличиваем счетчики */
                if (orig != upper) {
                    line_changed++;
                    total_changed++;
                }
                buffer[i] = (char)upper;
            }

            printf("[CHILD] Получено и преобразовано: %s", buffer);
            if (bytes_read > 0 && buffer[bytes_read - 1] != '\n') {
                printf("\n");
            }
            printf("[CHILD] Изменено символов в строке: %zu\n", line_changed);
            fflush(stdout);
        }

        printf("[CHILD] Всего за сессию изменено символов: %zu\n", total_changed);
        close(pipefd[0]);
        exit(EXIT_SUCCESS);

    } else {
        /* ================= РОДИТЕЛЬСКИЙ ПРОЦЕСС ================= */
        close(pipefd[0]);

        printf("[PARENT] Связь через канал установлена!\n");
        printf("[PARENT] Введите текст (пустая строка или Ctrl+D — выход):\n");
        fflush(stdout);

        char input[BUFFER_SIZE];

        while (1) {
            printf("> ");
            fflush(stdout);

            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("\n[PARENT] Получен EOF (Ctrl+D) — завершаю ввод.\n");
                break;
            }

            if (input[0] == '\n' || (input[0] == '\r' && input[1] == '\n')) {
                printf("[PARENT] Пустая строка — завершаю ввод.\n");
                break;
            }

            if (write(pipefd[1], input, strlen(input)) == -1) {
                perror("[PARENT] Ошибка записи в канал");
                break;
            }

            usleep(25000);
        }

        close(pipefd[1]);
        wait(NULL);

        printf("[PARENT] Потомок завершился. Родитель тоже завершает работу.\n");
    }

    return EXIT_SUCCESS;
}
