Route110_TrickHousePuzzle1_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, Route110_TrickHousePuzzle1_OnLoad
	.byte 0

Route110_TrickHousePuzzle1_OnLoad:
	goto_if_eq VAR_TRICK_HOUSE_PUZZLE_1_STATE, 2, Route110_TrickHousePuzzle1_EventScript_OpenDoor
	end

Route110_TrickHousePuzzle1_EventScript_OpenDoor::
	setmetatile 13, 1, METATILE_TrickHousePuzzle_Stairs_Down, FALSE
	end

Route110_TrickHousePuzzle1_EventScript_Scroll::
	lockall
	goto_if_eq VAR_TRICK_HOUSE_PUZZLE_1_STATE, 0, Route110_TrickHousePuzzle1_EventScript_FoundScroll
	goto Route110_TrickHousePuzzle_EventScript_ReadScrollAgain
	end

Route110_TrickHousePuzzle1_EventScript_FoundScroll::
	setvar VAR_TRICK_HOUSE_PUZZLE_1_STATE, 1
	goto Route110_TrickHousePuzzle_EventScript_FoundScroll
	end

Route110_TrickHousePuzzle1_EventScript_Sally::
	trainerbattle_single TRAINER_SALLY, Route110_TrickHousePuzzle1_Text_SallyIntro, Route110_TrickHousePuzzle1_Text_SallyDefeat
	msgbox Route110_TrickHousePuzzle1_Text_SallyPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle1_EventScript_Eddie::
	trainerbattle_single TRAINER_EDDIE, Route110_TrickHousePuzzle1_Text_EddieIntro, Route110_TrickHousePuzzle1_Text_EddieDefeat
	msgbox Route110_TrickHousePuzzle1_Text_EddiePostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle1_EventScript_Robin::
	trainerbattle_single TRAINER_ROBIN, Route110_TrickHousePuzzle1_Text_RobinIntro, Route110_TrickHousePuzzle1_Text_RobinDefeat
	msgbox Route110_TrickHousePuzzle1_Text_RobinPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle1_Text_WroteSecretCodeLockOpened::
	.string "{PLAYER} escreveu a senha secreta\n"
	.string "na porta.\p"
	.string "“O MESTRE DOS TRUQUES é fabuloso.”\n"
	.string "… … … … … … … …\p"
	.string "A fechadura abriu com um clique!$"

Route110_TrickHousePuzzle1_Text_SallyIntro:
	.string "Vou cortar e picar até a vitória\n"
	.string "com o CORTAR que acabamos de aprender!$"

Route110_TrickHousePuzzle1_Text_SallyDefeat:
	.string "Por que tanta seriedade?$"

Route110_TrickHousePuzzle1_Text_SallyPostBattle:
	.string "Nunca me canso de cortar\n"
	.string "e picar!$"

Route110_TrickHousePuzzle1_Text_EddieIntro:
	.string "Entrei nesta casa esquisita\n"
	.string "sem querer…$"

Route110_TrickHousePuzzle1_Text_EddieDefeat:
	.string "E agora perdi…$"

Route110_TrickHousePuzzle1_Text_EddiePostBattle:
	.string "Me perdi, perdi a batalha e agora estou\n"
	.string "mais perdido ainda… Não consigo sair…$"

Route110_TrickHousePuzzle1_Text_RobinIntro:
	.string "Quem é, afinal, o MESTRE DOS TRUQUES?$"

Route110_TrickHousePuzzle1_Text_RobinDefeat:
	.string "Perdi enquanto estava perdida\n"
	.string "nos pensamentos!$"

Route110_TrickHousePuzzle1_Text_RobinPostBattle:
	.string "Que força!\n"
	.string "Quem é você, afinal?$"

