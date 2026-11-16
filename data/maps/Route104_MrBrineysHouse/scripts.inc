Route104_MrBrineysHouse_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route104_MrBrineysHouse_OnTransition
	.byte 0

Route104_MrBrineysHouse_OnTransition:
	setflag FLAG_LANDMARK_MR_BRINEY_HOUSE
	call_if_eq VAR_BRINEY_HOUSE_STATE, 1, Route104_MrBrineysHouse_EventScript_SetBrineyPeekoPos
	call_if_set FLAG_RECEIVED_POKENAV, Route104_MrBrineysHouse_EventScript_HideRustboroRival
	end

Route104_MrBrineysHouse_EventScript_HideRustboroRival::
	setflag FLAG_HIDE_RUSTBORO_CITY_RIVAL
	return

Route104_MrBrineysHouse_EventScript_SetBrineyPeekoPos::
	setobjectxyperm LOCALID_BRINEYS_HOUSE_BRINEY, 9, 3
	setobjectmovementtype LOCALID_BRINEYS_HOUSE_BRINEY, MOVEMENT_TYPE_WALK_SEQUENCE_DOWN_LEFT_UP_RIGHT
	setobjectxyperm LOCALID_BRINEYS_HOUSE_PEEKO, 9, 6
	setobjectmovementtype LOCALID_BRINEYS_HOUSE_PEEKO, MOVEMENT_TYPE_WALK_SEQUENCE_LEFT_UP_RIGHT_DOWN
	return

Route104_MrBrineysHouse_EventScript_Briney::
	lock
	faceplayer
	call_if_unset FLAG_MR_BRINEY_SAILING_INTRO, Route104_MrBrineysHouse_EventScript_SailingIntro
	goto_if_unset FLAG_DELIVERED_STEVEN_LETTER, Route104_MrBrineysHouse_EventScript_SailBothDeliveries
	goto_if_unset FLAG_DELIVERED_DEVON_GOODS, Route104_MrBrineysHouse_EventScript_SailDeliverPackage
	goto Route104_MrBrineysHouse_EventScript_WhereAreWeBound
	end

Route104_MrBrineysHouse_EventScript_SailingIntro::
	setflag FLAG_MR_BRINEY_SAILING_INTRO
	msgbox Route104_MrBrineysHouse_Text_WaitUpPeeko, MSGBOX_DEFAULT
	msgbox Route104_MrBrineysHouse_Text_ItsYouLetsSailToDewford, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Route104_MrBrineysHouse_EventScript_DeclineDeliverySailing
	goto Route104_MrBrineysHouse_EventScript_SailToDewford
	end

Route104_MrBrineysHouse_EventScript_WhereAreWeBound::
	message Route104_MrBrineysHouse_Text_WhereAreWeBound
	waitmessage
	multichoicedefault 20, 8, MULTI_BRINEY_OFF_DEWFORD, 1, FALSE
	switch VAR_RESULT
	case 0, Route104_MrBrineysHouse_EventScript_SailToDewford
	case 1, Route104_MrBrineysHouse_EventScript_DeclineSailing
	case MULTI_B_PRESSED, Route104_MrBrineysHouse_EventScript_DeclineSailing
	end

Route104_MrBrineysHouse_EventScript_SailBothDeliveries::
	msgbox Route104_MrBrineysHouse_Text_NeedToMakeDeliveriesSailToDewford, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Route104_MrBrineysHouse_EventScript_DeclineDeliverySailing
	goto Route104_MrBrineysHouse_EventScript_SailToDewford
	end

Route104_MrBrineysHouse_EventScript_SailDeliverPackage::
	msgbox Route104_MrBrineysHouse_Text_NeedToDeliverPackageSailToDewford, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Route104_MrBrineysHouse_EventScript_DeclineDeliverySailing
	goto Route104_MrBrineysHouse_EventScript_SailToDewford
	end

Route104_MrBrineysHouse_EventScript_DeclineDeliverySailing::
	msgbox Route104_MrBrineysHouse_Text_DeclineDeliverySail, MSGBOX_DEFAULT
	release
	end

Route104_MrBrineysHouse_EventScript_DeclineSailing::
	msgbox Route104_MrBrineysHouse_Text_TellMeWheneverYouWantToSail, MSGBOX_DEFAULT
	release
	end

