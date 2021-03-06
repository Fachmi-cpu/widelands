/*
 * Copyright (C) 2002-2019 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "wui/load_or_save_game.h"

#include <ctime>
#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "base/i18n.h"
#include "base/log.h"
#include "base/time_string.h"
#include "game_io/game_loader.h"
#include "game_io/game_preload_packet.h"
#include "graphic/font_handler.h"
#include "graphic/text_layout.h"
#include "helper.h"
#include "io/filesystem/filesystem_exceptions.h"
#include "io/filesystem/layered_filesystem.h"
#include "logic/filesystem_constants.h"
#include "logic/game_controller.h"
#include "logic/game_settings.h"
#include "logic/replay.h"
#include "ui_basic/messagebox.h"

namespace {
// This function concatenates the filename and localized map name for a savegame/replay.
// If the filename starts with the map name, the map name is omitted.
// It also prefixes autosave files with a numbered and localized "Autosave" prefix.
std::string
map_filename(const std::string& filename, const std::string& mapname, bool localize_autosave) {
	std::string result = FileSystem::filename_without_ext(filename.c_str());

	if (localize_autosave && boost::starts_with(result, kAutosavePrefix)) {
		std::vector<std::string> autosave_name;
		boost::split(autosave_name, result, boost::is_any_of("_"));
		if (autosave_name.empty() || autosave_name.size() < 3) {
			/** TRANSLATORS: %1% is a map's name. */
			result = (boost::format(_("Autosave: %1%")) % mapname).str();
		} else {
			/** TRANSLATORS: %1% is a number, %2% a map's name. */
			result = (boost::format(_("Autosave %1%: %2%")) % autosave_name.back() % mapname).str();
		}
	} else if (!(boost::starts_with(result, mapname))) {
		/** TRANSLATORS: %1% is a filename, %2% a map's name. */
		result = (boost::format(pgettext("filename_mapname", "%1%: %2%")) % result % mapname).str();
	}
	return result;
}
}  // namespace

LoadOrSaveGame::LoadOrSaveGame(UI::Panel* parent,
                               Widelands::Game& g,
                               FileType filetype,
                               UI::PanelStyle style,
                               bool localize_autosave)
   : parent_(parent),
     table_box_(new UI::Box(parent, 0, 0, UI::Box::Vertical)),
     table_(table_box_, 0, 0, 0, 0, style, UI::TableRows::kMultiDescending),
     filetype_(filetype),
     show_filenames_(false),
     localize_autosave_(localize_autosave),
     // Savegame description
     game_details_(
        parent,
        style,
        filetype_ == FileType::kReplay ? GameDetails::Mode::kReplay : GameDetails::Mode::kSavegame),
     delete_(new UI::Button(game_details()->button_box(),
                            "delete",
                            0,
                            0,
                            0,
                            0,
                            style == UI::PanelStyle::kFsMenu ? UI::ButtonStyle::kFsMenuSecondary :
                                                               UI::ButtonStyle::kWuiSecondary,
                            _("Delete"))),
     game_(g) {
	table_.add_column(130, _("Save Date"), _("The date this game was saved"), UI::Align::kLeft);

	if (filetype_ != FileType::kGameSinglePlayer) {
		std::string game_mode_tooltip =
		   /** TRANSLATORS: Tooltip header for the "Mode" column when choosing a game/replay to load.
		    */
		   /** TRANSLATORS: %s is a list of game modes. */
		   g_gr->styles().font_style(UI::FontStyle::kTooltipHeader).as_font_tag(_("Game Mode"));

		if (filetype_ == FileType::kReplay) {
			/** TRANSLATORS: Tooltip for the "Mode" column when choosing a game/replay to load. */
			/** TRANSLATORS: Make sure that you keep consistency in your translation. */
			game_mode_tooltip += as_listitem(_("SP = Single Player"), UI::FontStyle::kTooltip);
		}
		/** TRANSLATORS: Tooltip for the "Mode" column when choosing a game/replay to load. */
		/** TRANSLATORS: Make sure that you keep consistency in your translation. */
		game_mode_tooltip += as_listitem(_("MP = Multiplayer"), UI::FontStyle::kTooltip);
		/** TRANSLATORS: Tooltip for the "Mode" column when choosing a game/replay to load. */
		/** TRANSLATORS: Make sure that you keep consistency in your translation. */
		game_mode_tooltip += as_listitem(_("H = Multiplayer (Host)"), UI::FontStyle::kTooltip);

		game_mode_tooltip += g_gr->styles()
		                        .font_style(UI::FontStyle::kTooltip)
		                        .as_font_tag(_("Numbers are the number of players."));

		table_.add_column(
		   65,
		   /** TRANSLATORS: Game Mode table column when choosing a game/replay to load. */
		   /** TRANSLATORS: Keep this to 5 letters maximum. */
		   /** TRANSLATORS: A tooltip will explain if you need to use an abbreviation. */
		   _("Mode"), game_mode_tooltip);
	}
	table_.add_column(0, _("Description"),
	                  _("The filename that the game was saved under followed by the map’s name, "
	                    "or the map’s name followed by the last objective achieved."),
	                  UI::Align::kLeft, UI::TableColumnType::kFlexible);
	table_.set_column_compare(
	   0, boost::bind(&LoadOrSaveGame::compare_date_descending, this, _1, _2));
	table_.set_sort_column(0);
	fill_table();

	table_box_->add(&table_, UI::Box::Resizing::kExpandBoth);

	game_details_.button_box()->add(delete_, UI::Box::Resizing::kAlign, UI::Align::kLeft);
	delete_->set_enabled(false);
	delete_->sigclicked.connect(boost::bind(&LoadOrSaveGame::clicked_delete, boost::ref(*this)));
}

