#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <SDL.h>
#include <SDL_ttf.h>
#define FULLSCREEN false   // Set if the game should start on fullscreen (F11 to toggle on/off)
#define ANTI_ALIASING "2"  // Set if the game should use anti aliasing for rendering
#define FPS 60             // Game target FPS
#define NB_ROWS 7          // Number of rows for the map
#define NB_COLLUMNS 15     // Number of collumns for the map
#define TILE_WIDTH 256     // Width of a tile in px
#define TILE_HEIGHT 192    // Height of a tile in px
#define SPRITE_SIZE 320    // Height and width of all sprites in px
#define BASE_CAM_SPEED 10  // Default camera speed when moving using WASD
#define CAM_SPEED_MULT 3   // Camera speed multiplier when using lShift or lCtrl

#define MAX_LENGTH_TOWER_NAME 20

/* Enemy types */
#define SLIME_ENEMY 'S'
#define GELLY_ENEMY 'G'
#define GOBLIN_ENEMY 'g'
#define ORC_ENEMY 'O'
/* Tower types */
#define ARCHER_TOWER 'A'
#define WALL_TOWER 'W'
#define BARRACK_TOWER 'B'
#define SOLIDER_TOWER 's'
#define CANON_TOWER 'C'
#define DESTROYER_TOWER 'D'
#define SORCERER_TOWER 'S'
#define MAGE_TOWER 'M'
/* Animation type */
#define IDLE_ANIMATION 'I'
#define HURT_ANIMATION 'H'
#define DEAD_ANIMATION 'D'
#define ATTACK_ANIMATION 'A'
#define MOVE_ANIMATION 'M'
#define PROJECTILE_ANIMATION 'P'
/* Game phases */
#define PRE_WAVE_PHASE 0
#define ENEMIES_MOVING_PHASE 1
#define TOWERS_ATTACKING_PHASE 2
#define ENEMIES_ATTACKING_PHASE 3
#define WAVE_DEAFEATED_PHASE 4
#define GAME_OVER_PHASE 5




/* Window's dimensions */
int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;

/* Data of the camera */
double CAM_SCALE = 0.325;
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
double power(double x, int n);
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
bool isTileEmpty(Enemy *enemy_list, Tower *tower_list, int collumn, int row);
bool doesTileExist(int collumn, int row);
Enemy *addEnemy(Enemy **enemy_list, char enemy_type, int spawn_collumn, int spawn_row);
void destroyEnemy(Enemy *enemy, Enemy **enemy_list);
Enemy **getFirstEnemyOfAllRows(Enemy *enemy_list);
Enemy *getFirstEnemyInRow(Enemy *enemy_list, int row);
int moveEnemy(Enemy *enemy, Enemy *enemy_list, Tower *tower_list, int delta, char axis);
void makeAllEnemiesMove(Enemy *enemy_list, Tower *tower_list);
void enemyAttack(Enemy *enemy, Tower **tower_list, Enemy **enemy_list);
void makeAllEnemiesAct(Enemy *enemy_list, Enemy **currently_acting_enemy);
bool damageEnemy(Enemy *enemy, int amount, Enemy **enemy_list, Tower *tower_list);
Tower *addTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_row, int placement_collumn);
Tower *buyTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_row, int placement_collumn, int *funds);
void destroyTower(Tower *tower, Tower **tower_list);
void sellTower(Tower *tower, Tower **tower_list, int *funds);
void towerAct(Tower *tower, Tower **tower_list, Enemy *enemy_list, Projectile **projectile_list);
void updateTowers(Tower **currently_acting_tower, Tower **tower_list, Enemy *enemy_list, Projectile **projectile_list);
void makeAllTowersAct(Tower *tower_list, Tower **currently_acting_tower);
bool damageTower(Tower *tower, int amount, Tower **tower_list);
Projectile *addProjectile(Projectile **projectile_list, Tower *origin, Enemy *target);
void destroyProjectile(Projectile *projectile, Projectile **projectile_list);
bool hasProjectileReachedTarget(Projectile *projectile);
void updateProjectiles(Projectile **projectile_list, Enemy **enemy_list, Tower *tower_list);
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
void freeEverything(Enemy **enemy_list,Tower **tower_list);
void write(SDL_Renderer *rend,TTF_Font *font,const char *text,int pos_x,int pos_y, int width,int height,int red,int green,int blue,int alpha);
Tower *upgradeTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_collumn, int placement_row, int *funds);

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
double power(double x, int n) {
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
        return y * power(x, n-i);
    }
    /* Negative exponents */
    else return 1.0 / power(x, -n);
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
    setAnim(anim, HURT_ANIMATION, 500, NULL);
}

/* Set animation to dead (TODO!) */
void setAnimDead(Animation *anim) {
    setAnim(anim, DEAD_ANIMATION, 1000, NULL);
}

/* Set animation to attack */
void setAnimAttack(Animation *anim, int side) {
    int *data = malloc(1 * sizeof(int));
    data[0] = side;
    setAnim(anim, ATTACK_ANIMATION, 500, data);
}

