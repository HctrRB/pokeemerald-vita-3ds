FortreeCity_House2_MapScripts::
	.byte 0

FortreeCity_House2_EventScript_HiddenPowerGiver::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_TM_HIDDEN_POWER, FortreeCity_House2_EventScript_ExplainHiddenPower
	call_if_unset FLAG_MET_HIDDEN_POWER_GIVER, FortreeCity_House2_EventScript_Greeting
	msgbox FortreeCity_House2_Text_CoinInWhichHand, MSGBOX_DEFAULT
	multichoice 21, 8, MULTI_RIGHTLEFT, TRUE
	switch VAR_RESULT
	case 1, FortreeCity_House2_EventScript_WrongGuess
	msgbox FortreeCity_House2_Text_CorrectTryAgainWhichHand, MSGBOX_DEFAULT
	multichoice 21, 8, MULTI_RIGHTLEFT, TRUE
	switch VAR_RESULT
	case 1, FortreeCity_House2_EventScript_WrongGuess
	msgbox FortreeCity_House2_Text_CorrectTryAgainWhichHand2, MSGBOX_DEFAULT
	multichoice 21, 8, MULTI_RIGHTLEFT, TRUE
	switch VAR_RESULT
	case 0, FortreeCity_House2_EventScript_WrongGuess
	msgbox FortreeCity_House2_Text_YourHiddenPowerHasAwoken, MSGBOX_DEFAULT
	giveitem ITEM_TM_HIDDEN_POWER
	goto_if_eq VAR_RESULT, 0, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM_HIDDEN_POWER
	msgbox FortreeCity_House2_Text_ExplainHiddenPower, MSGBOX_DEFAULT
	release
	end

FortreeCity_House2_EventScript_Greeting::
	msgbox FortreeCity_House2_Text_HiddenPowersArousedByNature, MSGBOX_DEFAULT
	setflag FLAG_MET_HIDDEN_POWER_GIVER
	return

FortreeCity_House2_EventScript_ExplainHiddenPower::
	msgbox FortreeCity_House2_Text_ExplainHiddenPower, MSGBOX_DEFAULT
	release
	end

FortreeCity_House2_EventScript_WrongGuess::
	msgbox FortreeCity_House2_Text_YouGuessedWrong, MSGBOX_DEFAULT
	release
	end

FortreeCity_House2_Text_HiddenPowersArousedByNature:
	.string "Pessoas… POKéMON…\p"
	.string "Os poderes ocultos despertam\n"
	.string "vivendo em meio à natureza…$"

FortreeCity_House2_Text_CoinInWhichHand:
	.string "Deixe esta velha ver se o seu poder\n"
	.string "oculto despertou…\p"
	.string "Tenho uma moeda na mão.\p"
	.string "Agora diga: escondi a moeda na mão\n"
	.string "direita? Ou na esquerda?$"

FortreeCity_House2_Text_CorrectTryAgainWhichHand:
	.string "Oh! Isso, acertou!\p"
	.string "Vamos de novo.\p"
	.string "Em que mão escondi a moeda?\n"
	.string "Direita ou esquerda?$"

FortreeCity_House2_Text_CorrectTryAgainWhichHand2:
	.string "Oh! Isso, acertou!\p"
	.string "Vamos de novo.\p"
	.string "Em que mão escondi a moeda?\n"
	.string "Direita ou esquerda?$"

FortreeCity_House2_Text_YourHiddenPowerHasAwoken:
	.string "Oh! Esplêndido!\n"
	.string "O seu poder oculto despertou!\p"
	.string "Tome, use isto para despertar o\n"
	.string "poder oculto dos seus POKéMON.$"

FortreeCity_House2_Text_ExplainHiddenPower:
	.string "PODER OCULTO é um golpe que muda\n"
	.string "conforme o POKéMON.$"

FortreeCity_House2_Text_YouGuessedWrong:
	.string "Não, que pena.\n"
	.string "Você errou.$"

