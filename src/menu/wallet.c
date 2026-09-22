#include "../include/menu/wallet.h"

#include "../include/graphics.h"
#include "../include/framebuffer.h"
#include "../include/text.h"

void menu_wallet_init(void)
{
    fb_clear(0x0000);

    draw_text_box(20, 100, 200, 60, "Wallet", 0xFFFF, 2);

    return;
}