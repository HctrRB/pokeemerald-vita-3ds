BattleFrontier_BattleFactoryLobby_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, BattleFrontier_BattleFactoryLobby_OnFrame
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, BattleFrontier_BattleFactoryLobby_OnWarp
	.byte 0

BattleFrontier_BattleFactoryLobby_OnWarp:
	map_script_2 VAR_TEMP_1, 0, BattleFrontier_BattleFactoryLobby_EventScript_TurnPlayerNorth
	.short 0

BattleFrontier_BattleFactoryLobby_EventScript_TurnPlayerNorth::
	setvar VAR_TEMP_1, 1
	turnobject LOCALID_PLAYER, DIR_NORTH
	end

BattleFrontier_BattleFactoryLobby_OnFrame:
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, 0, BattleFrontier_BattleFactoryLobby_EventScript_GetChallengeStatus
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_SAVING, BattleFrontier_BattleFactoryLobby_EventScript_QuitWithoutSaving
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_PAUSED, BattleFrontier_BattleFactoryLobby_EventScript_ResumeChallenge
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_WON, BattleFrontier_BattleFactoryLobby_EventScript_WonChallenge
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_LOST, BattleFrontier_BattleFactoryLobby_EventScript_LostChallenge
	.short 0

BattleFrontier_BattleFactoryLobby_EventScript_GetChallengeStatus::
	frontier_getstatus
	end

BattleFrontier_BattleFactoryLobby_EventScript_QuitWithoutSaving::
	lockall
	msgbox BattleFrontier_BattleFactoryLobby_Text_DidntSaveBeforeQuitting, MSGBOX_DEFAULT
	closemessage
	factory_setswapped
	factory_set FACTORY_DATA_WIN_STREAK_SWAPS, 0
	factory_set FACTORY_DATA_WIN_STREAK, 0
	factory_set FACTORY_DATA_WIN_STREAK_ACTIVE, FALSE
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	releaseall
	end

BattleFrontier_BattleFactoryLobby_EventScript_WonChallenge::
	lockall
	frontier_isbrain
	goto_if_eq VAR_RESULT, TRUE, BattleFrontier_BattleFactoryLobby_EventScript_DefeatedFactoryHead
	msgbox BattleFrontier_BattleFactoryLobby_Text_CongratsSevenWins, MSGBOX_DEFAULT
	waitmessage
	goto BattleFrontier_BattleFactoryLobby_EventScript_GiveBattlePoints

BattleFrontier_BattleFactoryLobby_EventScript_DefeatedFactoryHead::
	msgbox BattleFrontier_BattleFactoryLobby_Text_CongratsForDefeatingHead, MSGBOX_DEFAULT
	waitmessage
BattleFrontier_BattleFactoryLobby_EventScript_GiveBattlePoints::
	msgbox BattleFrontier_BattleFactoryLobby_Text_AwardBattlePoints, MSGBOX_DEFAULT
	frontier_givepoints
	msgbox BattleFrontier_Text_ObtainedXBattlePoints, MSGBOX_GETPOINTS
	message BattleFrontier_BattleFactoryLobby_Text_ExchangeMonsAndSave
	waitmessage
	frontier_checkairshow
	special LoadPlayerParty
	factory_save 0
	playse SE_SAVE
	waitse
	goto BattleFrontier_BattleFactoryLobby_EventScript_AskRecordBattle
	end

BattleFrontier_BattleFactoryLobby_EventScript_LostChallenge::
	lockall
	message BattleFrontier_BattleFactoryLobby_Text_ReturnMonsSaveResults
	waitmessage
	frontier_checkairshow
	special LoadPlayerParty
	factory_set FACTORY_DATA_WIN_STREAK_ACTIVE, FALSE
	factory_save 0
	playse SE_SAVE
	waitse

