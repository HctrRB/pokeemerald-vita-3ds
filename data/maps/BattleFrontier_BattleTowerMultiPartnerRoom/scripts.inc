BattleFrontier_BattleTowerMultiPartnerRoom_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, BattleFrontier_BattleTowerMultiPartnerRoom_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, BattleFrontier_BattleTowerMultiPartnerRoom_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, BattleFrontier_BattleTowerMultiPartnerRoom_OnWarp
	map_script MAP_SCRIPT_ON_FRAME_TABLE, BattleFrontier_BattleTowerMultiPartnerRoom_OnFrame
	.byte 0

BattleFrontier_BattleTowerMultiPartnerRoom_OnResume:
	pyramid_resetparty
	end

BattleFrontier_BattleTowerMultiPartnerRoom_OnTransition:
	goto_if_set FLAG_CHOSEN_MULTI_BATTLE_NPC_PARTNER, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_ChosePartner
	clearflag FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_1
	clearflag FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_2
	clearflag FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_3
	clearflag FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_4
	clearflag FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_5
	clearflag FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_6
	setflag FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_ALT_1
	setflag FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_ALT_2
	tower_loadpartners
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_ChosePartner::
	setvar VAR_OBJ_GFX_ID_0, OBJ_EVENT_GFX_BOY_1
	setvar VAR_OBJ_GFX_ID_1, OBJ_EVENT_GFX_BOY_1
	setvar VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_BOY_1
	setvar VAR_OBJ_GFX_ID_3, OBJ_EVENT_GFX_BOY_1
	setvar VAR_OBJ_GFX_ID_4, OBJ_EVENT_GFX_BOY_1
	setvar VAR_OBJ_GFX_ID_5, OBJ_EVENT_GFX_BOY_1
	setvar VAR_OBJ_GFX_ID_6, OBJ_EVENT_GFX_BOY_1
	setvar VAR_OBJ_GFX_ID_7, OBJ_EVENT_GFX_BOY_1
	setvar VAR_TEMP_1, 1
	setvar VAR_TEMP_3, 1
	setobjectxyperm LOCALID_TOWER_PARTNER_ROOM_ATTENDANT, 10, 2
	end

BattleFrontier_BattleTowerMultiPartnerRoom_OnWarp:
	map_script_2 VAR_TEMP_3, 1, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TurnPlayerNorth
	.short 0

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TurnPlayerNorth::
	turnobject LOCALID_PLAYER, DIR_NORTH
	end

BattleFrontier_BattleTowerMultiPartnerRoom_OnFrame:
	map_script_2 VAR_TEMP_1, 0, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_EnterRoom
	map_script_2 VAR_TEMP_3, 1, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_ExitRoom
	.short 0

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_ExitRoom::
	lockall
	setvar VAR_TEMP_3, 0
	goto BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_EnterElevator
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_EnterRoom::
	lockall
	applymovement LOCALID_PLAYER, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PlayerEnterRoom
	waitmovement 0
	applymovement LOCALID_TOWER_PARTNER_ROOM_ATTENDANT, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_AttendantBlockExit
	waitmovement 0
	copyobjectxytoperm LOCALID_TOWER_PARTNER_ROOM_ATTENDANT
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox BattleFrontier_BattleTowerMultiPartnerRoom_Text_PleaseFindPartner, MSGBOX_DEFAULT
	special HealPlayerParty
	setvar VAR_TEMP_1, 1
	releaseall
	end

BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PlayerEnterRoom:
	walk_down
	step_end

BattleFrontier_BattleTowerMultiPartnerRoom_Movement_AttendantBlockExit:
	walk_left
	walk_in_place_faster_down
	step_end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_Attendant::
	lock
	faceplayer
	message BattleFrontier_BattleTowerMultiPartnerRoom_Text_QuitLookingForPartner
	waitmessage
	multichoicedefault 20, 8, MULTI_YESNO, 1, FALSE
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_QuitChallenge
	msgbox BattleFrontier_BattleTowerMultiPartnerRoom_Text_PleaseFindPartner2, MSGBOX_DEFAULT
	release
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_QuitChallenge::
	frontier_set FRONTIER_DATA_RECORD_DISABLED, TRUE
	goto BattleFrontier_BattleTower_EventScript_WarpToLobbyLost
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_EnterElevator::
	msgbox BattleFrontier_BattleTowerMultiPartnerRoom_Text_ThankYouForChoosingPartner, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_TOWER_PARTNER_ROOM_ATTENDANT, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	opendoor 10, 1
	waitdooranim
	call BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_MoveToElevator
	closedoor 10, 1
	waitdooranim
	warp MAP_BATTLE_FRONTIER_BATTLE_TOWER_ELEVATOR, 1, 6
	waitstate
	releaseall
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_MoveToElevator::
	applymovement LOCALID_TOWER_PARTNER_ROOM_ATTENDANT, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_AttendantEnterElevator
	applymovement LOCALID_PLAYER, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PlayerEnterElevator
	waitmovement 0
	return

/* Unused*/
BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_MoveToElevatorEast:
	applymovement LOCALID_TOWER_PARTNER_ROOM_ATTENDANT, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_AttendantEnterElevator
	applymovement LOCALID_PLAYER, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PlayerEnterElevatorEast
	waitmovement 0
	return

/* Unused*/
BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_MoveToElevatorWest:
	applymovement LOCALID_TOWER_PARTNER_ROOM_ATTENDANT, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_AttendantEnterElevator
	applymovement LOCALID_PLAYER, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PlayerEneterElevatorWest
	waitmovement 0
	return

BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PlayerEnterElevator:
	walk_up
	walk_up
	set_invisible
	step_end

/* Functionally unused*/
BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PlayerEnterElevatorEast:
	walk_right
	walk_up
	set_invisible
	step_end

/* Functionally unused*/
BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PlayerEneterElevatorWest:
	walk_left
	walk_up
	set_invisible
	step_end

