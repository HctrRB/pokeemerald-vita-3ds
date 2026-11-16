RustboroCity_DevonCorp_3F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, RustboroCity_DevonCorp_3F_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, RustboroCity_DevonCorp_3F_OnWarp
	map_script MAP_SCRIPT_ON_FRAME_TABLE, RustboroCity_DevonCorp_3F_OnFrame
	.byte 0

RustboroCity_DevonCorp_3F_OnTransition:
	call_if_eq VAR_DEVON_CORP_3F_STATE, 0, RustboroCity_DevonCorp_3F_EventScript_SetEmployeePos
	end

RustboroCity_DevonCorp_3F_EventScript_SetEmployeePos::
	setobjectxyperm LOCALID_DEVON_CORP_3F_EMPLOYEE, 3, 2
	setobjectmovementtype LOCALID_DEVON_CORP_3F_EMPLOYEE, MOVEMENT_TYPE_FACE_LEFT
	return

RustboroCity_DevonCorp_3F_OnWarp:
	map_script_2 VAR_DEVON_CORP_3F_STATE, 0, RustboroCity_DevonCorp_3F_EventScript_PlayerFaceEast
	.short 0

RustboroCity_DevonCorp_3F_EventScript_PlayerFaceEast::
	turnobject LOCALID_PLAYER, DIR_EAST
	end

RustboroCity_DevonCorp_3F_OnFrame:
	map_script_2 VAR_DEVON_CORP_3F_STATE, 0, RustboroCity_DevonCorp_3F_EventScript_MeetPresident
	.short 0

RustboroCity_DevonCorp_3F_EventScript_MeetPresident::
	lockall
	msgbox RustboroCity_DevonCorp_3F_Text_ThisIs3rdFloorWaitHere, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_DEVON_CORP_3F_EMPLOYEE, RustboroCity_DevonCorp_3F_Movement_EmployeeWalkOffscreen
	waitmovement 0
	delay 80
	applymovement LOCALID_DEVON_CORP_3F_EMPLOYEE, RustboroCity_DevonCorp_3F_Movement_EmployeeReturnToPlayer
	waitmovement 0
	msgbox RustboroCity_DevonCorp_3F_Text_WordWithPresidentComeWithMe, MSGBOX_DEFAULT
	closemessage
	playbgm MUS_FOLLOW_ME, FALSE
	applymovement LOCALID_DEVON_CORP_3F_EMPLOYEE, RustboroCity_DevonCorp_3F_Movement_LeadPlayerToPresident
	applymovement LOCALID_PLAYER, RustboroCity_DevonCorp_3F_Movement_PlayerFollowToPresident
	waitmovement 0
	msgbox RustboroCity_DevonCorp_3F_Text_PleaseGoAhead, MSGBOX_DEFAULT
	closemessage
	fadedefaultbgm
	applymovement LOCALID_DEVON_CORP_3F_EMPLOYEE, RustboroCity_DevonCorp_3F_Movement_EmployeeFaceDesk
	applymovement LOCALID_PLAYER, RustboroCity_DevonCorp_3F_Movement_PlayerApproachDesk
	waitmovement 0
	msgbox RustboroCity_DevonCorp_3F_Text_MrStoneIHaveFavor, MSGBOX_DEFAULT
	giveitem ITEM_LETTER
	msgbox RustboroCity_DevonCorp_3F_Text_MrStoneWantYouToHaveThis, MSGBOX_DEFAULT
	playfanfare MUS_OBTAIN_ITEM
	message RustboroCity_DevonCorp_3F_Text_ReceivedPokenav
	waitfanfare
	setflag FLAG_SYS_POKENAV_GET
	setflag FLAG_RECEIVED_POKENAV
	setflag FLAG_HIDE_RUSTBORO_CITY_POKEMON_SCHOOL_SCOTT
	msgbox RustboroCity_DevonCorp_3F_Text_MrStoneExplainPokenavRestUp, MSGBOX_DEFAULT
	closemessage
	call Common_EventScript_OutOfCenterPartyHeal
	msgbox RustboroCity_DevonCorp_3F_Text_MrStoneGoWithCautionAndCare, MSGBOX_DEFAULT
	setflag FLAG_HIDE_ROUTE_116_WANDAS_BOYFRIEND
	clearflag FLAG_HIDE_RUSTURF_TUNNEL_WANDAS_BOYFRIEND
	clearflag FLAG_HIDE_RUSTURF_TUNNEL_WANDA
	clearflag FLAG_HIDE_BRINEYS_HOUSE_MR_BRINEY
	clearflag FLAG_HIDE_BRINEYS_HOUSE_PEEKO
	setvar VAR_BRINEY_LOCATION, 1
	clearflag FLAG_HIDE_RUSTBORO_CITY_RIVAL
	setvar VAR_DEVON_CORP_3F_STATE, 1
	setvar VAR_RUSTBORO_CITY_STATE, 6
	releaseall
	end

