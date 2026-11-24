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

    /* Создаем программный канал: pipefd[0] - чтение, pipefd[1] - запись */
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
        /* Закрываем неиспользуемый конец для записи */
        close(pipefd[1]);

        printf("[CHILD] Запущен. Ожидаю данные от родителя...\n");
        fflush(stdout);

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;

        /* Читаем порциями, пока родитель не закроет pipefd[1] (read вернет 0 / EOF) */
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';

            /* Приводим каждый символ к верхнему регистру */
            for (ssize_t i = 0; i < bytes_read; i++) {
                buffer[i] = (char)toupper((unsigned char)buffer[i]);
            }

            printf("[CHILD] Получено и преобразовано: %s", buffer);
            /* Если строка не заканчивалась на \n, добавим перенос */
            if (bytes_read > 0 && buffer[bytes_read - 1] != '\n') {
                printf("\n");
            }
            fflush(stdout);
        }

        /* Закрываем дескриптор чтения */
        close(pipefd[0]);
        exit(EXIT_SUCCESS);

    } else {
        /* ================= РОДИТЕЛЬСКИЙ ПРОЦЕСС ================= */
        /* Закрываем неиспользуемый конец для чтения */
        close(pipefd[0]);

        printf("[PARENT] Связь через канал установлена!\n");
        printf("[PARENT] Введите текст (пустая строка или Ctrl+D — выход):\n");
        fflush(stdout);

        char input[BUFFER_SIZE];

        while (1) {
            printf("> ");
            fflush(stdout);

            if (fgets(input, sizeof(input), stdin) == NULL) {
                /* Нажат Ctrl+D (EOF) */
                printf("\n[PARENT] Получен EOF (Ctrl+D) — завершаю ввод.\n");
                break;
            }

            /* Если введена пустая строка (только перенос строки) */
            if (input[0] == '\n' || (input[0] == '\r' && input[1] == '\n')) {
                printf("[PARENT] Пустая строка — завершаю ввод.\n");
                break;
            }

            /* Отправляем строку в канал потомку */
            if (write(pipefd[1], input, strlen(input)) == -1) {
                perror("[PARENT] Ошибка записи в канал");
                break;
            }

            /* Небольшая задержка, чтобы вывод потомка не перемешивался с приглашением ввода */
            usleep(20000);
        }

        /* КРИТИЧНО: закрываем конец записи, чтобы потомок в read() получил 0 (EOF) */
        close(pipefd[1]);

        /* Ждем завершения потомка, предотвращая появление зомби */
        wait(NULL);

        printf("[PARENT] Потомок завершился. Родитель тоже завершает работу.\n");
    }

    return EXIT_SUCCESS;
}
