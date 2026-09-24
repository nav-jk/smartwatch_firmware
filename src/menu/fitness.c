#include "../include/menu/fitness.h"
#include "../include/graphics.h"
#include "../include/framebuffer.h"
#include "../include/text.h"

int fetch_steps(void){
    return 6420;
}

int calorie_burnt(char weight){
    return (int)fetch_steps()*0.04*(weight/100.0f);
}

short fetch_hr(){
    return 72;
}

void menu_fitness_init(void)
{
    fb_clear(0x0000);

    draw_text_box(90, 15, 100, 25, "FITNESS", 0xFFFF, 2);

    circle motivation_ring = {
        .cen = {120, 105},
        .radius = 64,
        .color = 0xF800
    };

    draw_ring(&motivation_ring, 10);

    circle activity_ring = {
        .cen = {120, 105},
        .radius = 48,
        .color = 0x07E0
    };

    draw_ring(&activity_ring, 8);

    circle progress_ring = {
        .cen = {120, 105},
        .radius = 36,
        .color = 0x07FF
    };

    draw_ring(&progress_ring, 6);

    draw_text_box(105, 87, 90, 25, "6420", 0xFFFF, 2);
    draw_text_box(110, 115, 80, 20, "STEPS", 0x07FF, 1);

    draw_line(35, 190, 205, 190, 0x7BEF);

    draw_text_box(40, 165, 95, 25, "384", 0xFFE0, 2);
    draw_text_box(45, 178, 70, 18, "CAL", 0xFFFF, 1);

    draw_text_box(165, 165, 95, 25, "72 BPM", 0xF800, 2);
    draw_text_box(175, 178, 70, 18, "HEART", 0xFFFF, 1);
}