RustboroCity_DevonCorp_3F_Movement_Unused:
	walk_up
	walk_up
	walk_up
	walk_in_place_faster_left
	step_end

RustboroCity_DevonCorp_3F_Movement_LeadPlayerToPresident:
	walk_right
	walk_right
	walk_right
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_in_place_faster_left
	step_end

RustboroCity_DevonCorp_3F_Movement_EmployeeFaceDesk:
	delay_16
	walk_in_place_faster_down
	step_end

RustboroCity_DevonCorp_3F_Movement_EmployeeWalkOffscreen:
	walk_right
	walk_right
	walk_right
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

RustboroCity_DevonCorp_3F_Movement_EmployeeReturnToPlayer:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_up
	walk_left
	walk_left
	walk_left
	step_end

RustboroCity_DevonCorp_3F_Movement_PlayerFollowToPresident:
	walk_right
	walk_right
	walk_right
	walk_right
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

RustboroCity_DevonCorp_3F_Movement_PlayerApproachDesk:
	delay_16
	delay_16
	walk_down
	walk_down
	walk_in_place_faster_right
	step_end

RustboroCity_DevonCorp_3F_EventScript_MrStone::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_EXP_SHARE, RustboroCity_DevonCorp_3F_EventScript_MrStoneAfterFavor
	goto_if_set FLAG_DELIVERED_STEVEN_LETTER, RustboroCity_DevonCorp_3F_EventScript_GiveExpShare
	msgbox RustboroCity_DevonCorp_3F_Text_CountingOnYou, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

RustboroCity_DevonCorp_3F_EventScript_GiveExpShare::
	msgbox RustboroCity_DevonCorp_3F_Text_ThankYouForDeliveringLetter, MSGBOX_DEFAULT
	giveitem ITEM_EXP_SHARE
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_EXP_SHARE
	msgbox RustboroCity_DevonCorp_3F_Text_ExplainExpShare, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

RustboroCity_DevonCorp_3F_EventScript_MrStoneAfterFavor::
	msgbox RustboroCity_DevonCorp_3F_Text_NotFamiliarWithTrends, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

RustboroCity_DevonCorp_3F_EventScript_Employee::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_REPEAT_BALL, RustboroCity_DevonCorp_3F_EventScript_EmployeeBalls
	msgbox RustboroCity_DevonCorp_3F_Text_VisitCaptSternShipyard, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_3F_EventScript_EmployeeBalls::
	msgbox RustboroCity_DevonCorp_3F_Text_RepeatAndTimerHugelyPopular, MSGBOX_DEFAULT
	release
	end

RustboroCity_DevonCorp_3F_EventScript_RareRocksDisplay::
	msgbox RustboroCity_DevonCorp_3F_Text_RareRocksDisplay, MSGBOX_SIGN
	end

RustboroCity_DevonCorp_3F_Text_MrStoneIHaveFavor:
	.string "Eu sou o SR. STONE, PRESIDENTE da\n"
	.string "CORPORAÇÃO DEVON.\p"
	.string "Acabei de saber de você!\p"
	.string "Você salvou o nosso funcionário não\n"
	.string "uma, mas duas vezes!\p"
	.string "Tenho um favor a pedir a uma pessoa\n"
	.string "incrível como você.\p"
	.string "Soube que você vai entregar um pacote\n"
	.string "no ESTALEIRO de SLATEPORT.\p"
	.string "Bom, no caminho, poderia passar\n"
	.string "pela CIDADE DE DEWFORD?\p"
	.string "Eu queria que você entregasse uma\n"
	.string "CARTA ao STEVEN, em DEWFORD.$"

RustboroCity_DevonCorp_3F_Text_MrStoneWantYouToHaveThis:
	.string "SR. STONE: Agora, saiba que eu sou\n"
	.string "um grande PRESIDENTE.\p"
	.string "Eu jamais seria pão-duro a ponto de\n"
	.string "pedir um favor sem dar nada em troca.\p"
	.string "Por isso, quero que fique com isto!$"

RustboroCity_DevonCorp_3F_Text_ReceivedPokenav:
	.string "{PLAYER} recebeu um POKéNAV.$"

