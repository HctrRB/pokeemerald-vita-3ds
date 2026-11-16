MossdeepCity_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MossdeepCity_OnTransition
	.byte 0

MossdeepCity_OnTransition:
	clearflag FLAG_MOSSDEEP_GYM_SWITCH_1
	clearflag FLAG_MOSSDEEP_GYM_SWITCH_2
	clearflag FLAG_MOSSDEEP_GYM_SWITCH_3
	clearflag FLAG_MOSSDEEP_GYM_SWITCH_4
	call_if_set FLAG_SYS_WEATHER_CTRL, Common_EventScript_SetAbnormalWeather
	end

MossdeepCity_EventScript_PokefanF::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_HM_DIVE, MossdeepCity_EventScript_PokefanFMagmaGone
	msgbox MossdeepCity_Text_SpaceCenterReceivedLetter, MSGBOX_DEFAULT
	release
	end

MossdeepCity_EventScript_PokefanFMagmaGone::
	msgbox MossdeepCity_Text_SpaceCenterLaunchingRockets, MSGBOX_DEFAULT
	release
	end

MossdeepCity_EventScript_Sailor::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_HM_DIVE, MossdeepCity_EventScript_SailorMagmaGone
	msgbox MossdeepCity_Text_MossdeepTargetedByMagma, MSGBOX_DEFAULT
	release
	end

MossdeepCity_EventScript_SailorMagmaGone::
	msgbox MossdeepCity_Text_FeelReliefOnLand, MSGBOX_DEFAULT
	release
	end

MossdeepCity_EventScript_NinjaBoy::
	msgbox MossdeepCity_Text_WailmerWatching, MSGBOX_NPC
	end

MossdeepCity_EventScript_ExpertM::
	msgbox MossdeepCity_Text_LifeNeedsSeaToLive, MSGBOX_NPC
	end

MossdeepCity_EventScript_Girl::
	msgbox MossdeepCity_Text_NiceIfWorldCoveredByFlowers, MSGBOX_NPC
	end

MossdeepCity_EventScript_Woman::
	msgbox MossdeepCity_Text_SpecialSpaceCenterRock, MSGBOX_NPC
	end

MossdeepCity_EventScript_WhiteRock::
	msgbox MossdeepCity_Text_ItsAWhiteRock, MSGBOX_SIGN
	end

MossdeepCity_EventScript_GymSign::
	msgbox MossdeepCity_Text_GymSign, MSGBOX_SIGN
	end

MossdeepCity_EventScript_CitySign::
	msgbox MossdeepCity_Text_CitySign, MSGBOX_SIGN
	end

MossdeepCity_EventScript_SpaceCenterSign::
	msgbox MossdeepCity_Text_SpaceCenterSign, MSGBOX_SIGN
	end

MossdeepCity_EventScript_VisitedMossdeep::
	setflag FLAG_VISITED_MOSSDEEP_CITY
	setvar VAR_TEMP_1, 1
	end

MossdeepCity_EventScript_TeamMagmaEnterSpaceCenter::
	lockall
	applymovement LOCALID_MOSSDEEP_MAXIE, MossdeepCity_Movement_MaxieGestureToSpaceCenter
	waitmovement 0
	applymovement LOCALID_MOSSDEEP_GRUNT_1, MossdeepCity_Movement_GruntFaceSpaceCenter
	applymovement LOCALID_MOSSDEEP_GRUNT_2, MossdeepCity_Movement_GruntFaceSpaceCenter
	applymovement LOCALID_MOSSDEEP_GRUNT_3, MossdeepCity_Movement_GruntFaceSpaceCenter
	applymovement LOCALID_MOSSDEEP_GRUNT_4, MossdeepCity_Movement_GruntFaceSpaceCenter
	waitmovement 0
	applymovement LOCALID_MOSSDEEP_MAXIE, MossdeepCity_Movement_MaxieEnterSpaceCenter
	applymovement LOCALID_MOSSDEEP_GRUNT_1, MossdeepCity_Movement_Grunt1EnterSpaceCenter
	applymovement LOCALID_MOSSDEEP_GRUNT_2, MossdeepCity_Movement_Grunt2EnterSpaceCenter
	applymovement LOCALID_MOSSDEEP_GRUNT_3, MossdeepCity_Movement_Grunt3EnterSpaceCenter
	applymovement LOCALID_MOSSDEEP_GRUNT_4, MossdeepCity_Movement_Grunt4EnterSpaceCenter
	waitmovement 0
	removeobject LOCALID_MOSSDEEP_MAXIE
	removeobject LOCALID_MOSSDEEP_GRUNT_1
	removeobject LOCALID_MOSSDEEP_GRUNT_2
	removeobject LOCALID_MOSSDEEP_GRUNT_3
	removeobject LOCALID_MOSSDEEP_GRUNT_4
	delay 30
	setvar VAR_MOSSDEEP_CITY_STATE, 2
	setflag FLAG_HIDE_MOSSDEEP_CITY_TEAM_MAGMA
	releaseall
	end

