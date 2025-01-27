#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "src/SDL2/include/SDL2/SDL.h"
#define FULLSCREEN false   // Set if the game should start on fullscreen (F11 to toggle on/off)
#define FPS 60             // Game target FPS
#define NB_ROWS 7          // Number of rows for the map
#define NB_COLLUMNS 21     // Number of collumns for the map
#define TILE_WIDTH 256     // Width of a tile in px
#define TILE_HEIGHT 192    // Height of a tile in px
#define SPRITE_SIZE 320    // Height and width of all sprites in px
#define BASE_CAM_SPEED 10  // Default camera speed when moving using WASD
#define CAM_SPEED_MULT 3   // Camera speed multiplier when using lShift or lCtrl




/* Enemy types */
#define SLIME_ENEMY 'S'
#define GELLY_ENEMY 'G'
#define GOBLIN_ENEMY 'g'
/* Tower types */
#define ARCHER_TOWER 'A'
/* Animation type */
#define IDLE_ANIMATION 'I'
#define HURT_ANIMATION 'H'
#define DEAD_ANIMATION 'D'
#define ATTACK_ANIMATION 'A'
#define MOVE_ANIMATION 'M'
#define PROJECTILE_ANIMATION 'P'




/* Window's dimensions */
int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;

/* Data of the camera */
double CAM_SCALE = 0.225;
double CAM_POS_X = (NB_COLLUMNS*TILE_WIDTH)/2;
double CAM_POS_Y = (NB_ROWS*TILE_HEIGHT)/2;

/* Current game tick */
Uint64 CURRENT_TICK = 0;




/* Animations for all entities */
typedef struct {
    Uint64 start_tick;  // Tick on which the animation started
    Uint64 length;      // Length of the animation in ticks, -1 for infinite, afterward default to idle animation
    char type;          // Type of animation, 'I' for idle, 'H' for hurt, 'D' for dead, 'A' for attack, 'M' for move
    int *data;          // Additionnal data given to the animation
} Animation;

/* Towers */
typedef struct tower {
    int type;                  // tower type, determine it's abilities, look and upgrades
    int live_points;           // Live points of the tower, when it reaches 0 or bellow the tower is destroyed
    int row;                   // Row number of the tower, 0 being the topmost row
    int collumn;               // Collumn number of the tower, 0 being the leftmost collumn
    int cost;                  // Placement cost of the tower
    int base_attack_cooldown;  // Cooldown between each attack (1 or less being none)
    int attack_cooldown;       // Current cooldown before the next attack
    struct tower* next;        // Pointer to the next tower placed
    SDL_Surface *sprite;       // Sprite of the tower
    Animation *anim;           // Animation of the tower
} Tower;

/* Enemies */
typedef struct enemy {
    int type;                   // Enemy type, determine it's abilities and look
    int live_points;            // Live points of the enemy, when it reaches 0 or bellow the enemy is defeated
    int row;                    // Row number of the enemy, 0 being the topmost row
    int collumn;                // Collumn number of the enemy, 0 being the leftmost collumn
    int base_speed;             // Base number of collumn travelled per turn
    int speed;                  // Number of collumn travelled per turn, reseted to base_speed after moving
    int base_ability_cooldown;  // Cooldown between the use of this enemy special ability
    int ability_cooldown;       // Current cooldown of this enemy special ability, 0 meaning ready to use
    struct enemy* next;         // Next enemy (in order of apparition)
    struct enemy* next_on_row;  // Next enemy on the same row (behind this)
    struct enemy* prev_on_row;  // Previous enemy on the same row (in front of this)
    SDL_Surface *sprite;        // Sprite of the enemy
    Animation *anim;            // Animation of the enemy
} Enemy;

/* Projectile shoot by a tower */
typedef struct projectile {
    Tower *origin;            // Tower that shot the projectile
    Enemy *target;            // Enemy target of the projectile
    struct projectile* next;  // Next projectile (in order of apparition)
    SDL_Surface *sprite;      // Sprite of the projectile
    Animation *anim;          // Animation of the projectile
} Projectile;

/* Waves */
typedef struct {
    Enemy *enemies;  // Enemies of the wave
    int income;      // Funds gain before the wave to build towers
} Wave;

/* Game structure, composed of multiple waves */
typedef struct {
    Tower *towers;   // Tower
    Enemy *enemies;  // Enemies
    int funds;       // Availible funds to build tower
    int turn_nb;     // Turn number
} Game;




/* Header */
int randrange(int a, int b);
int sign(double x);
double min(double x, double y);
double max(double x, double y);
double pow(double x, int n);
char *concatString(const char *a, const char *b);
int stringToInt(const char *str);
int positive_div(int i, int n);
int positive_mod(int i, int n);
double periodicFunctionSub(double x);
double periodicFunction(Uint64 x);
Animation *newAnim();
void destroyAnim(Animation *anim);
void setAnim(Animation *anim, char type, Uint64 length, int *data);
void setAnimIdle(Animation *anim);
void setAnimHurt(Animation *anim);
void setAnimDead(Animation *anim);
void setAnimAttack(Animation *anim, int side);
void setAnimMove(Animation *anim, int dx, int dy);
void setAnimProjectile(Animation *anim, int x1, int y1, int x2, int y2, double speed);
bool applyAnim(Animation *anim, SDL_Rect *rect);
bool getEnemyAndTowerAt(Enemy *enemy_list, Tower *tower_list, int collumn, int row, Enemy **enemy, Tower **tower);
bool addEnemy(Enemy **enemy_list, char enemy_type, int spawn_row, int spawn_collumn);
void destroyEnemy(Enemy *enemy, Enemy **enemy_list);
Enemy **getFirstEnemyOfAllRows(Enemy *enemy_list);
Enemy **getEnemyInCollumn(Enemy *enemy_list, int collumn_nb);
Enemy *getFirstEnemyInRow(Enemy *enemy_list, int row);
int moveEnemy(Enemy *enemy, Enemy *enemy_list, Tower *tower_list, int delta, char axis);
void updateEnemies(Enemy *enemy_list, Tower *tower_list);
void damageEnemy(Enemy *enemy, int amount, Enemy **enemy_list);
Tower *addTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_row, int placement_collumn);
Tower *buyTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_row, int placement_collumn, int *funds);
void destroyTower(Tower *tower, Tower **tower_list);
void updateTowers(Tower **tower_list, Enemy *enemy_list, Projectile **projectile_list);
Projectile *addProjectile(Projectile **projectile_list, Tower *origin, Enemy *target);
void destroyProjectile(Projectile *projectile, Projectile **projectile_list);
bool hasProjectileReachedTarget(Projectile *projectile);
void updateProjectiles(Projectile **projectile_list, Enemy **enemy_list);
void drawProjectiles(SDL_Renderer *rend, Projectile *projectile_list);
bool isWhitespace(char c);
bool readValue(char **line, char **value);
bool readLine(FILE *file, char ***values, int *nb_values);
bool loadLevel(const char *path, Wave ***waves, int *nb_waves);
SDL_Surface *loadImg(const char *path);
void delImg(SDL_Surface *img);
void drawEnemiesAndTowers(SDL_Renderer *rend, Enemy *enemy_list, Tower *tower_list);
void dynamicToStatic(SDL_Rect *rect);
void staticToDynamic(SDL_Rect *rect);
void tileToPixel(int *x, int *y);
void pixelToTile(int *x, int *y);
void drawImgStatic(SDL_Renderer *rend, SDL_Surface *img, int pos_x, int pos_y, int width, int height, Animation *anim);
void drawImgDynamic(SDL_Renderer *rend, SDL_Surface *img, int pos_x, int pos_y, int width, int height, Animation *anim);
void drawRect(SDL_Renderer *rend, int pos_x, int pos_y, int width, int height, int red, int green, int blue, int alpha);
void drawFilledRect(SDL_Renderer *rend, int pos_x, int pos_y, int width, int height, int red, int green, int blue, int alpha);
// void ShootProjectile(Tower *tower,Projectile **projectile_list);
// void updateProjectile(Projectile *projectile_list);
// void drawProjectiles(SDL_Renderer *rend, Projectile *projectile_list);



