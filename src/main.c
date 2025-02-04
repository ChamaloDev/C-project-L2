#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <SDL.h>
#include <dirent.h>
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
#define FONT_WIDTH 32      // Width of the custom font in px
#define FONT_HEIGHT 48     // Height of the custom font in px

#define MAX_LENGTH_TOWER_NAME 32
#define MAX_LENGTH_NICKNAME 32

/* Enemy types */
#define SLIME_ENEMY 'S'
#define GELLY_ENEMY 'G'
#define GOBLIN_ENEMY 'g'
#define ORC_ENEMY 'O'
#define NECROMANCER_ENEMY 'N'
#define SKELETON_ENEMY 's'
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
#define SPAWN_ANIMATION 'S'
#define ATTACK_ANIMATION 'A'
#define MOVE_ANIMATION 'M'
#define PROJECTILE_ANIMATION 'P'
#define DAMAGE_NUMBER_ANIMATION 'd'
/* Game phases */
#define WAITING_FOR_USER_PHASE -1
#define PRE_WAVE_PHASE 0
#define ENEMIES_MOVING_PHASE 1
#define TOWERS_ATTACKING_PHASE 2
#define ENEMIES_ATTACKING_PHASE 3
#define VICTORY_PHASE 4
#define GAME_OVER_PHASE 5



/* Survival mode level name */
#define SURVIVAL_MODE "$urv1v@lM0d3"




/* Character name */
#define char_0 '0'
#define char_1 '1'
#define char_2 '2'
#define char_3 '3'
#define char_4 '4'
#define char_5 '5'
#define char_6 '6'
#define char_7 '7'
#define char_8 '8'
#define char_9 '9'
#define char_A_upper 'A'
#define char_B_upper 'B'
#define char_C_upper 'C'
#define char_D_upper 'D'
#define char_E_upper 'E'
#define char_F_upper 'F'
#define char_G_upper 'G'
#define char_H_upper 'H'
#define char_I_upper 'I'
#define char_J_upper 'J'
#define char_K_upper 'K'
#define char_L_upper 'L'
#define char_M_upper 'M'
#define char_N_upper 'N'
#define char_O_upper 'O'
#define char_P_upper 'P'
#define char_Q_upper 'Q'
#define char_R_upper 'R'
#define char_S_upper 'S'
#define char_T_upper 'T'
#define char_U_upper 'U'
#define char_V_upper 'V'
#define char_W_upper 'W'
#define char_X_upper 'X'
#define char_Y_upper 'Y'
#define char_Z_upper 'Z'
#define char_A_lower 'a'
#define char_B_lower 'b'
#define char_C_lower 'c'
#define char_D_lower 'd'
#define char_E_lower 'e'
#define char_F_lower 'f'
#define char_G_lower 'g'
#define char_H_lower 'h'
#define char_I_lower 'i'
#define char_J_lower 'j'
#define char_K_lower 'k'
#define char_L_lower 'l'
#define char_M_lower 'm'
#define char_N_lower 'n'
#define char_O_lower 'o'
#define char_P_lower 'p'
#define char_Q_lower 'q'
#define char_R_lower 'r'
#define char_S_lower 's'
#define char_T_lower 't'
#define char_U_lower 'u'
#define char_V_lower 'v'
#define char_W_lower 'w'
#define char_X_lower 'x'
#define char_Y_lower 'y'
#define char_Z_lower 'z'
#define char_whitespace ' '
#define char_plus '+'
#define char_minus '-'
#define char_percent '%'
#define char_dot '.'
#define char_comma ','
#define char_colon ':'
#define char_semicolon ';'
#define char_exclamation_mark '!'
#define char_question_mark '?'
#define char_dollar '$'
#define char_underscore '_'
#define char_slash '/'
#define char_antislash '\\'
#define char_left_bracket '('
#define char_right_bracket ')'
#define char_heart '&'
#define char_coin '*'




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

/* Text element */
typedef struct text_element {
    double scale;         // Text size scaling
    SDL_Rect rect;        // Rect of the text element
    bool dynamic_pos;     // Should the position of the text element be static or dynamic
    bool centered;        // Should the text element be centered or aligned to the left
    SDL_Surface *sprite;  // Sprite of the text element
    Animation *anim;      // Animation of the text element
    struct text_element *next;
} TextElement;

/* Towers */
typedef struct tower {
    int type;                  // Tower type, determine it's abilities, look and upgrades
    int max_life_points;       // Maximum life points of the tower
    int life_points;           // Life points of the tower, when it reaches 0 or bellow the tower is destroyed
    int row;                   // Row number of the tower, 0 being the topmost row
    int collumn;               // Collumn number of the tower, 0 being the leftmost collumn
    int cost;                  // Placement cost of the tower
    int base_attack_cooldown;  // Cooldown between each attack (1 or less being none)
    int attack_cooldown;       // Current cooldown before the next attack
    struct tower* next;        // Pointer to the next tower placed
    SDL_Surface *sprite;       // Sprite of the tower
    Animation *anim;           // Animation of the tower
    TextElement *life_bar;     // Life bar of the tower
} Tower;

/* Enemies */
typedef struct enemy {
    int type;                   // Enemy type, determine it's abilities and look
    int max_life_points;        // Maximum life points of the enemy
    int life_points;            // Life points of the enemy, when it reaches 0 or bellow the enemy is defeated
    int row;                    // Row number of the enemy, 0 being the topmost row
    int collumn;                // Collumn number of the enemy, 0 being the leftmost collumn
    int base_speed;             // Base number of collumn travelled per turn
    int speed;                  // Number of collumn travelled per turn, reseted to base_speed after moving
    struct enemy* next;         // Next enemy (in order of apparition)
    struct enemy* next_on_row;  // Next enemy on the same row (behind this)
    struct enemy* prev_on_row;  // Previous enemy on the same row (in front of this)
    SDL_Surface *sprite;        // Sprite of the enemy
    Animation *anim;            // Animation of the enemy
    TextElement *life_bar;      // Life bar of the enemy
    int score_on_kill;          // Score given when killing the enemy
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
    Enemy *enemy_list;  // Enemies of the wave
    int income;         // Funds gain before the wave to build towers
} Wave;

/* Game structure, composed of multiple waves */
typedef struct {
    Wave **waves;                    // Array of waves
    int nb_waves;                    // Number of waves
    int current_wave_nb;             // Current wave
    Tower *tower_list;               // Towers
    Tower *currently_acting_tower;   // Tower currently acting
    Enemy *enemy_list;               // Enemies
    Enemy *currently_acting_enemy;   // Enemy currently acting
    Projectile *projectile_list;     // Projectiles
    TextElement *text_element_list;  // Text elements representing damage numbers
    int funds;                       // Availible funds to build tower
    int score;                       // Player score
    int turn_nb;                     // Turn number
    int game_phase;                  // Current game phase, determine what type of actions to handle next
    char *level_name;                // Name of the played level
} Game;




/* Header */
int randrange(int a, int b);
bool roll(double p);
int sign(double x);
double min(double x, double y);
double max(double x, double y);
double power(double x, int n);
char *concatString(const char *a, const char *b);
char *duplicateString(const char *a);
int stringToInt(const char *str);
int positive_div(int i, int n);
int positive_mod(int i, int n);
double periodicFunctionSub(double x);
double periodicFunction(Uint64 x);
SDL_Surface *textSurface(char *text, SDL_Color main_color, SDL_Color outline_color);
Animation *newAnim();
void destroyAnim(Animation *anim);
void setAnim(Animation *anim, char type, Uint64 length, int *data);
void setAnimIdle(Animation *anim);
void setAnimHurt(Animation *anim);
void setAnimSpawn(Animation *anim);
void setAnimAttack(Animation *anim, int side);
void setAnimMove(Animation *anim, int dx, int dy);
void setAnimProjectile(Animation *anim, int x1, int y1, int x2, int y2, double speed);
void setAnimDamageNumber(Animation *anim);
bool applyAnim(Animation *anim, SDL_Rect *rect);
TextElement *addTextElement(TextElement **text_element_list, char *text, double scale, SDL_Color main_color, SDL_Color outline_color, SDL_Rect rect, bool centered, bool dynamic_pos, Animation *anim);
void destroyTextElement(TextElement *text_element, TextElement **text_element_list);
void drawTextElements(SDL_Renderer *rend, TextElement **text_element_list);
TextElement *addDamageNumber(TextElement **text_element_list, int amount, int collumn, int row);
bool getEnemyAndTowerAt(Enemy *enemy_list, Tower *tower_list, int collumn, int row, Enemy **enemy, Tower **tower);
bool isTileEmpty(Enemy *enemy_list, Tower *tower_list, int collumn, int row);
bool doesTileExist(int collumn, int row);
Enemy *addEnemy(Enemy **enemy_list, char enemy_type, int spawn_collumn, int spawn_row, int life_points);
void destroyEnemy(Enemy *enemy, Enemy **enemy_list);
Enemy **getFirstEnemyOfAllRows(Enemy *enemy_list);
Enemy *getFirstEnemyInRow(Enemy *enemy_list, int row);
void updateEnemies(Enemy **currently_acting_enemy, Enemy **enemy_list, Tower **tower_list, TextElement **text_element_list);
int moveEnemy(Enemy *enemy, Enemy *enemy_list, Tower *tower_list, int delta, char axis);
void makeAllEnemiesMove(Enemy *enemy_list, Tower *tower_list);
void enemyAttack(Enemy *enemy, Tower **tower_list, Enemy **enemy_list, TextElement **text_element_list);
void makeAllEnemiesAct(Enemy *enemy_list, Enemy **currently_acting_enemy);
bool damageEnemy(Enemy *enemy, int amount, Enemy **enemy_list, Tower *tower_list, TextElement **text_element_list, int *score);
Tower *addTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_row, int placement_collumn, int life_points);
Tower *buyTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_row, int placement_collumn, int *funds);
void destroyTower(Tower *tower, Tower **tower_list);
void sellTower(Tower *tower, Tower **tower_list, int *funds);
void towerAct(Tower *tower, Tower **tower_list, Enemy *enemy_list, Projectile **projectile_list);
void updateTowers(Tower **currently_acting_tower, Tower **tower_list, Enemy *enemy_list, Projectile **projectile_list);
void makeAllTowersAct(Tower *tower_list, Tower **currently_acting_tower);
bool damageTower(Tower *tower, int amount, Tower **tower_list, TextElement **text_element_list);
Projectile *addProjectile(Projectile **projectile_list, Tower *origin, Enemy *target);
void destroyProjectile(Projectile *projectile, Projectile **projectile_list);
bool hasProjectileReachedTarget(Projectile *projectile);
void updateProjectiles(Projectile **projectile_list, Enemy **enemy_list, Tower *tower_list, TextElement **text_element_list, int *score);
void drawProjectiles(SDL_Renderer *rend, Projectile *projectile_list);
Game *createNewGame(char *level_name);
Game *loadGameFromSave(char *save_file);
bool loadNextWave(Game *game);
void startNextWave(Game *game);
void updateGame(Game *game, const char *nickname);
void beguinNewSurvivalWave(Game *game);
Wave *newWave(int income, Enemy *enemy_list);
void destroyWaveList(Wave **wave_list, int nb_wave);
bool isWhitespace(char c);
bool readValue(char **line, char **value);
bool readLine(FILE *file, char ***values, int *nb_values);
bool loadLevel(const char *path, Wave ***waves, int *nb_waves);
SDL_Surface *loadImg(const char *path);
void delImg(SDL_Surface *img);
void drawEnemiesAndTowers(SDL_Renderer *rend, Enemy *enemy_list, Tower *tower_list, int game_phase);
void dynamicToStatic(SDL_Rect *rect);
void staticToDynamic(SDL_Rect *rect);
void tileToPixel(int *x, int *y);
void pixelToTile(int *x, int *y);
void drawImgStatic(SDL_Renderer *rend, SDL_Surface *img, int pos_x, int pos_y, int width, int height, Animation *anim);
void drawImgDynamic(SDL_Renderer *rend, SDL_Surface *img, int pos_x, int pos_y, int width, int height, Animation *anim);
void drawRect(SDL_Renderer *rend, int pos_x, int pos_y, int width, int height, int red, int green, int blue, int alpha);
void drawFilledRect(SDL_Renderer *rend, int pos_x, int pos_y, int width, int height, int red, int green, int blue, int alpha);