/* Set animation to move (dx dy) tiles */
void setAnimMove(Animation *anim, int dx, int dy) {
    int *data = malloc(2 * sizeof(int));
    data[0] = dx; data[1] = dy;
    setAnim(anim, MOVE_ANIMATION, 1500, data);
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
            var_b = periodicFunction(var_a*1000.0 * 3.0) / 15.0 - 1.0/30.0;
            rect->x -= rect->w * var_b;
            break;
        /* Attack animation (quickly move horizontaly) */
        case ATTACK_ANIMATION:
            var_a = (CURRENT_TICK - anim->start_tick) / (double) anim->length;
            var_b = periodicFunction(var_a*1000.0) / 2.5;
            rect->x += rect->w * var_b * anim->data[0];
            break;
        /* Movement animation (go to destination by doing small jumps) */
        case MOVE_ANIMATION:
            var_a = (CURRENT_TICK - anim->start_tick) / (double) anim->length;
            var_b = periodicFunction((CURRENT_TICK - anim->start_tick) * 4.0) / 15.0;
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
    /* Getting enemy */
    while (enemy_list) {
        if (enemy_list->collumn == collumn && enemy_list->row == row) {
            break;
        }
        enemy_list = enemy_list->next;
    }
    if (enemy) *enemy = enemy_list;
    /* Getting tower */
    while (tower_list) {
        if (tower_list->collumn == collumn && tower_list->row == row) {
            break;
        }
        tower_list = tower_list->next;
    }
    if (tower) *tower = tower_list;
    /* Return true if an enemy or tower was found, false otherwise (meaning the space is empty) */
    return (enemy_list || tower_list);
}

/* Return if the specified space is empty, if outside of the map return false */
bool isTileEmpty(Enemy *enemy_list, Tower *tower_list, int collumn, int row) {
    return !getEnemyAndTowerAt(enemy_list, tower_list, collumn, row, NULL, NULL);
}

/* Return if the tile is in the map */
bool doesTileExist(int collumn, int row) {
    return (1 <= collumn && collumn <= NB_COLLUMNS && 1 <= row && row <= NB_ROWS);
}




/* Add an enemy to the list of enemies, fail if cannot spawn enemy at specified location or if enemy type is not defined */
Enemy *addEnemy(Enemy **enemy_list, char enemy_type, int spawn_collumn, int spawn_row) {
    /* Can't summon enemies in not existing rows */
    if (1 > spawn_row || spawn_row > NB_ROWS) return NULL;

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
            new_enemy->live_points = 5;
            new_enemy->base_speed = 2;
            new_enemy->sprite = loadImg("enemies/Slime");
            break;
        case GELLY_ENEMY:
            new_enemy->live_points = 6;
            new_enemy->base_speed = 2;
            new_enemy->sprite = loadImg("enemies/Gelly");
            break;
        case GOBLIN_ENEMY:
            new_enemy->live_points = 10;
            new_enemy->base_speed = 3;
            new_enemy->sprite = loadImg("enemies/Goblin");
            break;
        case ORC_ENEMY:
            new_enemy->live_points = 20;
            new_enemy->base_speed = 1;
            new_enemy->sprite = loadImg("enemies/Orc");
            break;
        default:  /* Unknown enemy type */
            printf("[ERROR]    Unknown enemy type '%c'\n", enemy_type);
            destroyAnim(new_enemy->anim);
            free(new_enemy);
            return NULL;
    }

    /* Add the new enemy to the enemy list */
    /* If empty then no search is needed */
    if (!(*enemy_list)) {
        *enemy_list = new_enemy;
        return new_enemy;
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
                destroyAnim(new_enemy->anim);
                free(new_enemy);
                return NULL;
            }
        }
        if (!current->next) break;
        current = current->next;
    }
    /* Add the new enemy to the enemy list */
    current->next = new_enemy;
    if (new_enemy->prev_on_row) new_enemy->prev_on_row->next_on_row = new_enemy;
    if (new_enemy->next_on_row) new_enemy->next_on_row->prev_on_row = new_enemy;
    return new_enemy;
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
        for (int i = 0; i < abs(delta); i++) if (!doesTileExist(enemy->collumn, enemy->row + (i+1)*sign(delta)) || !isTileEmpty(enemy_list, tower_list, enemy->collumn, enemy->row + (i+1)*sign(delta))) {
            delta = i*sign(delta);
            break;
        }
        /* If delta == 0, nothing happens */
        if (!delta) return 0;
        /* Updating pointers */
        if (enemy->next_on_row) enemy->next_on_row->prev_on_row = enemy->prev_on_row;
        if (enemy->prev_on_row) enemy->prev_on_row->next_on_row = enemy->next_on_row;
        enemy->prev_on_row = NULL;
        enemy->next_on_row = NULL;
        enemy->row += delta;
        /* Moving on the y axis */
        Enemy *current = enemy_list;
        while (current) {
            if (current->row == enemy->row) {
                /* Enemy located on the same row and in front of this new enemy (to the left) */
                if (current->collumn < enemy->collumn) {
                    if (!enemy->prev_on_row || enemy->prev_on_row->collumn < current->collumn) enemy->prev_on_row = current;
                }
                /* Enemy located on the same row and behind this new enemy (to the right) */
                else if (current->collumn > enemy->collumn) {
                    if (!enemy->next_on_row || enemy->next_on_row->collumn > current->collumn) enemy->next_on_row = current;
                }
            }
            enemy = enemy->next;
        }
        return delta;
    }

    /* Invalid axis */
    printf("[ERROR]    Can only move an enemy on the 'x' or 'y' axis, not on the '%c' axis\n", axis);
    return 0;
}

/* Update all enemies */
void updateEnemies(Enemy **currently_acting_enemy, Enemy **enemy_list, Tower **tower_list) {
    Enemy *enemy;// = *enemy_list; Enemy *tmp;
    // while (enemy) {
    //     /* Destroy enemy when live points are bellow 0 */
    //     if (enemy->live_points <= 0) {
    //         tmp = enemy->next;
    //         destroyEnemy(enemy, enemy_list);
    //         enemy = tmp;
    //         continue;
    //     }
    //     tower = tower->next;
    // }
    while (currently_acting_enemy && *currently_acting_enemy) {
        /* Wait for previous enemy to finish his attack before attacking */
        enemy = *enemy_list;
        while (enemy && enemy->next != *currently_acting_enemy) enemy = enemy->next;
        if (enemy && enemy->anim && enemy->anim->type == ATTACK_ANIMATION) return;
        enemyAttack(*currently_acting_enemy, tower_list, enemy_list);
        *currently_acting_enemy = (*currently_acting_enemy)->next;
    }
}