const std::string LoadOrSaveGame::filename_list_string() const {
	return filename_list_string(table_.selections());
}

const std::string LoadOrSaveGame::filename_list_string(const std::set<uint32_t>& selections) const {
	boost::format message;
	for (const uint32_t index : selections) {
		const SavegameData& gamedata = games_data_[table_.get(table_.get_record(index))];

		if (gamedata.errormessage.empty()) {
			std::vector<std::string> listme;
			listme.push_back(richtext_escape(gamedata.mapname));
			listme.push_back(gamedata.savedonstring);
			message = (boost::format("%s\n%s") % message %
			           i18n::localize_list(listme, i18n::ConcatenateWith::COMMA));
		} else {
			message = boost::format("%s\n%s") % message % richtext_escape(gamedata.filename);
		}
	}
	return message.str();
}

bool LoadOrSaveGame::compare_date_descending(uint32_t rowa, uint32_t rowb) const {
	const SavegameData& r1 = games_data_[table_[rowa]];
	const SavegameData& r2 = games_data_[table_[rowb]];

	return r1.savetimestamp < r2.savetimestamp;
}

std::unique_ptr<SavegameData> LoadOrSaveGame::entry_selected() {
	std::unique_ptr<SavegameData> result(new SavegameData());
	size_t selections = table_.selections().size();
	if (selections == 1) {
		delete_->set_tooltip(
		   filetype_ == FileType::kReplay ?
		      /** TRANSLATORS: Tooltip for the delete button. The user has selected 1 file */
		      _("Delete this replay") :
		      /** TRANSLATORS: Tooltip for the delete button. The user has selected 1 file */
		      _("Delete this game"));
		result.reset(new SavegameData(games_data_[table_.get_selected()]));
	} else if (selections > 1) {
		delete_->set_tooltip(
		   filetype_ == FileType::kReplay ?
		      /** TRANSLATORS: Tooltip for the delete button. The user has selected multiple files */
		      _("Delete these replays") :
		      /** TRANSLATORS: Tooltip for the delete button. The user has selected multiple files */
		      _("Delete these games"));
		result->mapname =
		   (boost::format(ngettext("Selected %d file:", "Selected %d files:", selections)) %
		    selections)
		      .str();
		result->filename_list = filename_list_string();
	} else {
		delete_->set_tooltip("");
	}
	game_details_.update(*result);
	delete_->set_enabled(table().has_selection());
	// TODO(GunChleoc): Take care of the OK button too.
	return result;
}

bool LoadOrSaveGame::has_selection() const {
	return table_.has_selection();
}

void LoadOrSaveGame::clear_selections() {
	table_.clear_selections();
	game_details_.clear();
}

void LoadOrSaveGame::select_by_name(const std::string& name) {
	table_.clear_selections();
	for (size_t idx = 0; idx < table_.size(); ++idx) {
		const SavegameData& gamedata = games_data_[table_[idx]];
		if (name == gamedata.filename) {
			table_.select(idx);
			return;
		}
	}
}

UI::Table<uintptr_t const>& LoadOrSaveGame::table() {
	return table_;
}

UI::Box* LoadOrSaveGame::table_box() {
	return table_box_;
}

GameDetails* LoadOrSaveGame::game_details() {
	return &game_details_;
}

const std::string LoadOrSaveGame::get_filename(int index) const {
	return games_data_[table_.get(table_.get_record(index))].filename;
}

