MagmaHideout_2F_1R_MapScripts::
	.byte 0

MagmaHideout_2F_1R_EventScript_Grunt14::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_14, MagmaHideout_2F_1R_Text_Grunt14Intro, MagmaHideout_2F_1R_Text_Grunt14Defeat
	msgbox MagmaHideout_2F_1R_Text_Grunt14PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_1R_EventScript_Grunt3::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_3, MagmaHideout_2F_1R_Text_Grunt3Intro, MagmaHideout_2F_1R_Text_Grunt3Defeat
	msgbox MagmaHideout_2F_1R_Text_Grunt3PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_1R_EventScript_Grunt4::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_4, MagmaHideout_2F_1R_Text_Grunt4Intro, MagmaHideout_2F_1R_Text_Grunt4Defeat
	msgbox MagmaHideout_2F_1R_Text_Grunt4PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_1R_EventScript_Grunt5::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_5, MagmaHideout_2F_1R_Text_Grunt5Intro, MagmaHideout_2F_1R_Text_Grunt5Defeat
	msgbox MagmaHideout_2F_1R_Text_Grunt5PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_1R_Text_Grunt14Intro:
	.string "Quê, quê, quê?\p"
	.string "Só membros da EQUIPE MAGMA podem\n"
	.string "ficar aqui!\p"
	.string "Mas você não está com o uniforme da\n"
	.string "EQUIPE MAGMA…\p"
	.string "Que suspeito!\n"
	.string "Batalhe comigo!$"

MagmaHideout_2F_1R_Text_Grunt14Defeat:
	.string "Aiaiai…\n"
	.string "O meu orgulho de membro da MAGMA…$"

MagmaHideout_2F_1R_Text_Grunt14PostBattle:
	.string "Se você sofre de calafrios, devia\n"
	.string "entrar pra EQUIPE MAGMA.$"

MagmaHideout_2F_1R_Text_Grunt3Intro:
	.string "Pare aí!\p"
	.string "Acha mesmo que vou ficar de boca\n"
	.string "fechada e te deixar passar?$"

MagmaHideout_2F_1R_Text_Grunt3Defeat:
	.string "Uau!\n"
	.string "Me rendo!$"

MagmaHideout_2F_1R_Text_Grunt3PostBattle:
	.string "Talvez fosse melhor ter te deixado\n"
	.string "passar sem desafio…$"

MagmaHideout_2F_1R_Text_Grunt4Intro:
	.string "Arrá!\n"
	.string "Um intruso!$"

MagmaHideout_2F_1R_Text_Grunt4Defeat:
	.string "Graaah!$"

MagmaHideout_2F_1R_Text_Grunt4PostBattle:
	.string "Eu já perdi.\p"
	.string "Preciso mesmo continuar correndo\n"
	.string "em círculos?$"

MagmaHideout_2F_1R_Text_Grunt5Intro:
	.string "Opa, opa!\n"
	.string "Um intruso!$"

MagmaHideout_2F_1R_Text_Grunt5Defeat:
	.string "Resmungo…$"

MagmaHideout_2F_1R_Text_Grunt5PostBattle:
	.string "Na verdade, não sou bom de batalha…$"