/* Make all enemies move accordingly to their type */
void makeAllEnemiesMove(Enemy *enemy_list, Tower *tower_list) {
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

/* Make a singular enemy attack */
void enemyAttack(Enemy *enemy, Tower **tower_list, Enemy **enemy_list) {
    if (!enemy) return;
    /* Getting tower in front of the enemy (if there is one) */
    Enemy *e; Tower *tower; bool result;
    getEnemyAndTowerAt(*enemy_list, *tower_list, enemy->collumn-1, enemy->row, &e, &tower);
    /* Making enemy act accordingly to its type */
    switch (enemy->type) {
        case SLIME_ENEMY:
            if (tower) {
                result = damageTower(tower, 2, tower_list);
                if (result) {
                    setAnimAttack(enemy->anim, -1);
                    enemy->speed = 0;
                }
            }
            break;
        case GELLY_ENEMY:
            if (tower) {
                result = damageTower(tower, 2, tower_list);
                if (result) {
                    setAnimAttack(enemy->anim, -1);
                    enemy->speed = 0;
                }
            }
            break;
        case GOBLIN_ENEMY:
            if (tower) {
                result = damageTower(tower, 3, tower_list);
                if (result) {
                    setAnimAttack(enemy->anim, -1);
                    enemy->speed = 0;
                }
            }
            break;
        case ORC_ENEMY:
            if (tower) {
                result = damageTower(tower, 5, tower_list);
                if (result) {
                    setAnimAttack(enemy->anim, -1);
                    enemy->speed = 0;
                }
            }
            break;
        default:  /* Unknown enemy type */
            printf("[ERROR]    Unknown enemy type '%c'\n", enemy->type);
            destroyEnemy(enemy, enemy_list);
            break;
    }
}

/* Make all enemies attack */
void makeAllEnemiesAct(Enemy *enemy_list, Enemy **currently_acting_enemy) {
    if (!enemy_list) {
        *currently_acting_enemy = NULL;
        return;
    }
    *currently_acting_enemy = enemy_list;
}

/* Damage an enemy */
bool damageEnemy(Enemy *enemy, int amount, Enemy **enemy_list, Tower *tower_list) {
    if (!enemy || !amount || !enemy_list) return false;
    int n, x, y;

    /* Check that enemy still exist */
    Enemy *e = *enemy_list;
    while (e && e != enemy) e = e->next;
    if (!e) return false;
    /* Damage enemy */
    enemy->live_points -= amount;
    setAnimHurt(enemy->anim);

    /* Kill enemy if health reaches 0 or less */
    if (enemy->live_points <= 0) {
        n = enemy->type; x = enemy->collumn; y = enemy->row;
        destroyEnemy(enemy, enemy_list);
        /* Gelly splits into 2 slimes on death, one above and one bellow + one at current position or behind if a slime spawn position is blocked */
        if (n == GELLY_ENEMY) {
            n = 2;
            if (n-- && isTileEmpty(*enemy_list, tower_list, x, y - 1) && doesTileExist(x, y - 1) && (e = addEnemy(enemy_list, SLIME_ENEMY, x, y - 1))) {setAnimMove(e->anim, 0, -1); e->speed = 2;}
            else n++;
            if (n-- && isTileEmpty(*enemy_list, tower_list, x, y + 1) && doesTileExist(x, y + 1) && (e = addEnemy(enemy_list, SLIME_ENEMY, x, y + 1))) {setAnimMove(e->anim, 0, +1); e->speed = 2;}
            else n++;
            if (n-- && isTileEmpty(*enemy_list, tower_list, x + 1, y) && doesTileExist(x + 1, y) && (e = addEnemy(enemy_list, SLIME_ENEMY, x + 1, y))) {setAnimMove(e->anim, +1, 0); e->speed = 2;}
            else n++;
            if (n-- && isTileEmpty(*enemy_list, tower_list, x, y) && doesTileExist(x, y) && (e = addEnemy(enemy_list, SLIME_ENEMY, x, y))) {e->speed = 2;}
            else n++;
            return true;
        }
    }

    /* Goblin changes row on hit, depending on its hp left */
    if (enemy->type == GOBLIN_ENEMY) {
        if (enemy->live_points % 2) {
            n = moveEnemy(enemy, *enemy_list, tower_list, 1, 'y');
            if (!n) n = moveEnemy(enemy, *enemy_list, tower_list, -1, 'y');
        }
        else {
            n = moveEnemy(enemy, *enemy_list, tower_list, -1, 'y');
            if (!n) n = moveEnemy(enemy, *enemy_list, tower_list, 1, 'y');
        }
        if (n && enemy->anim) setAnimMove(enemy->anim, 0, n);
    }
    return true;
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
            new_tower->live_points = 6;
            new_tower->cost = 50;
            new_tower->base_attack_cooldown = 1;
            new_tower->sprite = loadImg("towers/Archer_tower");
            break;
        case WALL_TOWER:
            new_tower->live_points = 10;
            new_tower->cost = 30;
            new_tower->base_attack_cooldown = 1;
            new_tower->sprite = loadImg("towers/Empty_tower");
            break;
        case BARRACK_TOWER:
            new_tower->live_points = 15;
            new_tower->cost = 70;
            new_tower->base_attack_cooldown = 5;
            new_tower->sprite = loadImg("towers/barracks");
            break;
        case SOLIDER_TOWER:
            new_tower->live_points = 3;
            new_tower->cost = 0;
            new_tower->base_attack_cooldown = 1;
            new_tower->sprite = loadImg("towers/Spearman");
            break;
        case CANON_TOWER:
            new_tower->live_points = 4;
            new_tower->cost = 100;
            new_tower->base_attack_cooldown = 3;
            new_tower->sprite = loadImg("towers/canon");
            break;
        case DESTROYER_TOWER:
            new_tower->live_points = 8;
            new_tower->cost = 120;
            new_tower->base_attack_cooldown = 3;
            new_tower->sprite = loadImg("towers/canon_evolved");
            break;
        case SORCERER_TOWER:
            new_tower->live_points = 5;
            new_tower->cost = 70;
            new_tower->base_attack_cooldown = 2;
            new_tower->sprite = loadImg("towers/sorcerer");
            break;
        case MAGE_TOWER:
            new_tower->live_points = 7;
            new_tower->cost = 100;
            new_tower->base_attack_cooldown = 2;
            new_tower->sprite = loadImg("towers/sorcerer_evolved");
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
    /* Check if the player has enough funds to build the tower */
    if (*funds < new_tower->cost) {
        destroyTower(new_tower, tower_list);
        return NULL;
    }
    *funds -= new_tower->cost;
    return new_tower;
}

/* Try to upgrade a tower */
Tower *upgradeTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_collumn, int placement_row, int *funds) {
    Tower *old_tower,*new_tower;
    switch (tower_type){
        case WALL_TOWER:
            getEnemyAndTowerAt(NULL, *tower_list, placement_collumn, placement_row, NULL, &old_tower);
            destroyTower(old_tower, tower_list);
            new_tower = addTower(tower_list,enemy_list, BARRACK_TOWER, placement_collumn, placement_row);
            if (*funds < new_tower->cost){
                destroyTower(new_tower,tower_list);
                addTower(tower_list, enemy_list, tower_type, placement_collumn, placement_row);
                return NULL;
            }
            *funds -= new_tower->cost;
            break;
        case SORCERER_TOWER:
            getEnemyAndTowerAt(NULL, *tower_list, placement_collumn, placement_row, NULL, &old_tower);
            destroyTower(old_tower, tower_list);
            new_tower = addTower(tower_list, enemy_list, MAGE_TOWER, placement_collumn, placement_row);
            if (*funds < new_tower->cost){
                destroyTower(new_tower,tower_list);
                addTower(tower_list, enemy_list, tower_type, placement_collumn, placement_row);
                return NULL;
            }
            *funds -= new_tower->cost;
            break;
        case CANON_TOWER:
            getEnemyAndTowerAt(NULL, *tower_list, placement_collumn, placement_row, NULL, &old_tower);
            destroyTower(old_tower, tower_list);
            new_tower = addTower(tower_list, enemy_list, DESTROYER_TOWER, placement_collumn, placement_row);
            if (*funds < new_tower->cost) {
                destroyTower(new_tower, tower_list);
                addTower(tower_list, enemy_list, tower_type, placement_collumn, placement_row);
                return NULL;
            }
            *funds -= new_tower->cost;
            break;
        default:
            printf("No upgrade for that kind of tower");
            return NULL;
    }
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

/* Sell the tower and refund its cost (in case of miss click) */
void sellTower(Tower *tower, Tower **tower_list, int *funds){
    *funds += tower->cost;
    destroyTower(tower, tower_list);
}

/* Make a singular tower act */
void towerAct(Tower *tower, Tower **tower_list, Enemy *enemy_list, Projectile **projectile_list) {
    if (!tower || !tower_list) return;
    int i; Enemy *target; Tower *tmp;
    /* Can only act when action cooldown reaches 0 or less */
    tower->attack_cooldown--;
    if (tower->attack_cooldown <= 0) {
        switch (tower->type) {
            case ARCHER_TOWER:
                /* Attack the firt enemy on the same row at most 9 tiles away */
                for (i = 1; i <= 9; i++) if (doesTileExist(tower->collumn + i, tower->row) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row, &target, NULL)) {
                    tower->attack_cooldown = tower->base_attack_cooldown;
                    addProjectile(projectile_list, tower, target);
                    break;
                }
                break;
            case WALL_TOWER:
                tower->attack_cooldown = tower->base_attack_cooldown;
                break;
            case BARRACK_TOWER:
                tower->attack_cooldown = tower->base_attack_cooldown;
                tmp = NULL;
                if (isTileEmpty(enemy_list, *tower_list, tower->collumn, tower->row - 1) && doesTileExist(tower->collumn, tower->row - 1) && (tmp = addTower(tower_list, enemy_list, SOLIDER_TOWER, tower->collumn, tower->row - 1)));
                else if (isTileEmpty(enemy_list, *tower_list, tower->collumn, tower->row + 1) && doesTileExist(tower->collumn, tower->row + 1) && (tmp = addTower(tower_list, enemy_list, SOLIDER_TOWER, tower->collumn, tower->row + 1)));
                else if (isTileEmpty(enemy_list, *tower_list, tower->collumn + 1, tower->row) && doesTileExist(tower->collumn + 1, tower->row) && (tmp = addTower(tower_list, enemy_list, SOLIDER_TOWER, tower->collumn + 1, tower->row)));
                else if (isTileEmpty(enemy_list, *tower_list, tower->collumn - 1, tower->row) && doesTileExist(tower->collumn- 1, tower->row ) && (tmp = addTower(tower_list, enemy_list, SOLIDER_TOWER, tower->collumn - 1, tower->row)));
                else tower->attack_cooldown = 1;
                if (tmp) setAnimMove(tmp->anim, tmp->collumn - tower->collumn, tmp->row - tower->row);
                break;
            case SOLIDER_TOWER:
                /* Attack the firt enemy on the same or adjacent rows at most 2 tiles away */
                for (i = 1; i <= 2; i++) {
                    if (
                        (doesTileExist(tower->collumn + i, tower->row) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row, &target, NULL)) ||
                        (doesTileExist(tower->collumn + i, tower->row - 1) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row - 1, &target, NULL)) ||
                        (doesTileExist(tower->collumn + i, tower->row + 1) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row + 1, &target, NULL))
                    ) {
                        tower->attack_cooldown = tower->base_attack_cooldown;
                        addProjectile(projectile_list, tower, target);
                        break;
                    }
                }
                break;
            case CANON_TOWER:
                /* Attack the firt enemy on the same row at most 3 tiles away */
                for (i = 1; i <= 3; i++) if (doesTileExist(tower->collumn + i, tower->row) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row, &target, NULL)) {
                    tower->attack_cooldown = tower->base_attack_cooldown;
                    addProjectile(projectile_list, tower, target);
                    break;
                }
                break;
            case DESTROYER_TOWER:
                /* Attack the firt enemy on the same row at most 4 tiles away */
                for (i = 1; i <= 4; i++) if (doesTileExist(tower->collumn + i, tower->row) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row, &target, NULL)) {
                    tower->attack_cooldown = tower->base_attack_cooldown;
                    addProjectile(projectile_list, tower, target);
                    break;
                }
                break;
            case SORCERER_TOWER:
                /* Attack the firt enemy on the same row at most 7 tiles away */
                for (i = 1; i <= 7; i++) if (doesTileExist(tower->collumn + i, tower->row) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row, &target, NULL)) {
                    tower->attack_cooldown = tower->base_attack_cooldown;
                    addProjectile(projectile_list, tower, target);
                    break;
                }
                break;
            case MAGE_TOWER:
                /* Attack the firt enemy on the same row and on the adjacent rows at most 7 tiles away */
                for (i = 1; i <= 7; i++) if (doesTileExist(tower->collumn + i, tower->row) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row, &target, NULL)) {
                    tower->attack_cooldown = tower->base_attack_cooldown;
                    addProjectile(projectile_list, tower, target);
                    break;
                }
                for (i = 1; i <= 7; i++) if (doesTileExist(tower->collumn + i, tower->row - 1) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row - 1, &target, NULL)) {
                    tower->attack_cooldown = tower->base_attack_cooldown;
                    addProjectile(projectile_list, tower, target);
                    break;
                }
                for (i = 1; i <= 7; i++) if (doesTileExist(tower->collumn + i, tower->row + 1) && getEnemyAndTowerAt(enemy_list, NULL, tower->collumn + i, tower->row + 1, &target, NULL)) {
                    tower->attack_cooldown = tower->base_attack_cooldown;
                    addProjectile(projectile_list, tower, target);
                    break;
                }
                break;
            default:  /* Invalid tower type */
                printf("[ERROR]    Unknown tower type '%c'\n", tower->type);
                break;
        }
    }
}

