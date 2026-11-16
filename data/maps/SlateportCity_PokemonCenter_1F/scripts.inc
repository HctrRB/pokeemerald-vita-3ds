SlateportCity_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, SlateportCity_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

SlateportCity_PokemonCenter_1F_OnTransition:
	setrespawn HEAL_LOCATION_SLATEPORT_CITY
	call Common_EventScript_UpdateBrineyLocation
	end

SlateportCity_PokemonCenter_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_SLATEPORT_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

SlateportCity_PokemonCenter_1F_EventScript_Sailor::
	msgbox SlateportCity_PokemonCenter_1F_Text_RaiseDifferentTypesOfPokemon, MSGBOX_NPC
	end

SlateportCity_PokemonCenter_1F_EventScript_Woman::
	msgbox SlateportCity_PokemonCenter_1F_Text_TradedMonWithFriend, MSGBOX_NPC
	end

SlateportCity_PokemonCenter_1F_Text_RaiseDifferentTypesOfPokemon:
	.string "Quer uma dica pra batalhar?\p"
	.string "Eu diria que é criar tipos diferentes\n"
	.string "de POKéMON, de forma equilibrada.\p"
	.string "Não adianta deixar só um\n"
	.string "POKéMON forte.\p"
	.string "Se ele estiver em desvantagem de tipo,\n"
	.string "pode não ter a menor chance.$"

SlateportCity_PokemonCenter_1F_Text_TradedMonWithFriend:
	.string "Eu troco POKéMON com os meus amigos.\p"
	.string "Se o POKéMON da troca vem segurando\n"
	.string "um item, fico duas vezes mais feliz!$"