/* Return an integer between a and b (included) */
int randrange(int a, int b) {
    return positive_mod(rand(), abs(b-a)+1) + min(a, b);
}

/* Return the sign of a number */
int sign(double x) {return (x < 0.0) ? -1 : ((x > 0.0) ? +1 : 0);}

/* Return the minimum between x and y */
double min(double x, double y) {return (x < y) ? x : y;}

/* Return the maximum between x and y */
double max(double x, double y) {return (x > y) ? x : y;}

/* Compute x to the power of n */
double pow(double x, int n) {
    /* Base case */
    if (n == 0) return 1;
    if (x == 0.0) return 0;
    /* Recursive case */
    if (n > 0) {
        double y = x;
        int i = 1;
        while (i*2 <= n) {
            y *= y;
            i *= 2;
        }
        return y * pow(x, n-i);
    }
    /* Negative exponents */
    else return 1.0 / pow(x, -n);
}

/* Add 2 strings together into a new string, memory must be freed after use */
char *concatString(const char *a, const char *b) {
    char *c = malloc((strlen(a) + strlen(b) + 1) * sizeof(char));
    strcpy(c, a);
    strcat(c, b);
    return c;
}

/* Convert a string to an int */
int stringToInt(const char *str) {
    int n;
    sscanf(str, " %d", &n);
    return n;
}

/* Used to get "a = q*b + r" with "0 <= r < b" */
int positive_div(int i, int n) {
    return (i - positive_mod(i, n)) / n;
}

/* Used to get "a = q*b + r" with "0 <= r < b" */
int positive_mod(int i, int n) {
    return (n + (i % n)) % n;
}

double periodicFunctionSub(double x) {
    return 1.0 / (24.0*x + 2.0) - x/2.0;
}
/* 1000-periodic function similar to the sin function */
double periodicFunction(Uint64 x) {
    x = positive_mod(x, 1000);
    if (x >= 750) return periodicFunctionSub(positive_mod(x, 250) / 1000.0);
    if (x >= 500) return 1.0 - periodicFunctionSub(0.25 - positive_mod(x, 250) / 1000.0);
    if (x >= 250) return 1.0 - periodicFunctionSub(positive_mod(x, 250) / 1000.0);
    return periodicFunctionSub(0.25 - positive_mod(x, 250) / 1000.0);
}



/* Create a new animation object (set to idle animation) */
Animation *newAnim() {
    Animation *anim = malloc(sizeof(Animation));
    anim->data = NULL;
    setAnimIdle(anim);
    return anim;
}

/* Destroy an animation object and free its allocated memory */
void destroyAnim(Animation *anim) {
    if (anim->data) free(anim->data);
    free(anim);
}

/* Set the current annimation */
void setAnim(Animation *anim, char type, Uint64 length, int *data) {
    if (!anim) return;
    anim->start_tick = CURRENT_TICK;
    anim->type = type;
    anim->length = length;
    if (anim->data) free(anim->data);
    anim->data = data;
}

/* Set animation to idle */
void setAnimIdle(Animation *anim) {
    int *data = malloc(1 * sizeof(int));
    data[0] = randrange(40, 100);
    setAnim(anim, IDLE_ANIMATION, -1, data);
}

/* Set animation to hurt */
void setAnimHurt(Animation *anim) {
    setAnim(anim, HURT_ANIMATION, 1000, NULL);
}

/* Set animation to dead */
void setAnimDead(Animation *anim) {
    setAnim(anim, DEAD_ANIMATION, 1000, NULL);
}

/* Set animation to attack */
void setAnimAttack(Animation *anim, int side) {
    int *data = malloc(1 * sizeof(int));
    data[0] = side;
    setAnim(anim, ATTACK_ANIMATION, 1000, data);
}

/* Set animation to move (dx dy) tiles */
void setAnimMove(Animation *anim, int dx, int dy) {
    int *data = malloc(2 * sizeof(int));
    data[0] = dx; data[1] = dy;
    setAnim(anim, MOVE_ANIMATION, 3000, data);
}

/* Set animation to projectile, goes from tile (x1 y1) to tile (x2 y2) at speed tile per second */
void setAnimProjectile(Animation *anim, int x1, int y1, int x2, int y2, double speed) {
    int *data = malloc(4 * sizeof(int));
    data[0] = x1; data[1] = y1; data[2] = x2; data[3] = y2;
    setAnim(anim, PROJECTILE_ANIMATION, ((abs(x1-x2) + abs(y1-y2))*1000.0 / speed), data);
}

