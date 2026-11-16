Route119_House_MapScripts::
	.byte 0

Route119_House_EventScript_Woman::
	msgbox Route119_House_Text_RumorAboutCaveOfOrigin, MSGBOX_NPC
	end

Route119_House_EventScript_Wingull::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_WINGULL, CRY_MODE_NORMAL
	msgbox Route119_House_Text_Wingull, MSGBOX_DEFAULT
	waitmoncry
	release
	end

Route119_House_Text_RumorAboutCaveOfOrigin:
	.string "Ouvi falar de uma caverna chamada\n"
	.string "CAVERNA DA ORIGEM.\p"
	.string "Dizem que o espírito dos POKéMON\n"
	.string "revive lá. Será que algo assim\l"
	.string "pode mesmo acontecer?$"

Route119_House_Text_Wingull:
	.string "WINGULL: Pihyoh!$"

