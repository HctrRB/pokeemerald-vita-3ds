Route114_FossilManiacsHouse_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route114_FossilManiacsHouse_OnTransition
	.byte 0

Route114_FossilManiacsHouse_OnTransition:
	setflag FLAG_LANDMARK_FOSSIL_MANIACS_HOUSE
	end

Route114_FossilManiacsHouse_EventScript_FossilManiacsBrother::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_TM_DIG, Route114_FossilManiacsHouse_EventScript_ReceivedDig
	msgbox Route114_FossilManiacsHouse_Text_HaveThisToDigLikeMyBrother, MSGBOX_DEFAULT
	giveitem ITEM_TM_DIG
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM_DIG
	release
	end

Route114_FossilManiacsHouse_EventScript_ReceivedDig::
	msgbox Route114_FossilManiacsHouse_Text_DigReturnsYouToEntrance, MSGBOX_DEFAULT
	release
	end

Route114_FossilManiacsHouse_EventScript_RockDisplay::
	msgbox Route114_FossilManiacsHouse_Text_RocksFillDisplayCase, MSGBOX_SIGN
	end

Route114_FossilManiacsHouse_EventScript_Bookshelf::
	msgbox Route114_FossilManiacsHouse_Text_CrammedWithBooks, MSGBOX_SIGN
	end

Route114_FossilManiacsHouse_Text_HaveThisToDigLikeMyBrother:
	.string "O meu irmão mais velho é o MANÍACO\n"
	.string "POR FÓSSEIS… Um cara legal que ama\l"
	.string "FÓSSEIS…\p"
	.string "Ele também adora cavar buracos…\n"
	.string "Cavou este buraco sozinho…\p"
	.string "Pode ficar com isto, pra CAVAR\n"
	.string "buracos igual ao meu irmão…$"

Route114_FossilManiacsHouse_Text_DigReturnsYouToEntrance:
	.string "Se um POKéMON usar CAVAR dentro de\n"
	.string "uma caverna, você volta pra entrada…$"

Route114_FossilManiacsHouse_Text_RocksFillDisplayCase:
	.string "Pedras de formas estranhas enchem\n"
	.string "a vitrine…$"

Route114_FossilManiacsHouse_Text_CrammedWithBooks:
	.string "A COMPOSIÇÃO DOS ESTRATOS…\n"
	.string "COMO A CHUVA MOLDA A TERRA…\l"
	.string "PEDRAS, SOLO E ROCHA…\p"
	.string "Está abarrotada de livros.$"