BattleFrontier_BattleFactoryLobby_EventScript_AskRecordBattle::
	call BattleFrontier_EventScript_GetCantRecordBattle
	goto_if_eq VAR_RESULT, TRUE, BattleFrontier_BattleFactoryLobby_EventScript_EndRecordBattle
	message BattleFrontier_BattleFactoryLobby_Text_RecordLastMatch
	waitmessage
	multichoicedefault 20, 8, MULTI_YESNO, 1, FALSE
	switch VAR_RESULT
	case 1, BattleFrontier_BattleFactoryLobby_EventScript_EndRecordBattle
	case 0, BattleFrontier_BattleFactoryLobby_EventScript_RecordBattle
	case MULTI_B_PRESSED, BattleFrontier_BattleFactoryLobby_EventScript_EndRecordBattle

BattleFrontier_BattleFactoryLobby_EventScript_RecordBattle::
	call BattleFrontier_EventScript_SaveBattle
BattleFrontier_BattleFactoryLobby_EventScript_EndRecordBattle::
	msgbox BattleFrontier_BattleFactoryLobby_Text_LookForwardToNextVisit, MSGBOX_DEFAULT
	closemessage
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	releaseall
	end

BattleFrontier_BattleFactoryLobby_EventScript_ResumeChallenge::
	lockall
	message BattleFrontier_BattleFactoryLobby_Text_WaitingForYouToResume
	waitmessage
	factory_save CHALLENGE_STATUS_SAVING
	playse SE_SAVE
	waitse
	frontier_set FRONTIER_DATA_PAUSED, FALSE
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	setvar VAR_0x8006, 2
	goto BattleFrontier_BattleFactoryLobby_EventScript_EnterChallenge

BattleFrontier_BattleFactoryLobby_EventScript_SinglesAttendant::
	lock
	faceplayer
	setvar VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_FACTORY
	setvar VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES
	goto BattleFrontier_BattleFactoryLobby_EventScript_Attendant
	end

BattleFrontier_BattleFactoryLobby_EventScript_DoublesAttendant::
	setvar VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_FACTORY
	setvar VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_DOUBLES
	goto BattleFrontier_BattleFactoryLobby_EventScript_Attendant
	end

BattleFrontier_BattleFactoryLobby_EventScript_Attendant::
	special SavePlayerParty
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES, BattleFrontier_BattleFactoryLobby_EventScript_WelcomeForSingleBattle
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_DOUBLES, BattleFrontier_BattleFactoryLobby_EventScript_WelcomeForDoubleBattle
BattleFrontier_BattleFactoryLobby_EventScript_AskTakeChallenge::
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES, BattleFrontier_BattleFactoryLobby_EventScript_TakeSinglesChallenge
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_DOUBLES, BattleFrontier_BattleFactoryLobby_EventScript_TakeDoublesChallenge
	waitmessage
	multichoice 17, 6, MULTI_CHALLENGEINFO, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_BattleFactoryLobby_EventScript_TryEnterChallenge
	case 1, BattleFrontier_BattleFactoryLobby_EventScript_ExplainChallenge
	case 2, BattleFrontier_BattleFactoryLobby_EventScript_CancelChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleFactoryLobby_EventScript_CancelChallenge

BattleFrontier_BattleFactoryLobby_EventScript_TryEnterChallenge::
	message BattleFrontier_BattleFactoryLobby_Text_WhichLevelMode
	waitmessage
	multichoice 17, 6, MULTI_LEVEL_MODE, FALSE
	switch VAR_RESULT
	case FRONTIER_LVL_TENT, BattleFrontier_BattleFactoryLobby_EventScript_CancelChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleFactoryLobby_EventScript_CancelChallenge
	frontier_set FRONTIER_DATA_LVL_MODE, VAR_RESULT
	msgbox BattleFrontier_BattleFactoryLobby_Text_OkayToSaveBeforeChallenge, MSGBOX_YESNO
	switch VAR_RESULT
	case NO, BattleFrontier_BattleFactoryLobby_EventScript_LoadPartyAndCancelChallenge
	case YES, BattleFrontier_BattleFactoryLobby_EventScript_SaveBeforeChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattleFactoryLobby_EventScript_LoadPartyAndCancelChallenge