/* Apply an animation (affect the destination rect of a texture) */
bool applyAnim(Animation *anim, SDL_Rect *rect) {
    /* Go back to default animation (idle) when current one is over */
    if (anim->length != (Uint64) -1 && CURRENT_TICK - anim->start_tick >= anim->length) setAnimIdle(anim);
    double var_a, var_b;
    switch (anim->type) {
        /* Idle animation (shrink up and down periodically) */
        case IDLE_ANIMATION:
            var_a = 1.0 - periodicFunction((CURRENT_TICK - anim->start_tick) / (anim->data[0]/10.0)) / 20.0 + 1.0/40.0;
            var_b = 1.0 + periodicFunction((CURRENT_TICK - anim->start_tick) / (anim->data[0]/10.0)) / 10.0 - 1.0/20.0;
            rect->x -= rect->w * (var_a-1.0) / 2.0;
            rect->y -= rect->h * (var_b-1.0) * 2.0/3.0;
            rect->w *= var_a;
            rect->h *= var_b;
            break;
        /* Hurt animation (shake horizontaly) */
        case HURT_ANIMATION:
            var_a = (CURRENT_TICK - anim->start_tick) / (double) anim->length;
            var_b = periodicFunction(var_a*1000.0 * 6.0) / 20.0 - 1.0/40.0;
            rect->x -= rect->w * var_b;
            break;
        /* Movement animation (go to destination by doing small jumps) */
        case MOVE_ANIMATION:
            var_a = (CURRENT_TICK - anim->start_tick) / (double) anim->length;
            var_b = periodicFunction((CURRENT_TICK - anim->start_tick) * 3.0) / 20.0;
            rect->x -= anim->data[0] * (1.0-var_a) * TILE_WIDTH;
            rect->y -= anim->data[1] * (1.0-var_a) * TILE_HEIGHT + var_b * TILE_HEIGHT;
            break;
        /* Projectile animation (go to point a to point b) */
        case PROJECTILE_ANIMATION:
            var_a = (CURRENT_TICK - anim->start_tick) / (double) anim->length;
            rect->x = ((anim->data[0]-1) * (1.0-var_a) + (anim->data[2]-1) * var_a) * TILE_WIDTH;
            rect->y = ((anim->data[1]-1) * (1.0-var_a) + (anim->data[3]-1) * var_a) * TILE_HEIGHT;
            break;
        /* Unknown animation type */
        default:
            printf("[ERROR]    Undefined animation type '%c'\n", anim->type);
            setAnimIdle(anim);
            return false;
    }
    return true;
}




/* Get the tower and enemy at a specified position */
bool getEnemyAndTowerAt(Enemy *enemy_list, Tower *tower_list, int collumn, int row, Enemy **enemy, Tower **tower) {
    *enemy = NULL; *tower = NULL;
    /* Getting enemy */
    while (enemy_list) {
        if (enemy_list->collumn == collumn && enemy_list->row == row) {
            *enemy = enemy_list;
            break;
        }
        enemy_list = enemy_list->next;
    }
    /* Getting tower */
    while (tower_list) {
        if (tower_list->collumn == collumn && tower_list->row == row) {
            *tower = tower_list;
            break;
        }
        tower_list = tower_list->next;
    }
    /* Return true if an enemy or tower was found, false otherwise (meaning the space is empty) */
    return (*enemy || *tower);
}

/* Return if the specified space is empty */
bool isTileEmpty(Enemy *enemy_list, Tower *tower_list, int collumn, int row) {
    Enemy *e; Tower *t;
    return !getEnemyAndTowerAt(enemy_list, tower_list, collumn, row, &e, &t);
}



/* Add an enemy to the list of enemies, fail if cannot spawn enemy at specified location or if enemy type is not defined */
bool addEnemy(Enemy **enemy_list, char enemy_type, int spawn_row, int spawn_collumn) {
    /* Can't summon enemies in not existing rows */
    if (1 > spawn_row || spawn_row > NB_ROWS) return false;

    /* Initialize the new enemy */
    Enemy *new_enemy = malloc(sizeof(Enemy));
    new_enemy->type = enemy_type;
    new_enemy->collumn = spawn_collumn;
    new_enemy->row = spawn_row;
    new_enemy->next = new_enemy->next_on_row = new_enemy->prev_on_row = NULL;
    new_enemy->anim = newAnim();
    /* Match the enemy type to its stats */
    switch (enemy_type) {
        case SLIME_ENEMY:
            new_enemy->live_points = 4;
            new_enemy->base_speed = 3;
            new_enemy->sprite = loadImg("enemies/Slime");
            break;
        case GELLY_ENEMY:
            new_enemy->live_points = 7;
            new_enemy->base_speed = 2;
            new_enemy->sprite = loadImg("enemies/Gelly");
            break;
        case GOBLIN_ENEMY:
            new_enemy->live_points = 6;
            new_enemy->base_speed = 5;
            new_enemy->sprite = loadImg("enemies/Goblin");
            break;
        default:  /* Unknown enemy type */
            printf("[ERROR]    Unknown enemy type '%c'\n", enemy_type);
            free(new_enemy);
            return false;
    }

    /* Add the new enemy to the enemy list */
    /* If empty then no search is needed */
    if (!(*enemy_list)) {
        *enemy_list = new_enemy;
        return true;
    }
    Enemy *current = *enemy_list;
    /* Look for the previous and next enemy on the same row as this new enemy */
    while (true) {
        if (current->row == new_enemy->row) {
            /* Enemy located on the same row and in front of this new enemy (to the left) */
            if (current->collumn < new_enemy->collumn) {
                if (!new_enemy->prev_on_row || new_enemy->prev_on_row->collumn < current->collumn) new_enemy->prev_on_row = current;
            }
            /* Enemy located on the same row and behind this new enemy (to the right) */
            else if (current->collumn > new_enemy->collumn) {
                if (!new_enemy->next_on_row || new_enemy->next_on_row->collumn > current->collumn) new_enemy->next_on_row = current;
            }
            /* Enemy located on the same exact spot as this new enemy, cannot spawn properly */
            else {
                free(new_enemy);
                return false;
            }
        }
        if (!current->next) break;
        current = current->next;
    }
    /* Add the new enemy to the enemy list */
    current->next = new_enemy;
    if (new_enemy->prev_on_row) new_enemy->prev_on_row->next_on_row = new_enemy;
    if (new_enemy->next_on_row) new_enemy->next_on_row->prev_on_row = new_enemy;
    return true;
}

