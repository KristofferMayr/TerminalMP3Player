cmake_minimum_required(VERSION 3.20)
project(TerminalMP3Player C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED IMPORTED_TARGET portaudio-2.0)
pkg_check_modules(NCURSES REQUIRED IMPORTED_TARGET ncursesw)

add_executable(tmp3
    src/main.c
    src/app/app.c
    src/audio/player.c
    src/library/library.c
    src/ui/ui.c
)

target_include_directories(tmp3 PRIVATE src)
# SYSTEM, damit die Warnungen aus minimp3 nicht angezeigt werden
target_include_directories(tmp3 SYSTEM PRIVATE third_party/minimp3)
target_link_libraries(tmp3 PRIVATE PkgConfig::PORTAUDIO PkgConfig::NCURSES)
target_compile_options(tmp3 PRIVATE -Wall -Wextra)