BattleFrontier_BattleFactoryLobby_EventScript_SaveBeforeChallenge::
	setvar VAR_TEMP_CHALLENGE_STATUS, 0
	factory_init
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_SAVING
	factory_set FACTORY_DATA_WIN_STREAK_ACTIVE, TRUE
	frontier_set FRONTIER_DATA_PAUSED, FALSE
	closemessage
	delay 2
	call Common_EventScript_SaveGame
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattleFactoryLobby_EventScript_CancelChallengeSaveFailed
	setvar VAR_0x8006, 0
BattleFrontier_BattleFactoryLobby_EventScript_EnterChallenge::
	special SavePlayerParty
	msgbox BattleFrontier_BattleFactoryLobby_Text_StepThisWay, MSGBOX_DEFAULT
	closemessage
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES, BattleFrontier_BattleFactoryLobby_EventScript_TalkedToSinglesAttendant
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_DOUBLES, BattleFrontier_BattleFactoryLobby_EventScript_TalkedToDoublesAttendant
	applymovement VAR_LAST_TALKED, BattleFrontier_BattleFactoryLobby_Movement_AttendantEnterDoor
	applymovement LOCALID_PLAYER, BattleFrontier_BattleFactoryLobby_Movement_PlayerEnterDoor
	waitmovement 0
	warp MAP_BATTLE_FRONTIER_BATTLE_FACTORY_PRE_BATTLE_ROOM, 8, 13
	setvar VAR_TEMP_CHALLENGE_STATUS, 0
	waitstate
	end

BattleFrontier_BattleFactoryLobby_EventScript_TalkedToSinglesAttendant::
	setvar VAR_LAST_TALKED, LOCALID_FACTORY_ATTENDANT_SINGLES
	return

BattleFrontier_BattleFactoryLobby_EventScript_TalkedToDoublesAttendant::
	setvar VAR_LAST_TALKED, LOCALID_FACTORY_ATTENDANT_DOUBLES
	return

BattleFrontier_BattleFactoryLobby_EventScript_ExplainChallenge::
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES, BattleFrontier_BattleFactoryLobby_EventScript_ExplainSinglesChallenge
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_DOUBLES, BattleFrontier_BattleFactoryLobby_EventScript_ExplainDoublesChallenge
	goto BattleFrontier_BattleFactoryLobby_EventScript_AskTakeChallenge

BattleFrontier_BattleFactoryLobby_EventScript_CancelChallengeSaveFailed::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	goto BattleFrontier_BattleFactoryLobby_EventScript_CancelChallenge

BattleFrontier_BattleFactoryLobby_EventScript_LoadPartyAndCancelChallenge::
	special LoadPlayerParty
BattleFrontier_BattleFactoryLobby_EventScript_CancelChallenge::
	msgbox BattleFrontier_BattleFactoryLobby_Text_LookForwardToNextVisit, MSGBOX_DEFAULT
	release
	end

BattleFrontier_BattleFactoryLobby_Movement_AttendantEnterDoor:
	walk_up
	walk_up
	walk_up
	set_invisible
	step_end

BattleFrontier_BattleFactoryLobby_Movement_PlayerEnterDoor:
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

BattleFrontier_BattleFactoryLobby_EventScript_WelcomeForSingleBattle::
	msgbox BattleFrontier_BattleFactoryLobby_Text_WelcomeForSingleBattle, MSGBOX_DEFAULT
	return

BattleFrontier_BattleFactoryLobby_EventScript_WelcomeForDoubleBattle::
	msgbox BattleFrontier_BattleFactoryLobby_Text_WelcomeForDoubleBattle, MSGBOX_DEFAULT
	return

