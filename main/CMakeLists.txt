idf_component_register(
    SRCS "main.c"
         "../src/clock_module.c"
         "../src/framebuffer.c"
         "../src/menu.c"
         "../src/graphics.c"
         "../src/icons.c"
         "../src/text.c"

         "../src/menu/fitness.c"
         "../src/menu/phone.c"
         "../src/menu/international_time.c"
         "../src/menu/wallet.c"
         "../src/menu/calendar.c"
         "../src/menu/weather.c"
         "../src/menu/workout.c"
         "../src/menu/settings.c"

    INCLUDE_DIRS "." "../include"
)