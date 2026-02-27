set(COMPONENT_REQUIRES "RV3028C7")
idf_component_register(
    SRCS 
        "main.cpp"
        "laptimer_task.cpp"
        "sdcard_task.cpp"
        "lcd_task.cpp"
        "wifi_task.cpp"
        "lvgl_ui.c"
        "fonts/roboto_mono_10.c"
        "fonts/roboto_mono_14.c"
        "fonts/roboto_mono_18.c"
        "fonts/roboto_mono_38.c"
        "drivers/gpio.c"
<<<<<<< HEAD
        "drivers/timer.cpp"
        "drivers/sdcard.c"
        "drivers/wifi_driver.c"
        "fonts/font9x15-ISO8859-1.c"
        "fonts/font10x20-ISO8859-1.c"
=======
        "drivers/lcd.c"
        "drivers/timer.cpp"
        "drivers/sdcard.c"
        "drivers/wifi_driver.c"
>>>>>>> dev-lvgl
    INCLUDE_DIRS 
        "include"
        "drivers/include"
)