BattleFrontier_BattleFactoryLobby_EventScript_TakeSinglesChallenge::
	message BattleFrontier_BattleFactoryLobby_Text_TakeSinglesChallenge
	return

BattleFrontier_BattleFactoryLobby_EventScript_TakeDoublesChallenge::
	message BattleFrontier_BattleFactoryLobby_Text_TakeDoublesChallenge
	return

BattleFrontier_BattleFactoryLobby_EventScript_ExplainSinglesChallenge::
	msgbox BattleFrontier_BattleFactoryLobby_Text_ExplainSinglesChallenge, MSGBOX_DEFAULT
	return

BattleFrontier_BattleFactoryLobby_EventScript_ExplainDoublesChallenge::
	msgbox BattleFrontier_BattleFactoryLobby_Text_ExplainDoublesChallenge, MSGBOX_DEFAULT
	return

BattleFrontier_BattleFactoryLobby_EventScript_ShowSinglesResults::
	lockall
	frontier_results FRONTIER_FACILITY_FACTORY, FRONTIER_MODE_SINGLES
	waitbuttonpress
	special RemoveRecordsWindow
	releaseall
	end

BattleFrontier_BattleFactoryLobby_EventScript_ShowDoublesResults::
	lockall
	frontier_results FRONTIER_FACILITY_FACTORY, FRONTIER_MODE_DOUBLES
	waitbuttonpress
	special RemoveRecordsWindow
	releaseall
	end

BattleFrontier_BattleFactoryLobby_EventScript_Woman::
	msgbox BattleFrontier_BattleFactoryLobby_Text_NeedKnowledgeOfMonsMoves, MSGBOX_NPC
	end

BattleFrontier_BattleFactoryLobby_EventScript_Camper::
	msgbox BattleFrontier_BattleFactoryLobby_Text_SwappedForWeakMon, MSGBOX_NPC
	end

BattleFrontier_BattleFactoryLobby_EventScript_Picnicker::
	lock
	msgbox BattleFrontier_BattleFactoryLobby_Text_NeedToCheckOpponentsMons, MSGBOX_DEFAULT
	release
	end

BattleFrontier_BattleFactoryLobby_EventScript_FatMan::
	msgbox BattleFrontier_BattleFactoryLobby_Text_CantFigureOutStaffHints, MSGBOX_NPC
	end

BattleFrontier_BattleFactoryLobby_EventScript_RulesBoard::
	lockall
	msgbox BattleFrontier_BattleFactoryLobby_Text_RulesAreListed, MSGBOX_DEFAULT
	goto BattleFrontier_BattleFactoryLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleFactoryLobby_EventScript_ReadRulesBoard::
	message BattleFrontier_BattleFactoryLobby_Text_ReadWhichHeading
	waitmessage
	multichoice 17, 0, MULTI_BATTLE_FACTORY_RULES, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_BattleFactoryLobby_EventScript_RulesBasics
	case 1, BattleFrontier_BattleFactoryLobby_EventScript_RulesSwapPartner
	case 2, BattleFrontier_BattleFactoryLobby_EventScript_RulesSwapNumber
	case 3, BattleFrontier_BattleFactoryLobby_EventScript_RulesSwapNotes
	case 4, BattleFrontier_BattleFactoryLobby_EventScript_RulesOpenLv
	case 5, BattleFrontier_BattleFactoryLobby_EventScript_ExitRules
	case MULTI_B_PRESSED, BattleFrontier_BattleFactoryLobby_EventScript_ExitRules
	end

