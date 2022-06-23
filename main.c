#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>

#define GRID_HEIGHT 10
#define GRID_WIDTH 20
#define MAX_LENGTH GRID_WIDTH*GRID_HEIGHT

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Dir;

typedef struct {
    bool active;
    int x;
    int y;
} Point;

typedef struct {
    Point position;
    Dir direction;
    Dir last_direction;

    Point* tail;
} Player;

typedef struct {
    Player player;
    Point food;

    int score;
    bool paused;
    bool quit;
} Game;

void reset_cursor() {
    printf("\033[%dA\033[%dD", GRID_HEIGHT+1, GRID_WIDTH);
}

struct termios orig_termios;

void disable_raw_mode() {
   tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

bool istail(Game game, int x, int y) {
    int i;
    for (i = 0; i <= game.score; i++) {
        if (game.player.tail[i].x == x && game.player.tail[i].y == y && game.player.tail[i].active == true) return true;
    }
    return false;
}

void display(Game game) {
    int x, y;

    printf("[score]: %d %s\r\n", game.score, game.paused ? "(paused)" : "        ");
    for (y = 0; y < GRID_HEIGHT; y++) {
        for (x = 0; x < GRID_WIDTH; x++) {
            if (game.player.position.x == x && game.player.position.y == y) {
                putc('#', stdout);
            } else if (game.food.x == x && game.food.y == y) {
                putc('@', stdout);
            } else if (istail(game, x, y)) {
                putc('*', stdout);
            } else {
                putc('.', stdout);
            }
        }
        printf("\r\n");
    }
}

void control(Game *game) {
    char c;
    while (read(STDIN_FILENO, &c, 1) && c != 'q') {
        switch (c) {
            case 'w':
                if (game->player.last_direction != DIR_DOWN) game->player.direction = DIR_UP;
                break;
            case 'a':
                if (game->player.last_direction != DIR_RIGHT) game->player.direction = DIR_LEFT;
                break;
            case 's':
                if (game->player.last_direction != DIR_UP) game->player.direction = DIR_DOWN;
                break;
            case 'd':
                if (game->player.last_direction != DIR_LEFT) game->player.direction = DIR_RIGHT;
                break;
            case 'p':
                game->paused = !game->paused;
                break;
            default: {};
        }

    }

    game->quit = true;
}

void *f(void *p) {
    control((Game *)p);

    return NULL;
}

int randnum(int lower, int upper) {
    return (rand() % (upper - lower + 1)) + lower;
}

void spawn_food(Game* game) {
    int x = randnum(0, GRID_WIDTH-1);
    int y = randnum(0, GRID_HEIGHT-1);

    if (!(istail(*game, x, y) || (game->player.position.x == x && game->player.position.y == y))) {
        game->food.x = x;
        game->food.y = y;
    } else {
        spawn_food(game);
    }
}

void append_shift_right(Point* arr, int size, Point elem) {
    int i;
    for (i = size-1; i > 0; i--) {
        if (!i-1 < 0) {
            arr[i] = arr[i-1];
        }
    }
    arr[0] = elem;
}

int main(void) {
    enable_raw_mode();

    srand(time(0));

    Game game = {
        .player.position.active = true,
        .player.position.x = GRID_WIDTH/2,
        .player.position.y = GRID_HEIGHT/2,
        .player.direction = DIR_RIGHT,
        .player.tail = calloc(MAX_LENGTH, sizeof(Point)),
        .score = 0,
        .paused = true,
        .quit = false,
    };

    pthread_t thread;

    assert(pthread_create(&thread, NULL, f, (void *)&game) == 0);

    spawn_food(&game);

    display(game);

    while (!game.quit) {
        if (game.paused) {
            reset_cursor();
            display(game);
        } else {
            switch (game.player.direction) {
                case DIR_UP:
                    game.player.position.y -= 1;
                    break;
                case DIR_LEFT:
                    game.player.position.x -= 1;
                    break;
                case DIR_DOWN:
                    game.player.position.y += 1;
                    break;
                case DIR_RIGHT:
                    game.player.position.x += 1;
                    break;
                default: {};
            }
            game.player.last_direction = game.player.direction;

            if (game.player.position.x < 0 || game.player.position.x > GRID_WIDTH-1 || game.player.position.y < 0 || game.player.position.y > GRID_HEIGHT-1 || istail(game, game.player.position.x, game.player.position.y)) {
                game.quit = true;
                printf("gameover!\r\nyour score was %d\r\npress 'q' to quit\r\n", game.score);
            } else if (game.player.position.x == game.food.x && game.player.position.y == game.food.y && game.score == MAX_LENGTH-2) {
                game.quit = true;
                printf("you won!\r\nyour score was %d\r\npress 'q' to quit\r\n", game.score);
            } else {
                append_shift_right(game.player.tail, MAX_LENGTH, (Point) {.active = true, .x = game.player.position.x, .y = game.player.position.y});
                if (game.player.position.x == game.food.x && game.player.position.y == game.food.y) {
                    game.score++;
                    spawn_food(&game);
                }
                reset_cursor();
                display(game);
            };
        }
        usleep(200 * 1000);
    }

    pthread_join(thread, NULL);

    free(game.player.tail);

    return 0;
}
