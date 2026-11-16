SeafloorCavern_Room1_MapScripts::
	.byte 0

SeafloorCavern_Room1_EventScript_Grunt1::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_1, SeafloorCavern_Room1_Text_Grunt1Intro, SeafloorCavern_Room1_Text_Grunt1Defeat
	msgbox SeafloorCavern_Room1_Text_Grunt1PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room1_EventScript_Grunt2::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_2, SeafloorCavern_Room1_Text_Grunt2Intro, SeafloorCavern_Room1_Text_Grunt2Defeat
	msgbox SeafloorCavern_Room1_Text_Grunt2PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room1_Text_Grunt1Intro:
	.string "Não precisamos de criança aqui!\n"
	.string "Vai pra casa logo!$"

SeafloorCavern_Room1_Text_Grunt1Defeat:
	.string "Quero ir pra casa…$"

SeafloorCavern_Room1_Text_Grunt1PostBattle:
	.string "Quero uma promoção pra mandar nos\n"
	.string "CAPANGAS…$"

SeafloorCavern_Room1_Text_Grunt2Intro:
	.string "Aquele submarino… É apertado demais.\n"
	.string "Estou todo dolorido!$"

SeafloorCavern_Room1_Text_Grunt2Defeat:
	.string "Perder dói!$"

SeafloorCavern_Room1_Text_Grunt2PostBattle:
	.string "Aquele submarino que roubamos, cara,\n"
	.string "é uma viagem brutal.\l"
	.string "É apertado demais lá dentro!$"