BattleFrontier_BattleFactoryLobby_EventScript_RulesBasics::
	msgbox BattleFrontier_BattleFactoryLobby_Text_ExplainBasicRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattleFactoryLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleFactoryLobby_EventScript_RulesSwapPartner::
	msgbox BattleFrontier_BattleFactoryLobby_Text_ExplainSwapPartnerRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattleFactoryLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleFactoryLobby_EventScript_RulesSwapNumber::
	msgbox BattleFrontier_BattleFactoryLobby_Text_ExplainSwapNumberRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattleFactoryLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleFactoryLobby_EventScript_RulesSwapNotes::
	msgbox BattleFrontier_BattleFactoryLobby_Text_ExplainSwapNotesRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattleFactoryLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleFactoryLobby_EventScript_RulesOpenLv::
	msgbox BattleFrontier_BattleFactoryLobby_Text_ExplainOpenLvRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattleFactoryLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattleFactoryLobby_EventScript_ExitRules::
	releaseall
	end

BattleFrontier_BattleFactoryLobby_Text_WelcomeForSingleBattle:
	.string "Onde a inteligência dos TREINADORES\n"
	.string "é posta à prova!\p"
	.string "Boas-vindas à FÁBRICA DE BATALHA!\p"
	.string "Sou o guia da Troca de Batalha\n"
	.string "Simples.$"

BattleFrontier_BattleFactoryLobby_Text_TakeSinglesChallenge:
	.string "Quer encarar o desafio da Troca\n"
	.string "de Batalha Simples?$"

BattleFrontier_BattleFactoryLobby_Text_ExplainSinglesChallenge:
	.string "A Troca de Batalha Simples é uma\n"
	.string "competição de BATALHAS SIMPLES só\l"
	.string "com POKéMON de aluguel.\p"
	.string "Primeiro, você recebe emprestados\n"
	.string "três POKéMON só pra este evento.\p"
	.string "Com esses POKéMON de aluguel, você\n"
	.string "faz uma BATALHA SIMPLES.\p"
	.string "Se vencer, pode trocar um dos seus\n"
	.string "POKéMON de aluguel.\p"
	.string "Repita esse ciclo de batalhar e\n"
	.string "trocar: se vencer sete vezes\l"
	.string "seguidas, você ganha Pontos de\l"
	.string "Batalha.\p"
	.string "Se quiser interromper o desafio,\n"
	.string "salve o jogo.\p"
	.string "Se não salvar, o seu desafio\n"
	.string "será anulado.$"

BattleFrontier_BattleFactoryLobby_Text_LookForwardToNextVisit:
	.string "Esperamos a sua próxima visita.$"

BattleFrontier_BattleFactoryLobby_Text_WhichLevelMode:
	.string "Que nível você quer desafiar?\n"
	.string "Nível 50 ou Nível Livre?$"

BattleFrontier_BattleFactoryLobby_Text_OkayToSaveBeforeChallenge:
	.string "Antes de começar o desafio, preciso\n"
	.string "salvar o jogo. Tudo bem?$"

/* Unused*/
BattleFrontier_BattleFactoryLobby_Text_WillHoldMonsForSafekeeping:
	.string "Certo, vou guardar os seus POKéMON\n"
	.string "enquanto você compete.$"

BattleFrontier_BattleFactoryLobby_Text_StepThisWay:
	.string "Por aqui, por favor.$"

BattleFrontier_BattleFactoryLobby_Text_ReturnMonsSaveResults:
	.string "Obrigado por participar!\p"
	.string "Vou devolver os seus POKéMON em troca\n"
	.string "dos nossos POKéMON de aluguel.\p"
	.string "Também preciso salvar os resultados.\n"
	.string "Aguarde, por favor.$"

/* Unused*/
BattleFrontier_BattleFactoryLobby_Text_ReturnMons:
	.string "Vou devolver os seus POKéMON em troca\n"
	.string "dos nossos POKéMON de aluguel.$"

BattleFrontier_BattleFactoryLobby_Text_CongratsSevenWins:
	.string "Parabéns! Você venceu sete batalhas\n"
	.string "seguidas na Troca de Batalha!$"

