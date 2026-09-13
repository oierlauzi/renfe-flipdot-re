idf_component_register(
    SRCS
        "main.c"
        "cp437.c"
        "pit100.c"
        "panel.c"
        "config.c"
        "timesrc.c"
        "netcfg.c"
        "webui.c"
        "modes.c"
        "mode_message.c"
        "mode_clock.c"
        "mode_counter.c"
        "mode_playlist.c"
        "mode_blank.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_http_server nvs_flash esp_wifi esp_netif esp_timer)
