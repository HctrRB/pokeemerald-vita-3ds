Route110_TrickHousePuzzle6_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route110_TrickHousePuzzle6_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, Route110_TrickHousePuzzle6_OnWarp
	.byte 0

Route110_TrickHousePuzzle6_OnTransition:
	special RotatingGate_InitPuzzle
	end

Route110_TrickHousePuzzle6_OnWarp:
	map_script_2 VAR_TEMP_0, VAR_TEMP_0, Route110_TrickHousePuzzle6_EventScript_InitPuzzle
	.short 0

@ NOTE: This rotating gate puzzle makes use of VAR_TEMP_0 - VAR_TEMP_5
Route110_TrickHousePuzzle6_EventScript_InitPuzzle::
	special RotatingGate_InitPuzzleAndGraphics
	end

Route110_TrickHousePuzzle6_EventScript_Scroll::
	lockall
	goto_if_eq VAR_TRICK_HOUSE_PUZZLE_6_STATE, 0, Route110_TrickHousePuzzle6_EventScript_FoundScroll
	goto Route110_TrickHousePuzzle_EventScript_ReadScrollAgain
	end

Route110_TrickHousePuzzle6_EventScript_FoundScroll::
	setvar VAR_TRICK_HOUSE_PUZZLE_6_STATE, 1
	goto Route110_TrickHousePuzzle_EventScript_FoundScroll
	end

Route110_TrickHousePuzzle6_EventScript_Sophia::
	trainerbattle_single TRAINER_SOPHIA, Route110_TrickHousePuzzle6_Text_SophiaIntro, Route110_TrickHousePuzzle6_Text_SophiaDefeat
	msgbox Route110_TrickHousePuzzle6_Text_SophiaPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle6_EventScript_Benny::
	trainerbattle_single TRAINER_BENNY, Route110_TrickHousePuzzle6_Text_BennyIntro, Route110_TrickHousePuzzle6_Text_BennyDefeat
	msgbox Route110_TrickHousePuzzle6_Text_BennyPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle6_EventScript_Sebastian::
	trainerbattle_single TRAINER_SEBASTIAN, Route110_TrickHousePuzzle6_Text_SebastianIntro, Route110_TrickHousePuzzle6_Text_SebastianDefeat
	msgbox Route110_TrickHousePuzzle6_Text_SebastianPostBattle, MSGBOX_AUTOCLOSE
	end

Route110_TrickHousePuzzle6_Text_WroteSecretCodeLockOpened:
	.string "{PLAYER} escreveu a senha secreta\n"
	.string "na porta.\p"
	.string "“O MESTRE DOS TRUQUES é a minha vida.”\n"
	.string "… … … … … … … …\p"
	.string "A fechadura abriu com um clique!$"

Route110_TrickHousePuzzle6_Text_SophiaIntro:
	.string "Quando soube que tinha uma casa\n"
	.string "esquisita, tive que vir conferir.$"

Route110_TrickHousePuzzle6_Text_SophiaDefeat:
	.string "Descobri alguém que treina muito bem!$"

Route110_TrickHousePuzzle6_Text_SophiaPostBattle:
	.string "Estou me divertindo muito explorando\n"
	.string "este lugar.\p"
	.string "É um desafio que vale a pena\n"
	.string "repetir!$"

Route110_TrickHousePuzzle6_Text_BennyIntro:
	.string "Talvez o meu POKéMON PÁSSARO\n"
	.string "consiga voar por cima do muro…$"

Route110_TrickHousePuzzle6_Text_BennyDefeat:
	.string "Gwaaah! Estraguei tudo!$"

Route110_TrickHousePuzzle6_Text_BennyPostBattle:
	.string "Hehehe… Acho que perdi porque\n"
	.string "estava tentando trapacear.$"

Route110_TrickHousePuzzle6_Text_SebastianIntro:
	.string "Estou ficando tonto com essas portas\n"
	.string "giratórias…$"

Route110_TrickHousePuzzle6_Text_SebastianDefeat:
	.string "Está tudo girando e girando.\n"
	.string "Não aguento mais…$"

Route110_TrickHousePuzzle6_Text_SebastianPostBattle:
	.string "Você parece nem ligar.\n"
	.string "Ou está fazendo cara de paisagem?$"

