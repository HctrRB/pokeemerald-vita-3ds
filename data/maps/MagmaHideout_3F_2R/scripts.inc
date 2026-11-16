MagmaHideout_3F_2R_MapScripts::
	.byte 0

MagmaHideout_3F_2R_EventScript_Grunt10::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_10, MagmaHideout_3F_2R_Text_Grunt10Intro, MagmaHideout_3F_2R_Text_Grunt10Defeat
	msgbox MagmaHideout_3F_2R_Text_Grunt10PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_3F_2R_Text_Grunt10Intro:
	.string "Entendo tudo o que o nosso líder diz.\n"
	.string "Mas sabe de uma coisa?\p"
	.string "Desenterrar um POKéMON superantigo\n"
	.string "e roubar o METEORITO dos outros…\p"
	.string "Acho que a gente está passando\n"
	.string "dos limites. O que você acha?$"

MagmaHideout_3F_2R_Text_Grunt10Defeat:
	.string "É, acho que tem alguma coisa errada\n"
	.string "no que a gente faz.$"

MagmaHideout_3F_2R_Text_Grunt10PostBattle:
	.string "Sabe, perder pra você clareou as\n"
	.string "minhas ideias.\p"
	.string "Da próxima vez que vir o nosso líder,\n"
	.string "vou perguntar o que a gente faz.$"

