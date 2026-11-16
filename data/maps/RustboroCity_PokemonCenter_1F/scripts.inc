RustboroCity_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, RustboroCity_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

RustboroCity_PokemonCenter_1F_OnTransition:
	setrespawn HEAL_LOCATION_RUSTBORO_CITY
	call Common_EventScript_UpdateBrineyLocation
	end

RustboroCity_PokemonCenter_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_RUSTBORO_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

RustboroCity_PokemonCenter_1F_EventScript_Man::
	msgbox RustboroCity_PokemonCenter_1F_Text_PokemonHavePersonalities, MSGBOX_NPC
	end

RustboroCity_PokemonCenter_1F_EventScript_Boy::
	msgbox RustboroCity_PokemonCenter_1F_Text_MaleAndFemalePokemon, MSGBOX_NPC
	end

RustboroCity_PokemonCenter_1F_EventScript_Girl::
	msgbox RustboroCity_PokemonCenter_1F_Text_HMCutNextDoor, MSGBOX_NPC
	end

RustboroCity_PokemonCenter_1F_Text_PokemonHavePersonalities:
	.string "Meu POKéMON tem natureza INGÊNUA, e o\n"
	.string "do meu amigo tem natureza ALEGRE.\p"
	.string "É fascinante como os POKéMON têm\n"
	.string "personalidade!$"

RustboroCity_PokemonCenter_1F_Text_MaleAndFemalePokemon:
	.string "Assim como as pessoas, existem\n"
	.string "POKéMON machos e fêmeas.\p"
	.string "Mas ninguém parece saber direito\n"
	.string "qual é a diferença.$"

RustboroCity_PokemonCenter_1F_Text_HMCutNextDoor:
	.string "O homem da casa ao lado me deu uma MO!\p"
	.string "Usei pra ensinar meu POKéMON a\n"
	.string "CORTAR árvores finas.$"

