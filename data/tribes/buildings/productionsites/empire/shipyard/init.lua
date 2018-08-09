dirname = path.dirname(__file__)

tribes:new_productionsite_type {
   msgctxt = "empire_building",
   name = "empire_shipyard",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("empire_building", "Shipyard"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   size = "medium",
   needs_seafaring = true,

   buildcost = {
      log = 3,
      planks = 2,
      granite = 3,
      cloth = 2
   },
   return_on_dismantle = {
      log = 1,
      granite = 2,
      cloth = 1
   },

   animations = {
      idle = {
         pictures = path.list_files(dirname .. "idle_??.png"),
         hotspot = { 50, 63 },
      },
      build = {
         pictures = path.list_files(dirname .. "build_??.png"),
         hotspot = { 50, 63 },
      },
      unoccupied = {
         pictures = path.list_files(dirname .. "unoccupied_??.png"),
         hotspot = { 50, 63 },
      },
      working = {
         pictures = path.list_files(dirname .. "working_??.png"),
         hotspot = { 50, 63 },
      },
   },

   aihints = {
      needs_water = true,
      shipyard = true,
      prohibited_till = 1050
   },

   working_positions = {
      empire_shipwright = 1
   },

   inputs = {
      { name = "log", amount = 2 },
      { name = "planks", amount = 10 },
      { name = "cloth", amount = 4 }
   },

   programs = {
      work = {
         -- TRANSLATORS: Completed/Skipped/Did not start working because ...
         descname = _"working",
         actions = {
            -- best time total: 1243.2
            -- worst time total: 1732.8
            -- average: 1488
            "sleep=20000",
            "call=ship",
            "return=skipped"
         }
      },
      ship = {
         -- TRANSLATORS: Completed/Skipped/Did not start constructing a ship because ...
         descname = _"constructing a ship",
         actions = {
            "checkmap=seafaring",
            "construct=empire_shipconstruction buildship 6",
            "animate=working 35000",
            "return=completed"
         }
      },
   },
}