/* Unused*/
BattleFrontier_BattleFactoryLobby_Text_AwardBattlePointsForStreak:
	.string "Pela sequência de 7 vitórias, aqui\n"
	.string "estão os seus Pontos de Batalha.$"

/* Unused*/
BattleFrontier_BattleFactoryLobby_Text_MaxBattlePoints:
	.string "Opa, opa, opa!\p"
	.string "Os seus Pontos de Batalha estão\n"
	.string "no máximo.\p"
	.string "Volte depois de usar alguns\n"
	.string "Pontos de Batalha.$"

BattleFrontier_BattleFactoryLobby_Text_WaitingForYouToResume:
	.string "Estávamos esperando você!\p"
	.string "Antes de retomar o desafio, preciso\n"
	.string "salvar o jogo.$"

BattleFrontier_BattleFactoryLobby_Text_DidntSaveBeforeQuitting:
	.string "Sinto muito, mas você não salvou antes\n"
	.string "de parar de jogar da última vez.\p"
	.string "Por isso, o seu desafio foi anulado.$"

/* Unused*/
BattleFrontier_BattleFactoryLobby_Text_WellReturnMons:
	.string "Vamos devolver os seus POKéMON.$"

/* Unused*/
BattleFrontier_BattleFactoryLobby_Text_ReceivedPrizeItem:
	.string "{PLAYER} recebeu o prêmio:\n"
	.string "{STR_VAR_1}.$"

BattleFrontier_BattleFactoryLobby_Text_WelcomeForDoubleBattle:
	.string "Onde a inteligência dos TREINADORES\n"
	.string "é posta à prova!\p"
	.string "Boas-vindas à FÁBRICA DE BATALHA!\p"
	.string "Sou o guia da Troca de Batalha\n"
	.string "Dupla.$"

BattleFrontier_BattleFactoryLobby_Text_TakeDoublesChallenge:
	.string "Quer encarar o desafio da Troca\n"
	.string "de Batalha Dupla?$"

BattleFrontier_BattleFactoryLobby_Text_ExplainDoublesChallenge:
	.string "A Troca de Batalha Dupla é uma\n"
	.string "competição de BATALHAS DUPLAS só\l"
	.string "com POKéMON de aluguel.\p"
	.string "Primeiro, você recebe emprestados\n"
	.string "três POKéMON só pra este evento.\p"
	.string "Com esses POKéMON de aluguel, você\n"
	.string "faz uma BATALHA DUPLA.\p"
	.string "Se vencer, pode trocar um dos seus\n"
	.string "POKéMON de aluguel.\p"
	.string "Repita esse ciclo de batalhar e\n"
	.string "trocar: se vencer sete vezes\l"
	.string "seguidas, você ganha Pontos de\l"
	.string "Batalha.\p"
	.string "Se quiser interromper o desafio,\n"
	.string "salve o jogo.\p"
	.string "Se não salvar antes de interromper,\n"
	.string "o desafio será anulado.$"

BattleFrontier_BattleFactoryLobby_Text_CongratsForDefeatingHead:
	.string "Parabéns por vencer o CHEFE DA\n"
	.string "FÁBRICA e ganhar sete batalhas\l"
	.string "seguidas!$"

BattleFrontier_BattleFactoryLobby_Text_AwardBattlePoints:
	.string "Pelo seu conhecimento vasto e pela\n"
	.string "sua inteligência afiada, aqui estão\l"
	.string "os seus Pontos de Batalha.$"

BattleFrontier_BattleFactoryLobby_Text_ExchangeMonsAndSave:
	.string "Deixa eu trocar os seus POKéMON\n"
	.string "pelos nossos POKéMON de aluguel.\p"
	.string "Preciso salvar os dados da batalha,\n"
	.string "então aguarde, por favor.$"

BattleFrontier_BattleFactoryLobby_Text_RecordLastMatch:
	.string "Quer gravar a sua última batalha da\n"
	.string "FÁBRICA no PASSE DA FRONTEIRA?$"