/* Update all towers */
void updateTowers(Tower **currently_acting_tower, Tower **tower_list, Enemy *enemy_list, Projectile **projectile_list) {
    // Tower *tower = *tower_list; Tower *tmp;
    // while (tower) {
    //     /* Destroy tower when live points are bellow 0 */
    //     if (tower->live_points <= 0) {
    //         tmp = tower->next;
    //         destroyTower(tower, tower_list);
    //         tower = tmp;
    //         continue;
    //     }
    //     tower = tower->next;
    // }
    Enemy *e;
    while (currently_acting_tower && *currently_acting_tower && projectile_list && !(*projectile_list)) {
        e = enemy_list;
        while (e) {
            if (e->anim && e->anim->type != IDLE_ANIMATION) return;
            e = e->next;
        }
        towerAct(*currently_acting_tower, tower_list, enemy_list, projectile_list);
        *currently_acting_tower = (*currently_acting_tower)->next;
    }
}

/* Make all tower act */
void makeAllTowersAct(Tower *tower_list, Tower **currently_acting_tower) {
    if (!tower_list) {
        *currently_acting_tower = NULL;
        return;
    }
    *currently_acting_tower = tower_list;
}

/* Damage a tower */
bool damageTower(Tower *tower, int amount, Tower **tower_list) {
    if (!tower || !amount || !tower_list) return false;
    /* Check that tower still exist */
    Tower *t = *tower_list;
    while (t && t != tower) t = t->next;
    if (!t) return false;
    /* Damage tower */
    tower->live_points -= amount;
    setAnimHurt(tower->anim);
    /* Kill tower if health reaches 0 or less */
    if (tower->live_points <= 0) destroyTower(tower, tower_list);
    return true;
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
            projectile_speed = 15.0;
            break;
        case SOLIDER_TOWER:
            new_projectile->sprite = loadImg("projectiles/spear_hit");
            projectile_speed = 10.0;
            break;
        case CANON_TOWER:
            new_projectile->sprite = loadImg("projectiles/canon_bullet");
            projectile_speed = 20.0;
            break;
        case DESTROYER_TOWER:
            new_projectile->sprite = loadImg("projectiles/destroyer_bullet");
            projectile_speed = 20.0;
            break;
        case SORCERER_TOWER:
            new_projectile->sprite = loadImg("projectiles/magic_orb");
            projectile_speed = 10.0;
            break;
        case MAGE_TOWER:
            new_projectile->sprite = loadImg("projectiles/magic_orb");
            projectile_speed = 10.0;
            break;
        default:  /* Shoot by a tower of unknown type */
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
void updateProjectiles(Projectile **projectile_list, Enemy **enemy_list, Tower *tower_list) {
    Projectile *projectile = *projectile_list; Projectile *tmp;
    Enemy *enemy;
    bool result;
    while (projectile) {
        /* On target reached */
        if (hasProjectileReachedTarget(projectile)) {
            /* Apply projectile effects */
            switch (projectile->origin->type) {
                case ARCHER_TOWER:
                    damageEnemy(projectile->target, 2, enemy_list, tower_list);
                    break;
                case WALL_TOWER:
                    break;
                case BARRACK_TOWER:
                    break;
                case SOLIDER_TOWER:
                    damageEnemy(projectile->target, 2, enemy_list, tower_list);
                    break;
                case CANON_TOWER:
                    damageEnemy(projectile->target, 9, enemy_list, tower_list);
                    break;
                case DESTROYER_TOWER:
                    damageEnemy(projectile->target, 10, enemy_list, tower_list);
                    /* Area damage */
                    for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) if (dx || dy)
                        if (getEnemyAndTowerAt(*enemy_list, NULL, projectile->target->collumn + dx, projectile->target->row + dy, &enemy, NULL))
                            damageEnemy(enemy, 4, enemy_list, tower_list);
                    break;
                case SORCERER_TOWER:
                    result = damageEnemy(projectile->target, 3, enemy_list, tower_list);
                    /* Enemy slowdown on hit */
                    if (result && projectile->target) projectile->target->speed = min(max(projectile->target->speed - 1, 1), projectile->target->speed);
                    break;
                case MAGE_TOWER:
                    result = damageEnemy(projectile->target, 3, enemy_list, tower_list);
                    /* Enemy slowdown on hit */
                    if (result && projectile->target) projectile->target->speed = min(max(projectile->target->speed - 1, 1), projectile->target->speed);
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
        drawImgDynamic(rend, projectile_list->sprite, 1000000, 1000000, SPRITE_SIZE, SPRITE_SIZE, projectile_list->anim);
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
/* File must be located in "../assets/lvl/<path>.txt" */
bool loadLevel(const char *path, Wave ***waves, int *nb_waves) {
    /* Openning text file */
    char *partial_path = concatString("../assets/lvl/", path);
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
                (*waves)[*nb_waves - 1] = malloc(sizeof(Wave));
                (*waves)[*nb_waves - 1]->income = stringToInt(values[0]);
                (*waves)[*nb_waves - 1]->enemies = NULL;
                break;
            /* Add enemy (int spawn_delay, int row, char type) */
            case 3:
                if (!(*nb_waves)) {
                    printf("[ERROR]    Invalid syntax for level file \"%s\"\n", full_path);
                    free(full_path);
                    fclose(file);
                    return false;
                }
                addEnemy(&(*waves)[*nb_waves - 1]->enemies, values[2][0], NB_COLLUMNS + stringToInt(values[0]), stringToInt(values[1]));
                break;
            /* Invalid value count on line */
            default:
                printf("[ERROR]    Invalid syntax for level file \"%s\"\n", full_path);
                free(full_path);
                for (int i = 0; i < nb_values; i++) free(values[i]);
                free(values);
                fclose(file);
                return false;
        }
        for (int i = 0; i < nb_values; i++) free(values[i]);
        free(values);
    }
    free(full_path);
    fclose(file);
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

void freeEverything(Enemy **enemy_list,Tower **tower_list){
    while(*enemy_list){
        destroyEnemy(*enemy_list,enemy_list);
    }
    while(*tower_list){
        destroyTower(*tower_list,tower_list);
    }
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
/* File must be located in "../assets/img/<path>.bmp" */
SDL_Surface *loadImg(const char *path) {
    char *partial_path = concatString("../assets/img/", path);
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

void write(SDL_Renderer *rend,TTF_Font *font,const char *text,int pos_x,int pos_y, int width,int height,int red,int green,int blue,int alpha){
    SDL_Color TextColor = {red, green, blue, alpha} ;
    SDL_Rect dest = {pos_x, pos_y, width, height};
    SDL_Surface *textRender = TTF_RenderText_Blended(font, text,TextColor);
    if (!textRender){
        printf("Error when rendering text surface %s\n",SDL_GetError());
    }
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(rend,textRender);
    SDL_RenderCopy(rend,textTexture,NULL,&dest);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textRender);
}


int main(int argc, char* argv[]) {
    /* Initialize a new random seed */
    srand(time(NULL));

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 0;
    }

    /* Initialize SDL_ttf */
    if (TTF_Init() != 0){
        printf("Error initializing TTF %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }

    /* Initialize the font */
    TTF_Font *font = TTF_OpenFont("../assets/fonts/Almendra-Regular.ttf", 16);
    if (!font){
        printf("Error creating font : %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    /* Create a window */
    SDL_Window* wind = SDL_CreateWindow("Game of the year", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!wind) {
        printf("Error creating window: %s\n", SDL_GetError());
        TTF_Quit();
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
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    /* Antialiasing, "0" -> "nearest" ; "1" -> "linear" ; "2" -> "best" */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, ANTI_ALIASING);

    // [TEMPORARY]
    Wave **waves; int nb_waves;
    loadLevel("level_test", &waves, &nb_waves);
    Enemy *enemy_list = waves[0]->enemies; Tower *tower_list = NULL; Projectile *projectile_list = NULL;

    /* Load images */
    SDL_Surface *towers[] = {loadImg("towers/Archer_tower"),loadImg("towers/Empty_tower"),loadImg("towers/canon"),loadImg("towers/sorcerer")};
    SDL_Surface *towers_upgrades[] = {loadImg("towers/sorcerer_evolved"),loadImg("towers/canon_evolved")};
    SDL_Surface *grass_tiles[] = {loadImg("others/grass_tile_a"), loadImg("others/grass_tile_b"), loadImg("others/grass_tile_c"), loadImg("others/grass_tile_d")};
    SDL_Surface *highlighted_tile = loadImg("others/tile_choosed");
    SDL_Surface *delete_tower = loadImg("others/delete");
    char TowersNames[4][MAX_LENGTH_TOWER_NAME]={"Archer Tower 50G","Wall 25G","Canon 100G","Sorcerer tower 75G"};

    /* Main loop */
    Tower *towerOnTile;
    Enemy *enemy; Tower *tower; bool condition;
    int nb_turns=0,funds=0;
    funds+=waves[0]->income;
    int game_phase = PRE_WAVE_PHASE;
    Enemy *currently_acting_enemy = NULL; Tower *currently_acting_tower = NULL;
    int cam_x_speed = 0, cam_y_speed = 0, cam_speed_mult = 0;
    int *selected_tile_pos = malloc(2 * sizeof(int)); selected_tile_pos[0] = 0; selected_tile_pos[1] = 0;
    bool menu_hidden = true;
    bool mouse_dragging = false;
    bool fullscreen = FULLSCREEN;
    SDL_Event event;
    bool running = true;
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
                        /* Start the wave */
                        case SDL_SCANCODE_SPACE:
                            if (game_phase == PRE_WAVE_PHASE) game_phase = TOWERS_ATTACKING_PHASE;
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
                            else if (getEnemyAndTowerAt(NULL, tower_list, selected_tile_pos[0], selected_tile_pos[1], NULL, &towerOnTile)){
                                if (WINDOW_WIDTH-SPRITE_SIZE/2 <= event.button.x && event.button.x <= WINDOW_WIDTH && 0 <= event.button.y && event.button.y <= SPRITE_SIZE/2){
                                    /* Clicked on the X to destroy the tower and refund the money */
                                    sellTower(towerOnTile, &tower_list, &funds);
                                    menu_hidden = true;
                                }
                                else if (0 <= event.button.x && event.button.x <= SPRITE_SIZE/2 && 0 <= event.button.y && event.button.y <= SPRITE_SIZE/2){
                                    /* Clicked on the turret upgrade */
                                    upgradeTower(&tower_list, enemy_list, towerOnTile->type, selected_tile_pos[0], selected_tile_pos[1], &funds);
                                    menu_hidden=true;
                                }
                            }
                            /* If menu selected and player clicks on a tower icon, try to buy and place tower (if possible) */
                            else {
                                /* LVL 1 archer tower */
                                if (0 <= event.button.x && event.button.x <= SPRITE_SIZE/2 && 0 <= event.button.y && event.button.y <= SPRITE_SIZE/2) {
                                    if (buyTower(&tower_list, enemy_list, ARCHER_TOWER, selected_tile_pos[0], selected_tile_pos[1], &funds)) menu_hidden = true;
                                }
                                else if (SPRITE_SIZE/2 <= event.button.x && event.button.x <= 2*SPRITE_SIZE/2 && 0 <= event.button.y && event.button.y <= SPRITE_SIZE/2) {
                                    if (buyTower(&tower_list, enemy_list, WALL_TOWER, selected_tile_pos[0], selected_tile_pos[1], &funds)) menu_hidden = true;
                                }
                                else if (2*SPRITE_SIZE/2 <= event.button.x && event.button.x <= 3*SPRITE_SIZE/2 && 0 <= event.button.y && event.button.y <= SPRITE_SIZE/2) {
                                    if (buyTower(&tower_list, enemy_list, CANON_TOWER, selected_tile_pos[0], selected_tile_pos[1], &funds)) menu_hidden = true;
                                }
                                else if (3*SPRITE_SIZE/2 <= event.button.x && event.button.x <= 4*SPRITE_SIZE/2 && 0 <= event.button.y && event.button.y <= SPRITE_SIZE/2) {
                                    if (buyTower(&tower_list, enemy_list, SORCERER_TOWER, selected_tile_pos[0], selected_tile_pos[1], &funds)) menu_hidden = true;
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
                    CAM_SCALE = min(max(0.0002*WINDOW_HEIGHT, CAM_SCALE*power(1.05, event.wheel.y)), 0.002*WINDOW_HEIGHT);
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

        /* Update game phase */
        switch (game_phase) {
            case PRE_WAVE_PHASE:
                break;
            case ENEMIES_MOVING_PHASE:
                /* Change phase when all enemies have finished moving and gone back to their idle animation */
                condition = true; enemy = enemy_list;
                while (enemy && condition) {
                    if (enemy->anim && enemy->anim->type != IDLE_ANIMATION) condition = false;
                    enemy = enemy->next;
                }
                /* Change phase */
                if (condition) {
                    game_phase = TOWERS_ATTACKING_PHASE;
                    makeAllTowersAct(tower_list, &currently_acting_tower);
                    currently_acting_tower = tower_list;
                }
                break;
            case TOWERS_ATTACKING_PHASE:
                /* Change phase when towers have attacked, no projectiles are left and all enemies have stoped gone back to their idle animation */
                condition = true; enemy = enemy_list;
                while (enemy && condition) {
                    if (enemy->anim && enemy->anim->type != IDLE_ANIMATION) condition = false;
                    enemy = enemy->next;
                }
                /* Change phase */
                if (!currently_acting_tower && !projectile_list && condition) {
                    game_phase = ENEMIES_ATTACKING_PHASE;
                    makeAllEnemiesAct(enemy_list, &currently_acting_enemy);
                }
                break;
            case ENEMIES_ATTACKING_PHASE:
                /* Change phase when enemies have finished attacking and gone back to their idle animation, same for the towers */
                condition = true; enemy = enemy_list; tower = tower_list;
                while (enemy && condition) {
                    if (enemy->anim && enemy->anim->type != IDLE_ANIMATION) condition = false;
                    enemy = enemy->next;
                }
                while (tower && condition) {
                    if (tower->anim && tower->anim->type != IDLE_ANIMATION) condition = false;
                    tower = tower->next;
                }
                /* Change phase */
                if (!currently_acting_enemy && condition) {
                    game_phase = ENEMIES_MOVING_PHASE;
                    makeAllEnemiesMove(enemy_list, tower_list);
                }
                break;
            case WAVE_DEAFEATED_PHASE:
                write(rend, font, "YOU WIN", WINDOW_WIDTH/4, WINDOW_HEIGHT/4, WINDOW_WIDTH/2, WINDOW_HEIGHT/2, 0, 0, 0, 255);
                break;
            case GAME_OVER_PHASE:
            write(rend, font, "GAME OVER", WINDOW_WIDTH/4, WINDOW_HEIGHT/4, WINDOW_WIDTH/2, WINDOW_HEIGHT/2, 0, 0, 0, 255);
                break;
            default:
                break;
        }

        /* Update entities */
        updateProjectiles(&projectile_list, &enemy_list, tower_list);
        updateEnemies(&currently_acting_enemy, &enemy_list, &tower_list);
        updateTowers(&currently_acting_tower, &tower_list, enemy_list, &projectile_list);

        /* Clear screen */
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);
        
        /* Move camera */
        CAM_POS_X += BASE_CAM_SPEED * cam_x_speed * power(CAM_SPEED_MULT, cam_speed_mult) * power(1.4142/2, cam_x_speed && cam_y_speed) / CAM_SCALE;
        CAM_POS_Y += BASE_CAM_SPEED * cam_y_speed * power(CAM_SPEED_MULT, cam_speed_mult) * power(1.4142/2, cam_x_speed && cam_y_speed) / CAM_SCALE;
        /* Draw elements */
        for (int y = 0; y < NB_ROWS; y++) for (int x = 0; x < NB_COLLUMNS; x++) {
                drawImgDynamic(rend, grass_tiles[x%2 + (y%2) * 2], TILE_WIDTH * x, TILE_HEIGHT * y, SPRITE_SIZE, SPRITE_SIZE, NULL);
        }
        drawEnemiesAndTowers(rend, enemy_list, tower_list);
        drawProjectiles(rend, projectile_list);
        /* Draw the Menu if necessary */
        if (!menu_hidden){ 
            drawFilledRect(rend, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT/4 + 10, 128, 128, 128, 255);
            drawRect(rend, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT/4 + 10, 255, 255, 255, 255);

            if (!getEnemyAndTowerAt(NULL, tower_list, selected_tile_pos[0], selected_tile_pos[1], NULL, &towerOnTile)) {
                for (unsigned long long i = 0; i < sizeof(towers)/sizeof(towers[0]); i++) {
                    drawImgStatic(rend, towers[i], i*SPRITE_SIZE/2+50, 0, SPRITE_SIZE/2, SPRITE_SIZE/2, NULL);
                    write(rend, font, TowersNames[i], i*SPRITE_SIZE/2 + 50, SPRITE_SIZE/2 - 10, SPRITE_SIZE/2 - 10, 30, 255, 255, 255, 255);
                }
            }

            else{
                switch (towerOnTile->type){
                    case SORCERER_TOWER:
                        drawImgStatic(rend, towers_upgrades[0], 0, 0, SPRITE_SIZE/2, SPRITE_SIZE/2, NULL);
                        break;
                    case CANON_TOWER:
                        drawImgStatic(rend, towers_upgrades[1], 0, 0, SPRITE_SIZE/2, SPRITE_SIZE/2, NULL);
                        break;
                    default:
                        break;
                }
                drawImgStatic(rend, delete_tower, WINDOW_WIDTH - SPRITE_SIZE/2, 0, SPRITE_SIZE/2, SPRITE_SIZE/2, NULL);
            }
            drawImgDynamic(rend, highlighted_tile, (selected_tile_pos[0]-1)*TILE_WIDTH, (selected_tile_pos[1]-1)*TILE_HEIGHT, SPRITE_SIZE, SPRITE_SIZE, NULL);
        }

        /* Draw to window and loop */
        SDL_RenderPresent(rend);
        SDL_Delay(max(1000/FPS - (SDL_GetTicks64()-CURRENT_TICK), 0));
        CURRENT_TICK = SDL_GetTicks64();
    }
    
    /* Free allocated memory */
    delImg(delete_tower);
    delImg(towers[3]);delImg(towers[2]);delImg(towers[1]);delImg(towers[0]);
    delImg(grass_tiles[3]); delImg(grass_tiles[2]); delImg(grass_tiles[1]); delImg(grass_tiles[0]);
    delImg(towers_upgrades[0]);
    free(selected_tile_pos);
    freeEverything(&enemy_list,&tower_list);
    /* Release resources */
    TTF_CloseFont(font);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(wind);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
