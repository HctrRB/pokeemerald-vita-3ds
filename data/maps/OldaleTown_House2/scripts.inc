OldaleTown_House2_MapScripts::
	.byte 0

OldaleTown_House2_EventScript_Woman::
	msgbox OldaleTown_House2_Text_PokemonLevelUp, MSGBOX_NPC
	end

OldaleTown_House2_EventScript_Man::
	msgbox OldaleTown_House2_Text_YoullGoFurtherWithStrongPokemon, MSGBOX_NPC
	end

OldaleTown_House2_Text_PokemonLevelUp:
	.string "Batalhando, os POKéMON acabam subindo\n"
	.string "de nível e ficando mais fortes.$"

OldaleTown_House2_Text_YoullGoFurtherWithStrongPokemon:
	.string "Se os POKéMON que estão com você\n"
	.string "ficarem mais fortes, você vai poder ir\l"
	.string "bem mais longe daqui.$"