/* Destroy an enemy and free its allocated memory */
void destroyEnemy(Enemy *enemy, Enemy **enemy_list) {
    if (!enemy || !enemy_list || !(*enemy_list)) return;
    /* Change pointers of enemies accordingly */
    Enemy *prev_enemy = *enemy_list;
    if (prev_enemy == enemy) {
        *enemy_list = enemy->next;
    }
    else {
        while (prev_enemy && prev_enemy->next != enemy) prev_enemy = prev_enemy->next;
        if (prev_enemy) prev_enemy->next = enemy->next;
        if (enemy->prev_on_row) enemy->prev_on_row->next_on_row = enemy->next_on_row;
        if (enemy->next_on_row) enemy->next_on_row->prev_on_row = enemy->prev_on_row;
    }
    /* Destroy enemy data */
    if (enemy->sprite) delImg(enemy->sprite);
    if (enemy->anim) destroyAnim(enemy->anim);
    free(enemy);
}

/* Get an array containing the first enemy of each row, NULL if there is none on the row */
Enemy **getFirstEnemyOfAllRows(Enemy *enemy_list) {
    /* Initializing to NULL */
    Enemy **first_of_each_row = malloc(NB_ROWS * sizeof(Enemy *));
    for (int row_nb = 1; row_nb <= NB_ROWS; row_nb++) first_of_each_row[row_nb-1] = NULL;
    /* Go trough all enemies */
    Enemy *enemy = enemy_list;
    while (enemy) {
        if (!first_of_each_row[enemy->row-1] || first_of_each_row[enemy->row-1]->collumn > enemy->collumn) first_of_each_row[enemy->row-1] = enemy;
        enemy = enemy->next;
    }
    return first_of_each_row;
}

/* Get an array containing the enemy in a collumn */
Enemy **getEnemyInCollumn(Enemy *enemy_list, int collumn_nb) {
    /* Initializing to NULL */
    Enemy **enemy_collumn = malloc(NB_ROWS * sizeof(Enemy *));
    for (int row_nb = 1; row_nb <= NB_ROWS; row_nb++) enemy_collumn[row_nb-1] = NULL;
    /* Go trough all enemies */
    Enemy *enemy = enemy_list;
    while (enemy) {
        if (enemy->collumn == collumn_nb) enemy_collumn[enemy->row-1] = enemy;
        enemy = enemy->next;
    }
    return enemy_collumn;
}

/* Get the first enemy of a row */
Enemy *getFirstEnemyInRow(Enemy *enemy_list, int row) {
    Enemy **first_of_each_row = getFirstEnemyOfAllRows(enemy_list);
    Enemy *first_of_row = first_of_each_row[row-1];
    free(first_of_each_row);
    return first_of_row;
}

/* Move enemy, return number of tile moved */
int moveEnemy(Enemy *enemy, Enemy *enemy_list, Tower *tower_list, int delta, char axis) {
    /* Move on the x axis */
    if (axis == 'x' || axis == 'X') {
        /* Colliding with towers and other enemies */
        for (int i = 0; i < abs(delta); i++) if (!isTileEmpty(enemy_list, tower_list, enemy->collumn + (i+1)*sign(delta), enemy->row)) {
            delta = i*sign(delta);
            break;
        }
        /* Moving on the x axis */
        enemy->collumn += delta;
        return delta;
    }

    /* Move on the y axis */
    if (axis == 'y' || axis == 'Y') {
        /* Colliding with towers and other enemies */
        for (int i = 0; i < abs(delta); i++) if (!isTileEmpty(enemy_list, tower_list, enemy->collumn, enemy->row + (i+1)*sign(delta))) {
            delta = i*sign(delta);
            break;
        }
        /* If delta == 0, nothing happens */
        if (!delta) return 0;
        /* Updating pointers */
        if (enemy->next_on_row) enemy->next_on_row->prev_on_row = enemy->prev_on_row;
        if (enemy->prev_on_row) enemy->prev_on_row->next_on_row = enemy->next_on_row;
        /* Moving on the y axis */
        enemy->row += delta;
        Enemy *first_of_row = getFirstEnemyInRow(enemy_list, enemy->row);
        /* Getting the first enemy in front of the moved one to update pointers properly */
        while (first_of_row && first_of_row->next_on_row && !(first_of_row->collumn < enemy->collumn && enemy->collumn < first_of_row->next_on_row->collumn)) first_of_row = first_of_row->next_on_row;
        enemy->prev_on_row = first_of_row;
        if (first_of_row) {
            enemy->next_on_row = first_of_row->next_on_row;
            if (first_of_row->next_on_row) first_of_row->next_on_row->prev_on_row = enemy;
            first_of_row->next_on_row = enemy;
        }
        else enemy->next_on_row = NULL;
        return delta;
    }

    /* Invalid axis */
    printf("[ERROR]    Can only move an enemy on the 'x' or 'y' axis, not on the '%c' axis\n", axis);
    return 0;
}

/* Update all enemies by simulating a turn passing */
void updateEnemies(Enemy *enemy_list, Tower *tower_list) {
    /* Update enemies from left to right, from top to bottom */
    Enemy **first_of_each_row = getFirstEnemyOfAllRows(enemy_list);
    Enemy *enemy;
    int delta;
    /* From top to bottom */
    for (int row_nb = 1; row_nb <= NB_ROWS; row_nb++) {
        enemy = first_of_each_row[row_nb-1];
        /* From left to right */
        while (enemy) {
            if (enemy->collumn > NB_COLLUMNS) enemy->speed = 1;
            delta = moveEnemy(enemy, enemy_list, tower_list, -enemy->speed, 'x');
            if (delta) setAnimMove(enemy->anim, delta, 0);
            enemy->speed = enemy->base_speed;
            enemy = enemy->next_on_row;
        }
    }
    /* Free memory */
    free(first_of_each_row);
}

/* Damage an enemy */
void damageEnemy(Enemy *enemy, int amount, Enemy **enemy_list) {
    if (!enemy || !amount) return;
    /* Damage enemy */
    enemy->live_points -= amount;
    setAnimHurt(enemy->anim);
    /* Kill enemy if health reaches 0 or less */
    if (enemy->live_points <= 0) destroyEnemy(enemy, enemy_list);
}




