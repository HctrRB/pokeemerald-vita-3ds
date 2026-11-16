FortreeCity_House5_MapScripts::
	.byte 0

FortreeCity_House5_EventScript_PokefanF::
	msgbox FortreeCity_House5_Text_TreeHousesAreGreat, MSGBOX_NPC
	end

FortreeCity_House5_EventScript_Man::
	msgbox FortreeCity_House5_Text_AdaptedToNature, MSGBOX_NPC
	end

FortreeCity_House5_EventScript_Zigzagoon::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZIGZAGOON, CRY_MODE_NORMAL
	msgbox FortreeCity_House5_Text_Zigzagoon, MSGBOX_DEFAULT
	waitmoncry
	release
	end

FortreeCity_House5_Text_TreeHousesAreGreat:
	.string "As casas nas árvores de FORTREE são\n"
	.string "demais!\p"
	.string "Acho que é a melhor cidade pra\n"
	.string "morar junto com os POKéMON.$"

FortreeCity_House5_Text_AdaptedToNature:
	.string "POKéMON e pessoas se adaptaram à\n"
	.string "natureza pra sobreviver.\p"
	.string "Não precisa mudar a natureza pro\n"
	.string "jeito que a gente quer viver.$"

FortreeCity_House5_Text_Zigzagoon:
	.string "ZIGZAGOON: Bufuu!$"

