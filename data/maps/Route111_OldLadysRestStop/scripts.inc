Route111_OldLadysRestStop_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route111_OldLadysRestStop_OnTransition
	.byte 0

Route111_OldLadysRestStop_OnTransition:
	setflag FLAG_LANDMARK_OLD_LADY_REST_SHOP
	end

Route111_OldLadysRestStop_EventScript_OldLady::
	lock
	faceplayer
	msgbox Route111_OldLadysRestStop_Text_RestUpHere, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Route111_OldLadysRestStop_EventScript_Rest
	goto_if_eq VAR_RESULT, NO, Route111_OldLadysRestStop_EventScript_DeclineRest
	end

Route111_OldLadysRestStop_EventScript_Rest::
	msgbox Route111_OldLadysRestStop_Text_TakeYourTimeRestUp, MSGBOX_DEFAULT
	closemessage
	call Common_EventScript_OutOfCenterPartyHeal
	msgbox Route111_OldLadysRestStop_Text_StillTiredTakeAnotherRest, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Route111_OldLadysRestStop_EventScript_Rest
	goto_if_eq VAR_RESULT, NO, Route111_OldLadysRestStop_EventScript_DeclineRest
	end

Route111_OldLadysRestStop_EventScript_DeclineRest::
	msgbox Route111_OldLadysRestStop_Text_DontNeedToBeShy, MSGBOX_DEFAULT
	release
	end

Route111_OldLadysRestStop_Text_RestUpHere:
	.string "Ai, ai, ai.\n"
	.string "Os seus POKéMON não estão exaustos?\p"
	.string "Se quiser, descanse aqui.\n"
	.string "É uma ótima ideia! Faça isso.$"

Route111_OldLadysRestStop_Text_TakeYourTimeRestUp:
	.string "Isso mesmo.\n"
	.string "Descanse com calma!$"

Route111_OldLadysRestStop_Text_StillTiredTakeAnotherRest:
	.string "Ai, ai, ai.\n"
	.string "Os seus POKéMON ainda estão cansados?\p"
	.string "Descanse mais um pouco aqui.\n"
	.string "É uma ótima ideia. Faça isso.$"

Route111_OldLadysRestStop_Text_DontNeedToBeShy:
	.string "Ah, é?\n"
	.string "Não precisa ficar com vergonha.$"