Tower *addTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_collumn, int placement_row) {
    /* Invalid position */
    if (1 > placement_row || placement_row > NB_ROWS || 1 > placement_collumn || placement_collumn > NB_COLLUMNS) {
        printf("[ERROR]    Invalid position for tower (x=%d, y=%d)", placement_collumn, placement_row);
        return NULL;
    }

    /* Initialize a new tower object */
    Tower *new_tower = malloc(sizeof(Tower));
    new_tower->type = tower_type;
    new_tower->collumn = placement_collumn;
    new_tower->row = placement_row;
    new_tower->attack_cooldown = 1;
    new_tower->next = NULL;
    new_tower->anim = newAnim();
    switch (tower_type){
        case ARCHER_TOWER:
            new_tower->live_points = 8;
            new_tower->cost = 50;
            new_tower->base_attack_cooldown = 1;
            new_tower->sprite = loadImg("towers/Archer_tower");
            break;
        default:  /* Invalid tower type */
            printf("[ERROR]    Unknown tower type '%c'\n", tower_type);
            destroyTower(new_tower, tower_list);
            return NULL;
    }

    /* Add the tower to the list of towers */
    if (!(*tower_list)) {
        *tower_list = new_tower;
        return new_tower;
    }
    /* Verify if the creation space is empty, as towers cannot be build on an already occupied space */
    if (!isTileEmpty(enemy_list, *tower_list, placement_collumn, placement_row)) {
        destroyTower(new_tower, tower_list);
        return NULL;
    }
    Tower *prev_tower = *tower_list; 
    while (prev_tower->next != NULL) {
        prev_tower = prev_tower->next;
    }
    prev_tower->next = new_tower;
    return new_tower;
}

/* Try to buy a tower */
Tower *buyTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_collumn, int placement_row, int *funds) {
    Tower *new_tower;
    new_tower = addTower(tower_list, enemy_list, tower_type, placement_collumn, placement_row);
    /* If new_tower is NULL, it means it couldn't be build */
    if (!new_tower) return NULL;
    /* Check if the player has enougth funds to build the tower */
    if (*funds < new_tower->cost) {
        destroyTower(new_tower, tower_list);
        return NULL;
    }
    *funds -= new_tower->cost;
    return new_tower;
}

/* Destroy an tower and free its allocated memory */
void destroyTower(Tower *tower, Tower **tower_list) {
    if (!tower || !tower_list || !(*tower_list)) return;
    /* Change pointers of tower accordingly */
    Tower *prev_tower = *tower_list;
    if (prev_tower == tower) {
        *tower_list = tower->next;
    }
    else {
        while (prev_tower && prev_tower->next != tower) prev_tower = prev_tower->next;
        if (prev_tower) prev_tower->next = tower->next;
    }
    /* Destroy tower data */
    if (tower->sprite) delImg(tower->sprite);
    if (tower->anim) destroyAnim(tower->anim);
    free(tower);
}

/* Update all towers */
void updateTowers(Tower **tower_list, Enemy *enemy_list, Projectile **projectile_list) {
    Tower *tower = *tower_list; Tower *tmp;
    int i; Enemy *e; Tower *t;
    while (tower) {
        /* Destroy tower when live points are bellow 0 */
        if (tower->live_points <= 0) {
            tmp = tower->next;
            destroyTower(tower, tower_list);
            tower = tmp;
            continue;
        }
        /* Make tower shoot */
        tower->attack_cooldown--;
        if (tower->attack_cooldown <= 0) {
            tower->attack_cooldown = tower->base_attack_cooldown;
            switch (tower->type) {
                case ARCHER_TOWER:
                    /* Attack the firt enemy on the same row at most 10 tiles away */
                    for (i = 1; i <= 10 && tower->collumn+i <= NB_COLLUMNS; i++) if (getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row, &e, &t)) {
                        addProjectile(projectile_list, tower, e);
                        break;
                    }
                    break;
                default:  /* Invalid tower type */
                    printf("[ERROR]    Unknown tower type '%c'\n", tower->type);
                    tmp = tower->next;
                    destroyTower(tower, tower_list);
                    tower = tmp;
                    continue;
            }
        }
        /* Next tower action */
        tower = tower->next;
    }
}



/* Create and add a projectile to the projectile list */
Projectile *addProjectile(Projectile **projectile_list, Tower *origin, Enemy *target) {
    if (!projectile_list || !origin || !target) return NULL;

    /* Initialize a new projectile object */
    Projectile *new_projectile = malloc(sizeof(Projectile));
    new_projectile->origin = origin;
    new_projectile->target = target;
    new_projectile->next = NULL;
    new_projectile->sprite = NULL;
    new_projectile->anim = newAnim();
    double projectile_speed;
    switch (origin->type){
        /* Shoot by a level 1 archer tower */
        case ARCHER_TOWER:
            new_projectile->sprite = loadImg("projectiles/arrow");
            projectile_speed = 10.0;
            break;
        /* Shoot by a tower of unknown type */
        default:
            printf("[ERROR]    Unknown tower type '%c'\n", origin->type);
            destroyProjectile(new_projectile, projectile_list);
            return NULL;
    }
    setAnimProjectile(new_projectile->anim, origin->collumn, origin->row, target->collumn, target->row, projectile_speed);

    /* Add the projectile to the list of projectiles */
    if (!(*projectile_list)) {
        *projectile_list = new_projectile;
        return new_projectile;
    }
    Projectile *prev_projectile = *projectile_list; 
    while (prev_projectile->next != NULL) {
        prev_projectile = prev_projectile->next;
    }
    prev_projectile->next = new_projectile;
    return new_projectile;
}

/* Remove a projectile from the projectile list */
void destroyProjectile(Projectile *projectile, Projectile **projectile_list) {
    if (!projectile || !projectile_list || !(*projectile_list)) return;
    /* Change pointers of tower accordingly */
    Projectile *prev_projectile = *projectile_list;
    if (prev_projectile == projectile) {
        *projectile_list = projectile->next;
    }
    else {
        while (prev_projectile && prev_projectile->next != projectile) prev_projectile = prev_projectile->next;
        if (prev_projectile) prev_projectile->next = projectile->next;
    }
    /* Destroy tower data */
    if (projectile->sprite) delImg(projectile->sprite);
    if (projectile->anim) destroyAnim(projectile->anim);
    free(projectile);
}

/* Return if the projectile has visualy reached it's target */
bool hasProjectileReachedTarget(Projectile *projectile) {
    return (!projectile || !projectile->anim || projectile->anim->type != PROJECTILE_ANIMATION);
}

/* Update all projectiles */
void updateProjectiles(Projectile **projectile_list, Enemy **enemy_list) {
    Projectile *projectile = *projectile_list; Projectile *tmp;
    while (projectile) {
        /* On target reached */
        if (hasProjectileReachedTarget(projectile)) {
            /* Apply projectile effects */
            switch (projectile->origin->type) {
                case ARCHER_TOWER:
                    damageEnemy(projectile->target, 2, enemy_list);
                    break;
                default:  /* Invalid tower type */
                    printf("[ERROR]    Unknown tower type '%c'\n", projectile->origin->type);
                    break;
            }
            /* Delete projectile */
            tmp = projectile->next;
            destroyProjectile(projectile, projectile_list);
            projectile = tmp;
            continue;
        }
        projectile = projectile->next;
    }
}

