// snake.c
// Juego Snake simple en consola (ANSI colors). Controles: W A S D, q para salir.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

#define WIDTH 40
#define HEIGHT 20
#define MAX_LEN (WIDTH * HEIGHT)

typedef struct { int x, y; } Point;

static struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\x1b[?25h"); // show cursor
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // no echo, non-canonical
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf("\x1b[?25l"); // hide cursor
}

int kbhit_timeout(int ms) {
    fd_set set;
    struct timeval tv;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    return select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) > 0;
}

int read_char_nonblock() {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1) return c;
    return -1;
}

void clear_screen() {
    printf("\x1b[2J");
    printf("\x1b[H");
}

void draw_border() {
    for (int x = 0; x <= WIDTH + 1; x++) printf("#");
    printf("\n");
    for (int y = 0; y < HEIGHT; y++) {
        printf("#");
        for (int x = 0; x < WIDTH; x++) printf(" ");
        printf("#\n");
    }
    for (int x = 0; x <= WIDTH + 1; x++) printf("#");
    printf("\n");
}

void move_cursor(int row, int col) {
    printf("\x1b[%d;%dH", row, col);
}

int point_eq(Point a, Point b) { return a.x == b.x && a.y == b.y; }

int main() {
    srand(time(NULL));
    enable_raw_mode();

    Point snake[MAX_LEN];
    int len = 4;
    // inicializar snake en el centro
    for (int i = 0; i < len; i++) {
        snake[i].x = WIDTH/2 - i;
        snake[i].y = HEIGHT/2;
    }
    Point food;
    food.x = rand() % WIDTH;
    food.y = rand() % HEIGHT;

    char dir = 'r'; // l,r,u,d -> izquierda,derecha,arriba,abajo
    int score = 0;
    int speed_ms = 120; // menor = más rápido

    clear_screen();
    draw_border();

    int game_over = 0;
    while (!game_over) {
        // Entrada (non-blocking)
        if (kbhit_timeout(0)) {
            int c = read_char_nonblock();
            if (c == 'q' || c == 'Q') break;
            if (c == 'w' || c == 'W') if (dir != 'd') dir = 'u';
            if (c == 's' || c == 'S') if (dir != 'u') dir = 'd';
            if (c == 'a' || c == 'A') if (dir != 'r') dir = 'l';
            if (c == 'd' || c == 'D') if (dir != 'l') dir = 'r';
        }

        // mover snake: desplazar cuerpo
        Point next = snake[0];
        if (dir == 'u') next.x = snake[0].x, next.y = snake[0].y - 1;
        if (dir == 'd') next.x = snake[0].x, next.y = snake[0].y + 1;
        if (dir == 'l') next.x = snake[0].x - 1, next.y = snake[0].y;
        if (dir == 'r') next.x = snake[0].x + 1, next.y = snake[0].y;

        // colisiones con borde
        if (next.x < 0 || next.x >= WIDTH || next.y < 0 || next.y >= HEIGHT) {
            game_over = 1; break;
        }
        // colision con el cuerpo
        for (int i = 0; i < len; i++) {
            if (point_eq(next, snake[i])) { game_over = 1; break; }
        }
        if (game_over) break;

        // mover
        for (int i = len; i > 0; i--) snake[i] = snake[i-1];
        snake[0] = next;

        // comer comida?
        if (point_eq(snake[0], food)) {
            len++;
            score += 10;
            // generar nueva comida en posición no ocupada
            int placed = 0;
            for (int tries=0; tries<1000 && !placed; tries++) {
                Point cand = { rand() % WIDTH, rand() % HEIGHT };
                int ok = 1;
                for (int i = 0; i < len; i++) if (point_eq(cand, snake[i])) { ok = 0; break; }
                if (ok) { food = cand; placed = 1; }
            }
            if (speed_ms > 40) speed_ms -= 3;
        } else {
            // no crecer: eliminar último segmento (simplemente len se mantiene)
            // ya se hizo el desplazamiento conservando len
        }

        // dibujar escena
        move_cursor(1,1);
        // primer renglón: borde superior
        for (int x = 0; x <= WIDTH + 1; x++) printf("#");
        printf("\n");

        for (int y = 0; y < HEIGHT; y++) {
            printf("#");
            for (int x = 0; x < WIDTH; x++) {
                char ch = ' ';
                // verificar snake
                int printed = 0;
                if (point_eq(food, (Point){x,y})) {
                    // color comida: amarillo
                    printf("\x1b[33m%c\x1b[0m", '@');
                    printed = 1;
                } else {
                    for (int i = 0; i < len; i++) {
                        if (snake[i].x == x && snake[i].y == y) {
                            if (i == 0) {
                                // cabeza: verde brillante
                                printf("\x1b[1;32m%c\x1b[0m", 'O');
                            } else {
                                // cuerpo: verde normal
                                printf("\x1b[32m%c\x1b[0m", 'o');
                            }
                            printed = 1;
                            break;
                        }
                    }
                }
                if (!printed) printf(" ");
            }
            printf("#\n");
        }
        for (int x = 0; x <= WIDTH + 1; x++) printf("#");
        printf("\n");
        // Mostrar score y ayuda (en la siguiente línea)
        printf(" Puntaje: \x1b[36m%d\x1b[0m  Long: \x1b[36m%d\x1b[0m  Vel: %dms  (W A S D - q salir)\n", score, len, speed_ms);

        // pausa según velocidad
        usleep(speed_ms * 1000);
    }

    // fin del juego
    move_cursor(HEIGHT + 5, 1);
    printf("\n\n\x1b[1;31mJuego terminado!\x1b[0m Puntaje final: %d\n", score);
    disable_raw_mode();
    return 0;
}
