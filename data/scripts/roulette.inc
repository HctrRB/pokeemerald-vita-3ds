Roulette_EventScript_Table1::
	checkitem ITEM_COIN_CASE
	goto_if_eq VAR_RESULT, FALSE, MauvilleCity_GameCorner_EventScript_NoCoinCase
	setvar VAR_0x8004, 0
	getpokenewsactive POKENEWS_GAME_CORNER
	goto_if_eq VAR_RESULT, FALSE, Roulette_EventScript_Play
	addvar VAR_0x8004, ROULETTE_SPECIAL_RATE
	goto Roulette_EventScript_Play
	end

Roulette_EventScript_Table2::
	checkitem ITEM_COIN_CASE
	goto_if_eq VAR_RESULT, FALSE, MauvilleCity_GameCorner_EventScript_NoCoinCase
	setvar VAR_0x8004, 1
	getpokenewsactive POKENEWS_GAME_CORNER
	goto_if_eq VAR_RESULT, FALSE, Roulette_EventScript_Play
	addvar VAR_0x8004, ROULETTE_SPECIAL_RATE
	goto Roulette_EventScript_Play
	end

Roulette_EventScript_Play::
	special PlayRoulette
	waitstate
	end

Roulette_Text_PlayMinimumWagerIsX::
	.string "A aposta mínima desta mesa\n"
	.string "é {STR_VAR_1}. Quer jogar?$"

Roulette_Text_NotEnoughCoins::
	.string "Você não tem FICHAS suficientes.$"

Roulette_Text_SpecialRateTable::
	.string "Mesa com prêmio especial agora!$"

Roulette_Text_ControlsInstruction::
	.string "Aposte com o direcional +\n"
	.string "e aperte o botão A.$"

Roulette_Text_ItsAHit::
	.string "Acertou!$"

Roulette_Text_Jackpot::
	.string "Bolada!$"

Roulette_Text_NothingDoing::
	.string "Não foi dessa vez!$"

Roulette_Text_YouveWonXCoins::
	.string "Você ganhou {STR_VAR_1} FICHAS!$"

Roulette_Text_NoCoinsLeft::
	.string "Acabaram as FICHAS…$"

Roulette_Text_KeepPlaying::
	.string "Continuar jogando?$"

Roulette_Text_BoardWillBeCleared::
	.string "A mesa da ROLETA vai ser limpa.$"

/* Unused*/
Roulette_Text_YouDontHaveACoinCase::
	.string "Você não tem uma SACOLA DE MOEDAS.$"

Roulette_Text_CoinCaseIsFull::
	.string "Sua SACOLA DE MOEDAS tá cheia!\n"
	.string "Troque as FICHAS por prêmios.$"