Route104_MrBrineysHouse_EventScript_SailToDewford::
	msgbox Route104_MrBrineysHouse_Text_SetSailForDewford, MSGBOX_DEFAULT
	call EventScript_BackupMrBrineyLocation
	setvar VAR_BOARD_BRINEY_BOAT_STATE, 1
	clearflag FLAG_HIDE_ROUTE_104_MR_BRINEY
	setflag FLAG_HIDE_BRINEYS_HOUSE_MR_BRINEY
	setflag FLAG_HIDE_BRINEYS_HOUSE_PEEKO
	setvar VAR_RUSTBORO_CITY_STATE, 8
	setvar VAR_ROUTE104_STATE, 2
	setflag FLAG_HIDE_RUSTBORO_CITY_RIVAL
	setflag FLAG_HIDE_ROUTE_104_RIVAL
	warp MAP_ROUTE104, 13, 51
	waitstate
	releaseall
	end

Route104_MrBrineysHouse_EventScript_Peeko::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_WINGULL, CRY_MODE_NORMAL
	msgbox Route104_MrBrineysHouse_Text_Peeko, MSGBOX_DEFAULT
	waitmoncry
	release
	end

Route104_MrBrineysHouse_Text_WaitUpPeeko:
	.string "SR. BRINEY: Calma aí, mocinha!\n"
	.string "Me espera, PEEKO!$"

Route104_MrBrineysHouse_Text_ItsYouLetsSailToDewford:
	.string "Hm? Você é {PLAYER}{KUN}!\n"
	.string "Você salvou a minha querida PEEKO!\l"
	.string "Devemos tanto a você!\p"
	.string "Como é?\n"
	.string "Você quer navegar comigo?\p"
	.string "Hmhm…\p"
	.string "Então você tem uma CARTA para DEWFORD\n"
	.string "e um pacote para SLATEPORT?\p"
	.string "Que vida corrida você deve levar!\p"
	.string "Mas, claro, o que você está pedindo\n"
	.string "não é problema nenhum.\p"
	.string "Veio falar com a pessoa certa!\n"
	.string "Vamos zarpar para DEWFORD.$"

Route104_MrBrineysHouse_Text_SetSailForDewford:
	.string "SR. BRINEY: Então é DEWFORD!\p"
	.string "Levantar âncora!\n"
	.string "PEEKO, minha querida, vamos zarpar!$"

Route104_MrBrineysHouse_Text_DeclineDeliverySail:
	.string "SR. BRINEY: É mesmo?\n"
	.string "As entregas podem esperar?\p"
	.string "É só me avisar quando quiser\n"
	.string "zarpar!$"

Route104_MrBrineysHouse_Text_NeedToMakeDeliveriesSailToDewford:
	.string "SR. BRINEY: Ahoy!\n"
	.string "Eu sei bem o que você quer dizer!\p"
	.string "Você vai entregar uma CARTA em DEWFORD\n"
	.string "e um pacote em SLATEPORT.\p"
	.string "O que você precisa não é problema\n"
	.string "nenhum. Sou a pessoa certa pra isso!\p"
	.string "Primeiro, vamos zarpar para DEWFORD.$"

Route104_MrBrineysHouse_Text_NeedToDeliverPackageSailToDewford:
	.string "SR. BRINEY: Ahoy!\n"
	.string "Eu sei bem o que você quer dizer!\p"
	.string "Você vai entregar um pacote ao\n"
	.string "CAP. STERN, em SLATEPORT.\p"
	.string "O que você precisa não é problema\n"
	.string "nenhum. Sou a pessoa certa pra isso!\p"
	.string "Primeiro, vamos zarpar para DEWFORD.$"

Route104_MrBrineysHouse_Text_WhereAreWeBound:
	.string "SR. BRINEY: Ahoy!\n"
	.string "Por você, eu vou pro mar quando for!\p"
	.string "Agora, meu amigo, para onde vamos?$"

Route104_MrBrineysHouse_Text_TellMeWheneverYouWantToSail:
	.string "SR. BRINEY: É mesmo?\n"
	.string "Bom, a PEEKO deve a vida a você.\p"
	.string "É só me avisar quando quiser\n"
	.string "zarpar!$"

Route104_MrBrineysHouse_Text_Peeko:
	.string "PEEKO: Piii piiriu!$"

