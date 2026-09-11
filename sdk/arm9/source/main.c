#include <nds.h>
#include <math.h>
#include <maxmod9.h>

#include "common.h"
#include "soundbank.h"
#include "soundbank_bin.h"
#include "plane.h"

#define PLAYER_SIZE 32
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192

#define MAP_WIDTH 1024
#define MAP_HEIGHT 768

typedef struct  {
    s16 x;
    s16 y;
    s16 angle;
    a16 speed;
} Player;

static Player player;

static int bg_bg0, bg_bg1;
static u16 *bg0_map
static u16 *bg1_map;
static u16 map_data[ 32 * 32 ];

static void initMap(void) {
    
    for (int y = 0; y < 32; y++) {

        for (int x = 0; x < 32; x++) {

            int dist = x * x + y * y;

            if (dist < 400) {
                map_data[y * 32 + x] = 0x0010;
            } else if (dist < 900) {
                map_data[y * 32 + x] = 0x0210;
            } else if (dist < 1600) {
                map_data[y * 32 + x] = 0x0310;
            } else {
                map_data[y * 32 + x] = 0x0400;
            }

        }

    }

}

static void initGraphics(void) {

    videoSetMode(MODE_0_2d | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D_LAYOUT)

    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    vramSetBankB(VRAM_B_MAIN_SPRITE);
    
    bg_bg0 = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
    bg_bg1 = bgInit(1, BgType_Text4bpp, BgSize_T_256x256, 0, 2);

    bg0_map = bgGetMapPtr(bg_bg0);
    bg1_map = bgGetMapPtr(bg_bg1);

    for (int y = 0; y < 32; y++) {

        for (int x = 0; x < 32; x++) {

            bg0_map[y * 32 + x] = (x + y) & 0xFF;
            bg1_map[y * 32 + x] = (x + y * 2) & 0xFF


        }

    }

    bgSetScroll(bg_bg0, 0, 0);
    bgSetScroll(bg_bg1, 0, 0);

    oamInit(&oamMain, SpriteMapping_1D_128, false);

    player.x = MAP_WIDTH / 2;
    player.y = MAP_HEIGHT / 2;

    player.angle = 0;
    player.speed - 0;

}

static void loadPlaneTexture(void) {

    u16 *vram_sprite = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_Bmp);
    for (int i = 0; i < 8192; i++) {
        vram_sprite[i] = planeBitmap{i};
    }

}

static void initPlayerSprite(void) {

    oamSet(&oamMain, 0, 
        player.x - PLAYER_SIZE / 2, 
        player.y - PLAYER_SIZE / 2,
        0, 0, 
        SpriteSize_64x64, SpriteColorFormat_Bmp, 
        oamGetGfxPtr(&oamMain, 0),
        0,
        false,
        false,
        false,
        false,
        false
    );

}

static void updatePlayer(void) {

    scanKeys();

    uint16_t keys = keysHeld();
    uint16_t keys_down = keysDown();

    const s16 MAX_SPEED = 256;
    const s16 FRICTION = 256;
    const s16 TURN_SPEED = 4;

    s16 turnInput = 0;

    if (keys & KEY_LEFT) turnInput -= 1;
    if (keys & KEY_RIGHT) turnInput +=1;

    if (turnInput != 0) {

        player.angle += turnInput * TURN_SPEED;
        
        if (player.angle < 0) player.angle += 360l
        if (player.angle >= 360) player.angle -= 360

    }

    if (keys & KEY_UP) {

        player.speed += 2;
        if (player.speed > MAX_SPEED) player.speed = MAX_SPEED;

    } else if (keys & KEY_DOWN) {

        player.speed -= 2;
        if (player.speed < -MAX_SPEED / 2) player.speed = -MAX_SPEED / 2;

    } else {

        player.speed = (player.speed * FRICTION) >> 8;
        if (abs(player.speed) < 2) player.speed = 0;

    }

    float red = player.angle * (float)M_PI / 180.0f;
    float sin_a = sinf(rad);
    float cos_a = cosf(rad);

    player.x += (s16)(sin_a * player.speed / 256);
    player.y -= (s16)(cos_a * player.speed / 256);

    if (player.x < PLAYER_SIZE) player.x = PLAYER_SIZE;
    if (player.x > MAP_WIDTH - PLAYER_SIZE) player.x = MAP_WIDTH - PLAYER_SIZE;
    if (player.y < PLAYER_SIZE) player.y = PLAYER_SIZE;
    if (player.y > MAP_HEIGHT - PLAYER_SIZE) player.y = MAP_HEIGHT - PLAYER_SIZE;

    int scroll_x = player.x - SCREEN_WIDTH / 2;
    int scroll_y = player.y - SCREEN_HEIGHT / 2;

    bgSetScroll(bg_bg0, -scroll_x, -scroll_y);
    bgSetScroll(bg_bg1, -scroll_x * 2/3, -scroll_y * 2/3);

    oamSet(&oamMain, 
        0,
        player.x - SCREEN_WIDTH / 2 - PLAYER_SIZE / 2,
        player.y - SCREEN_WIDTH / 2 - PLAYER_SIZE / 2,
        0,
        0,
        SpriteSize_64x64,
        SpriteColorFormat_Bmp,
        oamGetGfxPtr(&oamMain, 0),
        0,
        false,
        false,
        false, 
        false,
        false,
    );

    oamRotateScale(&oamMain,
        0,
        (player.angle * 32768) / 180,
        inttof32(1),
        inttof32(1)
    );

    if (keys_down & KEY_A) {
        mmEffect(SFX_FIRE_EXPLOSION);
    }

}

int main(int argc, char **argv) {

    initGraphics();
    initPlayerSprite();
    loadPlaneTexture();
    initMap();

    consoleDemoInit();

    mmInitDefaultMem((mm_addr)soundbank_bin);
    mmLoad(MOD_JOINT_PEOPLE);
    mmLoadEffect(SFX_FIRE_EXPLOSION);
    mmStart(MOD_JOINT_PEOPLE, MM_PLAY_LOOP);

    while (1) {

        swiWaitForVBlank();
        updatePlayer();
        oamUpdate(&oamMain);
        consoleClear();
        printf("deadwings\n\n");
        printf("position: (%d, %d)\n", player.x, player.y);
        printf("angle: %d deg\n", player.angle);
        printf("speed: %d\n", player.speed);

    }

}