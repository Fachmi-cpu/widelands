wl_library(logic_exceptions
  SRCS
    game_data_error.cc
    game_data_error.h
  DEPENDS
    base_exceptions
    base_i18n
)

wl_library(logic_widelands_geometry
  SRCS
    widelands_geometry.cc
    widelands_geometry.h
)

wl_library(logic_widelands_geometry_io
  SRCS
    widelands_geometry_io.cc
    widelands_geometry_io.h
  DEPENDS
    io_fileread
    io_stream
    logic_constants
    logic_widelands_geometry
)

wl_library(logic_game_settings
  SRCS
    game_settings.cc
    game_settings.h
    player_end_result.h
  DEPENDS
    io_filesystem
    logic_constants
    logic_tribe_basic_info
    notifications
    scripting_lua_interface
    scripting_lua_table
)

wl_library(logic_single_player_game_settings
  SRCS
    single_player_game_settings_provider.cc
    single_player_game_settings_provider.h
  DEPENDS
    ai
    base_exceptions
    base_i18n
    logic_game_settings
    logic_tribe_basic_info
)

wl_library(logic_game_controller
  SRCS
    game_controller.h
    replay_game_controller.h
    replay_game_controller.cc
    single_player_game_controller.h
    single_player_game_controller.cc
  DEPENDS
    ai
    logic
    logic_commands
    logic_game_settings
    ui_basic
    widelands_options
    wui
)

wl_library(logic_constants
  SRCS
    roadtype.h
    widelands.cc
    widelands.h
  DEPENDS
    base_macros
    logic_map_objects
)

wl_library(logic_filesystem_constants
  SRCS
    filesystem_constants.h
    filesystem_constants.cc
)

wl_library(logic_objectives
  SRCS
    objective.cc
    objective.h
  DEPENDS
    base_i18n
)


wl_library(logic_generic_save_handler
  SRCS
    generic_save_handler.h
    generic_save_handler.cc
  DEPENDS
    base_i18n
    base_log
    base_time_string
    io_filesystem
    logic_filesystem_constants
)

wl_library(logic_map
  SRCS
    cookie_priority_queue.h
    field.cc
    field.h
    map.cc
    map.h
    map_revision.cc
    map_revision.h
    mapastar.cc
    mapastar.h
    mapdifferenceregion.cc
    mapdifferenceregion.h
    mapfringeregion.cc
    mapfringeregion.h
    maphollowregion.cc
    maphollowregion.h
    mapregion.h
    maptriangleregion.cc
    maptriangleregion.h
    nodecaps.h
    note_map_options.h
    path.cc
    path.h
    pathfield.cc
    pathfield.h
  DEPENDS
    base_exceptions
    base_i18n
    base_log
    base_macros
    base_scoped_timer
    build_info
    economy
    graphic_playercolor
    io_fileread
    io_filesystem
    logic_constants
    logic_exceptions
    logic_filesystem_constants
    logic_map_objects
    logic_objectives
    logic_tribe_basic_info
    logic_widelands_geometry
    logic_widelands_geometry_io
    map_io_map_loader
    notifications
    random
    wui_mapview_pixelfunctions # TODO(GunChleoc): Circular dependency
)

wl_library(logic_commands
  SRCS
    cmd_calculate_statistics.cc
    cmd_calculate_statistics.h
    cmd_delete_message.cc
    cmd_delete_message.h
    cmd_incorporate.cc
    cmd_incorporate.h
    cmd_luacoroutine.cc
    cmd_luacoroutine.h
    cmd_luascript.cc
    cmd_luascript.h
    cmd_queue.cc
    cmd_queue.h
    queue_cmd_factory.cc
    queue_cmd_factory.h
    queue_cmd_ids.h
    playercommand.cc
    playercommand.h
  DEPENDS
    base_exceptions
    base_i18n
    base_log
    base_macros
    economy # TODO(GunChleoc): Circular dependency
    graphic_text_layout
    io_fileread
    io_stream
    logic # TODO(GunChleoc): Circular dependency
    logic_exceptions
    logic_game_controller
    logic_map
    logic_map_objects
    logic_widelands_geometry_io
    map_io
    scripting_coroutine
    scripting_logic # TODO(GunChleoc): Circular dependency
    scripting_lua_table
)

wl_library(logic
  SRCS
    ai_dna_handler.cc
    ai_dna_handler.h
    editor_game_base.cc
    editor_game_base.h
    game.cc
    game.h
    message.h
    message_id.h
    message_queue.h
    player.cc
    player.h
    player_area.h
    playersmanager.cc
    playersmanager.h
    replay.cc
    replay.h
    save_handler.cc
    save_handler.h
    see_unsee_node.h
    trade_agreement.h
  # TODO(sirver): Uses SDL2 only on WIN32 for a dirty hack.
  USES_SDL2
  DEPENDS
    base_exceptions
    base_i18n
    base_log
    base_macros
    base_md5
    base_scoped_timer
    base_time_string
    build_info
    economy
    game_io
    graphic
    graphic_color
    graphic_playercolor
    io_fileread
    io_filesystem
    io_profile
    io_stream
    logic_commands
    logic_constants
    logic_exceptions
    logic_filesystem_constants
    logic_game_controller
    logic_game_settings
    logic_generic_save_handler
    logic_map
    logic_map_objects
    logic_tribe_basic_info
    logic_widelands_geometry
    map_io
    map_io_map_loader
    note_sound
    notifications
    random
    scripting_logic # TODO(GunChleoc): Circular dependency
    scripting_lua_interface
    scripting_lua_table
    sound
    sound_constants
    ui_basic
    widelands_options
    wui # TODO(GunChleoc): Circular dependency
)

add_subdirectory(map_objects)
