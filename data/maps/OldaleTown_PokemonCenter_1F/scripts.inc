OldaleTown_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, OldaleTown_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

OldaleTown_PokemonCenter_1F_OnTransition:
	setrespawn HEAL_LOCATION_OLDALE_TOWN
	call Common_EventScript_UpdateBrineyLocation
	end

OldaleTown_PokemonCenter_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_OLDALE_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

OldaleTown_PokemonCenter_1F_EventScript_Gentleman::
	msgbox OldaleTown_PokemonCenter_1F_Text_TrainersCanUsePC, MSGBOX_NPC
	end

OldaleTown_PokemonCenter_1F_EventScript_Boy::
	msgbox OldaleTown_PokemonCenter_1F_Text_PokemonCentersAreGreat, MSGBOX_NPC
	end

OldaleTown_PokemonCenter_1F_EventScript_Girl::
	lock
	faceplayer
	goto_if_set FLAG_SYS_POKEDEX_GET, OldaleTown_PokemonCenter_1F_EventScript_WirelessClubAvailable
	msgbox OldaleTown_PokemonCenter_1F_Text_WirelessClubNotAvailable, MSGBOX_DEFAULT
	release
	end

OldaleTown_PokemonCenter_1F_EventScript_WirelessClubAvailable::
	msgbox OldaleTown_PokemonCenter_1F_Text_TradedInWirelessClub, MSGBOX_DEFAULT
	release
	end

OldaleTown_PokemonCenter_1F_Text_TrainersCanUsePC:
	.string "Aquele PC ali no canto é para\n"
	.string "qualquer TREINADOR POKéMON usar.\p"
	.string "Então, claro, você também pode\n"
	.string "usar à vontade.$"

OldaleTown_PokemonCenter_1F_Text_PokemonCentersAreGreat:
	.string "Os CENTROS POKéMON são ótimos!\p"
	.string "Dá para usar os serviços quanto\n"
	.string "quiser, e é tudo de graça.\l"
	.string "Não precisa se preocupar com nada!$"

OldaleTown_PokemonCenter_1F_Text_WirelessClubNotAvailable:
	.string "O CLUBE SEM FIO POKéMON do\n"
	.string "segundo andar foi construído agora.\p"
	.string "Mas dizem que ainda estão fazendo\n"
	.string "ajustes.$"

OldaleTown_PokemonCenter_1F_Text_TradedInWirelessClub:
	.string "O CLUBE SEM FIO POKéMON do\n"
	.string "segundo andar foi construído agora.\p"
	.string "Eu já troquei POKéMON lá.$"

