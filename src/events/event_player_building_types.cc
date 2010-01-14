/*
 * Copyright (C) 2002-2004, 2006, 2008-2010 by the Widelands Development Team
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "event_player_building_types.h"

#include "logic/game_data_error.h"
#include "logic/editor_game_base.h"
#include "logic/tribe.h"
#include "profile/profile.h"
#include "wui/interactive_base.h"

#define EVENT_VERSION 3

namespace Widelands {

Event_Player_Building_Types::Event_Player_Building_Types
	(Section & s, Editor_Game_Base & egbase, Tribe_Descr const * tribe)
	: Event(s)
{
	try {
		uint32_t const packet_version = s.get_safe_positive("version");
		if (packet_version <= EVENT_VERSION) {
			if (not tribe) {
				Map const & map = egbase.map();
				m_player = s.get_Player_Number("player", map.get_nrplayers());
				egbase.get_ibase()->reference_player_tribe(m_player, this);
				tribe =
					&egbase.manually_load_tribe
						(map.get_scenario_player_tribe(m_player));
			}
			while (Section::Value const * const v = s.get_next_val("building"))
				if
					(not
					 m_building_types.insert
					 	(tribe->safe_building_index(v->get_string()))
					 .second)
					throw game_data_error
						(_("\"%s\" is duplicated"), v->get_string());
			if (m_building_types.empty())
				throw game_data_error(_("no building types"));
		} else
			throw game_data_error
				(_("unknown/unhandled version %u"), packet_version);
	} catch (_wexception const & e) {
		throw game_data_error(_("(player building types): %s"), e.what());
	}
}

void Event_Player_Building_Types::Write
	(Section & s, Editor_Game_Base const & egbase, Map_Map_Object_Saver const &)
	const
{
	s.set_int           ("version",  EVENT_VERSION);
	if (m_player != 1)
		s.set_int("player",  m_player);
	Tribe_Descr const & tribe =
		*egbase.get_tribe
			(egbase.map().get_scenario_player_tribe(m_player).c_str());
	container_iterate_const(Building_Types, m_building_types, i)
		s.set_string_duplicate
			("building", tribe.get_building_descr(*i.current)->name());
}

}