BattleFrontier_BattleTowerMultiPartnerRoom_Movement_AttendantEnterElevator:
	walk_up
	set_invisible
	step_end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_Partner1::
	lock
	faceplayer
	setvar VAR_TEMP_2, 0
	goto BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TalkToPotentialPartner
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TalkToPotentialPartner::
	goto_if_set FLAG_CHOSEN_MULTI_BATTLE_NPC_PARTNER, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_RejectPartner
	tower_dopartnermsg PARTNER_MSGID_INTRO
	waitmessage
	waitbuttonpress
	tower_dopartnermsg PARTNER_MSGID_MON1
	waitmessage
	waitbuttonpress
	tower_dopartnermsg PARTNER_MSGID_MON2_ASK
	waitmessage
	multichoicedefault 20, 8, MULTI_YESNO, 1, FALSE
	switch VAR_RESULT
	case 1, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_RejectPartner
	case MULTI_B_PRESSED, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_RejectPartner
	tower_dopartnermsg PARTNER_MSGID_ACCEPT
	waitmessage
	waitbuttonpress
	closemessage
	call_if_ne VAR_FACING, DIR_SOUTH, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_PartnerExit
	call_if_eq VAR_FACING, DIR_SOUTH, BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_PartnerExitSouth
	removeobject VAR_LAST_TALKED
	setflag FLAG_CHOSEN_MULTI_BATTLE_NPC_PARTNER
	warpsilent MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_PARTNER_ROOM, 10, 3
	waitstate
	release
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_RejectPartner::
	tower_dopartnermsg PARTNER_MSGID_REJECT
	waitmessage
	waitbuttonpress
	release
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_PartnerExit::
	applymovement VAR_LAST_TALKED, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PartnerExit
	waitmovement 0
	return

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_PartnerExitSouth::
	applymovement VAR_LAST_TALKED, BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PartnerExitSouth
	waitmovement 0
	return

BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PartnerExit:
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	step_end

BattleFrontier_BattleTowerMultiPartnerRoom_Movement_PartnerExitSouth:
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	step_end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_Partner2::
	lock
	faceplayer
	setvar VAR_TEMP_2, 1
	goto BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TalkToPotentialPartner
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_Partner3::
	lock
	faceplayer
	setvar VAR_TEMP_2, 2
	goto BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TalkToPotentialPartner
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_Partner4::
	lock
	faceplayer
	setvar VAR_TEMP_2, 3
	goto BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TalkToPotentialPartner
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_Partner5::
	lock
	faceplayer
	setvar VAR_TEMP_2, 4
	goto BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TalkToPotentialPartner
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_Partner6::
	lock
	faceplayer
	setvar VAR_TEMP_2, 5
	goto BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TalkToPotentialPartner
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_Partner7::
	lock
	faceplayer
	setvar VAR_TEMP_2, 6
	goto BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TalkToPotentialPartner
	end

BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_Partner8::
	lock
	faceplayer
	setvar VAR_TEMP_2, 7
	goto BattleFrontier_BattleTowerMultiPartnerRoom_EventScript_TalkToPotentialPartner
	end

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PleaseFindPartner:
	.string "Escolha um parceiro entre os\n"
	.string "TREINADORES reunidos aqui.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_QuitLookingForPartner:
	.string "{PLAYER}, você não achou um parceiro\n"
	.string "pra formar dupla.\p"
	.string "Quer parar de procurar e voltar\n"
	.string "ao balcão de recepção?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PleaseFindPartner2:
	.string "Então escolha um parceiro entre\n"
	.string "os TREINADORES reunidos aqui.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ThankYouForChoosingPartner::
	.string "Obrigado por escolher um parceiro.\p"
	.string "Agora vou te levar à SALA DE\n"
	.string "BATALHA MÚLTIPLA.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice1Intro::
	.string "Sou aprendiz nº {STR_VAR_2} de {STR_VAR_1}.\n"
	.string "Pode me chamar de {STR_VAR_3}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice1Mon1::
	.string "Por conselho de {STR_VAR_1}, trouxe\n"
	.string "um {STR_VAR_3} com {STR_VAR_2} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice1Mon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Por favor, quero formar dupla com você.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice1Accept::
	.string "Obrigado!\n"
	.string "Vou fazer a inscrição agora.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice1Reject::
	.string "Eu queria muito formar dupla\n"
	.string "com você, {PLAYER}…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice2Intro::
	.string "Sou o aprendiz nº {STR_VAR_2} de {STR_VAR_1}.\n"
	.string "O meu nome é {STR_VAR_3}.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice2Mon1::
	.string "Ouvi {STR_VAR_1} e escolhi\n"
	.string "um {STR_VAR_3} com {STR_VAR_2} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice2Mon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Por favor, vamos formar dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice2Accept::
	.string "Muito obrigado!\n"
	.string "Termino a inscrição rapidinho!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_UnusedApprentice2Reject::
	.string "{PLAYER}, eu esperava fazer dupla\n"
	.string "com você…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice1Intro::
	.string "Hã, o meu nome é {STR_VAR_3}, e sou\n"
	.string "o aprendiz nº {STR_VAR_2} de {STR_VAR_1}.\p"
	.string "Snif…\p"
	.string "Desculpa!\n"
	.string "O nervoso está me fazendo chorar…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice1Mon1::
	.string "Ouvi {STR_VAR_3}, então tenho\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice1Mon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Snif…\n"
	.string "Por favor, por favor, faz dupla comigo!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice1Accept::
	.string "Sério? Você topa?\n"
	.string "Demais! Irado! Uhuuu!\p"
	.string "Ah… Desculpa…\n"
	.string "Estou tão feliz que estou chorando…\p"
	.string "Vou fazer a inscrição agora.\n"
	.string "Por favor, não vá embora!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice1Reject::
	.string "Ah, m-mas…\n"
	.string "Snif… Buááááá!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice2Intro::
	.string "Oi! Sou {STR_VAR_3}!\n"
	.string "Aprendiz nº {STR_VAR_2} de {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice2Mon1::
	.string "{STR_VAR_3} indicou a minha turma.\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice2Mon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1},\n"
	.string "é o que eu tenho! Legal, né?\p"
	.string "Então vem!\n"
	.string "Vamos formar dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice2Accept::
	.string "Oba! Ótimo!\n"
	.string "Vou fazer a inscrição, tá?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice2Reject::
	.string "Ah, por quê?\n"
	.string "Eu queria fazer dupla, {PLAYER}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice3Intro::
	.string "Sou {STR_VAR_3}, o aprendiz nº {STR_VAR_2}\n"
	.string "do famoso {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice3Mon1::
	.string "Pedi conselho a {STR_VAR_3}.\n"
	.string "Um {STR_VAR_2} que usa {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice3Mon2Ask::
	.string "e um {STR_VAR_2} com {STR_VAR_1},\n"
	.string "é a minha dupla.\p"
	.string "Por favor, quer formar dupla\n"
	.string "comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice3Accept::
	.string "Obrigado!\n"
	.string "Vou fazer a inscrição agora!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice3Reject::
	.string "{PLAYER}, eu esperava formar dupla\n"
	.string "com você…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice4Intro::
	.string "Hã… Fico muito feliz que você tenha\n"
	.string "parado pra falar comigo.\p"
	.string "Sou {STR_VAR_3}.\n"
	.string "Aprendiz nº {STR_VAR_2} de {STR_VAR_1}…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice4Mon1::
	.string "{STR_VAR_3} me deu conselhos.\n"
	.string "Sou muito grato por isso.\p"
	.string "Tenho uma equipe com um {STR_VAR_2}\n"
	.string "com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice4Mon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\n"
	.string "Sei que é pedir demais…\p"
	.string "Acho que você não vai querer, mas\n"
	.string "posso ser o seu parceiro?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice4Accept::
	.string "Sério? Não acredito!\n"
	.string "Não acredito que vai me aceitar!\l"
	.string "E-eu não vou te decepcionar!\p"
	.string "Hã… Se estiver mesmo tudo bem, vou\n"
	.string "fazer a inscrição agora mesmo!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice4Reject::
	.string "Ah…\n"
	.string "Eu sabia que não era bom o bastante…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice5Intro::
	.string "Oi, sou {STR_VAR_3}.\n"
	.string "Aprendiz nº {STR_VAR_2} de {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice5Mon1::
	.string "{STR_VAR_3} me disse que seria bom\n"
	.string "montar esta equipe:\l"
	.string "um {STR_VAR_2} que usa {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice5Mon2Ask::
	.string "e um {STR_VAR_2} que sabe usar\n"
	.string "{STR_VAR_1}.\p"
	.string "Nada mal, né?\n"
	.string "Quer que eu faça dupla com você?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice5Accept::
	.string "Certo, vai ser um prazer!\n"
	.string "Espero que você não estrague tudo!\l"
	.string "Vou fazer a inscrição agora.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice5Reject::
	.string "Hã? Por que me recusou?\n"
	.string "Você não sabe julgar ninguém!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice6Intro::
	.string "Sou {STR_VAR_3} e sou TRIATLETA.\n"
	.string "Vivo ocupado todo dia: corro,\l"
	.string "treino e faço rap.\p"
	.string "E também sou o aprendiz\n"
	.string "nº {STR_VAR_2} de {STR_VAR_1}.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice6Mon1::
	.string "Foi isto que {STR_VAR_3} recomendou.\n"
	.string "Um {STR_VAR_2} que usa {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice6Mon2Ask::
	.string "e um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1}.\p"
	.string "Dei duro pra treinar eles, mesmo\n"
	.string "com a correria.\p"
	.string "Vamos formar dupla e\n"
	.string "mandar ver!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice6Accept::
	.string "Valeu, é assim que se fala!\n"
	.string "Espera aí que vou me inscrever, tá?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice6Reject::
	.string "Tirei tempo da minha agenda lotada\n"
	.string "pra vir aqui! Me dá um tempo!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice7Intro::
	.string "Oi!\n"
	.string "Tudo certo?\p"
	.string "Sou {STR_VAR_3}, o aprendiz\n"
	.string "nº {STR_VAR_2} de {STR_VAR_1}. Prazer!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice7Mon1::
	.string "Escuta, escuta! Olha os POKéMON\n"
	.string "que {STR_VAR_3} recomendou.\l"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice7Mon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\n"
	.string "Claro que treinei eles muito bem!\p"
	.string "Então, vamos fazer dupla?\n"
	.string "Vai ser uma ótima combinação!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice7Accept::
	.string "Oba, acho que vai ser divertido!\n"
	.string "Vou fazer a inscrição!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice7Reject::
	.string "Ah, que maldade!\n"
	.string "Eu vim recomendado, viu?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice8Intro::
	.string "Permita que eu me apresente.\n"
	.string "Sou {STR_VAR_3}.\p"
	.string "Sou o aprendiz nº {STR_VAR_2}\n"
	.string "de {STR_VAR_1}.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice8Mon1::
	.string "Busquei os sábios conselhos de\n"
	.string "{STR_VAR_3} e treinei a minha equipe:\l"
	.string "um {STR_VAR_2} com {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice8Mon2Ask::
	.string "e um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Por favor, aceite formar dupla comigo!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice8Accept::
	.string "Ah… Que alegria!\n"
	.string "Prometo dar o meu melhor!\p"
	.string "Claro que eu faço a inscrição!\n"
	.string "Aguarde, por favor!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice8Reject::
	.string "Eu esperava me juntar a você,\n"
	.string "{PLAYER}…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice9Intro::
	.string "Ai! Você falou comigo!\n"
	.string "E-eu estou nas nuvens!\p"
	.string "Sou {STR_VAR_3}! Sou o aprendiz\n"
	.string "nº {STR_VAR_2} de {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice9Mon1::
	.string "Por conselho de {STR_VAR_3}, treinei\n"
	.string "um {STR_VAR_2} que usa {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice9Mon2Ask::
	.string "e um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1}.\p"
	.string "Por favor, realiza o meu desejo?\n"
	.string "Quero ser o seu parceiro de dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice9Accept::
	.string "Ai! E-eu estou tonto de alegria!\n"
	.string "Muito obrigado!\l"
	.string "Vou fazer a nossa inscrição já!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice9Reject::
	.string "Buááá! Você não tem dó?\n"
	.string "Mas isso te deixa mais legal…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice10Intro::
	.string "Iiiha! Sou o aprendiz\n"
	.string "nº {STR_VAR_2} de {STR_VAR_1}!\p"
	.string "{STR_VAR_3} é o meu nome, olá, olá!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice10Mon1::
	.string "O meu mestre {STR_VAR_3} indicou\n"
	.string "um {STR_VAR_2} ás em {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice10Mon2Ask::
	.string "e um {STR_VAR_2} ás em\n"
	.string "{STR_VAR_1}.\p"
	.string "Coisa boa, né?\n"
	.string "Vai fazer dupla comigo, não vai?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice10Accept::
	.string "Certo, excelente!\n"
	.string "Vou fazer a inscrição rapidinho!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice10Reject::
	.string "Guaaah!\n"
	.string "Você é calculista, {PLAYER}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice11Intro::
	.string "Ei! Problema sério! Hoje é o último\n"
	.string "dia da TORRE DE BATALHA!\p"
	.string "… … … … … …\n"
	.string "Brincadeira!\p"
	.string "Me chamam de {STR_VAR_3}, e eu também!\n"
	.string "Sou o aprendiz nº 1.000 de {STR_VAR_1}!\l"
	.string "Na verdade, sou o nº {STR_VAR_2}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice11Mon1::
	.string "{STR_VAR_3} me deu uns conselhos.\n"
	.string "{STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice11Mon2Ask::
	.string "{STR_VAR_2} com {STR_VAR_1}.\n"
	.string "Tenho dez de cada!\l"
	.string "Na verdade, só um de cada!\p"
	.string "E aí?\n"
	.string "Quer tentar batalhas em dupla comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice11Accept::
	.string "Oba!\n"
	.string "Vou te dar um POKéMON de presente!\p"
	.string "Brincadeira! Mas vou mesmo fazer\n"
	.string "a inscrição, tá?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice11Reject::
	.string "Ah, que frieza! Vou ter que destruir\n"
	.string "a TORRE DE BATALHA por isso!\p"
	.string "Claro que não!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice12Intro::
	.string "Ei, tô no rock e tô no rolê!\n"
	.string "Os POKéMON eu domino, é!\p"
	.string "Sou {STR_VAR_3}, o MARINHEIRO rapper.\n"
	.string "Não me confunda com qualquer chorão!\p"
	.string "Aprendiz nº {STR_VAR_2} de {STR_VAR_1}.\n"
	.string "Não me dispense, não, irmão!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice12Mon1::
	.string "Com o conselho de {STR_VAR_3} na mão,\n"
	.string "montei a minha equipe campeã!\l"
	.string "Um {STR_VAR_2} que usa {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice12Mon2Ask::
	.string "e um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1}!\p"
	.string "O nosso encontro vamos celebrar,\n"
	.string "com uma vitória em dupla pra comemorar!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice12Accept::
	.string "Ei, ei, curti o seu estilo!\n"
	.string "Vou fazer a inscrição, tranquilo!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice12Reject::
	.string "Ah, ei, {PLAYER}, que frieza!\n"
	.string "Se me permite a franqueza!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice13Intro::
	.string "Iupi-iahu!\n"
	.string "Ah, não corre! Era só brincadeira!\p"
	.string "E aí! Sou {STR_VAR_3}!\n"
	.string "Aprendiz nº {STR_VAR_2} de {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice13Mon1::
	.string "{STR_VAR_3} me orientou.\n"
	.string "Então um {STR_VAR_2} com {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice13Mon2Ask::
	.string "e um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1} eu escolhi, sim.\p"
	.string "Então, o que me diz?\n"
	.string "Vamos, faz o meu dia feliz!\p"
	.string "Por favor, eu imploro!\n"
	.string "Me deixa entrar, senão eu choro!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice13Accept::
	.string "Iêêê! A sorte está comigo!\n"
	.string "Que alegria, amigo!\p"
	.string "Antes de a batalha começar,\n"
	.string "vou correndo me inscrever lá!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice13Reject::
	.string "Vai me recusar?\n"
	.string "{PLAYER}, assim vou chorar!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice14Intro::
	.string "Cof!\p"
	.string "Ah, desculpa, estou resfriado.\n"
	.string "Mas os meus POKéMON estão bem.\p"
	.string "O meu nome é {STR_VAR_3}, aprendiz\n"
	.string "nº {STR_VAR_2} de {STR_VAR_1}. Cof!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice14Mon1::
	.string "Ouvi bem {STR_VAR_3}\n"
	.string "e montei a minha equipe com\l"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice14Mon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Treinei eles todo dia, até com\n"
	.string "vento e chuva.\p"
	.string "Acho que foi por isso que\n"
	.string "peguei este resfriado.\p"
	.string "É isso aí.\n"
	.string "Quer ser o meu parceiro de dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice14Accept::
	.string "Valeu, fico grato!\n"
	.string "Cof, cof!\l"
	.string "Espera aí que vou me inscrever.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice14Reject::
	.string "Ah, não quer?\n"
	.string "{PLAYER}, a gente seria uma\l"
	.string "boa dupla.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice15Intro::
	.string "Ah, olá!\n"
	.string "Que nervoso.\p"
	.string "Sou {STR_VAR_3}.\n"
	.string "Sou o aprendiz nº {STR_VAR_2} de {STR_VAR_1}.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice15Mon1::
	.string "{STR_VAR_3} aprovou esta equipe:\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice15Mon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Estou meio sem graça, mas você\n"
	.string "me deixa formar dupla com você?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice15Accept::
	.string "Ah, puxa, obrigado!\n"
	.string "Estou com vergonha, mas vou dar o\l"
	.string "meu melhor!\p"
	.string "Vou fazer a inscrição.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice15Reject::
	.string "Ah, por favor, não diga não!\n"
	.string "Já estou sem graça o bastante…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice16Intro::
	.string "Sou {STR_VAR_3}, e não é mentira.\n"
	.string "Sou o aprendiz nº {STR_VAR_2} de {STR_VAR_1}.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice16Mon1::
	.string "{STR_VAR_3} recomendou a minha equipe.\p"
	.string "Como o conselho parecia sincero,\n"
	.string "decidi trazer comigo\l"
	.string "o meu {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice16Mon2Ask::
	.string "o meu {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Se possível, gostaria que me\n"
	.string "aceitasse como parceiro de dupla.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice16Accept::
	.string "Vai mesmo me aceitar?\n"
	.string "Seria terrível se não aceitasse.\p"
	.string "Mas, como aceitou, está tudo bem.\n"
	.string "Vou inscrever nós dois.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_Apprentice16Reject::
	.string "Hum? Por que recusaria?\n"
	.string "Isso é alguma piada?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LassIntro::
	.string "Sou {STR_VAR_1}, e sou GAROTA!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LassMon1::
	.string "O que eu tenho é um {STR_VAR_2}\n"
	.string "que usa {STR_VAR_1} e um$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LassMon2Ask::
	.string "{STR_VAR_2} que usa {STR_VAR_1}.\n"
	.string "É o que eu tenho comigo.\p"
	.string "Você vai ser a minha dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LassAccept::
	.string "Obrigada!\n"
	.string "Vou fazer a inscrição!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LassReject::
	.string "Não quer ser a minha dupla?\n"
	.string "Depois vai se arrepender!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_YoungsterIntro::
	.string "Olá!\p"
	.string "Sou o JOVEM {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_YoungsterMon1::
	.string "Quer saber o que eu tenho?\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_YoungsterMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Você vai ser a minha dupla,\n"
	.string "não vai?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_YoungsterAccept::
	.string "Oba!\n"
	.string "Vou fazer a inscrição, tá?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_YoungsterReject::
	.string "Ah! Se fizesse dupla com os meus\n"
	.string "POKéMON, a gente seria imbatível!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HikerIntro::
	.string "Iahu!\n"
	.string "Sou o ALPINISTA {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HikerMon1::
	.string "Sabe o que eu trouxe?\n"
	.string "O meu {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HikerMon2Ask::
	.string "o meu {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Parece bom, né?\n"
	.string "Quer formar dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HikerAccept::
	.string "Iahu!\n"
	.string "Então vou fazer a inscrição.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HikerReject::
	.string "Eu queria ter batalhado com você\n"
	.string "do meu lado.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BeautyIntro::
	.string "Olá!\n"
	.string "Sou {STR_VAR_1}, e sou BELDADE!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BeautyMon1::
	.string "Sabe o que eu ando treinando?\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BeautyMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "O que acha?\n"
	.string "Quer formar dupla comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BeautyAccept::
	.string "Maravilha!\n"
	.string "Vou fazer a inscrição agora!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BeautyReject::
	.string "Que decepção!\p"
	.string "Juntos, a gente seria\n"
	.string "imbatível!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_FishermanIntro::
	.string "E aí!\p"
	.string "Sabe quem eu sou?\n"
	.string "Sou {STR_VAR_1}, o PESCADOR!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_FishermanMon1::
	.string "Trouxe uma equipe com um\n"
	.string "{STR_VAR_2} que usa {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_FishermanMon2Ask::
	.string "um {STR_VAR_2} que usa {STR_VAR_1}.\p"
	.string "E aí, que tal?\n"
	.string "Vai batalhar do meu lado?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_FishermanAccept::
	.string "Boa, boa!\n"
	.string "Deixa comigo!\p"
	.string "Vou fazer a nossa inscrição agora.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_FishermanReject::
	.string "E a gente combinava tão bem…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LadyIntro::
	.string "É um prazer conhecer você.\n"
	.string "Sou {STR_VAR_1}, uma DAMA.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LadyMon1::
	.string "Estou acompanhada de uma equipe com\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LadyMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Espero ter a sua aprovação.\n"
	.string "Pois desejo a sua parceria.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LadyAccept::
	.string "Agradeço sinceramente.\n"
	.string "Cuidarei da inscrição.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_LadyReject::
	.string "Tenho certeza de que vai se\n"
	.string "arrepender de não me ter como parceira.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteFIntro::
	.string "Sou a TRIATLETA {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteFMon1::
	.string "O que eu tenho…\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteFMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\n"
	.string "É a minha dupla!\p"
	.string "Por favor?\n"
	.string "Vai formar dupla comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteFAccept::
	.string "Puxa, valeu!\n"
	.string "Vou me inscrever no balcão.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteFReject::
	.string "Nós dois juntos seríamos fortes,\n"
	.string "com certeza!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugCatcherIntro::
	.string "Oi!\n"
	.string "Sou o CAÇA-INSETOS {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugCatcherMon1::
	.string "Olha o que eu tenho!\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugCatcherMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Então, escuta!\n"
	.string "Quer formar dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugCatcherAccept::
	.string "Beleza!\p"
	.string "Vou fazer a inscrição lá no\n"
	.string "balcão.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugCatcherReject::
	.string "Ah, os meus POKéMON são demais.\n"
	.string "Espero que não se arrependa!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidMIntro::
	.string "Bom dia!\n"
	.string "Sou o ESTUDANTE {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidMMon1::
	.string "Os que eu ando treinando são um\n"
	.string "{STR_VAR_2} que usa {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidMMon2Ask::
	.string "e um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1}.\p"
	.string "Nada mal, não acha?\n"
	.string "Gostaria de formar dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidMAccept::
	.string "Muito obrigado!\n"
	.string "Vou terminar a inscrição.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidMReject::
	.string "Que pena…\p"
	.string "Eu queria aprender com você como\n"
	.string "seu parceiro…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RichBoyIntro::
	.string "E aí! Vou te dizer quem eu sou!\n"
	.string "Sou o RIQUINHO {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RichBoyMon1::
	.string "Adivinha o que eu tenho!\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RichBoyMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Te ofereço a chance de formar\n"
	.string "dupla comigo.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RichBoyAccept::
	.string "Boa escolha!\p"
	.string "Vou terminar a inscrição\n"
	.string "rapidinho!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RichBoyReject::
	.string "Recusar logo a mim?\n"
	.string "Vai se arrepender, com certeza!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BlackBeltIntro::
	.string "Hiá!\n"
	.string "Sou o FAIXA-PRETA {STR_VAR_1}.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BlackBeltMon1::
	.string "Como companheiros, tenho\n"
	.string "um {STR_VAR_2} que usa {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BlackBeltMon2Ask::
	.string "e um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1}.\p"
	.string "Por favor, atenda ao meu pedido!\n"
	.string "Deixe-me ser o seu parceiro!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BlackBeltAccept::
	.string "Hiá!\n"
	.string "Vou me inscrever imediatamente!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BlackBeltReject::
	.string "Entendo… Espero ter uma chance\n"
	.string "da próxima vez…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberFIntro::
	.string "Oi! Sou {STR_VAR_1}!\n"
	.string "Sou NADADORA MIRIM!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberFMon1::
	.string "Vou contar o que eu tenho.\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberFMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\n"
	.string "Posso fazer dupla com você?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberFAccept::
	.string "Obrigada!\n"
	.string "Vou fazer a nossa inscrição!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberFReject::
	.string "Se a gente fosse parceiro, seria\n"
	.string "tão forte!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HexManiacIntro::
	.string "Saudações…\n"
	.string "Sou a BRUXA {STR_VAR_1}…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HexManiacMon1::
	.string "Trago comigo um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1}, junto com um$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HexManiacMon2Ask::
	.string "{STR_VAR_2} que usa {STR_VAR_1}…\p"
	.string "Eu suplico…\n"
	.string "Junte-se a mim numa dupla…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HexManiacAccept::
	.string "Eu agradeço…\n"
	.string "Vou nos inscrever…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_HexManiacReject::
	.string "Eu tanto queria me juntar a você…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederMIntro::
	.string "Como vai? Sou {STR_VAR_1}, e sou\n"
	.string "CRIADOR POKéMON!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederMMon1::
	.string "Estou criando uns dois bons!\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "E aí?\n"
	.string "Afim de formar dupla comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederMAccept::
	.string "Muito obrigado!\p"
	.string "Vou cuidar da inscrição, então\n"
	.string "espera aqui!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederMReject::
	.string "Eu estava ansioso pra ser seu\n"
	.string "parceiro…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteFIntro::
	.string "Olá!\n"
	.string "Sou a TRIATLETA {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteFMon1::
	.string "Quer saber com quem eu corro?\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteFMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "E aí?\n"
	.string "Quer formar dupla comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteFAccept::
	.string "Boa!\p"
	.string "Vou ser rápida e terminar a\n"
	.string "inscrição!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteFReject::
	.string "Nós dois seríamos os melhores.\n"
	.string "Que pena…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteMIntro::
	.string "E aí! O meu nome é {STR_VAR_1}!\n"
	.string "Sou TRIATLETA!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteMMon1::
	.string "Corro com a minha equipe resistente:\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Nada mal, né?\n"
	.string "A gente devia formar dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteMAccept::
	.string "Beleza!\n"
	.string "Vou me inscrever num piscar de olhos!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RunningTriathleteMReject::
	.string "Eu queria muito batalhar como seu\n"
	.string "parceiro de dupla…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BattleGirlIntro::
	.string "Sou a LUTADORA {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BattleGirlMon1::
	.string "Ando fortalecendo um\n"
	.string "{STR_VAR_2} que usa {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BattleGirlMon2Ask::
	.string "um {STR_VAR_2} que usa {STR_VAR_1}!\p"
	.string "Gostou da combinação?\n"
	.string "Que tal a gente se juntar?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BattleGirlAccept::
	.string "Ora, valeu!\p"
	.string "Vou fazer a inscrição\n"
	.string "agora mesmo!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BattleGirlReject::
	.string "Com você, achei que a gente seria\n"
	.string "a dupla definitiva…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteMIntro::
	.string "Sou o TRIATLETA {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteMMon1::
	.string "Ando pedalando com um {STR_VAR_2}\n"
	.string "que sabe usar {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteMMon2Ask::
	.string "e um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "A gente podia formar dupla.\n"
	.string "Não seria ótimo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteMAccept::
	.string "Obrigado!\p"
	.string "Vou fazer a nossa inscrição, e é\n"
	.string "agora mesmo!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CyclingTriathleteMReject::
	.string "Ah, que pena. A gente seria a dupla\n"
	.string "mais forte da área!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberMIntro::
	.string "Eu?\n"
	.string "Sou o NADADOR MIRIM {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberMMon1::
	.string "O que eu tenho comigo?\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Ei?\n"
	.string "Vai fazer dupla comigo, né?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberMAccept::
	.string "Tá!\p"
	.string "Vou fazer a inscrição!\n"
	.string "Vamos arrasar juntos!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_TuberMReject::
	.string "Os meus POKéMON são fortes mesmo…\n"
	.string "Que pena que não quer fazer dupla.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GuitaristIntro::
	.string "Iê-ê!\n"
	.string "Me chame de GUITARRISTA {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GuitaristMon1::
	.string "Olha a minha banda!\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GuitaristMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Iê-ê! Bem doido, né?\n"
	.string "Vamos fazer um dueto em dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GuitaristAccept::
	.string "Iê-ê! Isso aí!\n"
	.string "Vou fazer a inscrição agora!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GuitaristReject::
	.string "Os meus POKéMON são puro rock!\n"
	.string "Vai se arrepender, viu!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GentlemanIntro::
	.string "Muito prazer.\n"
	.string "Sou {STR_VAR_1}, um CAVALHEIRO.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GentlemanMon1::
	.string "Estou acompanhado do meu fiel\n"
	.string "{STR_VAR_2} que usa {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GentlemanMon2Ask::
	.string "um {STR_VAR_2} que usa {STR_VAR_1}.\p"
	.string "Posso pedir que se junte a mim\n"
	.string "numa parceria de dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GentlemanAccept::
	.string "Ah, agradeço a confiança.\n"
	.string "Vou cuidar da inscrição.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_GentlemanReject::
	.string "É uma pena…\p"
	.string "Aguardarei a próxima oportunidade…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanMIntro::
	.string "Olá, sou {STR_VAR_1},\n"
	.string "e sou POKéFÃ.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanMMon1::
	.string "Estou agora com um {STR_VAR_2}\n"
	.string "que sabe o golpe {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Gostou do que viu?\n"
	.string "Por que não faz dupla comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanMAccept::
	.string "Obrigado!\n"
	.string "Eu cuido da inscrição!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanMReject::
	.string "Os meus POKéMON são de primeira…\n"
	.string "Pena que você não sabe valorizar.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertMIntro::
	.string "Hum! Sou {STR_VAR_1},\n"
	.string "e ESPECIALISTA eu sou!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertMMon1::
	.string "Os POKéMON que fortaleci são\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "O que me diz de uma dupla comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertMAccept::
	.string "Hum!\n"
	.string "Vou nos inscrever já!\l"
	.string "Vamos dar o nosso melhor!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertMReject::
	.string "Espero que a sua escolha esteja\n"
	.string "mesmo certa…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertFIntro::
	.string "Olá, olá.\n"
	.string "Sou {STR_VAR_1}, e sou ESPECIALISTA.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertFMon1::
	.string "Treinei bem os meus POKéMON.\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertFMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}:\n"
	.string "é o que eu tenho.\p"
	.string "Não gostaria de fazer dupla comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertFAccept::
	.string "Muito bem, muito bem.\n"
	.string "Vou cuidar da inscrição já.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ExpertFReject::
	.string "Quem sabe a gente forma dupla da\n"
	.string "próxima vez.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_DragonTamerIntro::
	.string "Sou o DOMADOR DE DRAGÕES\n"
	.string "{STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_DragonTamerMon1::
	.string "A equipe que ando fortalecendo é\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_DragonTamerMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "E aí?\n"
	.string "Quer ser o meu parceiro?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_DragonTamerAccept::
	.string "Certo, vou dar o meu melhor!\n"
	.string "Vou fazer a inscrição agora, tá?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_DragonTamerReject::
	.string "Você não vai achar muitos parceiros\n"
	.string "mais fortes que eu!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BirdKeeperIntro::
	.string "Sou o CRIADOR DE PÁSSAROS\n"
	.string "{STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BirdKeeperMon1::
	.string "Que POKéMON eu tenho?\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BirdKeeperMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Não acha que seríamos uma boa\n"
	.string "dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BirdKeeperAccept::
	.string "Ótimo, valeu!\n"
	.string "Eu cuido da inscrição!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BirdKeeperReject::
	.string "Eu e os meus POKéMON somos fortes.\n"
	.string "Que decepção.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_NinjaBoyIntro::
	.string "Sou o NINJA MIRIM {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_NinjaBoyMon1::
	.string "A minha equipe POKéMON tem um\n"
	.string "{STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_NinjaBoyMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Vamos formar dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_NinjaBoyAccept::
	.string "Oba!\n"
	.string "Deixa eu ir me inscrever!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_NinjaBoyReject::
	.string "Vai se arrepender de não ter os meus\n"
	.string "POKéMON fortes do seu lado!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ParasolLadyIntro::
	.string "Olá!\n"
	.string "Sou a DAMA DA SOMBRINHA {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ParasolLadyMon1::
	.string "Me acompanham o meu {STR_VAR_2}\n"
	.string "que usa {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ParasolLadyMon2Ask::
	.string "um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1}.\p"
	.string "Não são lindos?\n"
	.string "Quer se juntar a nós numa dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ParasolLadyAccept::
	.string "Muito obrigada!\p"
	.string "Vou me inscrever no balcão.\n"
	.string "Vamos não decepcionar uma à outra!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_ParasolLadyReject::
	.string "Os meus POKéMON são fortíssimos.\n"
	.string "Que decepção…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugManiacIntro::
	.string "Olá. Sou {STR_VAR_1},\n"
	.string "MANÍACO POR INSETOS!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugManiacMon1::
	.string "Achei os meus POKéMON, sim.\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugManiacMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}:\n"
	.string "foi o que eu achei.\p"
	.string "Posso te convencer a formar\n"
	.string "uma dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugManiacAccept::
	.string "Certo!\n"
	.string "Entendido!\p"
	.string "Não vou demorar com a inscrição!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_BugManiacReject::
	.string "Com os POKéMON que eu achei, a gente\n"
	.string "não teria perdido…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SailorIntro::
	.string "Ahoy!\n"
	.string "Sou o MARINHEIRO {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SailorMon1::
	.string "Deixa eu mostrar o meu orgulho!\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SailorMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Claro que você não vai me recusar.\n"
	.string "Vamos fazer dupla, né?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SailorAccept::
	.string "Não esperava menos!\n"
	.string "Vou fazer a inscrição agora.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SailorReject::
	.string "A gente ia passar por cima de todo\n"
	.string "mundo! Que pena!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CollectorIntro::
	.string "Oi, sou {STR_VAR_1}.\n"
	.string "Sou COLECIONADOR.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CollectorMon1::
	.string "As joias da minha coleção são o\n"
	.string "meu {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CollectorMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Demais, né?\n"
	.string "A gente devia formar uma dupla.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CollectorAccept::
	.string "Isso aí!\p"
	.string "Bom, não vamos perder tempo.\n"
	.string "Vou inscrever nós dois.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CollectorReject::
	.string "Bom, isso me chateia.\n"
	.string "Você não dá valor aos meus POKéMON.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerMIntro::
	.string "E aí, sou {STR_VAR_1}.\n"
	.string "Sou GUARDA POKéMON.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerMMon1::
	.string "Me fazem companhia um\n"
	.string "{STR_VAR_2} que usa {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Não acha que seríamos uma dupla\n"
	.string "impressionante?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerMAccept::
	.string "Que demais!\n"
	.string "Vou cuidar da inscrição agora.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerMReject::
	.string "Da próxima vez, escolha os meus\n"
	.string "POKéMON, tá?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerFIntro::
	.string "O meu nome é {STR_VAR_1}.\n"
	.string "Sou GUARDA POKéMON!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerFMon1::
	.string "Vou falar da minha equipe. Tenho\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerFMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Que tal formar dupla com a\n"
	.string "minha turminha?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerFAccept::
	.string "Vamos dar o nosso melhor!\n"
	.string "Vou fazer a inscrição rapidinho!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnRangerFReject::
	.string "Espero que escolha os meus POKéMON\n"
	.string "da próxima vez.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_AromaLadyIntro::
	.string "Prazer em conhecer. Sou {STR_VAR_1}.\n"
	.string "Me considero uma DAMA DOS AROMAS.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_AromaLadyMon1::
	.string "Viajo com um {STR_VAR_2}\n"
	.string "que usa {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_AromaLadyMon2Ask::
	.string "um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1}.\p"
	.string "Espero que te agradem.\n"
	.string "Gostaria de ser o meu parceiro?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_AromaLadyAccept::
	.string "É uma honra ser aceita.\n"
	.string "Vou me inscrever agora mesmo.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_AromaLadyReject::
	.string "Seria maravilhoso formar dupla da\n"
	.string "próxima vez que nos encontrarmos.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RuinManiacIntro::
	.string "Quer saber quem eu sou? Sou\n"
	.string "{STR_VAR_1}, o MANÍACO POR RUÍNAS!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RuinManiacMon1::
	.string "Os POKéMON que tenho comigo são\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RuinManiacMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Intrigante, né?\n"
	.string "Que tal formarmos dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RuinManiacAccept::
	.string "Decisão sensata!\n"
	.string "Vou cuidar da papelada da inscrição.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_RuinManiacReject::
	.string "Hmm…\n"
	.string "Acho que os meus POKéMON são fortes…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerMIntro::
	.string "Sou o TREINADOR ÁS {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerMMon1::
	.string "Os POKéMON que tenho agora são um\n"
	.string "{STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\n"
	.string "Legal, né?\p"
	.string "Não seria bem legal a gente\n"
	.string "formar dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerMAccept::
	.string "Legal!\n"
	.string "Vou me inscrever num piscar de olhos!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerMReject::
	.string "Achei que a gente seria a melhor\n"
	.string "dupla de todos os tempos.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerFIntro::
	.string "Sou a TREINADORA ÁS {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerFMon1::
	.string "A equipe que ando treinando tem um\n"
	.string "{STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerFMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Parece bom?\n"
	.string "Que tal a gente formar dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerFAccept::
	.string "Tudo certo!\n"
	.string "Melhor eu fazer a inscrição.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CoolTrainerFReject::
	.string "Eu estava pensando que seríamos\n"
	.string "uma dupla e tanto…$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokemaniacIntro::
	.string "E aí!\n"
	.string "Sou {STR_VAR_1}, o POKéMANÍACO!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokemaniacMon1::
	.string "O que um cara como eu tem?\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokemaniacMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1},\n"
	.string "isso aí!\p"
	.string "Vamos nessa!\n"
	.string "Vamos arrasar em dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokemaniacAccept::
	.string "Boa escolha!\n"
	.string "Vou inscrever nós dois!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokemaniacReject::
	.string "Os meus POKéMON são brutais!\n"
	.string "Se se arrepender, a culpa não é minha!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_KindlerIntro::
	.string "E aí!\n"
	.string "Sou o FOGUISTA {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_KindlerMon1::
	.string "Sabe o que o meu treino cozinhou?\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_KindlerMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Então, o que me diz?\n"
	.string "Quer formar dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_KindlerAccept::
	.string "Beleza!\n"
	.string "Vou cuidar da inscrição.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_KindlerReject::
	.string "Promete que faz dupla comigo da\n"
	.string "próxima vez que a gente se esbarrar.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CamperIntro::
	.string "Sou {STR_VAR_1}, e sou CAMPISTA!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CamperMon1::
	.string "Ando treinando um {STR_VAR_2} que\n"
	.string "usa {STR_VAR_1} e um$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CamperMon2Ask::
	.string "{STR_VAR_2} que usa {STR_VAR_1}.\p"
	.string "Acha que seria divertido fazer dupla?\n"
	.string "Aposto que sim!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CamperAccept::
	.string "Isso!\n"
	.string "Lá vou eu me inscrever!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_CamperReject::
	.string "Da próxima vez, tá?\n"
	.string "Quero estar na sua equipe.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PicnickerIntro::
	.string "Olá! Sou {STR_VAR_1},\n"
	.string "a PIQUENIQUEIRA!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PicnickerMon1::
	.string "Os POKéMON que ando levando são\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PicnickerMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Tem interesse em formar dupla\n"
	.string "comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PicnickerAccept::
	.string "Ora, obrigada!\n"
	.string "Vou fazer a inscrição agora.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PicnickerReject::
	.string "Seria bom fazer dupla com você\n"
	.string "outra hora.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicMIntro::
	.string "Sou o MÉDIUM {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicMMon1::
	.string "A dupla que ando treinando é\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Gostaria de formar dupla\n"
	.string "comigo?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicMAccept::
	.string "Claro!\n"
	.string "Eu cuido da inscrição!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicMReject::
	.string "Se nos virmos de novo, aí sim quero\n"
	.string "fazer dupla com você.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicFIntro::
	.string "Sou {STR_VAR_1}.\n"
	.string "Sou MÉDIUM.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicFMon1::
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicFMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}:\n"
	.string "são os meus discípulos.\p"
	.string "A ideia de formar uma dupla não\n"
	.string "te intriga?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicFAccept::
	.string "Obrigada.\n"
	.string "Vou cuidar da inscrição.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PsychicFReject::
	.string "Espero que haja outra chance de\n"
	.string "forjar uma aliança.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidFIntro::
	.string "Sou a ESTUDANTE {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidFMon1::
	.string "Um {STR_VAR_2} que usa {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidFMon2Ask::
	.string "e um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1} são a minha dupla.\p"
	.string "Posso, por favor, ser a sua\n"
	.string "parceira numa dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidFAccept::
	.string "Uh, obrigada!\n"
	.string "Vou me inscrever no balcão já!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SchoolKidFReject::
	.string "Por favor?\n"
	.string "Posso me juntar a você da próxima vez?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederFIntro::
	.string "Oi! O nome é {STR_VAR_1}!\n"
	.string "Sou CRIADORA POKéMON!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederFMon1::
	.string "Os POKéMON que criei são um\n"
	.string "{STR_VAR_2} que usa {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederFMon2Ask::
	.string "um {STR_VAR_2} que usa {STR_VAR_1}.\p"
	.string "Parece interessante?\n"
	.string "Então que tal formarmos dupla?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederFAccept::
	.string "Tudo bem!\n"
	.string "Deixa a inscrição comigo!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PkmnBreederFReject::
	.string "Da próxima vez você tem que fazer\n"
	.string "dupla comigo, tá?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanFIntro::
	.string "Sou {STR_VAR_1}, e tenho orgulho\n"
	.string "de dizer que sou POKéFÃ.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanFMon1::
	.string "Os POKéMON queridos que criei são\n"
	.string "um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanFMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}.\p"
	.string "Não são os mais fofos?\n"
	.string "A gente devia formar uma dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanFAccept::
	.string "Obrigada, meu bem!\n"
	.string "Vou lá me inscrever!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_PokefanFReject::
	.string "Os meus POKéMON queridos são os\n"
	.string "melhores, saiba disso. Que chato!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerFIntro::
	.string "Oi, sou a NADADORA {STR_VAR_1}.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerFMon1::
	.string "Um {STR_VAR_2} que usa {STR_VAR_1}$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerFMon2Ask::
	.string "e um {STR_VAR_2} que usa\n"
	.string "{STR_VAR_1} são os que eu treinei.\p"
	.string "Você e eu, vamos formar dupla.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerFAccept::
	.string "Legal!\n"
	.string "Vou inscrever a gente.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerFReject::
	.string "Se nos virmos de novo, você me\n"
	.string "deve uma dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteMIntro::
	.string "Qual é a boa?\n"
	.string "Sou {STR_VAR_1}, e sou TRIATLETA.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteMMon1::
	.string "Tenho uns POKéMON decentes.\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Seria legal formar uma dupla,\n"
	.string "então que tal?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteMAccept::
	.string "Isso aí!\n"
	.string "Espera enquanto me inscrevo, tá?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteMReject::
	.string "Espero que me deixe fazer dupla da\n"
	.string "próxima vez, combinado?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteFIntro::
	.string "Sou a TRIATLETA {STR_VAR_1}!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteFMon1::
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteFMon2Ask::
	.string "outro {STR_VAR_2} que sabe usar\n"
	.string "{STR_VAR_1}. É a minha dupla.\p"
	.string "O que acha?\n"
	.string "Seríamos uma boa equipe, eu diria.$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteFAccept::
	.string "Gostei da resposta!\n"
	.string "Vou terminar a inscrição rapidinho!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmingTriathleteFReject::
	.string "Vai me dar outra chance de formar\n"
	.string "uma parceria, não vai?$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerMIntro::
	.string "Oi! Olá!\n"
	.string "Sou {STR_VAR_1}, e sou NADADOR!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerMMon1::
	.string "Olha o que eu ando treinando!\n"
	.string "Um {STR_VAR_2} com {STR_VAR_1} e$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerMMon2Ask::
	.string "um {STR_VAR_2} com {STR_VAR_1}!\p"
	.string "Massa, né?\n"
	.string "Massa também seria formar dupla!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerMAccept::
	.string "Muito obrigado!\n"
	.string "Vou resolver essa inscrição!$"

BattleFrontier_BattleTowerMultiPartnerRoom_Text_SwimmerMReject::
	.string "Se nos virmos de novo, você tem que\n"
	.string "fazer dupla comigo. Combinado?$"

