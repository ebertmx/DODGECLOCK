cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(BT_DISPLAY)

target_sources(app PRIVATE src/main.c src/DCLK_client.c src/Interface_display.c)
