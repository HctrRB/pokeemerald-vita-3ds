SeafloorCavern_Room4_MapScripts::
	.byte 0

SeafloorCavern_Room4_EventScript_Grunt3::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_3, SeafloorCavern_Room4_Text_Grunt3Intro, SeafloorCavern_Room4_Text_Grunt3Defeat
	msgbox SeafloorCavern_Room4_Text_Grunt3PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room4_EventScript_Grunt4::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_4, SeafloorCavern_Room4_Text_Grunt4Intro, SeafloorCavern_Room4_Text_Grunt4Defeat
	msgbox SeafloorCavern_Room4_Text_Grunt4PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room4_Text_Grunt3Intro:
	.string "Quem é você?\n"
	.string "Por onde você entrou?$"

SeafloorCavern_Room4_Text_Grunt3Defeat:
	.string "Perdi…$"

SeafloorCavern_Room4_Text_Grunt3PostBattle:
	.string "Não acho a saída!\p"
	.string "Não estou com medo. Não me entenda\n"
	.string "mal!$"

SeafloorCavern_Room4_Text_Grunt4Intro:
	.string "Quem é você?\n"
	.string "Aonde pensa que vai?$"

SeafloorCavern_Room4_Text_Grunt4Defeat:
	.string "Não consegui vencer!$"

SeafloorCavern_Room4_Text_Grunt4PostBattle:
	.string "O meu parceiro esqueceu o mapa\n"
	.string "no submarino!\p"
	.string "Dá pra ser mais inútil?$"

