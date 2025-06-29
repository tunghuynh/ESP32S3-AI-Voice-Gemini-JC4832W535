SET(SOURCES screens/ui_Main.c
    screens/ui_Settings.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_config_png.c
    images/ui_img_back_png.c
    images/ui_img_save_png.c
    fonts/ui_font_lvfontnotoserif14.c)

add_library(ui ${SOURCES})
