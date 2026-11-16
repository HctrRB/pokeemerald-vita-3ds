MagmaHideout_3F_1R_MapScripts::
	.byte 0

MagmaHideout_3F_1R_EventScript_Grunt9::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_9, MagmaHideout_3F_1R_Text_Grunt9Intro, MagmaHideout_3F_1R_Text_Grunt9Defeat
	msgbox MagmaHideout_3F_1R_Text_Grunt9PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_3F_1R_EventScript_Grunt16::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_16, MagmaHideout_3F_1R_Text_Grunt16Intro, MagmaHideout_3F_1R_Text_Grunt16Defeat
	msgbox MagmaHideout_3F_1R_Text_Grunt16PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_3F_1R_Text_Grunt9Intro:
	.string "O que eu fiz pra merecer essa guarda?\p"
	.string "A minha orelha esquerda está\n"
	.string "pegando fogo!$"

MagmaHideout_3F_1R_Text_Grunt9Defeat:
	.string "Estou com insolação…$"

MagmaHideout_3F_1R_Text_Grunt9PostBattle:
	.string "Não é estranho usar capuz num\n"
	.string "vulcão cheio de magma?$"

MagmaHideout_3F_1R_Text_Grunt16Intro:
	.string "Entramos pra ajudar o nosso líder a\n"
	.string "realizar o sonho fantástico dele.\p"
	.string "Não importa se você é da EQUIPE AQUA\n"
	.string "ou só uma criança de passagem.\p"
	.string "Ninguém atrapalha a gente e sai\n"
	.string "impune!$"

MagmaHideout_3F_1R_Text_Grunt16Defeat:
	.string "Ah, não!\n"
	.string "Não dá pra confiar em você!$"

MagmaHideout_3F_1R_Text_Grunt16PostBattle:
	.string "Escute aqui.\n"
	.string "A EQUIPE MAGMA está certa!\p"
	.string "Não escute a EQUIPE AQUA.\n"
	.string "Não acredite nas mentiras deles!$"

