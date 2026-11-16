BattleFrontier_BattleArenaLobby_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, BattleFrontier_BattleArenaLobby_OnFrame
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, BattleFrontier_BattleArenaLobby_OnWarp
	.byte 0

BattleFrontier_BattleArenaLobby_OnWarp:
	map_script_2 VAR_TEMP_1, 0, BattleFrontier_BattleArenaLobby_EventScript_TurnPlayerNorth
	.short 0

BattleFrontier_BattleArenaLobby_EventScript_TurnPlayerNorth::
	setvar VAR_TEMP_1, 1
	turnobject LOCALID_PLAYER, DIR_NORTH
	end

BattleFrontier_BattleArenaLobby_OnFrame:
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, 0, BattleFrontier_BattleArenaLobby_EventScript_GetChallengeStatus
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_SAVING, BattleFrontier_BattleArenaLobby_EventScript_QuitWithoutSaving
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_PAUSED, BattleFrontier_BattleArenaLobby_EventScript_ResumeChallenge
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_WON, BattleFrontier_BattleArenaLobby_EventScript_WonChallenge
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_LOST, BattleFrontier_BattleArenaLobby_EventScript_LostChallenge
	.short 0

BattleFrontier_BattleArenaLobby_EventScript_GetChallengeStatus::
	frontier_getstatus
	end

BattleFrontier_BattleArenaLobby_EventScript_QuitWithoutSaving::
	lockall
	msgbox BattleFrontier_BattleArenaLobby_Text_DidntSaveBeforeShuttingDown, MSGBOX_DEFAULT
	closemessage
	arena_set ARENA_DATA_WIN_STREAK, 0
	arena_set ARENA_DATA_WIN_STREAK_ACTIVE, FALSE
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	releaseall
	end

BattleFrontier_BattleArenaLobby_EventScript_WonChallenge::
	lockall
	frontier_isbrain
	goto_if_eq VAR_RESULT, TRUE, BattleFrontier_BattleArenaLobby_EventScript_DefeatedTycoon
	msgbox BattleFrontier_BattleArenaLobby_Text_CongratsOnSevenWins, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaLobby_EventScript_GiveBattlePoints

BattleFrontier_BattleArenaLobby_EventScript_DefeatedTycoon::
	msgbox BattleFrontier_BattleArenaLobby_Text_CongratsOnDefeatingTycoon, MSGBOX_DEFAULT
BattleFrontier_BattleArenaLobby_EventScript_GiveBattlePoints::
	msgbox BattleFrontier_BattleArenaLobby_Text_PleaseAcceptBattlePoints, MSGBOX_DEFAULT
	frontier_givepoints
	msgbox BattleFrontier_Text_ObtainedXBattlePoints, MSGBOX_GETPOINTS
	message BattleFrontier_BattleArenaLobby_Text_RecordAchievement
	waitmessage
	call BattleFrontier_BattleArenaLobby_EventScript_SaveAfterChallenge
	msgbox BattleFrontier_BattleArenaLobby_Text_AwaitAnotherChallenge2, MSGBOX_DEFAULT
	closemessage
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	releaseall
	end

BattleFrontier_BattleArenaLobby_EventScript_LostChallenge::
	lockall
	message BattleFrontier_BattleArenaLobby_Text_ThankYouWaitWhileSave
	waitmessage
	arena_set ARENA_DATA_WIN_STREAK_ACTIVE, FALSE
	call BattleFrontier_BattleArenaLobby_EventScript_SaveAfterChallenge
	msgbox BattleFrontier_BattleArenaLobby_Text_AwaitAnotherChallenge2, MSGBOX_DEFAULT
	closemessage
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	releaseall
	end

BattleFrontier_BattleArenaLobby_EventScript_SaveAfterChallenge::
	frontier_checkairshow
	special LoadPlayerParty
	special HealPlayerParty
	arena_save 0
	playse SE_SAVE
	waitse
	call BattleFrontier_EventScript_GetCantRecordBattle
	goto_if_eq VAR_RESULT, TRUE, BattleFrontier_BattleArenaLobby_EventScript_EndSaveAfterChallenge
	message BattleFrontier_BattleArenaLobby_Text_RecordLastMatch
	waitmessage
	multichoicedefault 20, 8, MULTI_YESNO, 1, FALSE
	switch VAR_RESULT
	case 1, BattleFrontier_BattleArenaLobby_EventScript_EndSaveAfterChallenge
	case 0, BattleFrontier_BattleArenaLobby_EventScript_RecordMatch
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaLobby_EventScript_EndSaveAfterChallenge