/* Draw all projectiles */
void drawProjectiles(SDL_Renderer *rend, Projectile *projectile_list) {
    while (projectile_list) {
        drawImgDynamic(rend, projectile_list->sprite, 0, 0, SPRITE_SIZE, SPRITE_SIZE, projectile_list->anim);
        projectile_list = projectile_list->next;
    }
}




/* Return if the caracter is considered to be a whitespace */
bool isWhitespace(char c) {
    return (!c) || (c == ' ') || (c == '\n') || (c == '\r') || (c == '\t');
}

/* Read a singular value on a line, move line pointer after the value read */
bool readValue(char **line, char **value) {
    if (!line || !(*line)) return false;
    /* Get to the first value of the line, ignore all whitespaces before it */
    while (**line && isWhitespace(**line)) (*line)++;
    char *start = *line;
    /* Find the end of the word */
    while (!isWhitespace(**line)) (*line)++;
    /* If the value read is an empty string, it means that the line does not contain any value */
    if (start == *line) return NULL;
    /* Copy the value read into the value variable */
    *value = malloc((*line - start + 1) * sizeof(char));
    char c = **line;
    **line = '\0';
    strcpy(*value, start);
    **line = c;
    return true;
}

/* Read single file line */
bool readLine(FILE *file, char ***values, int *nb_values) {
    /* Get the full line */
    char *line_buffer = malloc(256 * sizeof(char));
    if (!fgets(line_buffer, 256, file)) {
        free(line_buffer);
        return false;
    }
    /* Split it into all of its individual values */
    *values = malloc(0); *nb_values = 0; char *value, *line = line_buffer;
    while (readValue(&line, &value)) {
        (*nb_values)++;
        *values = realloc(*values, (*nb_values) * sizeof(char *));
        (*values)[*nb_values - 1] = value;
    }
    return true;
}

/* Load a level */
/* File must be located in "./src/lvl/<path>.txt" */
bool loadLevel(const char *path, Wave ***waves, int *nb_waves) {
    /* Openning text file */
    char *partial_path = concatString("./src/lvl/", path);
    char *full_path = concatString(partial_path, ".txt");
    FILE *file = fopen(full_path, "r");
    free(partial_path);
    /* Checking if file was open successfully */
    if (!file) {
        printf("[ERROR]    Level file at \"%s\" not found\n", full_path);
        free(full_path);
        return false;
    }

    /* Retrieve all informations from the file */
    *waves = malloc(0); *nb_waves = 0; char **values; int nb_values;
    while (readLine(file, &values, &nb_values)) {
        switch (nb_values) {
            /* Empty line, ignore it */
            case 0:
                break;
            /* New wave (int income) */
            case 1:
                (*nb_waves)++;
                *waves = realloc(*waves, (*nb_waves) * sizeof(Wave *));
                *waves[*nb_waves - 1] = malloc(sizeof(Wave));
                (*waves[*nb_waves - 1])->income = stringToInt(values[0]);
                (*waves[*nb_waves - 1])->enemies = NULL;
                break;
            /* Add enemy (int spawn_delay, int row, char type) */
            case 3:
                if (!(*nb_waves)) {
                    printf("[ERROR]    Invalid syntax for level file \"%s\"\n", full_path);
                    free(full_path);
                    return false;
                }
                addEnemy(&((*waves)[*nb_waves - 1]->enemies), *(values[2]), stringToInt(values[1]), NB_COLLUMNS + stringToInt(values[0]));
                break;
            /* Invalid value count on line */
            default:
                printf("[ERROR]    Invalid syntax for level file \"%s\"\n", full_path);
                free(full_path);
                for (int i = 0; i < nb_values; i++) free(values[i]);
                free(values);
                return false;
        }
        for (int i = 0; i < nb_values; i++) free(values[i]);
        free(values);
    }
    free(full_path);
    return true;
}




/* Draw on screen enemies and towers, from top to bottom */
void drawEnemiesAndTowers(SDL_Renderer *rend, Enemy *enemy_list, Tower *tower_list) {
    Enemy **first_of_each_row = getFirstEnemyOfAllRows(enemy_list);
    Enemy *enemy; Tower *tower;
    SDL_Rect dest;
    /* Draw from top to bottom */
    for (int row_nb = 1; row_nb <= NB_ROWS; row_nb++) {
        /* Draw enemies on the current row */
        enemy = first_of_each_row[row_nb-1];
        while (enemy) {
            dest.x = (enemy->collumn - 1) * TILE_WIDTH; dest.y = (enemy->row - 1) * TILE_HEIGHT; dest.w = SPRITE_SIZE; dest.h = SPRITE_SIZE;
            drawImgDynamic(rend, enemy->sprite, dest.x, dest.y, dest.w, dest.h, enemy->anim);
            enemy = enemy->next_on_row;
        }
        /* Draw towers on the current row */
        tower = tower_list;
        while (tower) {
            if (tower->row == row_nb) {
                dest.x = (tower->collumn - 1) * TILE_WIDTH; dest.y = (tower->row - 1) * TILE_HEIGHT; dest.w = SPRITE_SIZE; dest.h = SPRITE_SIZE;
                drawImgDynamic(rend, tower->sprite, dest.x, dest.y, dest.w, dest.h, tower->anim);
                drawImgDynamic(rend, tower->sprite, dest.x, dest.y, SPRITE_SIZE, SPRITE_SIZE, tower->anim);
            }
            tower = tower->next;
        }
    }
    /* Free memory */
    free(first_of_each_row);
}




/* Convert a rect from a dynamic position (dependant of the camera position) to a static position (constant) */
void dynamicToStatic(SDL_Rect *rect) {
    (*rect).x = ((*rect).x - WINDOW_WIDTH/2) / CAM_SCALE + CAM_POS_X;
    (*rect).y = ((*rect).y - WINDOW_HEIGHT/2) / CAM_SCALE + CAM_POS_Y;
    (*rect).w /= CAM_SCALE;
    (*rect).h /= CAM_SCALE;
}

/* Convert a rect from a static position (constant) to a dynamic position (dependant of the camera position) */
void staticToDynamic(SDL_Rect *rect) {
    (*rect).x = ((*rect).x - CAM_POS_X) * CAM_SCALE + WINDOW_WIDTH/2;
    (*rect).y = ((*rect).y - CAM_POS_Y) * CAM_SCALE + WINDOW_HEIGHT/2;
    (*rect).w *= CAM_SCALE;
    (*rect).h *= CAM_SCALE;
}