MossdeepCity_Movement_MaxieGestureToSpaceCenter:
	delay_16
	face_right
	delay_16
	delay_16
	delay_16
	delay_16
	face_left
	delay_16
	delay_16
	step_end

MossdeepCity_Movement_GruntFaceSpaceCenter:
	face_right
	delay_16
	delay_16
	delay_16
	step_end

MossdeepCity_Movement_MaxieEnterSpaceCenter:
	walk_down
	walk_right
	walk_right
	walk_down
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

MossdeepCity_Movement_Grunt1EnterSpaceCenter:
	delay_16
	delay_8
	walk_down
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_down
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

MossdeepCity_Movement_Grunt2EnterSpaceCenter:
	delay_16
	delay_8
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_down
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

MossdeepCity_Movement_Grunt3EnterSpaceCenter:
	delay_16
	delay_8
	walk_down
	walk_right
	walk_right
	walk_right
	walk_down
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

MossdeepCity_Movement_Grunt4EnterSpaceCenter:
	delay_16
	delay_8
	walk_right
	walk_right
	walk_right
	walk_down
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

MossdeepCity_EventScript_Man::
	lock
	faceplayer
	msgbox MossdeepCity_Text_SurfExhilarating, MSGBOX_DEFAULT
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

MossdeepCity_EventScript_KingsRockBoy::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_KINGS_ROCK, MossdeepCity_EventScript_ReceivedKingsRock
	msgbox MossdeepCity_Text_WantKingsRockStevenGaveMe, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MossdeepCity_EventScript_DeclineKingsRock
	msgbox MossdeepCity_Text_YouCanKeepIt, MSGBOX_DEFAULT
	giveitem ITEM_KINGS_ROCK
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_KINGS_ROCK
	release
	end

MossdeepCity_EventScript_ReceivedKingsRock::
	msgbox MossdeepCity_Text_StevensHouseOverThere, MSGBOX_DEFAULT
	release
	end

MossdeepCity_EventScript_DeclineKingsRock::
	msgbox MossdeepCity_Text_WhatToDoWithWeirdRock, MSGBOX_DEFAULT
	release
	end

MossdeepCity_EventScript_BlackBelt::
	msgbox MossdeepCity_Text_SootopolisNewGymLeader, MSGBOX_NPC
	end

MossdeepCity_EventScript_Scott::
	lock
	faceplayer
	msgbox MossdeepCity_Text_ScottSomethingWrongWithTown, MSGBOX_DEFAULT
	closemessage
	call_if_eq VAR_FACING, DIR_NORTH, MossdeepCity_EventScript_ScottExitNorth
	call_if_eq VAR_FACING, DIR_EAST, MossdeepCity_EventScript_ScottExitEast
	addvar VAR_SCOTT_STATE, 1
	removeobject LOCALID_MOSSDEEP_SCOTT
	release
	end

MossdeepCity_EventScript_ScottExitNorth::
	applymovement LOCALID_PLAYER, MossdeepCity_Movement_PlayerWatchScottExit
	applymovement LOCALID_MOSSDEEP_SCOTT, MossdeepCity_Movement_ScottExitNorth
	waitmovement 0
	return

MossdeepCity_EventScript_ScottExitEast::
	applymovement LOCALID_PLAYER, MossdeepCity_Movement_PlayerWatchScottExit
	applymovement LOCALID_MOSSDEEP_SCOTT, MossdeepCity_Movement_ScottExitEast
	waitmovement 0
	return

MossdeepCity_Movement_PlayerWatchScottExit:
	delay_16
	delay_16
	walk_in_place_faster_left
	step_end

MossdeepCity_Movement_ScottExitNorth:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	step_end

MossdeepCity_Movement_ScottExitEast:
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	step_end

MossdeepCity_Text_WantKingsRockStevenGaveMe:
	.string "Ganhei isto do STEVEN, mas não sei\n"
	.string "pra que serve.\p"
	.string "Acho que se chama PEDRA DO REI.\n"
	.string "Você quer?$"

MossdeepCity_Text_YouCanKeepIt:
	.string "Pra que você quer isso?\n"
	.string "Que esquisitice.\p"
	.string "Pode ficar, mas não conte pro\n"
	.string "STEVEN.$"

MossdeepCity_Text_StevensHouseOverThere:
	.string "A casa do STEVEN é logo ali!$"

MossdeepCity_Text_WhatToDoWithWeirdRock:
	.string "É, você também acha! O que alguém\n"
	.string "vai fazer com uma pedra esquisita?$"