BattleFrontier_BattleFactoryLobby_Text_NeedKnowledgeOfMonsMoves:
	.string "Oi!\n"
	.string "Você aí!\p"
	.string "Acha que os eventos daqui são fáceis\n"
	.string "porque não precisa de uma equipe\l"
	.string "de POKéMON treinada?\p"
	.string "Eu não teria tanta certeza de que\n"
	.string "vai ganhar fácil assim.\p"
	.string "Sem conhecer bem os POKéMON e os\n"
	.string "golpes, vai ser difícil continuar\l"
	.string "vencendo.$"

BattleFrontier_BattleFactoryLobby_Text_SwappedForWeakMon:
	.string "Troquei por um POKéMON fraco…\n"
	.string "Achei que fosse um bom…\p"
	.string "Acabaram com a gente…$"

BattleFrontier_BattleFactoryLobby_Text_NeedToCheckOpponentsMons:
	.string "Nada está dando certo pra mim.\p"
	.string "Veja bem os POKéMON do oponente\n"
	.string "na batalha pra saber se são bons.$"

BattleFrontier_BattleFactoryLobby_Text_CantFigureOutStaffHints:
	.string "Sabe que o pessoal daqui dá umas\n"
	.string "dicas sobre o próximo oponente?\p"
	.string "Pois é, sou um homem feito, mas não\n"
	.string "consigo entender as dicas deles.$"

/* Unused*/
BattleFrontier_BattleFactoryLobby_Text_RentalMonsAreVaried:
	.string "Tipo, eu sou muito forte, mas canso\n"
	.string "fácil, então fui trocando e\l"
	.string "batalhando sem parar.\p"
	.string "Enfim, entrando sempre na Troca de\n"
	.string "Batalha, percebi que os POKéMON\l"
	.string "de aluguel variam.$"

BattleFrontier_BattleFactoryLobby_Text_RulesAreListed:
	.string "Aqui estão as regras da Troca\n"
	.string "de Batalha.$"

BattleFrontier_BattleFactoryLobby_Text_ReadWhichHeading:
	.string "Qual tópico você quer ler?$"

BattleFrontier_BattleFactoryLobby_Text_ExplainBasicRules:
	.string "Na Troca de Batalha, você só pode\n"
	.string "usar três POKéMON.\p"
	.string "Alugando ou trocando, a sua equipe\n"
	.string "não pode ter dois POKéMON da\l"
	.string "mesma espécie.$"

BattleFrontier_BattleFactoryLobby_Text_ExplainSwapPartnerRules:
	.string "Você só pode trocar POKéMON com o\n"
	.string "TREINADOR que acabou de vencer.\p"
	.string "Só dá pra trocar pelos POKéMON que\n"
	.string "esse TREINADOR usou.$"

BattleFrontier_BattleFactoryLobby_Text_ExplainSwapNumberRules:
	.string "A cada vitória, você pode trocar\n"
	.string "por um POKéMON do oponente\l"
	.string "derrotado.\p"
	.string "Não dá pra trocar POKéMON com o\n"
	.string "sétimo TREINADOR do evento.$"

BattleFrontier_BattleFactoryLobby_Text_ExplainSwapNotesRules:
	.string "Há dois pontos importantes ao trocar\n"
	.string "POKéMON.\p"
	.string "Primeiro: ao trocar, você não vê os\n"
	.string "atributos do POKéMON que vai\l"
	.string "receber.\p"
	.string "Segundo: os POKéMON da sua equipe\n"
	.string "ficam na ordem em que foram\l"
	.string "alugados.\p"
	.string "Essa ordem não muda, mesmo com\n"
	.string "as trocas.$"

BattleFrontier_BattleFactoryLobby_Text_ExplainOpenLvRules:
	.string "No Nível Livre, os POKéMON de\n"
	.string "aluguel e os dos TREINADORES\l"
	.string "oponentes são todos nível 100.$"
