DewfordTown_House1_MapScripts::
	.byte 0

DewfordTown_House1_EventScript_Man::
	msgbox DewfordTown_House1_Text_LotToBeSaidForLivingOnIsland, MSGBOX_NPC
	end

DewfordTown_House1_EventScript_Woman::
	msgbox DewfordTown_House1_Text_LifeGoesSlowlyOnIsland, MSGBOX_NPC
	end

DewfordTown_House1_EventScript_Zigzagoon::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZIGZAGOON, CRY_MODE_NORMAL
	msgbox DewfordTown_House1_Text_Zigzagoon, MSGBOX_DEFAULT
	waitmoncry
	release
	end

DewfordTown_House1_Text_LotToBeSaidForLivingOnIsland:
	.string "Viver numa ilhinha como esta, em\n"
	.string "harmonia com os POKéMON e a família,\l"
	.string "tem muitas vantagens.$"

DewfordTown_House1_Text_LifeGoesSlowlyOnIsland:
	.string "Saí do grande porto de SLATEPORT\n"
	.string "quando me casei com meu marido aqui.\p"
	.string "A vida passa devagar nesta ilhinha.\n"
	.string "Mas viver cercada por este mar\l"
	.string "lindo também é felicidade.$"

DewfordTown_House1_Text_Zigzagoon:
	.string "ZIGZAGOON: Guguuh!$"

