#include "../include/menu/phone.h"
#include "../include/graphics.h"
#include "../include/framebuffer.h"
#include "../include/text.h"

void menu_phone_init(void){
    fb_clear(0x0000);
    for (int y = 0; y < PHONE_IMAGE_H; y++) {
    for (int x = 0; x < PHONE_IMAGE_W; x++) {
        fb_set(
            10 + x,
            5 + y,
            phone_image_data[y * PHONE_IMAGE_W + x]
        );
    }
}
    draw_text_box(40, 180, 200, 60, "Phone Not Connected", 0xFFFF, 2);   
    return;
}