
#ifndef PDL_VARIABLES_H
#define PDL_VARIABLES_H
#undef PDL_VARIABLES_H // this file will be included several times

MACRO_CONFIG_INT(PdlQuickDownload, pdl_quick_download, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "The client downloads a map faster")

MACRO_CONFIG_INT(PdlDummyCam, pdl_dummy_cam, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "See the dummy in hud")

MACRO_CONFIG_INT(PdlAutojoin, pdl_autojoin, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Automatacly joins when the server has a free slot")

MACRO_CONFIG_INT(PdlGamelayer, pdl_gamelayer, 0, 0, 1, CFGFLAG_CLIENT, "Show gamelayer")
MACRO_CONFIG_INT(PdlGamelayerBack, pdl_gamelayer_back, 1, 0, 1, CFGFLAG_CLIENT, "Show own background")

MACRO_CONFIG_STR(PdlQuickMenuKey, pdl_quickmenu_key, 1, "c", CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(PdlSaveJumpShift, pdl_savejump_shift, 0, -32, 32, CFGFLAG_CLIENT, "Shift the jump trigger")
MACRO_CONFIG_INT(PdlSaveJumpShoot, pdl_savejump_shoot, 0, 0, 1, CFGFLAG_CLIENT, "Shoots when freezetile is thouched")

MACRO_CONFIG_INT(PdlNameplateOwn, pdl_nameplate_own, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show your own nameplate")

MACRO_CONFIG_INT(PdlClock, pdl_clock, 0, 0, 1, CFGFLAG_CLIENT| CFGFLAG_SAVE, "Clock mode")
MACRO_CONFIG_INT(PdlClockMode, pdl_clock_mode, 0, 0, 1, CFGFLAG_CLIENT| CFGFLAG_SAVE, "Clock mode")

MACRO_CONFIG_INT(PdlBlockInfo, pdl_blockinfo, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show Block Info")
MACRO_CONFIG_INT(PdlBlockInfoTicks, pdl_blockinfo_ticks, 40, 1, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show Block Info")

MACRO_CONFIG_INT(PdlZoom, pdl_zoom, 16, 1, 128, CFGFLAG_CLIENT, "Zoom")

MACRO_CONFIG_INT(PdlBlockscoreActive, pdl_blockscore_active, 0, 0, 1, CFGFLAG_CLIENT, "Active")
MACRO_CONFIG_INT(PdlBlockscoreCollect, pdl_blockscore_collect, 0, 0, 1, CFGFLAG_CLIENT, "Collect Block infos")
MACRO_CONFIG_INT(PdlBlockscoreDraw, pdl_blockscore_draw, 0, 0, 1, CFGFLAG_CLIENT, "draw Block score map")
MACRO_CONFIG_INT(PdlBlockscoreShow, pdl_blockscore_show, 0, 0, 1, CFGFLAG_CLIENT, "Show the block scores of the players")
MACRO_CONFIG_INT(PdlBlockscoreStay, pdl_blockscore_stay, 0, 0, 1, CFGFLAG_CLIENT, "Dont let the player leave the server")
MACRO_CONFIG_INT(PdlBlockscoreChat, pdl_blockscore_chat, 1, 0, 1, CFGFLAG_CLIENT, "Support the players by chat")
MACRO_CONFIG_INT(PdlBlockscoreShowType, pdl_blockscore_show_Type, 3, 0, 3, CFGFLAG_CLIENT, "Type of shown blockscore")

MACRO_CONFIG_INT(PdlAutorunActive, pdl_autorun_active, 0, 0, 1, CFGFLAG_CLIENT, "Active")
MACRO_CONFIG_INT(PdlAutorunDraw, pdl_autorun_draw, 0, 0, 1, CFGFLAG_CLIENT, "draw autorun map")

MACRO_CONFIG_INT(PdlBlockHelp, pdl_block_help, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Active")
MACRO_CONFIG_INT(PdlBlockArrow, pdl_block_arrow, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Shows an arrow to locate the enemy")
MACRO_CONFIG_INT(PdlBlockArrowDistance, pdl_block_arrow_distance, 512, 0, 1024, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Distance of the enemy to activate the arrow")

MACRO_CONFIG_STR(PdlTranslateOtherFrom, pdl_translate_other_from, 8, "", CFGFLAG_CLIENT, "")
MACRO_CONFIG_STR(PdlTranslateOtherTo, pdl_translate_other_to, 8, "de", CFGFLAG_CLIENT, "")
MACRO_CONFIG_STR(PdlTranslateChatFrom, pdl_translate_chat_from, 8, "", CFGFLAG_CLIENT, "")
MACRO_CONFIG_STR(PdlTranslateChatTo, pdl_translate_chat_to, 8, "en", CFGFLAG_CLIENT, "")


MACRO_CONFIG_INT(PdlGrenadeKills, pdl_grenade_kills, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Active")
MACRO_CONFIG_INT(PdlGrenadeDodge, pdl_grenade_dodge, 0, 0, 1, CFGFLAG_CLIENT, "Active")
MACRO_CONFIG_INT(PdlGrenadeCooldown, pdl_grenade_cooldown, 0, 0, 1, CFGFLAG_CLIENT, "Active")
MACRO_CONFIG_INT(PdlGrenadeJump, pdl_grenade_jump, 0, 0, 1, CFGFLAG_CLIENT, "Active")

MACRO_CONFIG_INT(PdlAutoHideConsole, pdl_auto_hide_console, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Active")

MACRO_CONFIG_INT(PdlUpdateIdentities, pdl_update_identities, 1, 0, 1, CFGFLAG_CLIENT, "UpdateIdentities")

MACRO_CONFIG_INT(PdlShowParticles, pdl_show_particles, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show particles")

MACRO_CONFIG_INT(PdlInputlockActive, pdl_inputlock_active, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Middle mousebutton to activate inputlock")
MACRO_CONFIG_INT(PdlInputCopyPaste, pdl_input_copypaste, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enables Copy/Paste option on input")

MACRO_CONFIG_INT(PdlFakeScoreboardOpen, pdl_fake_scoreboard_open, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Fake an opened Scoreboard the fetch ping data")
MACRO_CONFIG_INT(PdlScoreboardDeadActive, pdl_scoreboard_dead_active, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Shows the scoreboard when you are dead")
MACRO_CONFIG_STR(PdlScoreboardSortion, pdl_scoreboard_sortion, 16, "name", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Sortion of the scoreboard in DDRace-Mods (name/id/score/clan/latency/country)")

MACRO_CONFIG_INT(PdlMapinkerEnabled, pdl_mapinker_enabled, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enables the MapInker")
MACRO_CONFIG_INT(PdlMapinkerTimeEnable, pdl_mapinker_time_enable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enables the MapInker-Timer")
MACRO_CONFIG_INT(PdlMapinkerTimeFrom, pdl_mapinker_time_from, 20, 0, 24, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hour when Night-Design gets active")
MACRO_CONFIG_INT(PdlMapinkerTimeTo, pdl_mapinker_time_to, 7, 0, 24, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hour when Night-Design gets deactivated")

MACRO_CONFIG_INT(PdlBackgroundRipple, pdl_background_ribble, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Ripple animation in the background (shader)")
MACRO_CONFIG_INT(PdlBackgroundRays, pdl_background_rays, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rays in the background (shader)")

MACRO_CONFIG_INT(PdlAutoRename, pdl_auto_rename, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Automatically updates your information if it's not synchronized with the server")

MACRO_CONFIG_INT(PdlTaskFlashJoin, pdl_taskflash_join, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Flashes your taskbar when you are joining a server")
MACRO_CONFIG_INT(PdlTaskFlashChat, pdl_taskflash_chat, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Flashes your taskbar when you get a highlighted chat message")


#endif