BattleFrontier_BattleArenaLobby_EventScript_RecordMatch::
	call BattleFrontier_EventScript_SaveBattle
BattleFrontier_BattleArenaLobby_EventScript_EndSaveAfterChallenge::
	return

BattleFrontier_BattleArenaLobby_EventScript_ResumeChallenge::
	lockall
	message BattleFrontier_BattleArenaLobby_Text_LookingForwardToArrivalSaveGame
	waitmessage
	arena_save CHALLENGE_STATUS_SAVING
	playse SE_SAVE
	waitse
	frontier_set FRONTIER_DATA_PAUSED, FALSE
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	goto BattleFrontier_BattleArenaLobby_EventScript_EnterChallenge

BattleFrontier_BattleArenaLobby_EventScript_Attendant::
	lock
	faceplayer
	setvar VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_ARENA
	setvar VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES
	special SavePlayerParty
	msgbox BattleFrontier_BattleArenaLobby_Text_WelcomeToBattleArena, MSGBOX_DEFAULT
BattleFrontier_BattleArenaLobby_EventScript_AskTakeChallenge::
	message BattleFrontier_BattleArenaLobby_Text_WishToTakeChallenge
	waitmessage
	multichoice 17, 6, MULTI_CHALLENGEINFO, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_BattleArenaLobby_EventScript_TryEnterChallenge
	case 1, BattleFrontier_BattleArenaLobby_EventScript_ExplainChallenge
	case 2, BattleFrontier_BattleArenaLobby_EventScript_CancelChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaLobby_EventScript_CancelChallenge

BattleFrontier_BattleArenaLobby_EventScript_TryEnterChallenge::
	message BattleFrontier_BattleArenaLobby_Text_WhichLevelMode
	waitmessage
	multichoice 17, 6, MULTI_LEVEL_MODE, FALSE
	switch VAR_RESULT
	case FRONTIER_LVL_TENT, BattleFrontier_BattleArenaLobby_EventScript_CancelChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaLobby_EventScript_CancelChallenge
	frontier_checkineligible
	goto_if_eq VAR_0x8004, TRUE, BattleFrontier_BattleArenaLobby_EventScript_NotEnoughValidMons
	frontier_set FRONTIER_DATA_LVL_MODE, VAR_RESULT
	msgbox BattleFrontier_BattleArenaLobby_Text_SelectThreeMons, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	call BattleFrontier_EventScript_GetLvlMode
	copyvar VAR_0x8004, VAR_RESULT
	setvar VAR_0x8005, FRONTIER_PARTY_SIZE
	special ChoosePartyForBattleFrontier
	waitstate
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattleArenaLobby_EventScript_LoadPartyAndCancelChallenge
	msgbox BattleFrontier_BattleArenaLobby_Text_OkayToSave, MSGBOX_YESNO
	switch VAR_RESULT
	case NO, BattleFrontier_BattleArenaLobby_EventScript_LoadPartyAndCancelChallenge
	case YES, BattleFrontier_BattleArenaLobby_EventScript_SaveBeforeChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaLobby_EventScript_LoadPartyAndCancelChallenge

BattleFrontier_BattleArenaLobby_EventScript_SaveBeforeChallenge::
	setvar VAR_TEMP_CHALLENGE_STATUS, 0
	frontier_set FRONTIER_DATA_SELECTED_MON_ORDER
	arena_init
	arena_set ARENA_DATA_WIN_STREAK_ACTIVE, TRUE
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_SAVING
	frontier_set FRONTIER_DATA_PAUSED, FALSE
	special LoadPlayerParty
	closemessage
	delay 2
	call Common_EventScript_SaveGame
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattleArenaLobby_EventScript_CancelChallengeSaveFailed
BattleFrontier_BattleArenaLobby_EventScript_EnterChallenge::
	special SavePlayerParty
	frontier_setpartyorder FRONTIER_PARTY_SIZE
	msgbox BattleFrontier_BattleArenaLobby_Text_GuideYouToArena, MSGBOX_DEFAULT
	closemessage
	frontier_get FRONTIER_DATA_LVL_MODE
	call_if_eq VAR_RESULT, FRONTIER_LVL_50, BattleFrontier_BattleArenaLobby_EventScript_WalkToDoorLv50
	call_if_eq VAR_RESULT, FRONTIER_LVL_OPEN, BattleFrontier_BattleArenaLobby_EventScript_WalkToDoorLvOpen
	warp MAP_BATTLE_FRONTIER_BATTLE_ARENA_CORRIDOR, 9, 13
	setvar VAR_TEMP_CHALLENGE_STATUS, 0
	waitstate
	end

