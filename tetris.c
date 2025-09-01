#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>

#define WIDTH 10
#define HEIGHT 20
#define EMPTY 0

// Estructuras para las piezas
typedef struct {
    int x, y;
    int shape[4][4];
    int type;
} Tetromino;

// Piezas del Tetris
int shapes[7][4][4] = {
    // I
    {{0,0,0,0},
     {1,1,1,1},
     {0,0,0,0},
     {0,0,0,0}},
    // J
    {{1,0,0,0},
     {1,1,1,0},
     {0,0,0,0},
     {0,0,0,0}},
    // L
    {{0,0,1,0},
     {1,1,1,0},
     {0,0,0,0},
     {0,0,0,0}},
    // O
    {{0,1,1,0},
     {0,1,1,0},
     {0,0,0,0},
     {0,0,0,0}},
    // S
    {{0,1,1,0},
     {1,1,0,0},
     {0,0,0,0},
     {0,0,0,0}},
    // T
    {{0,1,0,0},
     {1,1,1,0},
     {0,0,0,0},
     {0,0,0,0}},
    // Z
    {{1,1,0,0},
     {0,1,1,0},
     {0,0,0,0},
     {0,0,0,0}}
};

int board[HEIGHT][WIDTH];
Tetromino current;
int score = 0;
int game_over = 0;

// Configuración de la terminal
struct termios original_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &original_termios);
    raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

// Funciones del juego
void init_board() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            board[y][x] = EMPTY;
        }
    }
}

Tetromino new_piece() {
    Tetromino piece;
    piece.type = rand() % 7;
    piece.x = WIDTH / 2 - 2;
    piece.y = 0;
    
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            piece.shape[y][x] = shapes[piece.type][y][x];
        }
    }
    return piece;
}

int check_collision(Tetromino piece) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (piece.shape[y][x]) {
                int board_x = piece.x + x;
                int board_y = piece.y + y;
                
                if (board_x < 0 || board_x >= WIDTH || board_y >= HEIGHT) {
                    return 1;
                }
                if (board_y >= 0 && board[board_y][board_x] != EMPTY) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void rotate_piece() {
    Tetromino rotated = current;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            rotated.shape[x][3-y] = current.shape[y][x];
        }
    }
    
    if (!check_collision(rotated)) {
        current = rotated;
    }
}

void merge_piece() {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (current.shape[y][x]) {
                int board_x = current.x + x;
                int board_y = current.y + y;
                if (board_y >= 0) {
                    board[board_y][board_x] = current.type + 1;
                }
            }
        }
    }
}

int clear_lines() {
    int lines_cleared = 0;
    for (int y = HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < WIDTH; x++) {
            if (board[y][x] == EMPTY) {
                full = 0;
                break;
            }
        }
        
        if (full) {
            lines_cleared++;
            for (int y2 = y; y2 > 0; y2--) {
                for (int x = 0; x < WIDTH; x++) {
                    board[y2][x] = board[y2-1][x];
                }
            }
            for (int x = 0; x < WIDTH; x++) {
                board[0][x] = EMPTY;
            }
            y++; // Revisar la misma línea otra vez
        }
    }
    return lines_cleared;
}

void draw() {
    printf("\033[H\033[J"); // Limpiar pantalla
    
    // Dibujar borde superior
    printf("┌");
    for (int x = 0; x < WIDTH; x++) printf("──");
    printf("┐ PUNTUACIÓN: %d\n", score);
    
    // Dibujar tablero
    for (int y = 0; y < HEIGHT; y++) {
        printf("│");
        for (int x = 0; x < WIDTH; x++) {
            int drawn = 0;
            
            // Dibujar pieza actual
            for (int py = 0; py < 4; py++) {
                for (int px = 0; px < 4; px++) {
                    if (current.shape[py][px] && 
                        current.y + py == y && 
                        current.x + px == x) {
                        printf("██");
                        drawn = 1;
                        break;
                    }
                }
                if (drawn) break;
            }
            
            if (!drawn) {
                if (board[y][x] != EMPTY) {
                    printf("██");
                } else {
                    printf("  ");
                }
            }
        }
        printf("│\n");
    }
    
    // Dibujar borde inferior
    printf("└");
    for (int x = 0; x < WIDTH; x++) printf("──");
    printf("┘\n");
    
    printf("Controles: ← → (mover), ↑ (rotar), ↓ (bajar), q (salir)\n");
}

void game_loop() {
    srand(time(NULL));
    init_board();
    current = new_piece();
    
    int last_drop = 0;
    int drop_delay = 500000; // 0.5 segundos en microsegundos
    
    while (!game_over) {
        draw();
        
        // Control de tiempo para caída automática
        if (usleep(10000) == 0) { // 10ms de espera
            last_drop += 10000;
            if (last_drop >= drop_delay) {
                last_drop = 0;
                Tetromino moved = current;
                moved.y++;
                
                if (check_collision(moved)) {
                    merge_piece();
                    int lines = clear_lines();
                    score += lines * lines * 100;
                    current = new_piece();
                    if (check_collision(current)) {
                        game_over = 1;
                    }
                } else {
                    current = moved;
                }
            }
        }
        
        // Manejo de entrada
        if (kbhit()) {
            char c = getchar();
            Tetromino moved = current;
            
            switch (c) {
                case 'q':
                    game_over = 1;
                    break;
                case 27: // Escape sequence
                    if (kbhit()) {
                        getchar(); // Leer el siguiente carácter
                        c = getchar();
                        switch (c) {
                            case 'A': // Flecha arriba
                                rotate_piece();
                                break;
                            case 'B': // Flecha abajo
                                moved.y++;
                                if (!check_collision(moved)) {
                                    current = moved;
                                }
                                break;
                            case 'C': // Flecha derecha
                                moved.x++;
                                if (!check_collision(moved)) {
                                    current = moved;
                                }
                                break;
                            case 'D': // Flecha izquierda
                                moved.x--;
                                if (!check_collision(moved)) {
                                    current = moved;
                                }
                                break;
                        }
                    }
                    break;
                case ' ': // Barra espaciadora (caída rápida)
                    while (!check_collision(moved)) {
                        current = moved;
                        moved.y++;
                    }
                    break;
            }
        }
    }
    
    printf("\n¡JUEGO TERMINADO! Puntuación final: %d\n", score);
}

int main() {
    enable_raw_mode();
    atexit(disable_raw_mode);
    
    printf("Bienvenido a TETRIS en terminal!\n");
    printf("Presiona cualquier tecla para comenzar...\n");
    getchar();
    
    game_loop();
    
    return 0;
}