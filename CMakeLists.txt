target_include_directories(app PRIVATE include)

target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_AUTH75_LED
    app
    PRIVATE
    src/behaviors/behavior_auth75_led.c
)
