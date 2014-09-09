/*
 * Copyright (C) 2002-2004, 2006-2010 by the Widelands Development Team
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

#include "game_io/game_player_info_data_packet.h"

#include "io/fileread.h"
#include "io/filewrite.h"
#include "logic/constants.h"
#include "logic/game.h"
#include "logic/game_data_error.h"
#include "logic/player.h"
#include "logic/tribe.h"
#include "wui/interactive_player.h"

namespace Widelands {

#define CURRENT_PACKET_VERSION 15


void Game_Player_Info_Data_Packet::Read
	(FileSystem & fs, Game & game, MapMapObjectLoader *)
{
	try {
		FileRead fr;
		fr.Open(fs, "binary/player_info");
		uint16_t const packet_version = fr.Unsigned16();
		if (5 <= packet_version && packet_version <= CURRENT_PACKET_VERSION) {
			uint32_t const max_players = fr.Unsigned16();
			for (uint32_t i = 1; i < max_players + 1; ++i) {
				game.remove_player(i);
				if (fr.Unsigned8()) {
					bool const see_all = fr.Unsigned8();

					int32_t const plnum = fr.Unsigned8();
					if (plnum < 1 || MAX_PLAYERS < plnum)
						throw GameDataError
							("player number (%i) is out of range (1 .. %u)",
							 plnum, MAX_PLAYERS);
					Widelands::TeamNumber team = 0;
					if (packet_version >= 9)
						team = fr.Unsigned8();

					char const * const tribe_name = fr.CString();

					std::string const name = fr.CString();

					game.add_player(plnum, 0, tribe_name, name, team);
					Player & player = game.player(plnum);
					player.set_see_all(see_all);

					player.setAI(fr.CString());

					if (packet_version >= 15)
						player.ReadStatistics(fr, 3);
					else if (packet_version >= 14)
						player.ReadStatistics(fr, 2);
					else if (packet_version >= 12)
						player.ReadStatistics(fr, 1);
					else
						player.ReadStatistics(fr, 0);

					player.m_casualties = fr.Unsigned32();
					player.m_kills      = fr.Unsigned32();
					player.m_msites_lost         = fr.Unsigned32();
					player.m_msites_defeated     = fr.Unsigned32();
					player.m_civil_blds_lost     = fr.Unsigned32();
					player.m_civil_blds_defeated = fr.Unsigned32();
				}
			}

			if (packet_version <= 10)
				game.ReadStatistics(fr, 3);
			else
				game.ReadStatistics(fr, 4);
		} else
			throw GameDataError
				("unknown/unhandled version %u", packet_version);
	} catch (const WException & e) {
		throw GameDataError("player info: %s", e.what());
	}
}


void Game_Player_Info_Data_Packet::Write
	(FileSystem & fs, Game & game, MapMapObjectSaver *)
{
	FileWrite fw;

	// Now packet version
	fw.Unsigned16(CURRENT_PACKET_VERSION);

	// Number of (potential) players
	Player_Number const nr_players = game.map().get_nrplayers();
	fw.Unsigned16(nr_players);
	iterate_players_existing_const(p, nr_players, game, plr) {
		fw.Unsigned8(1); // Player is in game.

		fw.Unsigned8(plr->m_see_all);

		fw.Unsigned8(plr->m_plnum);
		fw.Unsigned8(plr->team_number());

		fw.CString(plr->tribe().name().c_str());

		// Seen fields is in a map packet
		// Allowed buildings is in a map packet

		// Economies are in a packet after map loading

		fw.CString(plr->m_name.c_str());
		fw.CString(plr->m_ai.c_str());

		plr->WriteStatistics(fw);
		fw.Unsigned32(plr->casualties());
		fw.Unsigned32(plr->kills     ());
		fw.Unsigned32(plr->msites_lost        ());
		fw.Unsigned32(plr->msites_defeated    ());
		fw.Unsigned32(plr->civil_blds_lost    ());
		fw.Unsigned32(plr->civil_blds_defeated());
	} else
		fw.Unsigned8(0); //  Player is NOT in game.

	game.WriteStatistics(fw);

	fw.Write(fs, "binary/player_info");
}

}