Tower *upgradeTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_collumn, int placement_row, int *funds);
bool saveGame(const char *save_name, Game *game);




/* Return an integer between a and b (included) */
int randrange(int a, int b) {
    return positive_mod(rand(), abs(b-a)+1) + min(a, b);
}

/* Has p chance to return true, and 1-p chance to return false */
bool roll(double p) {
    /* Roll precision */
    int precision = 10000;
    /* p must be a value between 0.0 and 1.0 included */
    p = min(max(0.0, p), 1.0);
    /* Return the roll */
    return (p * precision > randrange(0, precision));
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

/* Duplicate a string and automaticaly allocate memory */
char *duplicateString(const char *a) {
    char *b = malloc(strlen(a) + 1);
    if (b) strcpy(b, a);
    return b;
}

/* Convert a string to an int */
int stringToInt(const char *str) {
    int n = 0;
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


/* Create a new text surface */
SDL_Surface *textSurface(char *text, SDL_Color main_color, SDL_Color outline_color) {
    /* Getting number of lines and the maximum line size in order to create a surface of appropriate size */
    int nb_lines = 1, max_line_length = 0, current_line_length = 0;
    char *c = text;
    while (*c) {
        /* Line break */
        if (*c == '\n') {
            max_line_length = max(max_line_length, current_line_length);
            current_line_length = 0;
            nb_lines++;
        }
        /* Any other character */
        else {
            current_line_length++;
        }
        c++;
    }
    max_line_length = max(max_line_length, current_line_length);
    /* Creating text zone surface */
    SDL_Surface *text_zone = SDL_CreateRGBSurface(0, max_line_length*FONT_WIDTH, nb_lines*FONT_HEIGHT, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    /* Putting text in the text zone surface */
    SDL_Surface *character; SDL_Rect dest; SDL_Rect char_rect = {0, 0, FONT_WIDTH, FONT_HEIGHT};
    int x = 0, y = 0;
    c = text;
    while (*c) {
        /* Line break */
        if (*c == '\n') {
            y++;
            x = 0;
        }
        /* Any other character */
        else {
            /* Load the character */
            character = NULL;
            switch ((int) *c) {
                case char_0:
                    character = loadImg("font/char_0");
                    break;
                case char_1:
                    character = loadImg("font/char_1");
                    break;
                case char_2:
                    character = loadImg("font/char_2");
                    break;
                case char_3:
                    character = loadImg("font/char_3");
                    break;
                case char_4:
                    character = loadImg("font/char_4");
                    break;
                case char_5:
                    character = loadImg("font/char_5");
                    break;
                case char_6:
                    character = loadImg("font/char_6");
                    break;
                case char_7:
                    character = loadImg("font/char_7");
                    break;
                case char_8:
                    character = loadImg("font/char_8");
                    break;
                case char_9:
                    character = loadImg("font/char_9");
                    break;
                case char_A_upper:
                    character = loadImg("font/char_A_upper");
                    break;
                case char_B_upper:
                    character = loadImg("font/char_B_upper");
                    break;
                case char_C_upper:
                    character = loadImg("font/char_C_upper");
                    break;
                case char_D_upper:
                    character = loadImg("font/char_D_upper");
                    break;
                case char_E_upper:
                    character = loadImg("font/char_E_upper");
                    break;
                case char_F_upper:
                    character = loadImg("font/char_F_upper");
                    break;
                case char_G_upper:
                    character = loadImg("font/char_G_upper");
                    break;
                case char_H_upper:
                    character = loadImg("font/char_H_upper");
                    break;
                case char_I_upper:
                    character = loadImg("font/char_I_upper");
                    break;
                case char_J_upper:
                    character = loadImg("font/char_J_upper");
                    break;
                case char_K_upper:
                    character = loadImg("font/char_K_upper");
                    break;
                case char_L_upper:
                    character = loadImg("font/char_L_upper");
                    break;
                case char_M_upper:
                    character = loadImg("font/char_M_upper");
                    break;
                case char_N_upper:
                    character = loadImg("font/char_N_upper");
                    break;
                case char_O_upper:
                    character = loadImg("font/char_O_upper");
                    break;
                case char_P_upper:
                    character = loadImg("font/char_P_upper");
                    break;
                case char_Q_upper:
                    character = loadImg("font/char_Q_upper");
                    break;
                case char_R_upper:
                    character = loadImg("font/char_R_upper");
                    break;
                case char_S_upper:
                    character = loadImg("font/char_S_upper");
                    break;
                case char_T_upper:
                    character = loadImg("font/char_T_upper");
                    break;
                case char_U_upper:
                    character = loadImg("font/char_U_upper");
                    break;
                case char_V_upper:
                    character = loadImg("font/char_V_upper");
                    break;
                case char_W_upper:
                    character = loadImg("font/char_W_upper");
                    break;
                case char_X_upper:
                    character = loadImg("font/char_X_upper");
                    break;
                case char_Y_upper:
                    character = loadImg("font/char_Y_upper");
                    break;
                case char_Z_upper:
                    character = loadImg("font/char_Z_upper");
                    break;
                case char_A_lower:
                    character = loadImg("font/char_A_lower");
                    break;
                case char_B_lower:
                    character = loadImg("font/char_B_lower");
                    break;
                case char_C_lower:
                    character = loadImg("font/char_C_lower");
                    break;
                case char_D_lower:
                    character = loadImg("font/char_D_lower");
                    break;
                case char_E_lower:
                    character = loadImg("font/char_E_lower");
                    break;
                case char_F_lower:
                    character = loadImg("font/char_F_lower");
                    break;
                case char_G_lower:
                    character = loadImg("font/char_G_lower");
                    break;
                case char_H_lower:
                    character = loadImg("font/char_H_lower");
                    break;
                case char_I_lower:
                    character = loadImg("font/char_I_lower");
                    break;
                case char_J_lower:
                    character = loadImg("font/char_J_lower");
                    break;
                case char_K_lower:
                    character = loadImg("font/char_K_lower");
                    break;
                case char_L_lower:
                    character = loadImg("font/char_L_lower");
                    break;
                case char_M_lower:
                    character = loadImg("font/char_M_lower");
                    break;
                case char_N_lower:
                    character = loadImg("font/char_N_lower");
                    break;
                case char_O_lower:
                    character = loadImg("font/char_O_lower");
                    break;
                case char_P_lower:
                    character = loadImg("font/char_P_lower");
                    break;
                case char_Q_lower:
                    character = loadImg("font/char_Q_lower");
                    break;
                case char_R_lower:
                    character = loadImg("font/char_R_lower");
                    break;
                case char_S_lower:
                    character = loadImg("font/char_S_lower");
                    break;
                case char_T_lower:
                    character = loadImg("font/char_T_lower");
                    break;
                case char_U_lower:
                    character = loadImg("font/char_U_lower");
                    break;
                case char_V_lower:
                    character = loadImg("font/char_V_lower");
                    break;
                case char_W_lower:
                    character = loadImg("font/char_W_lower");
                    break;
                case char_X_lower:
                    character = loadImg("font/char_X_lower");
                    break;
                case char_Y_lower:
                    character = loadImg("font/char_Y_lower");
                    break;
                case char_Z_lower:
                    character = loadImg("font/char_Z_lower");
                    break;
                case char_whitespace:
                    character = loadImg("font/char_whitespace");
                    break;
                case char_plus:
                    character = loadImg("font/char_plus");
                    break;
                case char_minus:
                    character = loadImg("font/char_minus");
                    break;
                case char_percent:
                    character = loadImg("font/char_percent");
                    break;
                case char_dot:
                    character = loadImg("font/char_dot");
                    break;
                case char_comma:
                    character = loadImg("font/char_comma");
                    break;
                case char_colon:
                    character = loadImg("font/char_colon");
                    break;
                case char_semicolon:
                    character = loadImg("font/char_semicolon");
                    break;
                case char_exclamation_mark:
                    character = loadImg("font/char_exclamation_mark");
                    break;
                case char_question_mark:
                    character = loadImg("font/char_question_mark");
                    break;
                case char_dollar:
                    character = loadImg("font/char_dollar");
                    break;
                case char_underscore:
                    character = loadImg("font/char_underscore");
                    break;
                case char_slash:
                    character = loadImg("font/char_slash");
                    break;
                case char_antislash:
                    character = loadImg("font/char_antislash");
                    break;
                case char_left_bracket:
                    character = loadImg("font/char_left_bracket");
                    break;
                case char_right_bracket:
                    character = loadImg("font/char_right_bracket");
                    break;
                case char_heart:
                    character = loadImg("font/char_heart");
                    break;
                case char_coin:
                    character = loadImg("font/char_coin");
                    break;
                default:
                    character = NULL;
                    break;
            }
            if (!character) {
                printf("[ERROR]    No font availible for character '%c'\n", *c);
                character = loadImg("font/char_question_mark");
            }
            /* Color the character */
            Uint32 *pixels = character->pixels;
            for (int i = 0; i < character->w * character->h; i++) {
                /* Main color (by default #000000 black) */
                if (pixels[i] == SDL_MapRGB(character->format, 0x00, 0x00, 0x00))
                    pixels[i] = SDL_MapRGB(character->format, main_color.r, main_color.g, main_color.b);
                /* Outline color (by default #FFFFFF white) */
                else if (pixels[i] == SDL_MapRGB(character->format, 0xFF, 0xFF, 0xFF))
                    pixels[i] = SDL_MapRGB(character->format, outline_color.r, outline_color.g, outline_color.b);
            }
            /* Draw the character */
            dest.x = x*FONT_WIDTH; dest.y = y*FONT_HEIGHT; dest.w = FONT_WIDTH; dest.h = FONT_HEIGHT;
            SDL_BlitSurface(character, &char_rect, text_zone, &dest);
            x++;
            /* Free the allocated memory */
            SDL_FreeSurface(character);
        }
        c++;
    }
    return text_zone;
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

/* Set animation to spawn */
void setAnimSpawn(Animation *anim) {
    setAnim(anim, SPAWN_ANIMATION, 1000, NULL);
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

/* Set animation to projectile (for projectiles), goes from tile (x1 y1) to tile (x2 y2) at speed tile per second */
void setAnimProjectile(Animation *anim, int x1, int y1, int x2, int y2, double speed) {
    int *data = malloc(4 * sizeof(int));
    data[0] = x1; data[1] = y1; data[2] = x2; data[3] = y2;
    setAnim(anim, PROJECTILE_ANIMATION, ((abs(x1-x2) + abs(y1-y2))*1000.0 / speed), data);
}

/* Set animation to damage number (for text elements) */
void setAnimDamageNumber(Animation *anim) {
    setAnim(anim, DAMAGE_NUMBER_ANIMATION, 1000, NULL);
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
        /* Spawn animation (appear on the tile while slightly droping from above) */
        case SPAWN_ANIMATION:
            var_a = (CURRENT_TICK - anim->start_tick) / (double) anim->length;
            rect->x += SPRITE_SIZE * (1 - var_a) / 2;
            rect->y += SPRITE_SIZE * (1 - var_a) / 2 - TILE_HEIGHT * (1 - var_a) * 5;
            rect->w *= var_a;
            rect->h *= var_a;
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
        case DAMAGE_NUMBER_ANIMATION:
            var_a = (CURRENT_TICK - anim->start_tick) / (double) anim->length;
            rect->y -= (var_a - 0.5) * TILE_HEIGHT;
            break;
        /* Unknown animation type */
        default:
            printf("[ERROR]    Undefined animation type '%c'\n", anim->type);
            setAnimIdle(anim);
            return false;
    }
    return true;
}




/* Add a text element */
TextElement *addTextElement(TextElement **text_element_list, char *text, double scale, SDL_Color main_color, SDL_Color outline_color, SDL_Rect rect, bool centered, bool dynamic_pos, Animation *anim) {
    /* Initialize a new text element */
    TextElement *new_text_element = malloc(sizeof(TextElement));
    new_text_element->scale = scale;
    new_text_element->rect = rect;
    new_text_element->centered = centered;
    new_text_element->dynamic_pos = dynamic_pos;
    new_text_element->sprite = NULL;
    new_text_element->anim = anim;
    new_text_element->next = NULL;
    /* Load the text sprite */
    new_text_element->sprite = textSurface(text, main_color, outline_color);
    /* Add it to the list */
    if (!text_element_list) return new_text_element;
    if (!(*text_element_list)) {
        *text_element_list = new_text_element;
        return new_text_element;
    }
    TextElement *current = *text_element_list;
    while (current && current->next) current = current->next;
    if (current) current->next = new_text_element;
    return new_text_element;
}

/* Destroy a text element */
void destroyTextElement(TextElement *text_element, TextElement **text_element_list) {
    if (!text_element) return;
    /* Modify pointers */
    if (text_element_list) {
        if (*text_element_list == text_element) *text_element_list = text_element->next;
        else {
            TextElement *current = *text_element_list;
            while (current && current->next != text_element) current = current->next;
            if (current) current->next = text_element->next;
        }
    }
    /* Free memory */
    if (text_element->sprite) SDL_FreeSurface(text_element->sprite);
    if (text_element->anim) destroyAnim(text_element->anim);
    free(text_element);
}

/* Draw all text elements, if a text element animation is set to idle, automaticaly destroy it */
void drawTextElements(SDL_Renderer *rend, TextElement **text_element_list) {
    TextElement *current = *text_element_list, *tmp;
    int x, y, w, h;
    while (current) {
        /* Automaticaly destroy text element that are currently in idle animation (or if doesn't have a sprite) */
        if (!current->sprite || (current->anim && ((current->anim->length != (Uint64) -1 && CURRENT_TICK - current->anim->start_tick >= current->anim->length) || current->anim->type == IDLE_ANIMATION))) {
            tmp = current->next;
            destroyTextElement(current, text_element_list);
            current = tmp;
            continue;
        }
        /* Adjusting draw rect */
        x = current->rect.x;
        y = current->rect.y;
        w = min(current->sprite->w * current->scale, current->rect.w);
        h = min(current->sprite->h * current->scale, current->rect.h);
        if (current->centered) {
            x += (current->rect.w - w)/2;
            y += (current->rect.h - h)/2;
        }
        /* Drawing text element */
        if (current->dynamic_pos) drawImgDynamic(rend, current->sprite, x, y, w, h, current->anim);
        else drawImgStatic(rend, current->sprite, x, y, w, h, current->anim);
        current = current->next;
    }
}

/* Add a damage number at the specified position */
TextElement *addDamageNumber(TextElement **text_element_list, int amount, int collumn, int row) {
    /* Text value */
    char text[16];
    sprintf(text, "-%d", amount);
    /* Colors */
    SDL_Color main_color = {127, 0, 0, 255};
    SDL_Color outline_color = {0, 0, 0, 255};
    /* Destination rect */
    SDL_Rect rect = {(collumn-1)*TILE_WIDTH, (row-1)*TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT};
    /* Damage number animation */
    Animation *anim = newAnim();
    setAnimDamageNumber(anim);
    /* Ceating the new text element */
    return addTextElement(text_element_list, text, 2.0, main_color, outline_color, rect, true, true, anim);
}

/* Generate a new text element representing the life points of an entity, destroying the previous one */
TextElement *updateLifeBarTextElement(TextElement **text_element, int current_life_points, int max_life_points) {
    if (!text_element || current_life_points > max_life_points || max_life_points <= 0) return NULL;
    /* Free previous text element representing the life bar */
    if (*text_element) destroyTextElement(*text_element, NULL);
    /* Text value for the new text element */
    char text_value[1000];
    sprintf(text_value, "%d/%d&", current_life_points, max_life_points);
    /* Colors */
    double life_ratio = max(0, current_life_points)/max_life_points;
    SDL_Color main_color = {(int) 255.0 * (1 - power(life_ratio, 2)), (int) 255.0 * (1.0 - power(1 - life_ratio, 2)), 0, 255};
    SDL_Color outline_color = {main_color.r/2, main_color.g/2, 0, 255};
    /* Create new text element representing the life bar */
    *text_element = addTextElement(NULL, text_value, 1.0, main_color, outline_color, (SDL_Rect) {0, 0, SPRITE_SIZE, 48}, true, true, NULL);
    return *text_element;
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
Enemy *addEnemy(Enemy **enemy_list, char enemy_type, int spawn_collumn, int spawn_row, int life_points) {
    if (!enemy_list) return NULL;
    /* Can't summon enemies in not existing rows */
    if (1 > spawn_row || spawn_row > NB_ROWS) return NULL;

    /* Initialize the new enemy */
    Enemy *new_enemy = malloc(sizeof(Enemy));
    new_enemy->type = enemy_type;
    new_enemy->collumn = spawn_collumn;
    new_enemy->row = spawn_row;
    new_enemy->next = new_enemy->next_on_row = new_enemy->prev_on_row = NULL;
    new_enemy->anim = newAnim();
    new_enemy->life_bar = NULL;
    /* Match the enemy type to its stats */
    switch (enemy_type) {
        case SLIME_ENEMY:
            new_enemy->max_life_points = new_enemy->life_points = 5;
            new_enemy->base_speed = new_enemy->speed = 2;
            new_enemy->sprite = loadImg("enemies/Slime");
            new_enemy->score_on_kill = 25;
            break;
        case GELLY_ENEMY:
            new_enemy->max_life_points = new_enemy->life_points = 6;
            new_enemy->base_speed = new_enemy->speed = 2;
            new_enemy->sprite = loadImg("enemies/Gelly");
            new_enemy->score_on_kill = 50;
            break;
        case GOBLIN_ENEMY:
            new_enemy->max_life_points = new_enemy->life_points = 10;
            new_enemy->base_speed = new_enemy->speed = 3;
            new_enemy->sprite = loadImg("enemies/Goblin");
            new_enemy->score_on_kill = 75;
            break;
        case ORC_ENEMY:
            new_enemy->max_life_points = new_enemy->life_points = 20;
            new_enemy->base_speed = new_enemy->speed = 1;
            new_enemy->sprite = loadImg("enemies/Orc");
            new_enemy->score_on_kill = 150;
            break;
        case NECROMANCER_ENEMY:
            new_enemy->max_life_points = new_enemy->life_points = 13;
            new_enemy->base_speed = new_enemy->speed = 1;
            new_enemy->sprite = loadImg("enemies/necromancer");
            new_enemy->score_on_kill = 200;
            break;
        case SKELETON_ENEMY:
            new_enemy->max_life_points = new_enemy->life_points = 4;
            new_enemy->base_speed = new_enemy->speed = 3;
            new_enemy->sprite = loadImg("enemies/skeleton");
            new_enemy->score_on_kill = 25;
            break;
        default:  /* Unknown enemy type */
            printf("[ERROR]    Unknown enemy type '%c'\n", enemy_type);
            destroyAnim(new_enemy->anim);
            free(new_enemy);
            return NULL;
    }
    /* Initialize life bar */
    
    /* If health is manualy set */
    if (!enemy_list) return new_enemy;
    if (life_points != -1){
        new_enemy->life_points = life_points;
    }
    updateLifeBarTextElement(&new_enemy->life_bar, new_enemy->life_points, new_enemy->max_life_points);
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
    if (!enemy) return;
    /* Change pointers of enemies accordingly */
    if (enemy_list) {
        if (*enemy_list == enemy) {
            *enemy_list = enemy->next;
        }
        else {
            Enemy *prev_enemy = *enemy_list;
            while (prev_enemy && prev_enemy->next != enemy) prev_enemy = prev_enemy->next;
            if (prev_enemy) prev_enemy->next = enemy->next;
        }
    }
    if (enemy->prev_on_row) enemy->prev_on_row->next_on_row = enemy->next_on_row;
    if (enemy->next_on_row) enemy->next_on_row->prev_on_row = enemy->prev_on_row;
    /* Destroy enemy data */
    if (enemy->sprite) delImg(enemy->sprite);
    if (enemy->anim) destroyAnim(enemy->anim);
    if (enemy->life_bar) destroyTextElement(enemy->life_bar, NULL);
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
        enemy->prev_on_row = enemy->next_on_row = NULL;
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
            current = current->next;
        }
        /* Updating pointers */
        if (enemy->prev_on_row) enemy->prev_on_row->next_on_row = enemy;
        if (enemy->next_on_row) enemy->next_on_row->prev_on_row = enemy;
        return delta;
    }

    /* Invalid axis */
    printf("[ERROR]    Can only move an enemy on the 'x' or 'y' axis, not on the '%c' axis\n", axis);
    return 0;
}

/* Update all enemies */
void updateEnemies(Enemy **currently_acting_enemy, Enemy **enemy_list, Tower **tower_list, TextElement **text_element_list) {
    Enemy *enemy;// = *enemy_list; Enemy *tmp;
    // while (enemy) {
    //     /* Destroy enemy when life points are bellow 0 */
    //     if (enemy->life_points <= 0) {
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
        enemyAttack(*currently_acting_enemy, tower_list, enemy_list, text_element_list);
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
            if (delta) {
                /* If the enemy just spawned in, play a special animation */
                if (enemy->collumn == NB_COLLUMNS) setAnimSpawn(enemy->anim);
                /* Default movement animation */
                else if (enemy->collumn < NB_COLLUMNS) setAnimMove(enemy->anim, delta, 0);
            }
            enemy->speed = enemy->base_speed;
            enemy = enemy->next_on_row;
        }
    }
    /* Free memory */
    free(first_of_each_row);
}

/* Make a singular enemy attack */
void enemyAttack(Enemy *enemy, Tower **tower_list, Enemy **enemy_list, TextElement **text_element_list) {
    if (!enemy) return;
    /* Getting tower in front of the enemy (if there is one) */
    Enemy *e; Tower *tower; bool result;
    getEnemyAndTowerAt(*enemy_list, *tower_list, enemy->collumn-1, enemy->row, &e, &tower);
    /* Making enemy act accordingly to its type */
    result = 0;
    switch (enemy->type) {
        case SLIME_ENEMY:
            if (tower) result = damageTower(tower, 2, tower_list, text_element_list);
            break;
        case GELLY_ENEMY:
            if (tower) result = damageTower(tower, 2, tower_list, text_element_list);
            break;
        case GOBLIN_ENEMY:
            if (tower) result = damageTower(tower, 3, tower_list, text_element_list);
            break;
        case ORC_ENEMY:
            if (tower) result = damageTower(tower, 5, tower_list, text_element_list);
            break;
        case NECROMANCER_ENEMY:
            if (tower) result = damageTower(tower, 4, tower_list,text_element_list);
            break;
        case SKELETON_ENEMY:
            if (tower) result = damageTower(tower, 2, tower_list,text_element_list);
            break;
        default:  /* Unknown enemy type */
            printf("[ERROR]    Unknown enemy type '%c'\n", enemy->type);
            destroyEnemy(enemy, enemy_list);
            break;
    }
    if (result) {
        setAnimAttack(enemy->anim, -1);
        enemy->speed = 0;
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
bool damageEnemy(Enemy *enemy, int amount, Enemy **enemy_list, Tower *tower_list, TextElement **text_element_list, int *score) {
    if (!enemy || !amount || !enemy_list) return false;
    int n, x, y;

    /* Check that enemy still exist */
    Enemy *e = *enemy_list;
    while (e && e != enemy) e = e->next;
    if (!e) return false;
    /* Show damage number */
    if (text_element_list) addDamageNumber(text_element_list, amount, enemy->collumn, enemy->row);
    /* Damage enemy */
    enemy->life_points -= amount;
    setAnimHurt(enemy->anim);
    /* Update life bar */
    updateLifeBarTextElement(&enemy->life_bar, enemy->life_points, enemy->max_life_points);
    /* Kill enemy if health reaches 0 or less */
    if (enemy->life_points <= 0) {
        n = enemy->type; x = enemy->collumn; y = enemy->row;
        *score += enemy->score_on_kill;
        destroyEnemy(enemy, enemy_list);
        /* Gelly splits into 2 slimes on death, one above and one bellow + one at current position or behind if a slime spawn position is blocked */
        if (n == GELLY_ENEMY) {
            n = 2;
            if (n-- && isTileEmpty(*enemy_list, tower_list, x, y - 1) && doesTileExist(x, y - 1) && (e = addEnemy(enemy_list, SLIME_ENEMY, x, y - 1, -1))) setAnimMove(e->anim, 0, -1);
            else n++;
            if (n-- && isTileEmpty(*enemy_list, tower_list, x, y + 1) && doesTileExist(x, y + 1) && (e = addEnemy(enemy_list, SLIME_ENEMY, x, y + 1, -1))) setAnimMove(e->anim, 0, +1);
            else n++;
            if (n-- && isTileEmpty(*enemy_list, tower_list, x + 1, y) && doesTileExist(x + 1, y) && (e = addEnemy(enemy_list, SLIME_ENEMY, x + 1, y, -1))) setAnimMove(e->anim, +1, 0);
            else n++;
            if (n-- && isTileEmpty(*enemy_list, tower_list, x, y) && doesTileExist(x, y) && addEnemy(enemy_list, SLIME_ENEMY, x, y, -1));
            else n++;
        }
        return true;
    }

    /* Goblin changes row on hit, depending on its hp left */
    if (enemy->type == GOBLIN_ENEMY) {
        n = 0;
        if (enemy->life_points % 2) {
            n = moveEnemy(enemy, *enemy_list, tower_list, 1, 'y');
            if (!n) n = moveEnemy(enemy, *enemy_list, tower_list, -1, 'y');
        }
        else {
            n = moveEnemy(enemy, *enemy_list, tower_list, -1, 'y');
            if (!n) n = moveEnemy(enemy, *enemy_list, tower_list, 1, 'y');
        }
        if (n && enemy->anim) setAnimMove(enemy->anim, 0, n);
    }
    /* Necromancer summons a skeleton nearby on hit */
    else if (enemy->type == NECROMANCER_ENEMY) {
        x = enemy->collumn; y = enemy->row;
        if (isTileEmpty(*enemy_list, tower_list, x - 1, y) && doesTileExist(x - 1, y) && (e = addEnemy(enemy_list, SKELETON_ENEMY, x - 1, y, -1))) setAnimSpawn(e->anim);
        else if (isTileEmpty(*enemy_list, tower_list, x, y - 1) && doesTileExist(x, y - 1) && (e = addEnemy(enemy_list, SKELETON_ENEMY, x, y - 1, -1))) setAnimSpawn(e->anim);
        else if (isTileEmpty(*enemy_list, tower_list, x, y + 1) && doesTileExist(x, y + 1) && (e = addEnemy(enemy_list, SKELETON_ENEMY, x, y + 1, -1))) setAnimSpawn(e->anim);
        else if (isTileEmpty(*enemy_list, tower_list, x + 1, y) && doesTileExist(x + 1, y) && (e = addEnemy(enemy_list, SKELETON_ENEMY, x + 1, y, -1))) setAnimSpawn(e->anim);
    }
    return true;
}




Tower *addTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_collumn, int placement_row, int life_points) {
    /* Invalid position (cannot place outside of the map or on the last collumn) */
    if (1 > placement_row || placement_row > NB_ROWS || 1 > placement_collumn || placement_collumn > NB_COLLUMNS-1) return NULL;

    /* Initialize a new tower object */
    Tower *new_tower = malloc(sizeof(Tower));
    new_tower->type = tower_type;
    new_tower->collumn = placement_collumn;
    new_tower->row = placement_row;
    new_tower->attack_cooldown = 1;
    new_tower->next = NULL;
    new_tower->anim = newAnim();
    new_tower->life_bar = NULL;
    switch (tower_type){
        case ARCHER_TOWER:
            new_tower->max_life_points = new_tower->life_points = 6;
            new_tower->cost = 50;
            new_tower->base_attack_cooldown = 1;
            new_tower->sprite = loadImg("towers/Archer_tower");
            break;
        case WALL_TOWER:
            new_tower->max_life_points = new_tower->life_points = 10;
            new_tower->cost = 30;
            new_tower->base_attack_cooldown = 1;
            new_tower->sprite = loadImg("towers/Empty_tower");
            break;
        case BARRACK_TOWER:
            new_tower->max_life_points = new_tower->life_points = 15;
            new_tower->cost = 70;
            new_tower->base_attack_cooldown = 5;
            new_tower->sprite = loadImg("towers/barracks");
            break;
        case SOLIDER_TOWER:
            new_tower->max_life_points = new_tower->life_points = 4;
            new_tower->cost = 0;
            new_tower->base_attack_cooldown = 1;
            new_tower->sprite = loadImg("towers/Spearman");
            break;
        case CANON_TOWER:
            new_tower->max_life_points = new_tower->life_points = 4;
            new_tower->cost = 100;
            new_tower->base_attack_cooldown = 3;
            new_tower->sprite = loadImg("towers/canon");
            break;
        case DESTROYER_TOWER:
            new_tower->max_life_points = new_tower->life_points = 8;
            new_tower->cost = 120;
            new_tower->base_attack_cooldown = 3;
            new_tower->sprite = loadImg("towers/canon_evolved");
            break;
        case SORCERER_TOWER:
            new_tower->max_life_points = new_tower->life_points = 5;
            new_tower->cost = 70;
            new_tower->base_attack_cooldown = 2;
            new_tower->sprite = loadImg("towers/sorcerer");
            break;
        case MAGE_TOWER:
            new_tower->max_life_points = new_tower->life_points = 7;
            new_tower->cost = 100;
            new_tower->base_attack_cooldown = 2;
            new_tower->sprite = loadImg("towers/sorcerer_evolved");
            break;
        default:  /* Invalid tower type */
            printf("[ERROR]    Unknown tower type '%c'\n", tower_type);
            destroyTower(new_tower, tower_list);
            return NULL;
    }
    /* Initialize life bar */
    

    /* If health is manually set */
    if (life_points !=-1){
        new_tower->life_points = life_points;
    }
    updateLifeBarTextElement(&new_tower->life_bar, new_tower->life_points, new_tower->max_life_points);
    if (!tower_list) return new_tower;
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
    new_tower = addTower(tower_list, enemy_list, tower_type, placement_collumn, placement_row,-1);
    /* If new_tower is NULL, it means it couldn't be build */
    if (!new_tower) return NULL;
    /* Check if the player has enough funds to build the tower */
    if (*funds < new_tower->cost) {
        destroyTower(new_tower, tower_list);
        return NULL;
    }
    *funds -= new_tower->cost;
    /* Play a spawning animation */
    setAnimSpawn(new_tower->anim);
    return new_tower;
}

/* Try to upgrade a tower */
Tower *upgradeTower(Tower **tower_list, Enemy *enemy_list, char tower_type, int placement_collumn, int placement_row, int *funds) {
    Tower *old_tower,*new_tower;
    switch (tower_type){
        case WALL_TOWER:
            getEnemyAndTowerAt(NULL, *tower_list, placement_collumn, placement_row, NULL, &old_tower);
            destroyTower(old_tower, tower_list);
            new_tower = addTower(tower_list,enemy_list, BARRACK_TOWER, placement_collumn, placement_row,-1);
            if (*funds < new_tower->cost){
                destroyTower(new_tower,tower_list);
                addTower(tower_list, enemy_list, tower_type, placement_collumn, placement_row,-1);
                return NULL;
            }
            *funds -= new_tower->cost;
            break;
        case SORCERER_TOWER:
            getEnemyAndTowerAt(NULL, *tower_list, placement_collumn, placement_row, NULL, &old_tower);
            destroyTower(old_tower, tower_list);
            new_tower = addTower(tower_list, enemy_list, MAGE_TOWER, placement_collumn, placement_row,-1);
            if (*funds < new_tower->cost){
                destroyTower(new_tower,tower_list);
                addTower(tower_list, enemy_list, tower_type, placement_collumn, placement_row,-1);
                return NULL;
            }
            *funds -= new_tower->cost;
            break;
        case CANON_TOWER:
            getEnemyAndTowerAt(NULL, *tower_list, placement_collumn, placement_row, NULL, &old_tower);
            destroyTower(old_tower, tower_list);
            new_tower = addTower(tower_list, enemy_list, DESTROYER_TOWER, placement_collumn, placement_row,-1);
            if (*funds < new_tower->cost) {
                destroyTower(new_tower, tower_list);
                addTower(tower_list, enemy_list, tower_type, placement_collumn, placement_row,-1);
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
    if (!tower) return;
    /* Change pointers of tower accordingly */
    if (tower_list) {
        Tower *prev_tower = *tower_list;
        if (prev_tower == tower) {
            *tower_list = tower->next;
        }
        else {
            while (prev_tower && prev_tower->next != tower) prev_tower = prev_tower->next;
            if (prev_tower) prev_tower->next = tower->next;
        }
    }
    /* Destroy tower data */
    if (tower->sprite) delImg(tower->sprite);
    if (tower->anim) destroyAnim(tower->anim);
    if (tower->life_bar) destroyTextElement(tower->life_bar, NULL);
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
                if (isTileEmpty(enemy_list, *tower_list, tower->collumn, tower->row - 1) && doesTileExist(tower->collumn, tower->row - 1) && (tmp = addTower(tower_list, enemy_list, SOLIDER_TOWER, tower->collumn, tower->row - 1,-1)));
                else if (isTileEmpty(enemy_list, *tower_list, tower->collumn, tower->row + 1) && doesTileExist(tower->collumn, tower->row + 1) && (tmp = addTower(tower_list, enemy_list, SOLIDER_TOWER, tower->collumn, tower->row + 1,-1)));
                else if (isTileEmpty(enemy_list, *tower_list, tower->collumn + 1, tower->row) && doesTileExist(tower->collumn + 1, tower->row) && (tmp = addTower(tower_list, enemy_list, SOLIDER_TOWER, tower->collumn + 1, tower->row,-1)));
                else if (isTileEmpty(enemy_list, *tower_list, tower->collumn - 1, tower->row) && doesTileExist(tower->collumn- 1, tower->row ) && (tmp = addTower(tower_list, enemy_list, SOLIDER_TOWER, tower->collumn - 1, tower->row,-1)));
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
    //     /* Destroy tower when life points are bellow 0 */
    //     if (tower->live_points <= 0) {
    //         tmp = tower->next;
    //         destroyTower(tower, tower_list);
    //         tower = tmp;
    //         continue;
    //     }
    //     tower = tower->next;
    // }
    Enemy *e; Tower *t;
    while (currently_acting_tower && *currently_acting_tower && projectile_list && !(*projectile_list)) {
        e = enemy_list;
        while (e) {
            if (e->anim && e->anim->type != IDLE_ANIMATION) return;
            e = e->next;
        }
        t = *tower_list;
        while (t) {
            if (t->anim && t->anim->type != IDLE_ANIMATION) return;
            t = t->next;
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
bool damageTower(Tower *tower, int amount, Tower **tower_list, TextElement **text_element_list) {
    if (!tower || !amount || !tower_list) return false;
    /* Check that tower still exist */
    Tower *t = *tower_list;
    while (t && t != tower) t = t->next;
    if (!t) return false;
    /* Show damage number */
    if (text_element_list) addDamageNumber(text_element_list, amount, tower->collumn, tower->row);
    /* Damage tower */
    tower->life_points -= amount;
    setAnimHurt(tower->anim);
    /* Update life bar */
    updateLifeBarTextElement(&tower->life_bar, tower->life_points, tower->max_life_points);
    /* Kill tower if health reaches 0 or less */
    if (tower->life_points <= 0) destroyTower(tower, tower_list);
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
void updateProjectiles(Projectile **projectile_list, Enemy **enemy_list, Tower *tower_list, TextElement **text_element_list,int *score) {
    Projectile *projectile = *projectile_list; Projectile *tmp;
    Enemy *enemy;
    bool result;
    while (projectile) {
        /* On target reached */
        if (hasProjectileReachedTarget(projectile)) {
            /* Apply projectile effects */
            switch (projectile->origin->type) {
                case ARCHER_TOWER:
                    damageEnemy(projectile->target, 2, enemy_list, tower_list,text_element_list,score);
                    break;
                case WALL_TOWER:
                    break;
                case BARRACK_TOWER:
                    break;
                case SOLIDER_TOWER:
                    damageEnemy(projectile->target, 2, enemy_list,tower_list, text_element_list, score);
                    break;
                case CANON_TOWER:
                    damageEnemy(projectile->target, 9, enemy_list, tower_list, text_element_list, score);
                    break;
                case DESTROYER_TOWER:
                    damageEnemy(projectile->target, 10, enemy_list, tower_list, text_element_list, score);
                    /* Area damage */
                    for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) if (dx || dy)
                        if (doesTileExist(projectile->target->collumn + dx, projectile->target->row + dy) && getEnemyAndTowerAt(*enemy_list, NULL, projectile->target->collumn + dx, projectile->target->row + dy, &enemy, NULL))
                            damageEnemy(enemy, 4, enemy_list, tower_list, text_element_list, score);
                    break;
                case SORCERER_TOWER:
                    result = damageEnemy(projectile->target, 3, enemy_list, tower_list, text_element_list, score);
                    /* Enemy slowdown on hit */
                    if (result && projectile->target) projectile->target->speed = min(max(projectile->target->speed - 1, 1), projectile->target->speed);
                    break;
                case MAGE_TOWER:
                    result = damageEnemy(projectile->target, 3, enemy_list, tower_list, text_element_list, score);
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




/* Create a new game */
Game *createNewGame(char *level_name) {
    /* Initialize the new game object */
    Game *new_game = malloc(sizeof(Game));
    new_game->waves = NULL;
    new_game->nb_waves = 0;
    new_game->current_wave_nb = 0;
    new_game->tower_list = NULL;
    new_game->currently_acting_tower = NULL;
    new_game->enemy_list = NULL;
    new_game->currently_acting_enemy = NULL;
    new_game->projectile_list = NULL;
    new_game->text_element_list = NULL;
    new_game->funds = 0;
    new_game->score = 0;
    new_game->turn_nb = 0;
    new_game->game_phase = PRE_WAVE_PHASE;
    new_game->level_name = duplicateString(level_name);
    /* Load the waves */
    if (!strcmp(level_name, SURVIVAL_MODE)) {
        /* Survival mode pre-wave, no enemies, only income */
        new_game->waves = malloc(sizeof(Wave *));
        new_game->waves[0] = newWave(2025, NULL);
        new_game->current_wave_nb = new_game->nb_waves = -1;
    }
    else {
        /* Load waves based on level file */
        loadLevel(level_name, &new_game->waves, &new_game->nb_waves);
    }
    /* Load initial wave */
    if (new_game->nb_waves) loadNextWave(new_game);
    return new_game;
}

/* Load a game from a save file */
Game *loadGameFromSave(char *save_file){
    /* Openning text file */
    char *partial_path = concatString("../assets/saves/", save_file);
    char *full_path = concatString(partial_path, ".txt");
    FILE *file = fopen(full_path, "r");
    free(partial_path);
    /* Checking if file was open successfully */
    if (!file) {
        printf("Save file at \"%s\" not found, creating new save file for player %s\n", full_path, save_file);
        free(full_path);
        return NULL;
    }
    Game *new_game = NULL;
    /* Retrieve all informations from the file */
    char **values; int nb_values;
    while (readLine(file, &values, &nb_values)) {
        if (nb_values == 5) {
            /* Header line */
            if (!new_game) {
                new_game = createNewGame(values[0]);
                new_game->current_wave_nb = stringToInt(values[1]);
                new_game->funds = stringToInt(values[2]);
                new_game->score = stringToInt(values[3]);
                new_game->game_phase = (stringToInt(values[4]) ? PRE_WAVE_PHASE : WAITING_FOR_USER_PHASE);
                /* Remove any enemy loaded with the level, as they will instead be loaded from this save file and not from the level file */
                while (new_game->enemy_list) destroyEnemy(new_game->enemy_list, &new_game->enemy_list);
                continue;
            }
            /* Enemy or torwer to add */
            else {
                if (values[0][0] == 'E') addEnemy(&new_game->enemy_list, values[1][0], stringToInt(values[3]), stringToInt(values[2]), stringToInt(values[4]));
                else if (values[0][0] == 'T') addTower(&new_game->tower_list,new_game->enemy_list, values[1][0], stringToInt(values[3]), stringToInt(values[2]), stringToInt(values[4]));
            }
            /* Free memory */
            for (int i = nb_values; i > 0; i--) free(values[i-1]);
            free(values);
        }
        else if (nb_values) {
            printf("[ERROR]    Invalid syntax for level file \"%s\"\n", full_path);
            free(full_path);
            for (int i = 0; i < nb_values; i++) free(values[i]);
            free(values);
            fclose(file);
            return NULL;
        }
    }
    free(full_path);
    fclose(file);
    return new_game;
}

/* Load next wave, return true if successfull */
bool loadNextWave(Game *game) {
    if (game->current_wave_nb >= game->nb_waves && game->nb_waves >= 0) return false;
    int wave_nb = max(game->current_wave_nb, 0);
    game->current_wave_nb++;
    /* Destroy any remaining enemy and load new wave */
    while (game->enemy_list) destroyEnemy(game->enemy_list, &game->enemy_list);
    game->enemy_list = game->waves[wave_nb]->enemy_list;
    /* Give wave income */
    game->funds += game->waves[wave_nb]->income;
    /* Reset game phase */
    game->game_phase = PRE_WAVE_PHASE;
    return true;
}

/* Launch next wave */
void startNextWave(Game *game) {
    game->game_phase = TOWERS_ATTACKING_PHASE;
}

/* Update game */
void updateGame(Game *game, const char *nickname) {
    /* Update game phase */
    bool condition; Enemy *enemy; Tower *tower;
    switch (game->game_phase) {
        case WAITING_FOR_USER_PHASE:
            break;
        case PRE_WAVE_PHASE:
            saveGame(nickname, game);
            break;
        case ENEMIES_MOVING_PHASE:
            /* Change phase when all enemies have finished moving and gone back to their idle animation */
            condition = true; enemy = game->enemy_list;
            while (enemy && condition) {
                if (enemy->anim && enemy->anim->type != IDLE_ANIMATION) condition = false;
                enemy = enemy->next;
            }
            /* Change phase */
            if (condition) {
                game->game_phase = TOWERS_ATTACKING_PHASE;
                makeAllTowersAct(game->tower_list, &game->currently_acting_tower);
                game->currently_acting_tower = game->tower_list;
            }
            break;
        case TOWERS_ATTACKING_PHASE:
            /* Change phase when towers have attacked, no projectiles are left and all enemies have stoped gone back to their idle animation */
            condition = true; enemy = game->enemy_list;
            while (enemy && condition) {
                if (enemy->anim && enemy->anim->type != IDLE_ANIMATION) condition = false;
                
                enemy = enemy->next;
            }
            /* Change phase */
            if (!game->currently_acting_tower && !game->projectile_list && condition) {
                /* Save game (turn ending) */
                saveGame(nickname, game);
                game->game_phase = ENEMIES_ATTACKING_PHASE;
                makeAllEnemiesAct(game->enemy_list, &game->currently_acting_enemy);
            }
            break;
        case ENEMIES_ATTACKING_PHASE:
            /* Change phase when enemies have finished attacking and gone back to their idle animation, same for the towers */
            condition = true; enemy = game->enemy_list; tower = game->tower_list;
            while (enemy && condition) {
                if (enemy->anim && enemy->anim->type != IDLE_ANIMATION) condition = false;
                enemy = enemy->next;
            }
            while (tower && condition) {
                if (tower->anim && tower->anim->type != IDLE_ANIMATION) condition = false;
                tower = tower->next;
            }
            /* Change phase */
            if (!game->currently_acting_enemy && condition) {
                game->game_phase = ENEMIES_MOVING_PHASE;
                makeAllEnemiesMove(game->enemy_list, game->tower_list);
            }
            break;
        case VICTORY_PHASE:
            break;
        case GAME_OVER_PHASE:
            break;
        default:
            break;
    }
    /* Defeat condition */
    // TODO!
    /* On wave defeated */
    if (!game->enemy_list && game->game_phase != PRE_WAVE_PHASE) {
        /* If currently on survival mode */
        if (!strcmp(game->level_name, SURVIVAL_MODE)) beguinNewSurvivalWave(game);
        /* If defeated a wave */
        else if (game->current_wave_nb < game->nb_waves) loadNextWave(game);
        /* If defeated last wave (victory) */
        else game->game_phase = VICTORY_PHASE;
    }
    /* Update all game entities */
    updateProjectiles(&game->projectile_list, &game->enemy_list, game->tower_list, &game->text_element_list, &game->score);
    updateEnemies(&game->currently_acting_enemy, &game->enemy_list, &game->tower_list, &game->text_element_list);
    updateTowers(&game->currently_acting_tower, &game->tower_list, game->enemy_list, &game->projectile_list);
}

/* Destroy a game structure and free its allocated memory */
void destroyGame(Game *game) {
    /* Destroy all enemies */
    while (game->enemy_list) destroyEnemy(game->enemy_list, &game->enemy_list);
    /* Destroy all towers */
    while (game->tower_list) destroyTower(game->tower_list, &game->tower_list);
    /* Destroy all projectiles */
    while (game->projectile_list) destroyProjectile(game->projectile_list, &game->projectile_list);
    /* Destroy all text elements (damage numbers) */
    while (game->text_element_list) destroyTextElement(game->text_element_list, &game->text_element_list);
    /* Destroy all waves */
    destroyWaveList(game->waves, game->nb_waves);
    /* Destroy game object */
    if (game->level_name) free(game->level_name);
    free(game);
}

/* Launch a new random wave of enemy for survival mode */
void beguinNewSurvivalWave(Game *game) {
    if (!game) return;

    /* Delete the old wave */
    while (game->enemy_list) destroyEnemy(game->enemy_list, &game->enemy_list);
    destroyWaveList(game->waves, game->nb_waves);

    /* Build the new survival wave */
    /* Wave power determine how strong are the wave enemies and how numerous they are */
    int wave_power = 1000 + power(game->current_wave_nb, 2) * 250;
    /* Initialize new wave object */
    Wave *new_wave = newWave(0, NULL);
    /* Add enemies to the wave */
    char enemy_type; int collumn, row; int nb_enemy = 0;
    while (wave_power > 0) {
        nb_enemy++;
        /* Chose enemy type */
        /* Add a necromancer enemy */
        if (roll(1.0 - 20000.0/(20000.0 + wave_power))) {
            enemy_type = NECROMANCER_ENEMY;
            wave_power -= 400;
        }
        /* Add an orc enemy */
        else if (roll(1.0 - 10000.0/(10000.0 + wave_power))) {
            enemy_type = ORC_ENEMY;
            wave_power -= 300;
        }
        /* Add a goblin enemy */
        else if (roll(1.0 - 2500.0/(2500.0 + wave_power))) {
            enemy_type = GOBLIN_ENEMY;
            wave_power -= 150;
        }
        /* Add a gelly enemy */
        else if (roll(1.0 - 1000.0/(1000.0 + wave_power))) {
            enemy_type = GELLY_ENEMY;
            wave_power -= 100;
        }
        /* Add a slime enemy */
        else {
            enemy_type = SLIME_ENEMY;
            wave_power -= 50;
        }
        /* Chose enemy position */
        collumn = randrange(0, nb_enemy/2) + NB_COLLUMNS + 1;
        row = randrange(0, NB_ROWS) + 1;
        /* Check if tile is free for the enemy to spawn, otherwise try another position further right */
        while (!isTileEmpty(new_wave->enemy_list, NULL, collumn, row)) {
            collumn += randrange(0, 3) + 1;
            row = randrange(0, NB_ROWS) + 1;
        }
        /* Add the enemy to the wave */
        addEnemy(&new_wave->enemy_list, enemy_type, collumn, row, -1);
    }

    /* Launch the new survival wave */
    int wave_nb = game->current_wave_nb;
    game->waves = malloc(sizeof(Wave *));
    game->waves[0] = new_wave;
    game->current_wave_nb = 0;
    game->nb_waves = 1;
    loadNextWave(game);
    game->nb_waves = -1;
    game->current_wave_nb = wave_nb + 1;
    startNextWave(game);
}




/* Initialize new wave object */
Wave *newWave(int income, Enemy *enemy_list) {
    Wave *new_wave = malloc(sizeof(Wave));
    new_wave->income = income;
    new_wave->enemy_list = enemy_list;
    return new_wave;
}

/* Initialize new wave object */
void destroyWaveList(Wave **wave_list, int nb_wave) {
    if (!wave_list) return;
    for (int i = nb_wave; i > 0; i--) free(wave_list[i-1]);
    free(wave_list);
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
    char line_buffer[64];
    if (!fgets(line_buffer, 64, file)) return false;
    /* Split it into all of its individual values */
    *values = malloc(sizeof(char *)); *nb_values = 0; char *value, *line = line_buffer;
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
    *waves = malloc(sizeof(Wave *)); *nb_waves = 0; char **values; int nb_values;
    while (readLine(file, &values, &nb_values)) {
        switch (nb_values) {
            /* Empty line, ignore it */
            case 0:
                break;
            /* New wave (int income) */
            case 1:
                (*nb_waves)++;
                *waves = realloc(*waves, (*nb_waves) * sizeof(Wave *));
                (*waves)[*nb_waves - 1] = newWave(stringToInt(values[0]), NULL);
                break;
            /* Add enemy (int spawn_delay, int row, char type) */
            case 3:
                if (!(*nb_waves)) {
                    printf("[ERROR]    Invalid syntax for level file \"%s\"\n", full_path);
                    free(full_path);
                    fclose(file);
                    return false;
                }
                addEnemy(&(*waves)[*nb_waves - 1]->enemy_list, values[2][0], NB_COLLUMNS + stringToInt(values[0]), stringToInt(values[1]), -1);
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

bool saveGame(const char *save_name, Game *game){
    /* Save the game in a text file */
    char *partial_path = concatString("../assets/saves/", save_name);
    char *full_path = concatString(partial_path, ".txt");
    FILE *file = fopen(full_path, "w");
    free(partial_path);
    if (!file) {
        printf("[ERROR]    Level file at \"%s\" not found\n", full_path);
        free(full_path);
        return false;
    }
    /* Write the header with all keys infos that need to be saved and used when charging the save */
    fprintf(file,"%s %d %d %d %d\n", game->level_name, game->current_wave_nb, game->funds, game->score, game->game_phase == PRE_WAVE_PHASE);
    /* Add all the tower and the enemy file with all their characteristics */
    Tower *current_tower = game->tower_list;
    while(current_tower) {
        fprintf(file, "T %c %d %d %d\n", current_tower->type, current_tower->row, current_tower->collumn, current_tower->life_points);
        current_tower = current_tower->next;
    }
    Enemy *current_enemy = game->enemy_list;
    while(current_enemy){
        fprintf(file, "E %c %d %d %d\n", current_enemy->type, current_enemy->row, current_enemy->collumn, current_enemy->life_points);
        current_enemy = current_enemy->next;
    }
    free(full_path);
    fclose(file);
    return true;
}




/* Draw on screen enemies and towers, from top to bottom */
void drawEnemiesAndTowers(SDL_Renderer *rend, Enemy *enemy_list, Tower *tower_list, int game_phase) {
    Enemy **first_of_each_row = getFirstEnemyOfAllRows(enemy_list);
    Enemy *enemy; Tower *tower;
    SDL_Rect dest;
    int w, h;
    /* Draw from top to bottom */
    for (int row_nb = 1; row_nb <= NB_ROWS; row_nb++) {
        /* Draw enemies on the current row */
        enemy = first_of_each_row[row_nb-1];
        while (enemy) {
            if (enemy->collumn <= NB_COLLUMNS || game_phase == PRE_WAVE_PHASE) {
                dest.x = (enemy->collumn - 1) * TILE_WIDTH; dest.y = (enemy->row - 1) * TILE_HEIGHT; dest.w = SPRITE_SIZE; dest.h = SPRITE_SIZE;
                drawImgDynamic(rend, enemy->sprite, dest.x, dest.y, dest.w, dest.h, enemy->anim);
                /* Life bar */
                if (enemy->life_bar && (!enemy->anim || enemy->anim->type != SPAWN_ANIMATION) && game_phase != PRE_WAVE_PHASE) {
                    enemy->life_bar->rect.x = dest.x;
                    enemy->life_bar->rect.y = dest.y + SPRITE_SIZE - enemy->life_bar->sprite->h;
                    /* Apply enemy anim to lifebar (except for size change) to match it's current visual position */
                    if (enemy->anim && enemy->anim->type != IDLE_ANIMATION) {
                        w = enemy->life_bar->rect.w; h = enemy->life_bar->rect.h;
                        applyAnim(enemy->anim, &enemy->life_bar->rect);
                        enemy->life_bar->rect.w = w; enemy->life_bar->rect.h = h;
                    }
                    drawTextElements(rend, &enemy->life_bar);
                }
            }
            enemy = enemy->next_on_row;
        }
        /* Draw towers on the current row */
        tower = tower_list;
        while (tower) {
            if (tower->row == row_nb) {
                dest.x = (tower->collumn - 1) * TILE_WIDTH; dest.y = (tower->row - 1) * TILE_HEIGHT; dest.w = SPRITE_SIZE; dest.h = SPRITE_SIZE;
                drawImgDynamic(rend, tower->sprite, dest.x, dest.y, dest.w, dest.h, tower->anim);
                drawImgDynamic(rend, tower->sprite, dest.x, dest.y, SPRITE_SIZE, SPRITE_SIZE, tower->anim);
                /* Life bar */
                if (tower->life_bar && (!tower->anim || tower->anim->type != SPAWN_ANIMATION)) {
                    tower->life_bar->rect.x = dest.x;
                    tower->life_bar->rect.y = dest.y + SPRITE_SIZE - tower->life_bar->sprite->h;
                    /* Apply tower anim to lifebar (except for size change) to match it's current visual position */
                    if (tower->anim && tower->anim->type != IDLE_ANIMATION) {
                        w = tower->life_bar->rect.w; h = tower->life_bar->rect.h;
                        applyAnim(tower->anim, &tower->life_bar->rect);
                        tower->life_bar->rect.w = w; tower->life_bar->rect.h = h;
                    }
                    drawTextElements(rend, &tower->life_bar);
                }
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
    if (img) SDL_FreeSurface(img);
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


    /* Part in terminal */
    /* Get username */
    char nickname[MAX_LENGTH_NICKNAME];
    printf("Enter nickname: ");
    fgets(nickname, MAX_LENGTH_NICKNAME, stdin);
    char *c;
    if ((c = strchr(nickname, '\r'))) *c = '\0';
    if ((c = strchr(nickname, '\n'))) *c = '\0';

    /* Getting all availible levels (up to 64) */
    char *availible_levels[64] = {NULL}; int nb_availible_levels = 0;
    DIR *d; struct dirent *dir;
    d = opendir("../assets/lvl/");
    if (d) {
        printf("Availible levels:\n");
        /* List level names */
        while ((dir = readdir(d)) != NULL) {
            /* Ignores "." et ".." repertories, as we do not need them */
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) continue;
            availible_levels[nb_availible_levels] = duplicateString(dir->d_name);
            /* Remove the .txt part of the name */
            if ((c = strchr(availible_levels[nb_availible_levels], '.'))) *c = '\0';
            printf("  %2d) %s\n", nb_availible_levels+1, availible_levels[nb_availible_levels]);
            nb_availible_levels++;
        }
        closedir(d);
    }
    else {
        printf("[ERROR]    Unable to get availible levels name\n");
    }

    /* Getting additionnal levels (special levels and save file) */
    printf("Additionnal levels:\n");
    printf("  %2d) Survival mode\n", nb_availible_levels+1);
    char *save_path = malloc(64 * sizeof(char)); sprintf(save_path, "../assets/saves/%s.txt", nickname);
    FILE *save_file;
    bool save_availible = false;
    if ((save_file = fopen(save_path, "r"))) {
        fclose(save_file);
        printf("  %2d) Load save\n", nb_availible_levels+2);
        save_availible = true;
    }
    free(save_path);

    /* Allow user to chose which level they want to play */
    Game *game = NULL;
    while (!game) {
        char buffer[64];
        printf("Please chose a level by entering its numerical ID or its name\n");
        printf("Enter \"quit\" / \"exit\" / \"\" to exit program\n");
        printf(">>> ");
        fgets(buffer, 64, stdin);
        /* Remove line break */
        if ((c = strchr(buffer, '\r'))) *c = '\0';
        if ((c = strchr(buffer, '\n'))) *c = '\0';
        /* Exit program */
        if (!strcmp(buffer, "quit") || !strcmp(buffer, "exit") || !strcmp(buffer, "")) return 0;
        /* Load a level by entering its name */
        for (int i = 0; i < nb_availible_levels; i++) if (!strcmp(buffer, availible_levels[i])) {
            game = createNewGame(buffer);
            break;
        }
        /* Load a level by entering its numerical ID */
        if (!game) if (0 < stringToInt(buffer) && stringToInt(buffer) <= nb_availible_levels)
            game = createNewGame(availible_levels[stringToInt(buffer)]);
        /* Load survival mode */
        if (!game) if (!strcmp(buffer, "Survival mode") || stringToInt(buffer) == nb_availible_levels+1)
            game = createNewGame(SURVIVAL_MODE);
        /* Load savefile */
        if (!game) if (save_availible && (!strcmp(buffer, "Load save") || stringToInt(buffer) == nb_availible_levels+2))
            game = loadGameFromSave(nickname);
        /* Error */
        if (!game) printf("Could not find level \"%s\", please try again\n", buffer);
    }
    printf("Level loaded successfully, have fun!\n");


    /* Graphic part */
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
    /* Antialiasing, "0" -> "nearest" ; "1" -> "linear" ; "2" -> "best" */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, ANTI_ALIASING);

    /* Set focus to window */
    SDL_RaiseWindow(wind);

    // [TEMPORARY]
    TextElement *win_text_surface = addTextElement(NULL, "YOU DEFEATED THE SWARM !", 4.0, (SDL_Color) {255, 255, 255, 255}, (SDL_Color) {0, 0, 0, 255}, (SDL_Rect) {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT}, true, false, NULL);
    TextElement *save_text_surface = addTextElement(NULL, "SAVED SUCCESSFULY !", 4.0, (SDL_Color) {255, 255, 255, 255}, (SDL_Color) {0, 0, 0, 255}, (SDL_Rect) {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT}, true, false, NULL);
    /* Load images */
    SDL_Surface *towers[] = {loadImg("towers/Archer_tower"), loadImg("towers/Empty_tower"), loadImg("towers/canon"), loadImg("towers/sorcerer")};
    SDL_Surface *towers_upgrades[] = {loadImg("towers/sorcerer_evolved"), loadImg("towers/canon_evolved"), loadImg("towers/barracks")};
    SDL_Surface *grass_tiles[] = {loadImg("others/grass_tile_a"), loadImg("others/grass_tile_b"), loadImg("others/grass_tile_c"), loadImg("others/grass_tile_d"), loadImg("others/grass_tile_alt_a"), loadImg("others/grass_tile_alt_b"), loadImg("others/grass_tile_alt_c"), loadImg("others/grass_tile_alt_d")};
    SDL_Surface *highlighted_tile = loadImg("others/tile_choosed"); SDL_Surface *delete_tower = loadImg("others/delete"); SDL_Surface *quit_menu = loadImg("others/quit");
    SDL_Surface *background = loadImg("others/grass_background");
    SDL_Surface *castle = loadImg("others/castle");

    /* Load UI text */
    TextElement *ui_text_element = NULL;
    /* (1 : top left) Funds */
    addTextElement(&ui_text_element, "", 0.5, (SDL_Color) {0, 0, 0, 0}, (SDL_Color) {0, 0, 0, 0}, (SDL_Rect) {0, 0, WINDOW_WIDTH/3, 48}, true, false, NULL);
    /* (2 : top middle) Wave number */
    addTextElement(&ui_text_element, "", 0.5, (SDL_Color) {0, 0, 0, 0}, (SDL_Color) {0, 0, 0, 0}, (SDL_Rect) {WINDOW_WIDTH/3, 0, WINDOW_WIDTH/3, 48}, true, false, NULL);
    /* (3 : top right) Score */
    addTextElement(&ui_text_element, "", 0.5, (SDL_Color) {0, 0, 0, 0}, (SDL_Color) {0, 0, 0, 0}, (SDL_Rect) {WINDOW_WIDTH*2/3, 0, WINDOW_WIDTH/3, 48}, true, false, NULL);

    /* Main loop */
    Tower *towerOnTile;
    int var; Enemy *enemy;
    int score = -1, wave_nb = -1, nb_waves = -1, funds = -1; char text_value[256];
    int cam_x_speed = 0, cam_y_speed = 0, cam_speed_mult = 0;
    int *selected_tile_pos = malloc(2 * sizeof(int)); selected_tile_pos[0] = 0; selected_tile_pos[1] = 0;
    bool menu_hidden = true;
    bool mouse_dragging = false;
    bool fullscreen = FULLSCREEN;
    SDL_Event event;
    bool running = true;
    while (running) {
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
                            if (game->game_phase == PRE_WAVE_PHASE || game->game_phase == WAITING_FOR_USER_PHASE) startNextWave(game);
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
                                /* Check if tile is on map (last collumn cannot be build on) */
                                if (1 <= selected_tile_pos[0] && selected_tile_pos[0] <= NB_COLLUMNS-1 && 1 <= selected_tile_pos[1] && selected_tile_pos[1] <= NB_ROWS) {
                                    menu_hidden = (game->game_phase != PRE_WAVE_PHASE);
                                }
                                else {
                                    selected_tile_pos[0] = 0; selected_tile_pos[1] = 0;
                                    menu_hidden = true;
                                }
                            }
                            /* Construction menu selected */
                            /* Tile with tower selected */
                            else if (getEnemyAndTowerAt(NULL, game->tower_list, selected_tile_pos[0], selected_tile_pos[1], NULL, &towerOnTile)){
                                if (WINDOW_WIDTH - WINDOW_HEIGHT/4 <= event.button.x && event.button.x <= WINDOW_WIDTH && 0 <= event.button.y && event.button.y <= WINDOW_HEIGHT/4){
                                    /* Clicked on the X to quit the menu */
                                    menu_hidden = true;
                                }
                                else if (WINDOW_HEIGHT/4 <= event.button.x && event.button.x <= WINDOW_HEIGHT*2/4 && 0 <= event.button.y && event.button.y <= WINDOW_HEIGHT/4){
                                    sellTower(towerOnTile, &game->tower_list, &game->funds);
                                    menu_hidden = true;
                                }
                                else if (0 <= event.button.x && event.button.x <= WINDOW_HEIGHT/4 && 0 <= event.button.y && event.button.y <= WINDOW_HEIGHT/4){
                                    /* Clicked on the turret upgrade */
                                    upgradeTower(&game->tower_list, game->enemy_list, towerOnTile->type, selected_tile_pos[0], selected_tile_pos[1], &game->funds);
                                    menu_hidden = true;
                                }
                            }
                            /* Tile with no tower selected */
                            else {
                                /* Archer tower */
                                if (WINDOW_HEIGHT*0/4 <= event.button.x && event.button.x <= WINDOW_HEIGHT*1/4 && 0 <= event.button.y && event.button.y <= WINDOW_HEIGHT/4) {
                                    if (buyTower(&game->tower_list, game->enemy_list, ARCHER_TOWER, selected_tile_pos[0], selected_tile_pos[1], &game->funds)) menu_hidden = true;
                                }
                                /* Wall tower */
                                else if (WINDOW_HEIGHT*1/4 <= event.button.x && event.button.x <= WINDOW_HEIGHT*2/4 && 0 <= event.button.y && event.button.y <= WINDOW_HEIGHT/4) {
                                    if (buyTower(&game->tower_list, game->enemy_list, WALL_TOWER, selected_tile_pos[0], selected_tile_pos[1], &game->funds)) menu_hidden = true;
                                }
                                /* Canon tower */
                                else if (WINDOW_HEIGHT*2/4 <= event.button.x && event.button.x <= WINDOW_HEIGHT*3/4 && 0 <= event.button.y && event.button.y <= WINDOW_HEIGHT/4) {
                                    if (buyTower(&game->tower_list, game->enemy_list, CANON_TOWER, selected_tile_pos[0], selected_tile_pos[1], &game->funds)) menu_hidden = true;
                                }
                                /* Sorcerer tower */
                                else if (WINDOW_HEIGHT*3/4 <= event.button.x && event.button.x <= WINDOW_HEIGHT*4/4 && 0 <= event.button.y && event.button.y <= WINDOW_HEIGHT/4) {
                                    if (buyTower(&game->tower_list, game->enemy_list, SORCERER_TOWER, selected_tile_pos[0], selected_tile_pos[1], &game->funds)) menu_hidden = true;
                                }
                                else if (WINDOW_WIDTH-WINDOW_HEIGHT/4 <= event.button.x && event.button.x <= WINDOW_WIDTH && 0 <= event.button.y && event.button.y <= WINDOW_HEIGHT/4){
                                    /* Clicked on the X to quit the menu */
                                    menu_hidden = true;
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

        /* Update game */
        updateGame(game, nickname);

        /* Automaticaly close building menu durring waves (canot build in the middle of a wave) */
        if (game->game_phase != PRE_WAVE_PHASE) menu_hidden = true;

        /* Update UI if needed */
        /* Update score display */
        if (score != game->score) {
            score = game->score;
            delImg(ui_text_element->sprite);
            sprintf(text_value, "Score: %d points", game->score);
            ui_text_element->sprite = textSurface(text_value, (SDL_Color) {255, 255, 255, 255}, (SDL_Color) {127, 127, 127, 255});
        }
        /* Update wave display */
        if (wave_nb != game->current_wave_nb || nb_waves != game->nb_waves) {
            wave_nb = game->current_wave_nb; nb_waves = game->nb_waves;
            delImg(ui_text_element->next->sprite);
            if (game->nb_waves >= 0) sprintf(text_value, "Wave: %d/%d", game->current_wave_nb, game->nb_waves);
            else sprintf(text_value, "Wave: %d", game->current_wave_nb);
            ui_text_element->next->sprite = textSurface(text_value, (SDL_Color) {255, 127, 0, 255}, (SDL_Color) {127, 63, 0, 255});
        }
        /* Update funds display */
        if (funds != game->funds) {
            funds = game->funds;
            delImg(ui_text_element->next->next->sprite);
            sprintf(text_value, "Funds: %d*", game->funds);
            ui_text_element->next->next->sprite = textSurface(text_value, (SDL_Color) {255, 255, 127, 255}, (SDL_Color) {127, 127, 63, 255});
        }

        /* Clear screen */
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);
        
        /* Move camera */
        CAM_POS_X += BASE_CAM_SPEED * cam_x_speed * power(CAM_SPEED_MULT, cam_speed_mult) * power(1.4142/2, cam_x_speed && cam_y_speed) / CAM_SCALE;
        CAM_POS_Y += BASE_CAM_SPEED * cam_y_speed * power(CAM_SPEED_MULT, cam_speed_mult) * power(1.4142/2, cam_x_speed && cam_y_speed) / CAM_SCALE;

        /* Draw elements */
        /* Draw background */
        drawImgStatic(rend, background, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL);
        /* Draw castle (element to defend from enemies) */
        for (int y = 0; y < NB_ROWS; y++) for (int x = -NB_ROWS * TILE_HEIGHT/TILE_WIDTH + 1; x < 0; x++)
            drawImgDynamic(rend, grass_tiles[4 + positive_mod(x, 2) + positive_mod(y, 2) * 2], TILE_WIDTH * x, TILE_HEIGHT * y, SPRITE_SIZE, SPRITE_SIZE, NULL);
        drawImgDynamic(rend, castle, -(NB_ROWS-2)*TILE_HEIGHT, TILE_HEIGHT, (NB_ROWS-2)*TILE_HEIGHT, (NB_ROWS-2)*TILE_HEIGHT, NULL);
        /* Draw grass tiles */
        for (int y = 0; y < NB_ROWS; y++) for (int x = 0; x < NB_COLLUMNS; x++) 
            drawImgDynamic(rend, grass_tiles[x%2 + (y%2) * 2], TILE_WIDTH * x, TILE_HEIGHT * y, SPRITE_SIZE, SPRITE_SIZE, NULL);
        /* Draw additional grass tiles for enemy preview, only in pre-wave game phase */
        if (game->game_phase == PRE_WAVE_PHASE) {
            var = 0; enemy = game->enemy_list;
            while (enemy) {
                var = max(var, enemy->collumn - NB_COLLUMNS);
                enemy = enemy->next;
            }
            for (int y = 0; y < NB_ROWS; y++) for (int x = NB_COLLUMNS; x < NB_COLLUMNS + var; x++)
                drawImgDynamic(rend, grass_tiles[4 + x%2 + (y%2) * 2], TILE_WIDTH * x, TILE_HEIGHT * y, SPRITE_SIZE, SPRITE_SIZE, NULL);
        }
        /* Draw selection cursor */
        if (!menu_hidden) drawImgDynamic(rend, highlighted_tile, (selected_tile_pos[0]-1)*TILE_WIDTH, (selected_tile_pos[1]-1)*TILE_HEIGHT, SPRITE_SIZE, SPRITE_SIZE, NULL);
        /* Draw entities */
        drawEnemiesAndTowers(rend, game->enemy_list, game->tower_list, game->game_phase);
        drawProjectiles(rend, game->projectile_list);
        /* Draw damage numbers */
        drawTextElements(rend, &game->text_element_list);

        /* Draw the Menu if necessary */
        if (!menu_hidden) {
            drawFilledRect(rend, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT/4, 128, 128, 128, 255);
            drawRect(rend, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT/4, 255, 255, 255, 255);
            if (!getEnemyAndTowerAt(NULL, game->tower_list, selected_tile_pos[0], selected_tile_pos[1], NULL, &towerOnTile))
                for (unsigned long long i = 0; i < sizeof(towers)/sizeof(towers[0]); i++)
                    drawImgStatic(rend, towers[i], i*WINDOW_HEIGHT/4, 0, WINDOW_HEIGHT/4, WINDOW_HEIGHT/4, NULL);
            else{
                switch (towerOnTile->type){
                    case SORCERER_TOWER:
                        drawImgStatic(rend, towers_upgrades[0], 0, 0, WINDOW_HEIGHT/4, WINDOW_HEIGHT/4, NULL);
                        break;
                    case CANON_TOWER:
                        drawImgStatic(rend, towers_upgrades[1], 0, 0, WINDOW_HEIGHT/4, WINDOW_HEIGHT/4, NULL);
                        break;
                    case WALL_TOWER:
                        drawImgStatic(rend, towers_upgrades[2], 0, 0, WINDOW_HEIGHT/4, WINDOW_HEIGHT/4, NULL);
                        break;
                    default:
                        break;
                }
                drawImgStatic(rend, delete_tower, WINDOW_HEIGHT/4, 0, WINDOW_HEIGHT/4, WINDOW_HEIGHT/4, NULL);
            }
            drawImgStatic(rend, quit_menu, WINDOW_WIDTH - WINDOW_HEIGHT/4, 0, WINDOW_HEIGHT/4, WINDOW_HEIGHT/4, NULL);
        }

        /* Draw text UI (lower display if menu is openned) */
        ui_text_element->rect.y = ui_text_element->next->rect.y = ui_text_element->next->next->rect.y = !menu_hidden * WINDOW_HEIGHT/4;
        drawTextElements(rend, &ui_text_element);

        /* Draw to window and loop */
        SDL_RenderPresent(rend);
        SDL_Delay(max(1000/FPS - (SDL_GetTicks64()-CURRENT_TICK), 0));
        CURRENT_TICK = SDL_GetTicks64();
    }

    /* Free allocated memory */
    delImg(delete_tower); delImg(quit_menu); delImg(background); delImg(castle);
    for (int i = 4; i > 0; i--) delImg(towers[i-1]);
    for (int i = 8; i > 0; i--) delImg(grass_tiles[i-1]);
    for (int i = 3; i > 0; i--) delImg(towers_upgrades[i-1]);
    free(selected_tile_pos);
    while (ui_text_element) destroyTextElement(ui_text_element, &ui_text_element);
    destroyGame(game);
    /* Release resources */
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(wind);
    SDL_Quit();
    printf("Goodbye %s!\n", nickname);
    return 0;
}
