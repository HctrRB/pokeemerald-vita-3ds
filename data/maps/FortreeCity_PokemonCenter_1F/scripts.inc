FortreeCity_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, FortreeCity_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

FortreeCity_PokemonCenter_1F_OnTransition:
	setrespawn HEAL_LOCATION_FORTREE_CITY
	end

FortreeCity_PokemonCenter_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_FORTREE_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

FortreeCity_PokemonCenter_1F_EventScript_Gentleman::
	msgbox FortreeCity_PokemonCenter_1F_Text_GoToSafariZone, MSGBOX_NPC
	end

FortreeCity_PokemonCenter_1F_EventScript_Man::
	msgbox FortreeCity_PokemonCenter_1F_Text_RecordCornerIsNeat, MSGBOX_NPC
	end

FortreeCity_PokemonCenter_1F_EventScript_Boy::
	msgbox FortreeCity_PokemonCenter_1F_Text_DoYouKnowAboutPokenav, MSGBOX_NPC
	end

FortreeCity_PokemonCenter_1F_Text_GoToSafariZone:
	.string "Escuta, criança, você está\n"
	.string "completando uma POKéDEX?\p"
	.string "Hmm… Vá à ZONA DE SAFÁRI.\n"
	.string "É a minha sugestão.$"

FortreeCity_PokemonCenter_1F_Text_RecordCornerIsNeat:
	.string "Já usou o CANTO DOS RECORDES?\p"
	.string "É bem legal. Mistura e combina os\n"
	.string "recordes dos TREINADORES.\p"
	.string "Não sei bem como funciona, mas é\n"
	.string "legal. É até emocionante!$"

FortreeCity_PokemonCenter_1F_Text_DoYouKnowAboutPokenav:
	.string "Uau, você tem um POKéNAV!\n"
	.string "Igualzinho ao meu!\p"
	.string "Conhece o sistema de CHAMADAS\n"
	.string "do POKéNAV?\p"
	.string "Com ele, dá pra conversar com\n"
	.string "TREINADORES registrados à vontade.\p"
	.string "Também mostra que TREINADORES\n"
	.string "querem revanche com você.\p"
	.string "É muito prático! O pessoal da DEVON\n"
	.string "sabe mesmo o que faz!$"

