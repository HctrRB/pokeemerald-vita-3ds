MagmaHideout_2F_2R_MapScripts::
	.byte 0

MagmaHideout_2F_2R_EventScript_Grunt15::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_15, MagmaHideout_2F_2R_Text_Grunt15Intro, MagmaHideout_2F_2R_Text_Grunt15Defeat
	msgbox MagmaHideout_2F_2R_Text_Grunt15PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_2R_EventScript_Grunt6::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_6, MagmaHideout_2F_2R_Text_Grunt6Intro, MagmaHideout_2F_2R_Text_Grunt6Defeat
	msgbox MagmaHideout_2F_2R_Text_Grunt6PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_2R_EventScript_Grunt7::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_7, MagmaHideout_2F_2R_Text_Grunt7Intro, MagmaHideout_2F_2R_Text_Grunt7Defeat
	msgbox MagmaHideout_2F_2R_Text_Grunt7PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_2R_EventScript_Grunt8::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_8, MagmaHideout_2F_2R_Text_Grunt8Intro, MagmaHideout_2F_2R_Text_Grunt8Defeat
	msgbox MagmaHideout_2F_2R_Text_Grunt8PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_2R_Text_Grunt15Intro:
	.string "Não tenho nada contra você.\n"
	.string "Só estou cumprindo ordens!$"

MagmaHideout_2F_2R_Text_Grunt15Defeat:
	.string "Posso ter perdido, mas…$"

MagmaHideout_2F_2R_Text_Grunt15PostBattle:
	.string "Desenterramos uma coisa inacreditável!\n"
	.string "E pegamos o ORBE AZUL!\p"
	.string "Agora só falta o nosso líder…\n"
	.string "Fufufu… Fwahahaha!$"

MagmaHideout_2F_2R_Text_Grunt6Intro:
	.string "Não aguento calor. Talvez eu devesse\n"
	.string "ter entrado na EQUIPE AQUA…$"

MagmaHideout_2F_2R_Text_Grunt6Defeat:
	.string "É, acho que não nasci pra\n"
	.string "EQUIPE MAGMA…$"

MagmaHideout_2F_2R_Text_Grunt6PostBattle:
	.string "Num lugar desses, você não sente\n"
	.string "saudade do mar?$"

MagmaHideout_2F_2R_Text_Grunt7Intro:
	.string "Às vezes dá pra sentir tremores aqui.\p"
	.string "Será o vulcão sacudindo?\n"
	.string "Ou será o GROU…\p"
	.string "Opa!\n"
	.string "Não, não, esquece!$"

MagmaHideout_2F_2R_Text_Grunt7Defeat:
	.string "Você batalha com fogo.\n"
	.string "Igual a um vulcão!$"

MagmaHideout_2F_2R_Text_Grunt7PostBattle:
	.string "Você ganhou por sorte desta vez,\n"
	.string "mas isso não importa.\p"
	.string "O objetivo da EQUIPE MAGMA está\n"
	.string "quase cumprido!$"

MagmaHideout_2F_2R_Text_Grunt8Intro:
	.string "Um dos nossos estava surtando porque\n"
	.string "perdeu o EMBLEMA MAGMA…\p"
	.string "Pera aí!\n"
	.string "Foi você que achou?$"

MagmaHideout_2F_2R_Text_Grunt8Defeat:
	.string "Não consigo acreditar nisso…$"

MagmaHideout_2F_2R_Text_Grunt8PostBattle:
	.string "Estou com a sensação de que o nosso\n"
	.string "plano vai fracassar…$"

