/*
 * Looney Race Game
 * Authors: Chidubem Okoye, Yvonne Onmapko
 * Description: A multi-threaded C program simulating a race between characters
 * (Bugs Bunny, Daffy Duck, Tweety, and Marvin) on a 5x5 grid. Characters aim to
 * collect carrots and place them on a mountain to win. Marvin can eliminate others
 * and use a time machine to move the mountain.
 * Date: April 23, 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

// Constants for game configuration
#define SIZE 5                  // Size of the game board (5x5 grid)
#define CARROTS 2               // Number of carrots needed to win
#define CYCLES_PER_TIME_MACHINE 3  // Cycles before Marvin's time machine activates
#define MAX_STEPS 100           // Maximum steps to ensure a winner

// Game state variables
char board[SIZE][SIZE];         // 5x5 game board
int carrots_placed = 0;         // Tracks number of carrots placed on the mountain
int cycle_count = 0;            // Counts total cycles for time machine activation
int game_over = 0;              // Flag to indicate if the game is over
int step_count = 0;             // Tracks total steps to enforce MAX_STEPS limit

// Mutex for thread synchronization to prevent race conditions
pthread_mutex_t mutex;

// Structure to represent a character in the game
typedef struct {
    char symbol;                // Character's symbol (B, D, T, M)
    int x, y;                   // Position on the board
    int has_carrot;             // Flag to indicate if the character has a carrot
    int alive;                  // Flag to indicate if the character is still in the game
    int id;                     // Unique identifier for the character
} Character;

// Array to store the four characters
Character characters[4];

// Arrays to define movement directions (right, left, down, up)
int dx[4] = {0, 0, 1, -1};
int dy[4] = {1, -1, 0, 0};

// Helper function to check if a position is within the board boundaries
int is_valid(int x, int y) {
    return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

// Function to print the current state of the game board
void print_board() {
    // Create a display array to hold symbols, with space for "(C)" suffix
    char display[SIZE][SIZE][6]; // Increased size to 6 to hold "X(C)" + null terminator
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            display[i][j][0] = board[i][j];
            display[i][j][1] = '\0'; // Initialize as a single character string
        }
    }

    // Update display to show "(C)" for characters holding carrots
    for (int i = 0; i < 4; i++) {
        if (characters[i].alive && board[characters[i].x][characters[i].y] == characters[i].symbol) {
            if (characters[i].has_carrot) {
                snprintf(display[characters[i].x][characters[i].y], 6, "%c(C)", characters[i].symbol);
            }
        }
    }

    // Print the board with aligned formatting
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%-4s ", display[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Function to move the mountain to a new random position (used by Marvin's time machine)
void move_mountain() {
    int old_x = -1, old_y = -1, new_x, new_y;
    // Find the current position of the mountain
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (board[i][j] == 'F') {
                old_x = i;
                old_y = j;
                break;
            }

    // Find a new random empty position for the mountain
    do {
        new_x = rand() % SIZE;
        new_y = rand() % SIZE;
    } while (board[new_x][new_y] != '.');

    // Move the mountain to the new position
    board[old_x][old_y] = '.';
    board[new_x][new_y] = 'F';
    printf("Marvin activated the time machine! Mountain moved to (%d, %d)\n", new_x, new_y);
}

// Thread function to handle each character's actions
void *character_thread(void *arg) {
    Character *c = (Character *)arg; // Cast the argument to a Character pointer

    // Continue until the game is over or the character is eliminated
    while (!game_over && c->alive) {
        usleep(200000); // Simulate movement delay (200ms)

        // Lock the mutex to safely access shared resources
        pthread_mutex_lock(&mutex);
        if (!c->alive || game_over) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        cycle_count++;   // Increment cycle counter for time machine
        step_count++;   // Increment step counter to enforce MAX_STEPS

        // Check if the maximum steps have been reached
        if (step_count >= MAX_STEPS) {
            // Force a win by selecting the first alive character
            for (int i = 0; i < 4; i++) {
                if (characters[i].alive) {
                    printf("Max steps reached! %c is declared the winner!\n", characters[i].symbol);
                    game_over = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Remove the character from their current position on the board
        if (board[c->x][c->y] == c->symbol)
            board[c->x][c->y] = '.';

        // Randomly choose a direction to move (right, left, down, up)
        int dir = rand() % 4;
        int nx = c->x + dx[dir], ny = c->y + dy[dir];
        if (!is_valid(nx, ny)) {
            nx = c->x; ny = c->y; // Stay in place if the move is invalid
        }

        char target = board[nx][ny]; // Check the target position

        // Marvin's elimination logic: eliminate other characters on the same spot
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

        // Rule: Cannot step onto the mountain without a carrot
        if (target == 'F' && !c->has_carrot) {
            nx = c->x; ny = c->y; // Stay in place
        }

        // Pick up a carrot if available and the character doesn't already have one
        if (target == 'C' && !c->has_carrot) {
            c->has_carrot = 1;
            printf("%c picked up a carrot at (%d,%d)\n", c->symbol, nx, ny);
        }

        // Drop a carrot on the mountain if the character has one
        if (target == 'F' && c->has_carrot) {
            carrots_placed++;
            printf("%c placed a carrot on the mountain! Total: %d\n", c->symbol, carrots_placed);
            c->has_carrot = 0;
            if (carrots_placed == CARROTS) {
                printf("%c wins the race!\n", c->symbol);
                game_over = 1; // End the game if 2 carrots are placed
            }
        }

        // Update the character's position on the board
        c->x = nx;
        c->y = ny;
        board[nx][ny] = c->symbol;

        // Print the updated board after the move
        printf("Board after %c's move:\n", c->symbol);
        print_board();

        // Activate Marvin's time machine every 3 cycles
        if (cycle_count % CYCLES_PER_TIME_MACHINE == 0 && c->symbol == 'M') {
            move_mountain();
        }

        // Unlock the mutex to allow other threads to proceed
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

// Function to initialize the game board with the mountain, carrots, and characters
void init_board() {
    // Clear the board by setting all positions to empty ('.')
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = '.';

    // Place the mountain ('F') at a random empty position
    int fx, fy;
    do {
        fx = rand() % SIZE; fy = rand() % SIZE;
    } while (board[fx][fy] != '.');
    board[fx][fy] = 'F';

    // Place 2 carrots ('C') at random empty positions
    for (int i = 0; i < CARROTS; i++) {
        int cx, cy;
        do {
            cx = rand() % SIZE; cy = rand() % SIZE;
        } while (board[cx][cy] != '.');
        board[cx][cy] = 'C';
    }

    // Place the four characters (B, D, T, M) at random empty positions
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

// Main function to set up and run the game
int main() {
    srand(time(NULL)); // Seed the random number generator
    pthread_mutex_init(&mutex, NULL); // Initialize the mutex

    init_board(); // Set up the initial game board
    printf("Initial Board:\n");
    print_board(); // Print the initial board

    // Create a thread for each character
    pthread_t threads[4];
    for (int i = 0; i < 4; i++)
        pthread_create(&threads[i], NULL, character_thread, &characters[i]);

    // Wait for all threads to finish
    for (int i = 0; i < 4; i++)
        pthread_join(threads[i], NULL);

    // Clean up and end the game
    pthread_mutex_destroy(&mutex);
    printf("Game Over.\n");
    return 0;
}