BattleFrontier_BattleArenaLobby_EventScript_ExplainChallenge::
	msgbox BattleFrontier_BattleArenaLobby_Text_ExplainChallenge, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaLobby_EventScript_AskTakeChallenge

BattleFrontier_BattleArenaLobby_EventScript_NotEnoughValidMons::
	switch VAR_RESULT
	case FRONTIER_LVL_50, BattleFrontier_BattleArenaLobby_EventScript_NotEnoughValidMonsLv50
	case FRONTIER_LVL_OPEN, BattleFrontier_BattleArenaLobby_EventScript_NotEnoughValidMonsLvOpen

BattleFrontier_BattleArenaLobby_EventScript_NotEnoughValidMonsLv50::
	msgbox BattleFrontier_BattleArenaLobby_Text_NotEnoughValidMonsLv50, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaLobby_EventScript_EndCancelChallenge

BattleFrontier_BattleArenaLobby_EventScript_NotEnoughValidMonsLvOpen::
	msgbox BattleFrontier_BattleArenaLobby_Text_NotEnoughValidMonsLvOpen, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaLobby_EventScript_EndCancelChallenge

BattleFrontier_BattleArenaLobby_EventScript_CancelChallengeSaveFailed::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	goto BattleFrontier_BattleArenaLobby_EventScript_CancelChallenge

BattleFrontier_BattleArenaLobby_EventScript_LoadPartyAndCancelChallenge::
	special LoadPlayerParty
BattleFrontier_BattleArenaLobby_EventScript_CancelChallenge::
	msgbox BattleFrontier_BattleArenaLobby_Text_AwaitAnotherChallenge, MSGBOX_DEFAULT
BattleFrontier_BattleArenaLobby_EventScript_EndCancelChallenge::
	release
	end

BattleFrontier_BattleArenaLobby_EventScript_WalkToDoorLv50::
	applymovement LOCALID_ARENA_ATTENDANT, BattleFrontier_BattleArenaLobby_Movement_AttendantWalkToLeftDoor
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaLobby_Movement_PlayerWalkToLeftDoor
	waitmovement 0
	opendoor 2, 2
	waitdooranim
	applymovement LOCALID_ARENA_ATTENDANT, BattleFrontier_BattleArenaLobby_Movement_AttendantEnterDoor
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaLobby_Movement_PlayerEnterDoor
	waitmovement 0
	closedoor 2, 2
	waitdooranim
	return

BattleFrontier_BattleArenaLobby_Movement_AttendantWalkToLeftDoor:
	walk_up
	walk_up
	walk_up
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_up
	step_end

BattleFrontier_BattleArenaLobby_Movement_AttendantEnterDoor:
	walk_up
	set_invisible
	step_end

BattleFrontier_BattleArenaLobby_Movement_PlayerWalkToLeftDoor:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_in_place_faster_up
	step_end

BattleFrontier_BattleArenaLobby_Movement_PlayerEnterDoor:
	walk_up
	walk_up
	set_invisible
	step_end

BattleFrontier_BattleArenaLobby_EventScript_WalkToDoorLvOpen::
	applymovement LOCALID_ARENA_ATTENDANT, BattleFrontier_BattleArenaLobby_Movement_AttendantWalkToRightDoor
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaLobby_Movement_PlayerWalkToRightDoor
	waitmovement 0
	opendoor 11, 2
	waitdooranim
	applymovement LOCALID_ARENA_ATTENDANT, BattleFrontier_BattleArenaLobby_Movement_AttendantEnterDoor
	applymovement LOCALID_PLAYER, BattleFrontier_BattleArenaLobby_Movement_PlayerEnterDoor
	waitmovement 0
	closedoor 11, 2
	waitdooranim
	return

BattleFrontier_BattleArenaLobby_Movement_AttendantWalkToRightDoor:
	walk_up
	walk_up
	walk_up
	walk_right
	walk_right
	walk_right
	walk_right
	walk_up
	step_end

BattleFrontier_BattleArenaLobby_Movement_PlayerWalkToRightDoor:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_right
	walk_right
	walk_right
	walk_right
	walk_in_place_faster_up
	step_end

