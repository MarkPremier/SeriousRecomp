/* Uses the same native text gadgets, animation and input paths as the PC pages. */
#define CHEATS_VISIBLE 8
static uint32_t cheat_rows[CHEAT_ROWS], cheat_values[CHEAT_ROWS], cheat_back, cheat_help;
static int cheat_selected, cheat_scroll;
static uint32_t pause_original_last, pause_original_first;

static void update_cheats(void)
{
    if (cheat_selected < cheat_scroll) cheat_scroll = cheat_selected;
    if (cheat_selected < CHEAT_ROWS && cheat_selected >= cheat_scroll + CHEATS_VISIBLE)
        cheat_scroll = cheat_selected - CHEATS_VISIBLE + 1;
    if (cheat_scroll > CHEAT_ROWS - CHEATS_VISIBLE) cheat_scroll = CHEAT_ROWS - CHEATS_VISIBLE;
    char value[48], label[64];
    for (int i = 0; i < CHEAT_ROWS; ++i) {
        int visible = i >= cheat_scroll && i < cheat_scroll + CHEATS_VISIBLE;
        WORD(cheat_rows[i] + GADGET_VISIBLE) = WORD(cheat_values[i] + GADGET_VISIBLE) = visible;
        WORD(cheat_rows[i] + GADGET_FOCUSED) = WORD(cheat_values[i] + GADGET_FOCUSED) = i == cheat_selected;
        place(cheat_rows[i], 76, 154 + 27.0f * (i - cheat_scroll), 0.54f);
        place(cheat_values[i], 454, 154 + 27.0f * (i - cheat_scroll), 0.54f);
        pc_cheats_value(i, value, sizeof value);
        snprintf(label, sizeof label, "%s%s%s", i == CHEAT_RESET ? "" : "< ", value, i == CHEAT_RESET ? "" : " >");
        text(cheat_values[i], label);
    }
    WORD(cheat_back + GADGET_FOCUSED) = cheat_selected == CHEAT_ROWS;
    WORD(cheats_page + SCREEN_FOCUS) = cheat_selected == CHEAT_ROWS ? cheat_back : cheat_rows[cheat_selected];
    text(cheat_help, pc_cheats_single_player() || cheats_in_frontend
        ? "Single player only. Choices are remembered."
        : "Cheats are inactive in multiplayer.");
}
static void build_cheats(void)
{
    cheats_page = alloc(SCREEN_SIZE + TEXT_SIZE);
    if (!cheats_page) return;
    call(sub_0002F12B, cheats_page, NULL, 0);
    memcpy(PTR(cheats_page + SCREEN_VTABLE_COPY), PTR(SCREEN_VTABLE), VTABLE_SIZE);
    WORD(cheats_page) = cheats_page + SCREEN_VTABLE_COPY;
    WORD(cheats_page + SCREEN_VTABLE_COPY + VTABLE_KEY) = CHEATS_KEY;
    WORD(cheats_page + SCREEN_TITLE) = cheats_page + SCREEN_TITLE_TEXT;
    snprintf((char *)PTR(cheats_page + SCREEN_TITLE_TEXT), TEXT_SIZE, "Cheats");
    if (!button(cheats_page, "CHEATS", 90, 88, 1.05f)) return;
    for (int i = 0; i < CHEAT_ROWS; ++i) {
        cheat_rows[i] = button(cheats_page, pc_cheats_label(i), 76, 154, 0.54f);
        cheat_values[i] = button(cheats_page, "", 454, 154, 0.54f);
        if (!cheat_rows[i] || !cheat_values[i]) return;
    }
    cheat_help = button(cheats_page, "", 76, 377, 0.40f);
    cheat_back = button(cheats_page, "BACK", 510, 400, 0.5f);
    if (!cheat_help || !cheat_back || !button(cheats_page, "Arrows: select   Left/Right/A: change   Esc/B: back", 55, 434, 0.38f)) return;
    cheats_entry = button(MAIN_MENU, "CHEATS", 391, 339, 0.875f);
    pause_cheats_entry = button(PAUSE_MENU, "CHEATS", 140, 350, 0.65f);
    if (!cheats_entry || !pause_cheats_entry) return;
    WORD(cheats_entry + GADGET_TRIGGER) = WORD(pause_cheats_entry + GADGET_TRIGGER) = OPEN_CHEATS;
    WORD(cheats_entry + GADGET_CENTRED) = 0;
    WORD(cheats_entry + GADGET_UP) = MAIN_MENU_LAST;
    WORD(cheats_entry + GADGET_DOWN) = quit_entry ? quit_entry : TEMPLATE_BUTTON;
    if (quit_entry) {
        WORD(quit_entry + GADGET_UP) = cheats_entry;
        place(quit_entry, 391, 373, 0.875f);
    }
    slide_in(cheats_entry, 5);
    slide_in(quit_entry, 6);
    update_cheats();
}
static void cheats_entries_frame(void)
{
    static uint32_t previous_menu;
    uint32_t menu = shown_menu();
    int entering = menu != previous_menu;
    previous_menu = menu;
    /* These extra entries are outside the title's original transition
     * pairs. Supply their alternating endpoints on each arrival, using
     * the same native menu clock as the original five rows. */
    if (entering && menu == MAIN_MENU) {
        if (cheats_entry) { position(cheats_entry, 391, 339, 0.875f); slide_in(cheats_entry, 5); }
        if (quit_entry) { position(quit_entry, 391, cheats_entry ? 373 : 339, 0.875f); slide_in(quit_entry, cheats_entry ? 6 : 5); }
    }
    if (menu == MAIN_MENU) cheats_in_frontend = 1;
    pc_cheats_frontend(cheats_in_frontend);
    if (cheats_entry) {
        WORD(cheats_entry + GADGET_VISIBLE) = menu == MAIN_MENU;
        /* Hide the original code-unlocked menu, whose native transition
         * handler may reinsert it. Our page replaces its entry. */
        WORD(0x46F7B0 + GADGET_VISIBLE) = 0;
    }
    if (!pause_cheats_entry) return;
    int show = pc_cheats_pause_menu() && pc_cheats_single_player();
    WORD(pause_cheats_entry + GADGET_VISIBLE) = menu == PAUSE_MENU && show;
    if (menu != PAUSE_MENU) return;
    /* The native pause list changes with session type. Find its visible
     * navigation ring, excluding our extra gadget and controller hints. */
    uint32_t buttons[MAX_MENU_GADGETS], focus;
    int count = menu_buttons(PAUSE_MENU, buttons, &focus);
    uint32_t first = 0, last = 0;
    float bottom = 0;
    for (int i = 0; i < count; ++i) {
        uint32_t b = buttons[i];
        if (b == pause_cheats_entry) continue;
        float y = FLOAT(b + GADGET_Y);
        if (!first || y < FLOAT(first + GADGET_Y)) first = b;
        if (!last || y > bottom) { last = b; bottom = y; }
    }
    if (first && last) { pause_original_first = first; pause_original_last = last; }
    first = pause_original_first; last = pause_original_last;
    if (!first || !last) return;
    WORD(last + GADGET_DOWN) = show ? pause_cheats_entry : first;
    WORD(first + GADGET_UP) = show ? pause_cheats_entry : last;
    WORD(pause_cheats_entry + GADGET_UP) = last;
    WORD(pause_cheats_entry + GADGET_DOWN) = first;
    if (show) {
        place(pause_cheats_entry, FLOAT(last + GADGET_X), bottom + 30, FLOAT(last + GADGET_SCALE));
        if (entering) slide_in(pause_cheats_entry, WORD(last + GADGET_TRANSITION) == 2 ? 1 : 0);
    }
    else if (WORD(PAUSE_MENU + SCREEN_FOCUS) == pause_cheats_entry) {
        WORD(pause_cheats_entry + GADGET_FOCUSED) = 0;
        WORD(first + GADGET_FOCUSED) = 1;
        WORD(PAUSE_MENU + SCREEN_FOCUS) = first;
    }
}
static void open_cheats(void)
{
    uint32_t menu = shown_menu();
    if (cheats_page && (menu == MAIN_MENU || (menu == PAUSE_MENU && pc_cheats_pause_menu() && pc_cheats_single_player()))) {
        uint32_t args[] = {cheats_page, 0};
        int w, h;
        cheat_selected = cheat_scroll = 0;
        cancel_capture(); update_cheats(); remember_menu_keys();
        for (int i = 0; i < CHEAT_ROWS; ++i) { slide_in(cheat_rows[i], i); slide_in(cheat_values[i], i); }
        slide_in(cheat_back, CHEAT_ROWS);
        xbox_FramebufferPointer(&pointer_x, &pointer_y, &w, &h);
        for (int i = 0; i < 5; ++i) xbox_FramebufferMouseClicks(i);
        xbox_KbmMenuWheel();
        WORD(cheats_entry + GADGET_VISIBLE) = WORD(pause_cheats_entry + GADGET_VISIBLE) = 0;
        call(sub_0002A807, 0, args, 2);
    }
    g_esp += 4;
}
static void cheats_key(void)
{
    uint32_t key = WORD(g_esp + 4);
    if (key == VK_ESCAPE) call(sub_0002B52A, 0, NULL, 0);
    else if (key == VK_UP) cheat_selected = (cheat_selected + CHEAT_ROWS) % (CHEAT_ROWS + 1);
    else if (key == VK_DOWN) cheat_selected = (cheat_selected + 1) % (CHEAT_ROWS + 1);
    else if (key == VK_RETURN || key == 0x64 || key == VK_LEFT || key == VK_RIGHT) {
        if (cheat_selected == CHEAT_ROWS) {
            if (key == VK_RETURN || key == 0x64) call(sub_0002B52A, 0, NULL, 0);
        } else pc_cheats_step(cheat_selected, key == VK_LEFT ? -1 : 1);
    }
    update_cheats(); g_eax = 1; g_esp += 12;
}
static void cheats_frame(void)
{
    int old_selected = cheat_selected;
    int x, y, w, h, click = xbox_FramebufferMouseClicks(0), right = xbox_FramebufferMouseClicks(1);
    int wheel = xbox_KbmMenuWheel();
    if (xbox_FramebufferPointer(&x, &y, &w, &h) && w > 0 && h > 0) {
        float vw = (float)w, vh = vw * 3 / 4;
        if (vh > h) { vh = (float)h; vw = vh * 4 / 3; }
        float gx = (x - (w - vw) / 2) * 640 / vw, gy = (y - (h - vh) / 2) * 480 / vh;
        int moved = x != pointer_x || y != pointer_y;
        if (gx >= 495 && gx <= 610 && gy >= 395 && gy < 430) {
            if (moved) cheat_selected = CHEAT_ROWS;
            if (click) call(sub_0002B52A, 0, NULL, 0);
        } else if (gx >= 65 && gx <= 625 && gy >= 150 && gy < 150 + CHEATS_VISIBLE * 27 && (moved || click || right)) {
            cheat_selected = cheat_scroll + (int)((gy - 150) / 27);
            if (click || right) pc_cheats_step(cheat_selected, right ? -1 : 1);
        } else if (right) call(sub_0002B52A, 0, NULL, 0);
        pointer_x = x; pointer_y = y;
    }
    if (wheel) {
        cheat_selected += wheel > 0 ? -1 : 1;
        if (cheat_selected < 0) cheat_selected = 0;
        if (cheat_selected > CHEAT_ROWS) cheat_selected = CHEAT_ROWS;
    }
    if (cheat_selected != old_selected) menu_change_sound();
    update_cheats();
}
