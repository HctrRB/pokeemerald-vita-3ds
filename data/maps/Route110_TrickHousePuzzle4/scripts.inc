Route110_TrickHousePuzzle4_MapScripts::
	.byte 0

Route110_TrickHousePuzzle4_EventScript_Scroll::
	lockall
	goto_if_eq VAR_TRICK_HOUSE_PUZZLE_4_STATE, 0, Route110_TrickHousePuzzle4_EventScript_FoundScroll
	goto Route110_TrickHousePuzzle_EventScript_ReadScrollAgain
	end

Route110_TrickHousePuzzle4_EventScript_FoundScroll::
	setvar VAR_TRICK_HOUSE_PUZZLE_4_STATE, 1
	goto Route110_TrickHousePuzzle_EventScript_FoundScroll
	end

Route110_TrickHousePuzzle4_EventScript_Cora::
	trainerbattle_single TRAINER_CORA, Route110_TrickHousePuzzle4_Text_CoraIntro, Route110_TrickHousePuzzle4_Text_CoraDefeat
	msgbox Route110_TrickHousePuzzle4_Text_CoraPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle4_EventScript_Yuji::
	trainerbattle_single TRAINER_YUJI, Route110_TrickHousePuzzle4_Text_YujiIntro, Route110_TrickHousePuzzle4_Text_YujiDefeat
	msgbox Route110_TrickHousePuzzle4_Text_YujiPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle4_EventScript_Paula::
	trainerbattle_single TRAINER_PAULA, Route110_TrickHousePuzzle4_Text_PaulaIntro, Route110_TrickHousePuzzle4_Text_PaulaDefeat
	msgbox Route110_TrickHousePuzzle4_Text_PaulaPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle4_Text_WroteSecretCodeLockOpened:
	.string "{PLAYER} escreveu a senha secreta\n"
	.string "na porta.\p"
	.string "“O MESTRE DOS TRUQUES é demais.”\n"
	.string "… … … … … … … …\p"
	.string "A fechadura abriu com um clique!$"

Route110_TrickHousePuzzle4_Text_CoraIntro:
	.string "Dá muito trabalho pensar nisso.\n"
	.string "Eu só queria batalhar!$"

Route110_TrickHousePuzzle4_Text_CoraDefeat:
	.string "Mesmo perdendo, o que eu mais gosto\n"
	.string "ainda é batalhar!$"

Route110_TrickHousePuzzle4_Text_CoraPostBattle:
	.string "Não concorda? A gente vai a qualquer\n"
	.string "lugar se tiver TREINADORES lá.$"

Route110_TrickHousePuzzle4_Text_YujiIntro:
	.string "Heh! Pedras assim eu tiro do caminho\n"
	.string "com um dedo!$"

Route110_TrickHousePuzzle4_Text_YujiDefeat:
	.string "Consigo empurrar pedras, mas não\n"
	.string "consigo resolver o enigma…$"

Route110_TrickHousePuzzle4_Text_YujiPostBattle:
	.string "Não basta ser forte…\n"
	.string "Tem que usar a cabeça. Ser esperto!$"

Route110_TrickHousePuzzle4_Text_PaulaIntro:
	.string "A CASA DOS TRUQUES está cada vez\n"
	.string "mais complicada, né?$"

Route110_TrickHousePuzzle4_Text_PaulaDefeat:
	.string "Aaak!$"

Route110_TrickHousePuzzle4_Text_PaulaPostBattle:
	.string "Alguém já chegou ao fim?$"