BattleFrontier_BattleArenaLobby_EventScript_ShowResults::
	lockall
	frontier_results FRONTIER_FACILITY_ARENA
	waitbuttonpress
	special RemoveRecordsWindow
	releaseall
	end

BattleFrontier_BattleArenaLobby_EventScript_Youngster::
	msgbox BattleFrontier_BattleArenaLobby_Text_BadIdeaToNotAttack, MSGBOX_NPC
	end

BattleFrontier_BattleArenaLobby_EventScript_Man::
	msgbox BattleFrontier_BattleArenaLobby_Text_LandingHitsWorked, MSGBOX_NPC
	end

BattleFrontier_BattleArenaLobby_EventScript_Camper::
	msgbox BattleFrontier_BattleArenaLobby_Text_MatchWasDeclaredDraw, MSGBOX_NPC
	end

BattleFrontier_BattleArenaLobby_EventScript_Woman::
	msgbox BattleFrontier_BattleArenaLobby_Text_OrderOfMonsImportant, MSGBOX_NPC
	end

BattleFrontier_BattleArenaLobby_EventScript_RulesBoard::
	lockall
	msgbox BattleFrontier_BattleArenaLobby_Text_RulesAreListed, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleArenaLobby_EventScript_ReadRulesBoard::
	message BattleFrontier_BattleArenaLobby_Text_ReadWhichHeading
	waitmessage
	multichoice 17, 2, MULTI_BATTLE_ARENA_RULES, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_BattleArenaLobby_EventScript_BattleRules
	case 1, BattleFrontier_BattleArenaLobby_EventScript_MindRules
	case 2, BattleFrontier_BattleArenaLobby_EventScript_SkillRules
	case 3, BattleFrontier_BattleArenaLobby_EventScript_BodyRules
	case 4, BattleFrontier_BattleArenaLobby_EventScript_ExitRules
	case MULTI_B_PRESSED, BattleFrontier_BattleArenaLobby_EventScript_ExitRules
	end

BattleFrontier_BattleArenaLobby_EventScript_BattleRules::
	msgbox BattleFrontier_BattleArenaLobby_Text_ExplainBattleRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleArenaLobby_EventScript_MindRules::
	msgbox BattleFrontier_BattleArenaLobby_Text_ExplainMindRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleArenaLobby_EventScript_SkillRules::
	msgbox BattleFrontier_BattleArenaLobby_Text_ExplainSkillRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleArenaLobby_EventScript_BodyRules::
	msgbox BattleFrontier_BattleArenaLobby_Text_ExplainBodyRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattleArenaLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleArenaLobby_EventScript_ExitRules::
	releaseall
	end

BattleFrontier_BattleArenaLobby_Text_WelcomeToBattleArena:
	.string "Onde o espírito de luta dos\n"
	.string "TREINADORES é posto à prova!\p"
	.string "Boas-vindas à ARENA DE BATALHA!\p"
	.string "Sou o guia do Torneio de Nocaute!$"

BattleFrontier_BattleArenaLobby_Text_WishToTakeChallenge:
	.string "Agora, deseja encarar o desafio da\n"
	.string "ARENA DE BATALHA?$"

BattleFrontier_BattleArenaLobby_Text_AwaitAnotherChallenge:
	.string "Aguardamos o seu desafio em outra\n"
	.string "ocasião!$"

BattleFrontier_BattleArenaLobby_Text_ExplainChallenge:
	.string "Na ARENA DE BATALHA, fazemos o\n"
	.string "Torneio de Nocaute.\p"
	.string "Todo participante entra com uma\n"
	.string "equipe de três POKéMON.\p"
	.string "Os três POKéMON ficam na ordem em\n"
	.string "que vão entrar.\p"
	.string "Na batalha, os POKéMON entram um\n"
	.string "de cada vez, na ordem definida.\p"
	.string "Depois que um POKéMON entra, fica\n"
	.string "em campo até a disputa se decidir.\p"
	.string "Se nada se decidir em três turnos,\n"
	.string "vai pra decisão dos juízes.\p"
	.string "Se quiser interromper o desafio,\n"
	.string "salve o jogo.\p"
	.string "Se não salvar, não vai poder\n"
	.string "continuar o desafio.\p"
	.string "Se vencer sete TREINADORES em\n"
	.string "sequência, damos Pontos de Batalha.$"

