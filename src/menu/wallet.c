#include "../include/menu/wallet.h"
#include "../include/graphics.h"
#include "../include/framebuffer.h"
#include "../include/text.h"

void menu_wallet_init(void)
{
    fb_clear(0x0000);

    // Title
    draw_text_box(75, 8, 90, 25, "WALLET", 0xFFFF, 2);

    // Back card
    draw_rounded_rect(25, 48, 190, 105, 14, 0x39E7);

    // Back card stripe
    draw_rect(25, 82, 190, 18, 0x2104);

    // Middle card
    draw_rounded_rect(25, 42, 190, 105, 14, 0x7BEF);

    // Middle card stripe
    draw_rect(25, 76, 190, 18, 0x2104);

    // Main card
    draw_rounded_rect(25, 36, 190, 105, 14, 0x001F);

    // Card chip
    draw_rounded_rect(43, 58, 35, 27, 5, 0xFFE0);

    // Chip lines
    draw_line(60, 58, 60, 85, 0xC618);
    draw_line(43, 71, 78, 71, 0xC618);
    draw_line(50, 58, 50, 85, 0xC618);
    draw_line(70, 58, 70, 85, 0xC618);

    // Contactless symbol
    draw_line(168, 62, 177, 71, 0xFFFF);
    draw_line(177, 71, 168, 80, 0xFFFF);

    draw_line(160, 66, 166, 71, 0xFFFF);
    draw_line(166, 71, 160, 76, 0xFFFF);

    // Card number
    draw_text_box(45, 103, 150, 20, "1234 5678", 0xFFFF, 2);

    // Card holder
    draw_text_box(45, 123, 80, 15, "NAVANEET", 0xFFFF, 1);

    // Bottom separator
    draw_line(30, 160, 210, 160, 0x7BEF);

    // Balance / selected card information
    draw_text_box(35, 170, 75, 20, "VISA", 0xFFFF, 2);
    draw_text_box(135, 170, 70, 20, "ACTIVE", 0x07E0, 1);

    // Wallet hint
    draw_text_box(45, 205, 150, 20, "TAP TO PAY", 0xFFFF, 1);
}