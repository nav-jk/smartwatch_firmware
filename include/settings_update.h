#pragma once

#include <stdbool.h>

/*
 * Cloud-style update screen shown over Settings.
 * All calls are safe from any task (they take the LVGL lock) and are
 * no-ops if the Settings page is not open.
 */

/* Show the screen in "checking" (spinning) state. title may be NULL. */
void settings_update_open(const char *title);

/* Change the text. Pass NULL to leave a line unchanged. */
void settings_update_status(const char *title, const char *detail);

/* 0..100 fills the ring; a negative value goes back to spinning. */
void settings_update_progress(int pct);

/* Show success/failure, then return to Settings after ~1.8 s. */
void settings_update_done(bool ok, const char *msg);