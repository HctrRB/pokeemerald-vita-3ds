Route110_TrickHousePuzzle8_MapScripts::
	.byte 0

Route110_TrickHousePuzzle8_EventScript_Scroll::
	lockall
	goto_if_eq VAR_TRICK_HOUSE_PUZZLE_8_STATE, 0, Route110_TrickHousePuzzle8_EventScript_FoundScroll
	goto Route110_TrickHousePuzzle_EventScript_ReadScrollAgain
	end

Route110_TrickHousePuzzle8_EventScript_FoundScroll::
	setvar VAR_TRICK_HOUSE_PUZZLE_8_STATE, 1
	goto Route110_TrickHousePuzzle_EventScript_FoundScroll
	end

Route110_TrickHousePuzzle8_EventScript_Vincent::
	trainerbattle_single TRAINER_VINCENT, Route110_TrickHousePuzzle8_Text_VincentIntro, Route110_TrickHousePuzzle8_Text_VincentDefeat
	msgbox Route110_TrickHousePuzzle8_Text_VincentPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle8_EventScript_Keira::
	trainerbattle_single TRAINER_KEIRA, Route110_TrickHousePuzzle8_Text_KeiraIntro, Route110_TrickHousePuzzle8_Text_KeiraDefeat
	msgbox Route110_TrickHousePuzzle8_Text_KeiraPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle8_EventScript_Leroy::
	trainerbattle_single TRAINER_LEROY, Route110_TrickHousePuzzle8_Text_LeroyIntro, Route110_TrickHousePuzzle8_Text_LeroyDefeat
	msgbox Route110_TrickHousePuzzle8_Text_LeroyPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle8_Text_WroteSecretCodeLockOpened::
	.string "{PLAYER} escreveu a senha secreta\n"
	.string "na porta.\p"
	.string "“Eu amo o MESTRE DOS TRUQUES.”\n"
	.string "… … … … … … … …\p"
	.string "A fechadura abriu com um clique!$"

Route110_TrickHousePuzzle8_Text_VincentIntro:
	.string "Poucos TREINADORES chegaram\n"
	.string "até aqui.$"

Route110_TrickHousePuzzle8_Text_VincentDefeat:
	.string "Então você também tem força…$"

Route110_TrickHousePuzzle8_Text_VincentPostBattle:
	.string "Você venceu o CAMPEÃO da LIGA\n"
	.string "POKéMON? Aí já é demais!$"

Route110_TrickHousePuzzle8_Text_KeiraIntro:
	.string "Considere-se com sorte por batalhar\n"
	.string "comigo!$"

Route110_TrickHousePuzzle8_Text_KeiraDefeat:
	.string "Isso não está certo!\n"
	.string "Eu não posso perder!$"

Route110_TrickHousePuzzle8_Text_KeiraPostBattle:
	.string "É um milagre você ter me vencido.\n"
	.string "Pode se gabar disso.$"

Route110_TrickHousePuzzle8_Text_LeroyIntro:
	.string "Você também está encarando o desafio\n"
	.string "da CASA DOS TRUQUES.$"

Route110_TrickHousePuzzle8_Text_LeroyDefeat:
	.string "Entendo…\n"
	.string "Você tem um estilo extraordinário.$"

Route110_TrickHousePuzzle8_Text_LeroyPostBattle:
	.string "Ver alguém como você deve agradar\n"
	.string "o MESTRE DOS TRUQUES.$"

