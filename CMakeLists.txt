cmake_minimum_required(VERSION 3.10)
project(q2gloombot C)

set(CMAKE_C_STANDARD 99)

# -----------------------------------------------------------------------
# Output name: gamex86.dll (Windows) or gamei386.so (Linux)
# -----------------------------------------------------------------------
if(WIN32)
    set(DLL_OUTPUT_NAME "gamex86")
else()
    set(DLL_OUTPUT_NAME "gamei386")
endif()

# -----------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------
set(GAME_SOURCES
    src/game/g_main.c
    src/game/q_shared.c
)

set(BOT_SOURCES
    src/bot/bot_main.c
    src/bot/bot_upgrade.c
    src/bot/bot_debug.c
    src/bot/bot_commands.c
    src/bot/bot_personality.c
    src/bot/bot_humanize.c
    src/bot/bot_chat.c
    src/bot/bot_cvars.c
    src/bot/bot_config.c
    src/bot/bot_autofill.c
    src/bot/nav/bot_nav.c
    src/bot/nav/bot_nodes.c
    src/bot/combat/bot_combat.c
    src/bot/team/bot_team.c
)

# Per-class bot behaviour modules (one per Gloom class)
set(BOT_CLASS_SOURCES
    # Legacy stub classes (kept for compatibility)
    src/bot/classes/bot_class_marine_light.c
    src/bot/classes/bot_class_marine_assault.c
    src/bot/classes/bot_class_marine_heavy.c
    src/bot/classes/bot_class_marine_laser.c
    src/bot/classes/bot_class_marine_battle.c
    src/bot/classes/bot_class_marine_elite.c
    src/bot/classes/bot_class_builder.c
    src/bot/classes/bot_class_builder_adv.c
    src/bot/classes/bot_class_granger.c
    src/bot/classes/bot_class_dretch.c
    src/bot/classes/bot_class_spiker.c
    src/bot/classes/bot_class_marauder.c
    src/bot/classes/bot_class_dragoon.c
    src/bot/classes/bot_class_tyrant.c
    # Gloom alien class AI modules
    src/bot/classes/bot_class_breeder.c
    src/bot/classes/bot_class_hatchling.c
    src/bot/classes/bot_class_drone.c
    src/bot/classes/bot_class_wraith.c
    src/bot/classes/bot_class_kamikaze.c
    src/bot/classes/bot_class_stinger.c
    src/bot/classes/bot_class_guardian.c
    src/bot/classes/bot_class_stalker.c
    src/bot/classes/bot_alien_common.c
    # Gloom human class AI modules
    src/bot/classes/bot_class_engineer.c
    src/bot/classes/bot_class_grunt.c
    src/bot/classes/bot_class_st.c
    src/bot/classes/bot_class_biotech.c
    src/bot/classes/bot_class_ht.c
    src/bot/classes/bot_class_commando.c
    src/bot/classes/bot_class_exterminator.c
    src/bot/classes/bot_class_mech.c
    src/bot/classes/bot_human_common.c
)

set(BOT_BUILD_SOURCES
    src/bot/build/bot_build.c
    src/bot/build/bot_build_placement.c
)

set(BOT_TEAM_EXTRA_SOURCES
    src/bot/team/bot_strategy.c
    src/bot/team/bot_teamwork.c
    src/bot/team/bot_mapcontrol.c
)

set(GLOOM_SOURCES
    src/gloom/gloom_classes.c
)

# -----------------------------------------------------------------------
# Shared library target
# -----------------------------------------------------------------------
add_library(${DLL_OUTPUT_NAME} SHARED
    ${GAME_SOURCES}
    ${BOT_SOURCES}
    ${BOT_CLASS_SOURCES}
    ${BOT_BUILD_SOURCES}
    ${BOT_TEAM_EXTRA_SOURCES}
    ${GLOOM_SOURCES}
)

# Include directories
target_include_directories(${DLL_OUTPUT_NAME} PRIVATE
    src/game
    src/gloom
    src/bot
    src/bot/nav
    src/bot/combat
    src/bot/team
    src/bot/build
    src/bot/classes
)

# -----------------------------------------------------------------------
# Compiler flags
# -----------------------------------------------------------------------
if(MSVC)
    target_compile_options(${DLL_OUTPUT_NAME} PRIVATE
        /W3
        /wd4244  # conversion possible loss of data
        /wd4136  # float/double conversion
        /wd4051  # type conversion
        /wd4018  # signed/unsigned mismatch
        /wd4305  # truncation from double to float
    )
    target_compile_definitions(${DLL_OUTPUT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32
        _WINDOWS
    )
else()
    target_compile_options(${DLL_OUTPUT_NAME} PRIVATE
        -Wall
        -Wno-unused-function
        -fvisibility=default
    )
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^i[3-6]86$")
        target_compile_options(${DLL_OUTPUT_NAME} PRIVATE -m32)
        target_link_options(${DLL_OUTPUT_NAME} PRIVATE -m32)
    endif()
endif()

# On Linux Quake 2 expects the .so without the "lib" prefix
set_target_properties(${DLL_OUTPUT_NAME} PROPERTIES
    PREFIX ""
)

# -----------------------------------------------------------------------
# Install: copy into a "gloom" game directory for quick testing
# -----------------------------------------------------------------------
install(TARGETS ${DLL_OUTPUT_NAME}
    LIBRARY DESTINATION gloom
    RUNTIME DESTINATION gloom
)

# -----------------------------------------------------------------------
# Test executable — standalone harness (no Quake 2 engine required)
# -----------------------------------------------------------------------
set(TEST_SOURCES
    test/bot_test.c
    src/bot/bot_main.c
    src/bot/bot_upgrade.c
    src/bot/bot_debug.c
    src/bot/bot_commands.c
    src/bot/bot_personality.c
    src/bot/bot_humanize.c
    src/bot/bot_chat.c
    src/bot/bot_cvars.c
    src/bot/bot_config.c
    src/bot/bot_autofill.c
    src/bot/nav/bot_nav.c
    src/bot/nav/bot_nodes.c
    src/bot/combat/bot_combat.c
    src/bot/team/bot_team.c
    src/bot/team/bot_strategy.c
    src/bot/team/bot_teamwork.c
    src/bot/team/bot_mapcontrol.c
    src/bot/build/bot_build.c
    src/bot/build/bot_build_placement.c
    src/gloom/gloom_classes.c
)

add_executable(bot_test ${TEST_SOURCES})

target_include_directories(bot_test PRIVATE
    src/game
    src/gloom
    src/bot
    src/bot/nav
    src/bot/combat
    src/bot/team
    src/bot/build
    src/bot/classes
)

target_compile_definitions(bot_test PRIVATE BOT_TEST_MODE)

if(NOT MSVC)
    target_link_libraries(bot_test m)
endif()

if(NOT MSVC)
    target_compile_options(bot_test PRIVATE
        -Wall
        -Wno-unused-function
    )
endif()
