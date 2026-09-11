#include <nds.h>

#define W 256
#define H 192

#define MAX_SPEED 8
#define MIN_SPEED 1

typedef struct
{
    int x;
    int pitch;
    int roll;
    int speed;
    int altitude;
    int distance;
} Plane;

static Plane plane;

static void reset_game(void)
{
    plane.x = 128;
    plane.pitch = 0;
    plane.roll = 0;
    plane.speed = 3;
    plane.altitude = 100;
    plane.distance = 0;
}

static void put_pixel(u16 *fb, int x, int y, u16 color)
{
    if (x >= 0 && x < W && y >= 0 && y < H)
        fb[y * W + x] = color;
}

static void fill_screen(u16 *fb, u16 color)
{
    for (int i = 0; i < W * H; i++)
        fb[i] = color;
}

static void draw_rect(
    u16 *fb,
    int x,
    int y,
    int w,
    int h,
    u16 color
)
{
    for (int yy = y; yy < y + h; yy++)
    {
        for (int xx = x; xx < x + w; xx++)
            put_pixel(fb, xx, yy, color);
    }
}

static void draw_line(
    u16 *fb,
    int x0,
    int y0,
    int x1,
    int y1,
    u16 color
)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;

    int steps = adx > ady ? adx : ady;

    if (steps == 0)
    {
        put_pixel(fb, x0, y0, color);
        return;
    }

    for (int i = 0; i <= steps; i++)
    {
        int x = x0 + dx * i / steps;
        int y = y0 + dy * i / steps;

        put_pixel(fb, x, y, color);
    }
}

static void update_game(u16 keys, u16 down)
{
    if (down & KEY_SELECT)
        reset_game();

    if (keys & KEY_LEFT)
        plane.x -= 3;

    if (keys & KEY_RIGHT)
        plane.x += 3;

    if (keys & KEY_UP)
    {
        plane.pitch += 2;
        plane.altitude += 2;
    }

    if (keys & KEY_DOWN)
    {
        plane.pitch -= 2;
        plane.altitude -= 2;
    }

    if (keys & KEY_L)
        plane.roll -= 3;

    if (keys & KEY_R)
        plane.roll += 3;

    if (keys & KEY_A)
        plane.speed++;

    if (keys & KEY_B)
        plane.speed--;

    if (plane.speed > MAX_SPEED)
        plane.speed = MAX_SPEED;

    if (plane.speed < MIN_SPEED)
        plane.speed = MIN_SPEED;

    if (plane.x < 20)
        plane.x = 20;

    if (plane.x > 236)
        plane.x = 236;

    if (plane.pitch > 40)
        plane.pitch = 40;

    if (plane.pitch < -40)
        plane.pitch = -40;

    if (plane.altitude < 10)
        plane.altitude = 10;

    plane.distance += plane.speed;
}

static void draw_fpv(u16 *fb)
{
    int horizon = 80 - plane.pitch;

    if (horizon < 20)
        horizon = 20;

    if (horizon > 160)
        horizon = 160;

    u16 sky = RGB15(8, 12, 31);
    u16 ground = RGB15(4, 12, 4);
    u16 white = RGB15(31, 31, 31);
    u16 grid = RGB15(8, 20, 8);
    u16 red = RGB15(31, 0, 0);

    fill_screen(fb, sky);

    draw_rect(
        fb,
        0,
        horizon,
        256,
        192 - horizon,
        ground
    );

    draw_line(
        fb,
        0,
        horizon,
        255,
        horizon,
        white
    );

    for (int i = 1; i <= 12; i++)
    {
        int y = horizon + i * i;

        if (y >= 192)
            break;

        draw_line(
            fb,
            0,
            y,
            255,
            y,
            grid
        );
    }

    for (int i = -10; i <= 10; i++)
    {
        int horizon_x = 128 + i * 5;
        int bottom_x = 128 + i * 35;

        draw_line(
            fb,
            horizon_x,
            horizon,
            bottom_x,
            191,
            grid
        );
    }

    int cx = plane.x;
    int cy = 96;

    draw_line(
        fb,
        cx - 20,
        cy,
        cx - 6,
        cy,
        red
    );

    draw_line(
        fb,
        cx + 6,
        cy,
        cx + 20,
        cy,
        red
    );

    draw_line(
        fb,
        cx,
        cy - 20,
        cx,
        cy - 6,
        red
    );

    draw_line(
        fb,
        cx,
        cy + 6,
        cx,
        cy + 20,
        red
    );

    draw_rect(
        fb,
        cx - 2,
        cy - 2,
        5,
        5,
        red
    );
}

int main(void)
{
    videoSetMode(MODE_FB0);

    vramSetBankA(VRAM_A_LCD);

    u16 *framebuffer = (u16 *)VRAM_A;

    reset_game();

    while (1)
    {
        swiWaitForVBlank();
        scanKeys();

        u16 keys = keysHeld();
        u16 down = keysDown();

        if (keys & KEY_START)
            break;

        update_game(keys, down);

        draw_fpv(framebuffer);
    }

    return 0;
}