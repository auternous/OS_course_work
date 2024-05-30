
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct {
    long msg_type;
    char command[100];
    int terminal_id;
    float amount;
    int availability;  // Добавляем поле для изменения доступности терминала и состояния АЗС
    char new_fuel_type[20]; // Добавляем поле для изменения типа топлива
} Message;

int main() {
    key_t key = ftok("gas_station", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    Message message;

    while (1) {
        printf("\nEnter a command (status, sell, change_availability, change_fuel_type, change_status, exit): ");
        scanf("%s", message.command);

        message.msg_type = 1;

        if (strcmp(message.command, "sell") == 0) {
            printf("Enter terminal ID and amount to sell: ");
            scanf("%d %f", &message.terminal_id, &message.amount);
        } else if (strcmp(message.command, "change_availability") == 0) {
            printf("Enter terminal ID and availability (1 for Yes, 0 for No): ");
            scanf("%d %d", &message.terminal_id, &message.availability);
        } else if (strcmp(message.command, "change_fuel_type") == 0) {
            printf("Enter terminal ID and new fuel type: ");
            scanf("%d %s", &message.terminal_id, message.new_fuel_type);
        } else if (strcmp(message.command, "change_status") == 0) {
            printf("Enter new status (1 for Open, 0 for Closed): ");
            scanf("%d", &message.availability);
        } else if (strcmp(message.command, "exit") == 0) {
            msgsnd(msgid, &message, sizeof(message), 0);
            break;
        }

        msgsnd(msgid, &message, sizeof(message), 0);
    }

    return 0;
}
