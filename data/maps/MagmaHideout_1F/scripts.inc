MagmaHideout_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MagmaHideout_1F_OnTransition
	.byte 0

MagmaHideout_1F_OnTransition:
	setvar VAR_JAGGED_PASS_ASH_WEATHER, 0
	end

MagmaHideout_1F_EventScript_Grunt1::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_1, MagmaHideout_1F_Text_Grunt1Intro, MagmaHideout_1F_Text_Grunt1Defeat
	msgbox MagmaHideout_1F_Text_Grunt1PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_1F_EventScript_Grunt2::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_2, MagmaHideout_1F_Text_Grunt2Intro, MagmaHideout_1F_Text_Grunt2Defeat
	msgbox MagmaHideout_1F_Text_Grunt2PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_1F_Text_Grunt1Intro:
	.string "Quando a EQUIPE MAGMA faz a chamada,\n"
	.string "a gente pega as guardas importantes\l"
	.string "na ordem da fila.\p"
	.string "Por isso me largaram neste canto.\n"
	.string "Eu sempre chego atrasado na chamada!$"

MagmaHideout_1F_Text_Grunt1Defeat:
	.string "Também chego atrasado nos treinos!\p"
	.string "Odeio admitir, mas sou fracote…$"

MagmaHideout_1F_Text_Grunt1PostBattle:
	.string "Tá, vou tentar me esforçar um\n"
	.string "pouquinho mais daqui pra frente…$"

MagmaHideout_1F_Text_Grunt2Intro:
	.string "O nosso líder mandou a gente cavar\n"
	.string "o MONTE CHIMNEY, e a gente cavou.\p"
	.string "E, no meio da escavação, achamos\n"
	.string "uma coisa de cair o queixo!\p"
	.string "O que a gente achou?\p"
	.string "Fuhahaha!\n"
	.string "Conto se você me vencer!$"

MagmaHideout_1F_Text_Grunt2Defeat:
	.string "Argh!\n"
	.string "Derrubado!$"

MagmaHideout_1F_Text_Grunt2PostBattle:
	.string "Mudei de ideia, não vou contar.\n"
	.string "Você descobre quando chegar lá!\p"
	.string "Surpresa é melhor no final,\n"
	.string "não acha?$"