/* Convert a position in tile to a position in pixel (dynamic position) */
void tileToPixel(int *x, int *y) {
    SDL_Rect rect = {(SPRITE_SIZE - TILE_WIDTH) / 2 + (*x - 1) * TILE_WIDTH, (SPRITE_SIZE - TILE_HEIGHT) + (*y - 1) * TILE_HEIGHT, 0, 0};
    staticToDynamic(&rect);
    *x = rect.x;
    *y = rect.y;
}

/* Convert a position in pixel (dynamic position) to a position in tile */
void pixelToTile(int *x, int *y) {
    SDL_Rect rect = {*x, *y, 0, 0};
    dynamicToStatic(&rect);
    *x = positive_div(rect.x - (SPRITE_SIZE - TILE_WIDTH) / 2, TILE_WIDTH) + 1;
    *y = positive_div(rect.y - (SPRITE_SIZE - TILE_HEIGHT), TILE_HEIGHT) + 1;
}




/* Load an image in bitmap (.bmp) format */
/* File must be located in "./src/img/<path>.bmp" */
SDL_Surface *loadImg(const char *path) {
    char *partial_path = concatString("./src/img/", path);
    char *full_path = concatString(partial_path, ".bmp");
    SDL_Surface *img = SDL_LoadBMP(full_path);
    if (!img) printf("[ERROR]    Bitmap file at \"%s\" not found\n", full_path);
    free(partial_path);
    free(full_path);
    return img;
}

/* Delete an image (surface) */
void delImg(SDL_Surface *img) {
    SDL_FreeSurface(img);
}

/* Draw an image on the window surface */
void drawImgStatic(SDL_Renderer *rend, SDL_Surface *img, int pos_x, int pos_y, int width, int height, Animation *anim) {
    /* Destination area */
    SDL_Rect dest = {pos_x, pos_y, width, height};
    /* Apply animation if one is set */
    if (anim) applyAnim(anim, &dest);
    /* Convert surface to texture and draw it */
    SDL_Texture *sprite = SDL_CreateTextureFromSurface(rend, img);
    SDL_RenderCopy(rend, sprite, NULL, &dest);
    SDL_DestroyTexture(sprite);
}

/* Draw an image on the window surface, affected by camera position */
void drawImgDynamic(SDL_Renderer *rend, SDL_Surface *img, int pos_x, int pos_y, int width, int height, Animation *anim) {
    /* Destination area */
    SDL_Rect dest = {pos_x, pos_y, width, height};
    /* Apply animation if one is set */
    if (anim) applyAnim(anim, &dest);
    /* Convert static position to dynamic one */
    staticToDynamic(&dest);
    /* Draw the image */
    drawImgStatic(rend, img, dest.x, dest.y, dest.w, dest.h, NULL);
}

/* Draw a rectangle */
void drawRect(SDL_Renderer *rend, int pos_x, int pos_y, int width, int height, int red, int green, int blue, int alpha) {
    SDL_SetRenderDrawColor(rend, red, green, blue, alpha);
    SDL_Rect rect = {pos_x, pos_y, width, height};
    SDL_RenderDrawRect(rend, &rect);
}
void drawFilledRect(SDL_Renderer *rend, int pos_x, int pos_y, int width, int height, int red, int green, int blue, int alpha) {
    SDL_SetRenderDrawColor(rend, red, green, blue, alpha);
    SDL_Rect rect = {pos_x, pos_y, width, height};
    SDL_RenderFillRect(rend, &rect);
}

void drawRectDynamic(SDL_Renderer *rend, int pos_x, int pos_y, int width, int height, int red, int green, int blue, int alpha) {
    SDL_Rect rect = {pos_x, pos_y, width, height};
    staticToDynamic(&rect);
    drawRect(rend, rect.x, rect.y, rect.w, rect.h, red, green, blue, alpha);
}




