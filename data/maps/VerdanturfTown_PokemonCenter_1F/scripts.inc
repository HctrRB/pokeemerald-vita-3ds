VerdanturfTown_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, VerdanturfTown_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

VerdanturfTown_PokemonCenter_1F_OnTransition:
	setrespawn HEAL_LOCATION_VERDANTURF_TOWN
	call Common_EventScript_UpdateBrineyLocation
	end

VerdanturfTown_PokemonCenter_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_VERDANTURF_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

VerdanturfTown_PokemonCenter_1F_EventScript_Gentleman::
	msgbox VerdanturfTown_PokemonCenter_1F_Text_FaithInYourPokemon, MSGBOX_NPC
	end

VerdanturfTown_PokemonCenter_1F_EventScript_ExpertM::
	msgbox VerdanturfTown_PokemonCenter_1F_Text_VisitForBattleTent, MSGBOX_NPC
	end

VerdanturfTown_PokemonCenter_1F_Text_FaithInYourPokemon:
	.string "Ninguém treina POKéMON de verdade\n"
	.string "se não tiver fé nos\l"
	.string "seus POKéMON.\p"
	.string "Só quem acredita nos POKéMON que\n"
	.string "batalham consegue vencer até\l"
	.string "o fim.$"

VerdanturfTown_PokemonCenter_1F_Text_VisitForBattleTent:
	.string "O motivo de alguém visitar\n"
	.string "VERDANTURF…\p"
	.string "É a TENDA DE BATALHA. Nem\n"
	.string "precisa dizer.\p"
	.string "Ou você veio ver alguém\n"
	.string "aqui?$"

