z#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define SIZE 5
#define CARROTS 2
#define CYCLES_PER_TIME_MACHINE 3

// Game state
char board[SIZE][SIZE];
int carrots_placed = 0;
int cycle_count = 0;
int game_over = 0;

// Mutex for synchronization
pthread_mutex_t mutex;

// Character structure
typedef struct {
    char symbol;
    int x, y;
    int has_carrot;
    int alive;
    int id;
} Character;

Character characters[4];

// Movement directions
int dx[4] = {0, 0, 1, -1};
int dy[4] = {1, -1, 0, 0};

// Check if move is within board
int is_valid(int x, int y) {
    return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

// Enhanced print function
void print_board() {
    printf("  ");
    for (int j = 0; j < SIZE; j++) printf("%d ", j);
    printf("\n");

    for (int i = 0; i < SIZE; i++) {
        printf("%d ", i);
        for (int j = 0; j < SIZE; j++) {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void move_mountain() {
    int old_x = -1, old_y = -1, new_x, new_y;
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (board[i][j] == 'F') {
                old_x = i;
                old_y = j;
                break;
            }

    do {
        new_x = rand() % SIZE;
        new_y = rand() % SIZE;
    } while (board[new_x][new_y] != '.');

    board[old_x][old_y] = '.';
    board[new_x][new_y] = 'F';
    printf("Marvin activated the time machine! Mountain moved to (%d, %d)\n", new_x, new_y);
}

void *character_thread(void *arg) {
    Character *c = (Character *)arg;

    while (!game_over && c->alive) {
        usleep(200000); // Delay to simulate movement

        pthread_mutex_lock(&mutex);
        if (!c->alive || game_over) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        cycle_count++;

        // Remove character from current position
        if (board[c->x][c->y] == c->symbol)
            board[c->x][c->y] = '.';

        // Random direction
        int dir = rand() % 4;
        int nx = c->x + dx[dir];
        int ny = c->y + dy[dir];
        if (!is_valid(nx, ny)) {
            nx = c->x;
            ny = c->y;
        }

        char target = board[nx][ny];

        // Marvin's logic
        if (c->symbol == 'M') {
            for (int i = 0; i < 4; i++) {
                if (i != c->id && characters[i].alive &&
                    characters[i].x == nx && characters[i].y == ny) {
                    if (characters[i].has_carrot) {
                        c->has_carrot = 1;
                        characters[i].has_carrot = 0;
                        printf("Marvin stole a carrot from %c!\n", characters[i].symbol);
                    }
                    characters[i].alive = 0;
                    board[characters[i].x][characters[i].y] = '.';
                    printf("Marvin eliminated %c at (%d,%d)\n", characters[i].symbol, nx, ny);
                }
            }
        }

        if (target == 'F' && !c->has_carrot) {
            nx = c->x; ny = c->y; // Cannot enter mountain
        }

        if (target == 'C' && !c->has_carrot) {
            c->has_carrot = 1;
            printf("%c picked up a carrot at (%d,%d)\n", c->symbol, nx, ny);
        }

        if (target == 'F' && c->has_carrot) {
            carrots_placed++;
            printf("%c placed a carrot on the mountain! Total: %d\n", c->symbol, carrots_placed);
            c->has_carrot = 0;
            if (carrots_placed == CARROTS) {
                printf("%c wins the race!\n", c->symbol);
                game_over = 1;
            }
        }

        // Move character
        c->x = nx;
        c->y = ny;
        board[nx][ny] = c->symbol;

        printf("Board after %c's move:\n", c->symbol);
        print_board();

        if (cycle_count % CYCLES_PER_TIME_MACHINE == 0 && c->symbol == 'M') {
            move_mountain();
        }

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void init_board() {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = '.';

    // Mountain
    int fx, fy;
    do {
        fx = rand() % SIZE; fy = rand() % SIZE;
    } while (board[fx][fy] != '.');
    board[fx][fy] = 'F';

    // Carrots
    for (int i = 0; i < CARROTS; i++) {
        int cx, cy;
        do {
            cx = rand() % SIZE; cy = rand() % SIZE;
        } while (board[cx][cy] != '.');
        board[cx][cy] = 'C';
    }

    // Characters
    char syms[4] = {'B', 'D', 'T', 'M'};
    for (int i = 0; i < 4; i++) {
        int x, y;
        do {
            x = rand() % SIZE; y = rand() % SIZE;
        } while (board[x][y] != '.');
        characters[i] = (Character){syms[i], x, y, 0, 1, i};
        board[x][y] = syms[i];
    }
}

int main() {
    srand(time(NULL));
    pthread_mutex_init(&mutex, NULL);

    init_board();
    printf("Initial Board:\n");
    print_board();

    pthread_t threads[4];
    for (int i = 0; i < 4; i++)
        pthread_create(&threads[i], NULL, character_thread, &characters[i]);

    for (int i = 0; i < 4; i++)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&mutex);
    printf("Game Over.\n");
    return 0;
}