int main(int argc, char* argv[]) {
    /* Initialize a new random seed */
    srand(time(NULL));

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 0;
    }

    /* Create a window */
    SDL_Window* wind = SDL_CreateWindow("Game of the year", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!wind) {
        printf("Error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }
    SDL_SetWindowFullscreen(wind, SDL_WINDOW_FULLSCREEN_DESKTOP*FULLSCREEN);
    /* Create a renderer */
    Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    SDL_Renderer* rend = SDL_CreateRenderer(wind, -1, render_flags);
    if (!rend) {
        printf("Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(wind);
        SDL_Quit();
        return 0;
    }
    /* Antialiasing */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    // [TEMPORARY]
    Wave **waves; int nb_waves;
    loadLevel("level_test", &waves, &nb_waves);
    Enemy *enemy_list = waves[0]->enemies; Tower *tower_list = NULL; Projectile *projectile_list = NULL;
    /* Load images */
    SDL_Surface *towers[] = {loadImg("towers/Archer_tower")};
    SDL_Surface *grass_tiles[] = {loadImg("others/grass_tile_a"), loadImg("others/grass_tile_b"), loadImg("others/grass_tile_c"), loadImg("others/grass_tile_d")};

    /* Main loop */
    int cam_x_speed = 0, cam_y_speed = 0, cam_speed_mult = 0;
    int *selected_tile_pos = malloc(2 * sizeof(int)); selected_tile_pos[0] = 0; selected_tile_pos[1] = 0;
    bool menu_hidden = true;
    bool mouse_dragging = false;
    bool fullscreen = FULLSCREEN;
    SDL_Event event;
    bool running = true;
    
    int nb_turns=0,funds=0;
    funds+=waves[0]->income;
    
    while (running) {
        /* Process events */
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                /* Quit the game */
                case SDL_QUIT:
                    running = false;
                    break;
                /* Key pressed */
                case SDL_KEYDOWN:
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_ESCAPE:
                            running = false;
                            break;
                        /* Turn on/off fullscreen mode */
                        case SDL_SCANCODE_F11:
                            fullscreen = !fullscreen;
                            SDL_SetWindowFullscreen(wind, SDL_WINDOW_FULLSCREEN_DESKTOP*fullscreen);
                            /* Update WINDOW_WIDTH and WINDOW_HEIGHT variables, also change CAM_SCALE to keep the same view */
                            CAM_SCALE /= WINDOW_HEIGHT;
                            SDL_GetWindowSize(wind, &WINDOW_WIDTH, &WINDOW_HEIGHT);
                            CAM_SCALE *= WINDOW_HEIGHT;
                            break;
                        /* Show/Hide menu */
                        case SDL_SCANCODE_F:
                            menu_hidden = !menu_hidden;
                            break;
                        /* Camera movement */
                        case SDL_SCANCODE_W:
                            cam_y_speed = -1;
                            break;
                        case SDL_SCANCODE_A:
                            cam_x_speed = -1;
                            break;
                        case SDL_SCANCODE_S:
                            cam_y_speed = +1;
                            break;
                        case SDL_SCANCODE_D:
                            cam_x_speed = +1;
                            break;
                        case SDL_SCANCODE_LSHIFT:
                            cam_speed_mult = +1;
                            break;
                        case SDL_SCANCODE_LCTRL:
                            cam_speed_mult = -1;
                            break;
                        /* [TEMPORARY] Play 1 turn */
                        case SDL_SCANCODE_SPACE:
                            updateEnemies(enemy_list, tower_list);
                            break;
                        /* [TEMPORARY] Make towers shoot */
                        case SDL_SCANCODE_C:
                            updateTowers(&tower_list, enemy_list, &projectile_list);
                            break;
                        default:
                            break;
                    }
                    break;
                /* Key released */
                case SDL_KEYUP:
                    switch (event.key.keysym.scancode) {
                        /* Camera movement */
                        case SDL_SCANCODE_W:
                            if (cam_y_speed == -1) cam_y_speed = 0;
                            break;
                        case SDL_SCANCODE_A:
                            if (cam_x_speed == -1) cam_x_speed = 0;
                            break;
                        case SDL_SCANCODE_S:
                            if (cam_y_speed == +1) cam_y_speed = 0;
                            break;
                        case SDL_SCANCODE_D:
                            if (cam_x_speed == +1) cam_x_speed = 0;
                            break;
                        case SDL_SCANCODE_LSHIFT:
                            if (cam_speed_mult == +1) cam_speed_mult = 0;
                            break;
                        case SDL_SCANCODE_LCTRL:
                            if (cam_speed_mult == -1) cam_speed_mult = 0;
                            break;
                        default:
                            break;
                    }
                    break;
                /* Mouse button pressed */
                case SDL_MOUSEBUTTONDOWN:
                    switch (event.button.button) {
                        /* Click and drag for camera movement */
                        case SDL_BUTTON_RIGHT:
                            mouse_dragging = true;
                            break;
                        /* Tower placement */
                        case SDL_BUTTON_LEFT:
                            /* If tile selected */
                            if (event.button.y > WINDOW_HEIGHT/4 || menu_hidden) {
                                selected_tile_pos[0] = event.button.x;
                                selected_tile_pos[1] = event.button.y;
                                pixelToTile(&selected_tile_pos[0], &selected_tile_pos[1]);
                                if (1 <= selected_tile_pos[0] && selected_tile_pos[0] <= NB_COLLUMNS && 1 <= selected_tile_pos[1] && selected_tile_pos[1] <= NB_ROWS) {
                                    menu_hidden = false;
                                }
                                else {
                                    selected_tile_pos[0] = 0; selected_tile_pos[1] = 0;
                                    menu_hidden = true;
                                }
                            }
                            /* If menu selected and player clicks on a tower icon, try to buy and place tower (if possible) */
                            else {
                                /* LVL 1 archer tower */
                                if (0 <= event.motion.x && event.motion.x <= SPRITE_SIZE/2 && 0 <= event.motion.y && event.motion.y <= SPRITE_SIZE/2) {
                                    if (buyTower(&tower_list, enemy_list, ARCHER_TOWER, selected_tile_pos[0], selected_tile_pos[1], &funds)) menu_hidden = true;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                /* Mouse button released */
                case SDL_MOUSEBUTTONUP:
                    switch (event.button.button) {
                        case SDL_BUTTON_RIGHT:
                            mouse_dragging = false;
                            break;
                        case SDL_BUTTON_LEFT:
                            break;
                        default:
                            break;
                        }
                    break;
                /* Mouse wheel used */
                case SDL_MOUSEWHEEL:
                    CAM_SCALE = min(max(0.0002*WINDOW_HEIGHT, CAM_SCALE*pow(1.05, event.wheel.y)), 0.002*WINDOW_HEIGHT);
                    break;
                /* Mouse moved */
                case SDL_MOUSEMOTION:
                    if (mouse_dragging) {
                        CAM_POS_X -= event.motion.xrel / CAM_SCALE;
                        CAM_POS_Y -= event.motion.yrel / CAM_SCALE;
                    }
                    break;
                default:
                    break;
            }
        
        }

        /* Update entities */
        updateProjectiles(&projectile_list, &enemy_list);

        /* Clear screen */
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);
        /* Move camera */
        CAM_POS_X += BASE_CAM_SPEED * cam_x_speed * pow(CAM_SPEED_MULT, cam_speed_mult) * pow(1.4142/2, cam_x_speed && cam_y_speed) / CAM_SCALE;
        CAM_POS_Y += BASE_CAM_SPEED * cam_y_speed * pow(CAM_SPEED_MULT, cam_speed_mult) * pow(1.4142/2, cam_x_speed && cam_y_speed) / CAM_SCALE;
        /* Draw elements */
        for (int y = 0; y < NB_ROWS; y++) for (int x = 0; x < NB_COLLUMNS; x++) {
                drawImgDynamic(rend, grass_tiles[x%2 + (y%2) * 2], TILE_WIDTH * x, TILE_HEIGHT * y, SPRITE_SIZE, SPRITE_SIZE, NULL);
        }
        drawEnemiesAndTowers(rend, enemy_list, tower_list);
        drawProjectiles(rend, projectile_list);
        /* Draw the Menu if necessary */
        if (!menu_hidden){
            drawFilledRect(rend, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT/4, 128, 128, 128, 255);
            drawRect(rend, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT/4, 255, 255, 255, 255);
            drawImgStatic(rend, towers[0], 0, 0, SPRITE_SIZE/2, SPRITE_SIZE/2, NULL);
        }
        /* Draw to window and loop */
        SDL_RenderPresent(rend);
        SDL_Delay(max(1000/FPS - (SDL_GetTicks64()-CURRENT_TICK), 0));
        CURRENT_TICK = SDL_GetTicks64();
    }
    
    /* Free allocated memory */
    
    delImg(grass_tiles[3]); delImg(grass_tiles[2]); delImg(grass_tiles[1]); delImg(grass_tiles[0]);
    free(selected_tile_pos);
    /* Release resources */
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(wind);
    SDL_Quit();
    return 0;
}
