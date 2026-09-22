#include "../include/menu/fitness.h"
#include "../include/graphics.h"
#include "../include/framebuffer.h"
#include "../include/text.h"

void menu_fitness_init(void){
    fb_clear(0x0000);
    draw_text_box(20, 100, 200, 60, "Fitness", 0xFFFF, 2);   
    return;
}