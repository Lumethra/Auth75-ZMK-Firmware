target_include_directories(app PRIVATE include)

target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_AUTH75_LED
    app
    PRIVATE
    src/behaviors/behavior_auth75_led.c
)

target_sources_ifdef(CONFIG_GPIO_AUTH75_PCA9536
    app
    PRIVATE
    src/drivers/gpio_auth75_pca9536.c
)