MossdeepCity_Text_WailmerWatching:
	.string "Perto de MOSSDEEP dá pra ver\n"
	.string "WAILMER selvagens.\p"
	.string "Isso se chama, hã…\n"
	.string "Como era mesmo…\p"
	.string "WAI, WAI, WAI…\p"
	.string "Observação de WAILMER!$"

MossdeepCity_Text_SpaceCenterReceivedLetter:
	.string "O CENTRO ESPACIAL da ilha anda\n"
	.string "lançando foguetes enormes.\p"
	.string "Deu um alvoroço por causa de uma\n"
	.string "carta que eles receberam há pouco.$"

MossdeepCity_Text_SpaceCenterLaunchingRockets:
	.string "O CENTRO ESPACIAL da ilha anda\n"
	.string "lançando foguetes enormes.$"

MossdeepCity_Text_MossdeepTargetedByMagma:
	.string "Um colega MARINHEIRO me contou que a\n"
	.string "EQUIPE AQUA se instalou em LILYCOVE.\p"
	.string "E também que alguém apareceu e\n"
	.string "acabou com eles!\p"
	.string "Mas aqui em MOSSDEEP o alvo é\n"
	.string "aquela EQUIPE MAGMA.\p"
	.string "Se quiser saber o que eles tramam,\n"
	.string "vá ao CENTRO ESPACIAL.$"

MossdeepCity_Text_FeelReliefOnLand:
	.string "Sou MARINHEIRO, então o mar é mais\n"
	.string "importante pra mim, claro.\p"
	.string "Mas sabe? Quando volto pra terra\n"
	.string "depois de uma viagem longa, que alívio!$"

MossdeepCity_Text_NiceIfWorldCoveredByFlowers:
	.string "Não seria bom?\p"
	.string "Se o mundo todo fosse coberto de\n"
	.string "plantas e flores como esta ilha?$"

MossdeepCity_Text_LifeNeedsSeaToLive:
	.string "Toda vida precisa do mar pra viver,\n"
	.string "mesmo morando em terra.\p"
	.string "A vida, quando chega ao fim, vira\n"
	.string "solo e volta pra terra.\p"
	.string "E o mar está sempre ligado à terra.\p"
	.string "Sim, como esta praia aqui.$"

MossdeepCity_Text_SurfExhilarating:
	.string "Uma viagem de navio é boa.\p"
	.string "Mas atravessar o mar com POKéMON\n"
	.string "usando SURFAR…\p"
	.string "Isso sim é uma viagem empolgante!\n"
	.string "Não concorda, jovem?$"

MossdeepCity_Text_SpecialSpaceCenterRock:
	.string "Esta pedra é especial pro pessoal\n"
	.string "do CENTRO ESPACIAL.\p"
	.string "Eles botaram aqui como um pedido\n"
	.string "pra que os foguetes voem em segurança.\p"
	.string "Quando você faz um pedido, o que usa?\n"
	.string "Pede pra uma estrela?\p"
	.string "Eu uso uma fitinha da sorte.\n"
	.string "É assim que eu faço.$"

MossdeepCity_Text_ItsAWhiteRock:
	.string "É uma pedra branca.$"

MossdeepCity_Text_GymSign:
	.string "GINÁSIO POKéMON DE MOSSDEEP\n"
	.string "LÍDERES: LIZA & TATE\p"
	.string "“A combinação mística!”$"

MossdeepCity_Text_CitySign:
	.string "CIDADE DE MOSSDEEP\n"
	.string "“O nosso lema: valorize os POKéMON!”$"

MossdeepCity_Text_SpaceCenterSign:
	.string "CENTRO ESPACIAL DE MOSSDEEP\n"
	.string "“O lugar mais perto do espaço.”$"

MossdeepCity_Text_ScottSomethingWrongWithTown:
	.string "SCOTT: {PLAYER}{KUN}, tudo bem?\n"
	.string "Eu estou ótimo!\p"
	.string "Ouvi dizer que o LÍDER do GINÁSIO\n"
	.string "de MOSSDEEP é bem forte, então\l"
	.string "vim dar uma olhada.\p"
	.string "Mas tem algo estranho nesta cidade.\p"
	.string "O pessoal só fala de uma carta de\n"
	.string "aviso e do CENTRO ESPACIAL…\p"
	.string "Mas acho que isso não tem nada a\n"
	.string "ver comigo.$"

MossdeepCity_Text_SootopolisNewGymLeader:
	.string "Ahh… Que delícia deixar as ondas\n"
	.string "baterem nos pés…\p"
	.string "Falando em ondas, conhece aquela\n"
	.string "cidade-ilha, SOOTOPOLIS?\p"
	.string "Acho que o GINÁSIO de lá ganhou\n"
	.string "um LÍDER novo.\p"
	.string "Dizem que o novo LÍDER já foi\n"
	.string "mestre do WALLACE.$"