BattleFrontier_BattleArenaLobby_Text_OkayToSave:
	.string "Antes de te levar à ARENA DE\n"
	.string "BATALHA, preciso salvar. Tudo bem?$"

BattleFrontier_BattleArenaLobby_Text_WhichLevelMode:
	.string "A ARENA DE BATALHA tem duas\n"
	.string "modalidades, Nível 50 e Nível Livre.\l"
	.string "Qual você escolhe?$"

BattleFrontier_BattleArenaLobby_Text_SelectThreeMons:
	.string "Muito bem, agora escolha os seus\n"
	.string "três POKéMON, por favor.$"

BattleFrontier_BattleArenaLobby_Text_NotEnoughValidMonsLvOpen:
	.string "Ilustre desafiante!\p"
	.string "Você não tem os três POKéMON\n"
	.string "exigidos pra inscrição.\p"
	.string "Pra participar, precisa de três\n"
	.string "POKéMON de espécies diferentes.\p"
	.string "Também não podem segurar o mesmo\n"
	.string "tipo de item.\p"
	.string "OVOS{STR_VAR_1} proibidos.\p"
	.string "Quando estiver tudo pronto, volte,\n"
	.string "por favor.$"

BattleFrontier_BattleArenaLobby_Text_NotEnoughValidMonsLv50:
	.string "Ilustre desafiante!\p"
	.string "Você não tem os três POKéMON\n"
	.string "exigidos pra inscrição.\p"
	.string "Pra participar, precisa de três\n"
	.string "POKéMON de espécies diferentes.\p"
	.string "Os três precisam ser de nível\n"
	.string "50 ou menos.\p"
	.string "Também não podem segurar o mesmo\n"
	.string "tipo de item.\p"
	.string "OVOS{STR_VAR_1} proibidos.\p"
	.string "Quando estiver tudo pronto, volte,\n"
	.string "por favor.$"

BattleFrontier_BattleArenaLobby_Text_GuideYouToArena:
	.string "Agora vou te levar à\n"
	.string "ARENA DE BATALHA.$"

BattleFrontier_BattleArenaLobby_Text_DidntSaveBeforeShuttingDown:
	.string "Ilustre desafiante!\p"
	.string "Você não salvou antes de desligar,\n"
	.string "não foi?\p"
	.string "É uma pena, mas isso anulou o\n"
	.string "seu desafio.\p"
	.string "Claro, você pode começar um\n"
	.string "desafio novo.$"

BattleFrontier_BattleArenaLobby_Text_CongratsOnSevenWins:
	.string "Parabéns pela esplêndida sequência\n"
	.string "de vitórias sobre sete TREINADORES!$"

BattleFrontier_BattleArenaLobby_Text_RecordAchievement:
	.string "A sua conquista será registrada.\n"
	.string "Aguarde enquanto salvo o jogo.$"

/* Unused*/
BattleFrontier_BattleArenaLobby_Text_PresentYouWithPrize:
	.string "Pra comemorar as 7 vitórias seguidas,\n"
	.string "aqui está este prêmio.$"

/* Unused*/
BattleFrontier_BattleArenaLobby_Text_ReceivedPrize:
	.string "{PLAYER} recebeu o prêmio:\n"
	.string "{STR_VAR_1}.$"

/* Unused*/
BattleFrontier_BattleArenaLobby_Text_BagFullReturnForPrize:
	.string "Opa?\n"
	.string "A sua BOLSA parece cheia.\p"
	.string "Abra espaço e volte pra pegar o\n"
	.string "seu prêmio.$"

BattleFrontier_BattleArenaLobby_Text_ThankYouWaitWhileSave:
	.string "Muito obrigado por participar!\p"
	.string "Aguarde enquanto salvo o jogo.$"

BattleFrontier_BattleArenaLobby_Text_AwaitAnotherChallenge2:
	.string "Aguardamos o seu desafio em outra\n"
	.string "ocasião!$"

BattleFrontier_BattleArenaLobby_Text_LookingForwardToArrivalSaveGame:
	.string "Estávamos esperando a sua chegada.\p"
	.string "Antes de te levar à ARENA DE\n"
	.string "BATALHA, preciso salvar o jogo.\l"
	.string "Aguarde, por favor.$"

BattleFrontier_BattleArenaLobby_Text_RecordLastMatch:
	.string "Quer gravar a sua última batalha da\n"
	.string "ARENA no PASSE DA FRONTEIRA?$"