void LoadOrSaveGame::clicked_delete() {
	if (!has_selection()) {
		return;
	}
	std::set<uint32_t> selections = table().selections();
	const size_t no_selections = selections.size();
	std::string header = "";
	if (filetype_ == FileType::kReplay) {
		header = no_selections == 1 ? _("Do you really want to delete this replay?") :
		                              /** TRANSLATORS: Used with multiple replays, 1 replay has a
		                      separate string. DO NOT omit the placeholder in your translation. */
		            (boost::format(ngettext("Do you really want to delete this %d replay?",
		                                    "Do you really want to delete these %d replays?",
		                                    no_selections)) %
		             no_selections)
		               .str();
	} else {
		header = no_selections == 1 ? _("Do you really want to delete this game?") :
		                              /** TRANSLATORS: Used with multiple games, 1 game has a separate
		                     string. DO NOT omit the placeholder in your translation. */
		            (boost::format(ngettext("Do you really want to delete this %d game?",
		                                    "Do you really want to delete these %d games?",
		                                    no_selections)) %
		             no_selections)
		               .str();
	}

	bool do_delete = SDL_GetModState() & KMOD_CTRL;
	if (!do_delete) {
		const std::string message = (boost::format("%s\n%s") % header % filename_list_string()).str();

		UI::WLMessageBox confirmationBox(
		   parent_->get_parent()->get_parent(),
		   no_selections == 1 ? _("Confirm Deleting File") : _("Confirm Deleting Files"), message,
		   UI::WLMessageBox::MBoxType::kOkCancel);
		do_delete = confirmationBox.run<UI::Panel::Returncodes>() == UI::Panel::Returncodes::kOk;
		table_.focus();
	}
	if (do_delete) {
		// Failed deletions aren't a serious problem, we just catch the errors
		// and keep track to notify the player.
		std::set<uint32_t> failed_selections;
		bool failed;
		for (const uint32_t index : selections) {
			failed = false;
			const std::string& deleteme = get_filename(index);
			try {
				g_fs->fs_unlink(deleteme);
			} catch (const FileError& e) {
				log("player-requested file deletion failed: %s", e.what());
				failed = true;
			}
			if (filetype_ == FileType::kReplay) {
				try {
					g_fs->fs_unlink(deleteme + kSavegameExtension);
					// If at least one of the two relevant files of a replay are
					// successfully deleted then count it as success.
					// (From the player perspective the replay is gone.)
					failed = false;
					// If it was a multiplayer replay, also delete the synchstream. Do
					// it here, so it's only attempted if replay deletion was successful.
					if (g_fs->file_exists(deleteme + kSyncstreamExtension)) {
						g_fs->fs_unlink(deleteme + kSyncstreamExtension);
					}
				} catch (const FileError& e) {
					log("player-requested file deletion failed: %s", e.what());
				}
			}
			if (failed) {
				failed_selections.insert(index);
			}
		}
		if (!failed_selections.empty()) {
			const uint32_t no_failed = failed_selections.size();
			// Notify the player.
			const std::string caption =
			   (no_failed == 1) ? _("Error Deleting File!") : _("Error Deleting Files!");
			if (filetype_ == FileType::kReplay) {
				if (selections.size() == 1) {
					header = _("The replay could not be deleted.");
				} else {
					header = (boost::format(ngettext("%d replay could not be deleted.",
					                                 "%d replays could not be deleted.", no_failed)) %
					          no_failed)
					            .str();
				}
			} else {
				if (selections.size() == 1) {
					header = _("The game could not be deleted.");
				} else {
					header = (boost::format(ngettext("%d game could not be deleted.",
					                                 "%d games could not be deleted.", no_failed)) %
					          no_failed)
					            .str();
				}
			}
			std::string message =
			   (boost::format("%s\n%s") % header % filename_list_string(failed_selections)).str();
			UI::WLMessageBox msgBox(
			   parent_->get_parent()->get_parent(), caption, message, UI::WLMessageBox::MBoxType::kOk);
			msgBox.run<UI::Panel::Returncodes>();
		}
		fill_table();

		// Select something meaningful if possible, then scroll to it.
		const uint32_t selectme = *selections.begin();
		if (selectme < table_.size() - 1) {
			table_.select(selectme);
		} else if (!table_.empty()) {
			table_.select(table_.size() - 1);
		}
		if (table_.has_selection()) {
			table_.scroll_to_item(table_.selection_index() + 1);
		}
		// Make sure that the game details are updated
		entry_selected();
	}
}

UI::Button* LoadOrSaveGame::delete_button() {
	return delete_;
}