RustboroCity_DevonCorp_3F_Text_MrStoneExplainPokenavRestUp:
	.string "SR. STONE: Esse aparelho…\p"
	.string "É um NAVEGADOR POKéMON, ou POKéNAV,\n"
	.string "pra encurtar.\p"
	.string "É uma ferramenta indispensável pra\n"
	.string "qualquer aventura com POKéMON.\p"
	.string "Ele tem um mapa da região de HOENN.\p"
	.string "Dá pra ver fácil onde ficam\n"
	.string "DEWFORD e SLATEPORT!\p"
	.string "Aliás, ouvi dizer que uns criminosos\n"
	.string "sinistros, MAGMA e AQUA, eu acho,\l"
	.string "estão causando problemas por aí.\p"
	.string "Acho melhor você descansar\n"
	.string "antes de seguir viagem.$"

RustboroCity_DevonCorp_3F_Text_MrStoneGoWithCautionAndCare:
	.string "SR. STONE: Bom, então, {PLAYER}{KUN},\n"
	.string "vá com cuidado e atenção!$"

RustboroCity_DevonCorp_3F_Text_CountingOnYou:
	.string "SR. STONE: Conto com você!$"

RustboroCity_DevonCorp_3F_Text_ThankYouForDeliveringLetter:
	.string "SR. STONE: Você entregou a minha CARTA?\n"
	.string "Muito obrigado!\p"
	.string "É o meu jeito de agradecer.\n"
	.string "Deve ajudar você nos treinos.$"

RustboroCity_DevonCorp_3F_Text_ExplainExpShare:
	.string "SR. STONE: Um POKéMON segurando esse\n"
	.string "COMPARTILHADOR DE EXP. recebe parte\l"
	.string "da EXP das batalhas.\p"
	.string "Ele ganha EXP mesmo sem ter\n"
	.string "batalhado de verdade.\p"
	.string "Eu diria que o COMPARTILHADOR DE EXP.\n"
	.string "é bem útil pra treinar POKéMON fracos.$"

RustboroCity_DevonCorp_3F_Text_NotFamiliarWithTrends:
	.string "SR. STONE: Desde jovem, mergulhei\n"
	.string "no trabalho.\p"
	.string "Por isso, não entendo muito de\n"
	.string "modas e dessas coisas.\p"
	.string "Mas os jovens todos querem treinar\n"
	.string "POKéMON e ir pra LIGA POKéMON?$"

RustboroCity_DevonCorp_3F_Text_ThisIs3rdFloorWaitHere:
	.string "Este é o terceiro andar da\n"
	.string "CORPORAÇÃO DEVON.\p"
	.string "O ESCRITÓRIO do nosso PRESIDENTE\n"
	.string "fica neste andar.\p"
	.string "Enfim, nem sei dizer o quanto sou\n"
	.string "grato pelo que você fez.\p"
	.string "Hum… Aliás, aquele pacote que\n"
	.string "você recuperou pra gente…\p"
	.string "Você poderia entregá-lo no\n"
	.string "ESTALEIRO de SLATEPORT?\p"
	.string "Seria horrível se aqueles ladrões\n"
	.string "tentassem pegar de novo.\p"
	.string "Hã… Hehehe, então, hã, posso contar\n"
	.string "com você pra isso?\p"
	.string "Ah, é mesmo.\n"
	.string "Pode esperar aqui um segundo?$"

RustboroCity_DevonCorp_3F_Text_WordWithPresidentComeWithMe:
	.string "O nosso PRESIDENTE quer dar\n"
	.string "uma palavrinha com você.\p"
	.string "Venha comigo, por favor.$"

RustboroCity_DevonCorp_3F_Text_PleaseGoAhead:
	.string "Por favor, pode entrar.$"

RustboroCity_DevonCorp_3F_Text_VisitCaptSternShipyard:
	.string "Se for ao ESTALEIRO de SLATEPORT,\n"
	.string "procure o CAP. STERN.$"

RustboroCity_DevonCorp_3F_Text_RepeatAndTimerHugelyPopular:
	.string "Os novos produtos da DEVON, a BOLA\n"
	.string "REPETIÇÃO e a BOLA TEMPO, fizeram\l"
	.string "muito sucesso entre os TREINADORES.$"

RustboroCity_DevonCorp_3F_Text_RareRocksDisplay:
	.string "É uma coleção de pedras e rochas\n"
	.string "raras reunidas pelo PRESIDENTE.$"