BattleFrontier_BattleArenaLobby_Text_BadIdeaToNotAttack:
	.string "Perdi na decisão do JUIZ…\p"
	.string "Acho que não foi boa ideia usar só\n"
	.string "golpes de defesa e não atacar…$"

BattleFrontier_BattleArenaLobby_Text_LandingHitsWorked:
	.string "Ganhei na decisão!\p"
	.string "Acertar golpes sem parar nos POKéMON\n"
	.string "do oponente funcionou!$"

BattleFrontier_BattleArenaLobby_Text_MatchWasDeclaredDraw:
	.string "A nossa batalha deu empate.\p"
	.string "Quando o tempo acabou, o meu POKéMON\n"
	.string "e o do oponente tinham quase a\l"
	.string "mesma quantidade de PS.$"

BattleFrontier_BattleArenaLobby_Text_OrderOfMonsImportant:
	.string "Na ARENA DE BATALHA, a ordem dos\n"
	.string "POKéMON é importantíssima.\p"
	.string "Por exemplo, se o seu primeiro\n"
	.string "POKéMON tem desvantagens de tipo,\l"
	.string "faça o segundo ter golpes\l"
	.string "superefetivos contra o que vence\l"
	.string "o primeiro.\p"
	.string "Acho que é um bom jeito de montar\n"
	.string "uma equipe eficiente.$"

BattleFrontier_BattleArenaLobby_Text_RulesAreListed:
	.string "Aqui estão as regras do Torneio de\n"
	.string "Nocaute.$"

BattleFrontier_BattleArenaLobby_Text_ReadWhichHeading:
	.string "Qual tópico você quer ler?$"

BattleFrontier_BattleArenaLobby_Text_ExplainBattleRules:
	.string "O Torneio de Nocaute tem regras\n"
	.string "especiais, diferentes das batalhas\l"
	.string "comuns.\p"
	.string "Primeiro, cada batalha dura só\n"
	.string "três turnos.\p"
	.string "Se não se decidir em três turnos,\n"
	.string "vai pra decisão dos juízes.\p"
	.string "O JUIZ escolhe o vencedor pelo\n"
	.string "jeito como os POKéMON batalharam.\p"
	.string "E um POKéMON não pode ser trocado\n"
	.string "até a disputa dele se decidir.$"

BattleFrontier_BattleArenaLobby_Text_ExplainMindRules:
	.string "O primeiro critério é “Mente”.\n"
	.string "Avalia o quanto os POKéMON foram\l"
	.string "agressivos.\p"
	.string "A nota se baseia em quantas vezes\n"
	.string "os TREINADORES mandaram usar\l"
	.string "golpes ofensivos.$"

BattleFrontier_BattleArenaLobby_Text_ExplainSkillRules:
	.string "O segundo critério é “Técnica”.\n"
	.string "Avalia o quanto os golpes foram\l"
	.string "bem usados.\p"
	.string "Se um golpe funcionou, a nota de\n"
	.string "Técnica sobe.\p"
	.string "Se um golpe falhou, a nota de\n"
	.string "Técnica desce.\p"
	.string "Num golpe ofensivo, a nota sobe se\n"
	.string "foi “superefetivo” e desce se foi\l"
	.string "“pouco efetivo”.\p"
	.string "A nota de Técnica não sobe com\n"
	.string "golpes como PROTEÇÃO e DETECTAR.\p"
	.string "Se o oponente usou PROTEÇÃO ou\n"
	.string "DETECTAR e o seu POKéMON errou o\l"
	.string "golpe, a nota de Técnica não desce.$"

BattleFrontier_BattleArenaLobby_Text_ExplainBodyRules:
	.string "O terceiro critério é “Físico”.\n"
	.string "Se baseia em quantos PS sobraram\l"
	.string "no fim da batalha.\p"
	.string "A nota leva em conta os PS do POKéMON\n"
	.string "no começo da batalha e o que\l"
	.string "sobrou no fim.$"

BattleFrontier_BattleArenaLobby_Text_CongratsOnDefeatingTycoon:
	.string "Uma vitória arrancada da MAGNATA DA\n"
	.string "ARENA e sete TREINADORES vencidos!\p"
	.string "Parabéns por um desafio\n"
	.string "esplêndido!$"

BattleFrontier_BattleArenaLobby_Text_PleaseAcceptBattlePoints:
	.string "Ilustre desafiante, pelo seu espírito\n"
	.string "incansável, aceite estes Pontos\l"
	.string "de Batalha.$"