void LoadOrSaveGame::fill_table() {

	clear_selections();
	table_.clear();

	FilenameSet gamefiles;
	if (filetype_ == FileType::kReplay) {
		gamefiles = g_fs->filter_directory(kReplayDir, [](const std::string& fn) {
			return boost::algorithm::ends_with(fn, kReplayExtension);
		});
		// Update description column title for replays
		table_.set_column_tooltip(2, show_filenames_ ? _("Filename: Map name (start of replay)") :
		                                               _("Map name (start of replay)"));
	} else {
		gamefiles = g_fs->filter_directory(kSaveDir, [](const std::string& fn) {
			return boost::algorithm::ends_with(fn, kSavegameExtension);
		});
	}

	Widelands::GamePreloadPacket gpdp;

	for (const std::string& gamefilename : gamefiles) {
		SavegameData gamedata;

		std::string savename = gamefilename;
		if (filetype_ == FileType::kReplay) {
			savename += kSavegameExtension;
		}

		if (!g_fs->file_exists(savename.c_str())) {
			continue;
		}

		gamedata.filename = gamefilename;

		try {
			Widelands::GameLoader gl(savename.c_str(), game_);
			gl.preload_game(gpdp);

			gamedata.gametype = gpdp.get_gametype();

			// Skip singleplayer games in multiplayer mode and vice versa
			if (filetype_ != FileType::kReplay && filetype_ != FileType::kShowAll) {
				if (filetype_ == FileType::kGameMultiPlayer) {
					if (gamedata.gametype == GameController::GameType::kSingleplayer) {
						continue;
					}
				} else if ((gamedata.gametype != GameController::GameType::kSingleplayer) &&
				           (gamedata.gametype != GameController::GameType::kReplay)) {
					continue;
				}
			}

			gamedata.set_mapname(gpdp.get_mapname());
			gamedata.set_gametime(gpdp.get_gametime());
			gamedata.set_nrplayers(gpdp.get_number_of_players());
			gamedata.version = gpdp.get_version();

			gamedata.savetimestamp = gpdp.get_savetimestamp();
			time_t t;
			time(&t);
			struct tm* currenttime = localtime(&t);
			// We need to put these into variables because of a sideeffect of the localtime function.
			int8_t current_year = currenttime->tm_year;
			int8_t current_month = currenttime->tm_mon;
			int8_t current_day = currenttime->tm_mday;

			struct tm* savedate = localtime(&gamedata.savetimestamp);

			if (gamedata.savetimestamp > 0) {
				if (savedate->tm_year == current_year && savedate->tm_mon == current_month &&
				    savedate->tm_mday == current_day) {  // Today

					// Adding the 0 padding in a separate statement so translators won't have to deal
					// with it
					const std::string minute = (boost::format("%02u") % savedate->tm_min).str();

					gamedata.savedatestring =
					   /** TRANSLATORS: Display date for choosing a savegame/replay. Placeholders are:
					      hour:minute */
					   (boost::format(_("Today, %1%:%2%")) % savedate->tm_hour % minute).str();
					gamedata.savedonstring =
					   /** TRANSLATORS: Display date for choosing a savegame/replay. Placeholders are:
					      hour:minute. This is part of a list. */
					   (boost::format(_("saved today at %1%:%2%")) % savedate->tm_hour % minute).str();
				} else if ((savedate->tm_year == current_year && savedate->tm_mon == current_month &&
				            savedate->tm_mday == current_day - 1) ||
				           (savedate->tm_year == current_year - 1 && savedate->tm_mon == 11 &&
				            current_month == 0 && savedate->tm_mday == 31 &&
				            current_day == 1)) {  // Yesterday
					// Adding the 0 padding in a separate statement so translators won't have to deal
					// with it
					const std::string minute = (boost::format("%02u") % savedate->tm_min).str();

					gamedata.savedatestring =
					   /** TRANSLATORS: Display date for choosing a savegame/replay. Placeholders are:
					      hour:minute */
					   (boost::format(_("Yesterday, %1%:%2%")) % savedate->tm_hour % minute).str();
					gamedata.savedonstring =
					   /** TRANSLATORS: Display date for choosing a savegame/replay. Placeholders are:
					      hour:minute. This is part of a list. */
					   (boost::format(_("saved yesterday at %1%:%2%")) % savedate->tm_hour % minute)
					      .str();
				} else {  // Older
					gamedata.savedatestring =
					   /** TRANSLATORS: Display date for choosing a savegame/replay. Placeholders are:
					      month day, year */
					   (boost::format(_("%1% %2%, %3%")) % localize_month(savedate->tm_mon) %
					    savedate->tm_mday % (1900 + savedate->tm_year))
					      .str();
					gamedata.savedonstring =
					   /** TRANSLATORS: Display date for choosing a savegame/replay. Placeholders are:
					      month (short name) day (number), year (number). This is part of a list. */
					   (boost::format(_("saved on %1% %2%, %3%")) % savedate->tm_mday %
					    localize_month(savedate->tm_mon) % (1900 + savedate->tm_year))
					      .str();
				}
			}

			gamedata.wincondition = gpdp.get_localized_win_condition();
			gamedata.minimap_path = gpdp.get_minimap_path();
			games_data_.push_back(gamedata);

			UI::Table<uintptr_t const>::EntryRecord& te = table_.add(games_data_.size() - 1);
			te.set_string(0, gamedata.savedatestring);

			if (filetype_ != FileType::kGameSinglePlayer) {
				std::string gametypestring;
				switch (gamedata.gametype) {
				case GameController::GameType::kSingleplayer:
					/** TRANSLATORS: "Single Player" entry in the Game Mode table column. */
					/** TRANSLATORS: "Keep this to 6 letters maximum. */
					/** TRANSLATORS: A tooltip will explain the abbreviation. */
					/** TRANSLATORS: Make sure that this translation is consistent with the tooltip. */
					gametypestring = _("SP");
					break;
				case GameController::GameType::kNetHost:
					/** TRANSLATORS: "Multiplayer Host" entry in the Game Mode table column. */
					/** TRANSLATORS: "Keep this to 2 letters maximum. */
					/** TRANSLATORS: A tooltip will explain the abbreviation. */
					/** TRANSLATORS: Make sure that this translation is consistent with the tooltip. */
					/** TRANSLATORS: %1% is the number of players */
					gametypestring = (boost::format(_("H (%1%)")) % gamedata.nrplayers).str();
					break;
				case GameController::GameType::kNetClient:
					/** TRANSLATORS: "Multiplayer" entry in the Game Mode table column. */
					/** TRANSLATORS: "Keep this to 2 letters maximum. */
					/** TRANSLATORS: A tooltip will explain the abbreviation. */
					/** TRANSLATORS: Make sure that this translation is consistent with the tooltip. */
					/** TRANSLATORS: %1% is the number of players */
					gametypestring = (boost::format(_("MP (%1%)")) % gamedata.nrplayers).str();
					break;
				case GameController::GameType::kReplay:
					gametypestring = "";
					break;
				case GameController::GameType::kUndefined:
					NEVER_HERE();
				}
				te.set_string(1, gametypestring);
				if (filetype_ == FileType::kReplay) {
					const std::string map_basename =
					   show_filenames_ ?
					      map_filename(gamedata.filename, gamedata.mapname, localize_autosave_) :
					      gamedata.mapname;
					te.set_string(2, (boost::format(pgettext("mapname_gametime", "%1% (%2%)")) %
					                  map_basename % gamedata.gametime)
					                    .str());
				} else {
					te.set_string(
					   2, map_filename(gamedata.filename, gamedata.mapname, localize_autosave_));
				}
			} else {
				te.set_string(1, map_filename(gamedata.filename, gamedata.mapname, localize_autosave_));
			}
		} catch (const std::exception& e) {
			std::string errormessage = e.what();
			boost::replace_all(errormessage, "\n", "<br>");
			gamedata.errormessage =
			   ((boost::format("<p>%s</p><p>%s</p><p>%s</p>"))
			    /** TRANSLATORS: Error message introduction for when an old savegame can't be loaded */
			    % _("This file has the wrong format and can’t be loaded."
			        " Maybe it was created with an older version of Widelands.")
			    /** TRANSLATORS: This text is on a separate line with an error message below */
			    % _("Error message:") % errormessage)
			      .str();

			gamedata.mapname = FileSystem::filename_without_ext(gamedata.filename.c_str());
			games_data_.push_back(gamedata);

			UI::Table<uintptr_t const>::EntryRecord& te = table_.add(games_data_.size() - 1);
			te.set_string(0, "");
			if (filetype_ != FileType::kGameSinglePlayer) {
				te.set_string(1, "");
				/** TRANSLATORS: Prefix for incompatible files in load game screens */
				te.set_string(2, (boost::format(_("Incompatible: %s")) % gamedata.mapname).str());
			} else {
				te.set_string(1, (boost::format(_("Incompatible: %s")) % gamedata.mapname).str());
			}
		}
	}
	table_.sort();
	table_.focus();
}

void LoadOrSaveGame::set_show_filenames(bool show_filenames) {
	if (filetype_ != FileType::kReplay) {
		return;
	}
	show_filenames_ = show_filenames;
}
