
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_FUEL_TYPES 2  // Устанавливаем количество видов топлива равным 2
#define MAX_TERMINALS 4   // Устанавливаем количество терминалов равным 4

typedef struct {
    char name[20];
    float lower_limit;
    float upper_limit;
    float current_level;
    int available;
} Storage;

typedef struct {
    int id;
    char fuel_type[20];
    int available;
} Terminal;

typedef struct {
    long msg_type;
    char command[100];
    int terminal_id;
    float amount;
    int availability;  // Добавляем поле для изменения доступности терминала
    char new_fuel_type[20]; // Добавляем поле для изменения типа топлива
} Message;

typedef struct {
    Storage storages[MAX_FUEL_TYPES];
    Terminal terminals[MAX_TERMINALS];
    int open; // Добавляем состояние АЗС
} GasStation;

void initialize_storage(Storage* storage, const char* name, float lower_limit, float upper_limit) {
    strcpy(storage->name, name);
    storage->lower_limit = lower_limit;
    storage->upper_limit = upper_limit;
    storage->current_level = upper_limit;
    storage->available = 1;
}

void initialize_terminal(Terminal* terminal, int id, const char* fuel_type, int available) {
    terminal->id = id;
    strcpy(terminal->fuel_type, fuel_type);
    terminal->available = available;
}

void initialize_gas_station(GasStation* station) {
    station->open = 1;

    // Initialize storages
    initialize_storage(&station->storages[0], "Petrol", 100.0, 1000.0);
    initialize_storage(&station->storages[1], "Diesel", 100.0, 1000.0);

    // Initialize terminals
    initialize_terminal(&station->terminals[0], 0, "Petrol", 1);
    initialize_terminal(&station->terminals[1], 1, "Petrol", 1);
    initialize_terminal(&station->terminals[2], 2, "Diesel", 1);
    initialize_terminal(&station->terminals[3], 3, "Diesel", 0);  // Этот терминал недоступен
}

void display_storage_status(Storage* storage) {
    printf("Storage: %s\n", storage->name);
    printf("  Lower limit: %.2f\n", storage->lower_limit);
    printf("  Upper limit: %.2f\n", storage->upper_limit);
    printf("  Current level: %.2f\n", storage->current_level);
    printf("  Available: %s\n", storage->available ? "Yes" : "No");
}

void display_terminal_status(Terminal* terminal) {
    printf("Terminal ID: %d\n", terminal->id);
    printf("  Fuel type: %s\n", terminal->fuel_type);
    printf("  Available: %s\n", terminal->available ? "Yes" : "No");
}

void display_gas_station_status(GasStation* station) {
    printf("Gas Station Status: %s\n", station->open ? "Open" : "Closed");
    for (int i = 0; i < MAX_FUEL_TYPES; i++) {
        display_storage_status(&station->storages[i]);
    }
    for (int i = 0; i < MAX_TERMINALS; i++) {
        display_terminal_status(&station->terminals[i]);
    }
}

void sell_fuel(GasStation* station, int terminal_id, float amount) {
    if (!station->open) {
        printf("Gas station is closed.\n");
        return;
    }

    if (terminal_id < 0 || terminal_id >= MAX_TERMINALS) {
        printf("Invalid terminal ID: %d\n", terminal_id);
        return;
    }

    Terminal* terminal = &station->terminals[terminal_id];
    if (!terminal->available) {
        printf("Terminal %d is not available.\n", terminal_id);
        return;
    }

    Storage* storage = NULL;
    for (int i = 0; i < MAX_FUEL_TYPES; i++) {
        if (strcmp(station->storages[i].name, terminal->fuel_type) == 0) {
            storage = &station->storages[i];
            break;
        }
    }

    if (storage == NULL || !storage->available) {
        printf("Storage for %s is not available.\n", terminal->fuel_type);
        return;
    }

    if (storage->current_level < amount) {
        printf("Not enough fuel in storage. Available: %.2f\n", storage->current_level);
        return;
    }

    // Check if the remaining fuel would be below the lower limit
    if ((storage->current_level - amount) < storage->lower_limit) {
        printf("Cannot sell %.2f of %s. The remaining fuel level would be below the minimum limit of %.2f\n", 
                amount, storage->name, storage->lower_limit);
        return;
    }

    storage->current_level -= amount;
    printf("Sold %.2f of %s from terminal %d. New level: %.2f\n", amount, storage->name, terminal_id, storage->current_level);
}


void change_terminal_availability(GasStation* station, int terminal_id, int availability) {
    if (terminal_id < 0 || terminal_id >= MAX_TERMINALS) {
        printf("Invalid terminal ID: %d\n", terminal_id);
        return;
    }

    station->terminals[terminal_id].available = availability;
    printf("Terminal %d availability changed to: %s\n", terminal_id, availability ? "Yes" : "No");
}

void change_terminal_fuel_type(GasStation* station, int terminal_id, const char* new_fuel_type) {
    if (terminal_id < 0 || terminal_id >= MAX_TERMINALS) {
        printf("Invalid terminal ID: %d\n", terminal_id);
        return;
    }

    strcpy(station->terminals[terminal_id].fuel_type, new_fuel_type);
    printf("Terminal %d fuel type changed to: %s\n", terminal_id, new_fuel_type);
}

void change_gas_station_status(GasStation* station, int status) {
    station->open = status;
    printf("Gas station status changed to: %s\n", status ? "Open" : "Closed");
}

int main() {
    GasStation station;
    initialize_gas_station(&station);

    key_t key = ftok("gas_station", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    Message message;

    // Display initial status
    display_gas_station_status(&station);

    while (1) {
        msgrcv(msgid, &message, sizeof(message), 1, 0);

        if (strcmp(message.command, "status") == 0) {
            display_gas_station_status(&station);
        } else if (strcmp(message.command, "sell") == 0) {
            sell_fuel(&station, message.terminal_id, message.amount);
        } else if (strcmp(message.command, "change_availability") == 0) {
            change_terminal_availability(&station, message.terminal_id, message.availability);
        } else if (strcmp(message.command, "change_fuel_type") == 0) {
            change_terminal_fuel_type(&station, message.terminal_id, message.new_fuel_type);
        } else if (strcmp(message.command, "change_status") == 0) {
            change_gas_station_status(&station, message.availability);
        } else if (strcmp(message.command, "exit") == 0) {
            break;
        } else {
            printf("Unknown command: %s\n", message.command);
        }
    }

    msgctl(msgid, IPC_RMID, NULL);
    return 0;
}
