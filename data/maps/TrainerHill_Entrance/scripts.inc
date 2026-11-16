TrainerHill_Entrance_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, TrainerHill_Entrance_OnResume
	map_script MAP_SCRIPT_ON_RETURN_TO_FIELD, TrainerHill_Entrance_OnReturn
	map_script MAP_SCRIPT_ON_TRANSITION, TrainerHill_Entrance_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, TrainerHill_Entrance_OnLoad
	map_script MAP_SCRIPT_ON_FRAME_TABLE, TrainerHill_Entrance_OnFrame
	.byte 0

TrainerHill_Entrance_OnTransition:
	setflag FLAG_LANDMARK_TRAINER_HILL
	getplayerxy VAR_TEMP_D, VAR_RESULT
	end

/* Unused*/
TrainerHill_Entrance_OnWarp:
	end

TrainerHill_Entrance_OnResume:
	trainerhill_resumetimer
	setvar VAR_TEMP_0, 0
	trainerhill_getusingereader
	goto_if_eq VAR_RESULT, FALSE, TrainerHill_Entrance_EventScript_TryFaceAttendant  @ VAR_RESULT always FALSE here
	setobjectxy LOCALID_PLAYER, 9, 6
	applymovement LOCALID_PLAYER, TrainerHill_Entrance_Movement_PlayerFaceAttendant
	end

TrainerHill_Entrance_EventScript_TryFaceAttendant::
	trainerhill_getwon
	goto_if_eq VAR_RESULT, TRUE, TrainerHill_Entrance_EventScript_PlayerDontFaceAttendant
	applymovement LOCALID_PLAYER, TrainerHill_Entrance_Movement_PlayerFaceAttendant
	end

TrainerHill_Entrance_EventScript_PlayerDontFaceAttendant::
	end

TrainerHill_Entrance_OnReturn:
	addobject LOCALID_TRAINER_HILL_NURSE
	addobject LOCALID_TRAINER_HILL_ATTENDANT
	addobject LOCALID_TRAINER_HILL_MAN
	addobject LOCALID_TRAINER_HILL_GIRL
	end

TrainerHill_Entrance_OnLoad:
	call_if_eq VAR_TEMP_D, 17, TrainerHill_Entrance_EventScript_OpenCounterDoor
	end

TrainerHill_Entrance_EventScript_OpenCounterDoor::
	setmetatile 17, 10, METATILE_TrainerHill_GreenFloorTile, FALSE
	return

TrainerHill_Entrance_OnFrame:
	map_script_2 VAR_TEMP_0, 0, TrainerHill_Entrance_EventScript_ExitChallenge
	map_script_2 VAR_TEMP_D, 17, TrainerHill_Entrance_EventScript_ExitElevator
	map_script_2 VAR_TEMP_5, 1, TrainerHill_Entrance_EventScript_EntryTrigger
	map_script_2 VAR_TEMP_1, 1, TrainerHill_EventScript_WarpToEntranceCounter
	.short 0

TrainerHill_Entrance_EventScript_ExitElevator::
	lockall
	applymovement LOCALID_PLAYER, TrainerHill_Entrance_Movement_PlayerExitElevator
	waitmovement 0
	setmetatile 17, 10, METATILE_TrainerHill_CounterDoor, TRUE
	special DrawWholeMapView
	playse SE_CLICK
	waitse
	setvar VAR_TEMP_D, 0
	releaseall
	end

TrainerHill_Entrance_EventScript_ExitChallenge::
	setvar VAR_TEMP_0, 1
	trainerhill_getstatus
	switch VAR_RESULT
	case TRAINER_HILL_PLAYER_STATUS_LOST, TrainerHill_Entrance_EventScript_ExitChallengeLost
	case TRAINER_HILL_PLAYER_STATUS_ECARD_SCANNED, TrainerHill_Entrance_EventScript_ExitChallengeECard
	case TRAINER_HILL_PLAYER_STATUS_NORMAL, TrainerHill_Entrance_EventScript_EndExitChallenge

TrainerHill_Entrance_EventScript_ExitChallengeLost::
	lockall
	applymovement LOCALID_PLAYER, TrainerHill_Entrance_Movement_PlayerFaceAttendant
	msgbox TrainerHill_Entrance_Text_TooBadTremendousEffort, MSGBOX_DEFAULT
	goto TrainerHill_Entrance_EventScript_PlayerExitChallenge

TrainerHill_Entrance_EventScript_ExitChallengeECard::
	lockall
	applymovement LOCALID_PLAYER, TrainerHill_Entrance_Movement_PlayerFaceAttendant
	msgbox TrainerHill_Entrance_Text_MovedReceptionHereForSwitch, MSGBOX_DEFAULT
TrainerHill_Entrance_EventScript_PlayerExitChallenge::
	closemessage
	applymovement LOCALID_PLAYER, TrainerHill_Entrance_Movement_PushPlayerBackFromCounter
	waitmovement 0
	setvar VAR_TRAINER_HILL_IS_ACTIVE, 0
	special HealPlayerParty
	releaseall
TrainerHill_Entrance_EventScript_EndExitChallenge::
	end

TrainerHill_Entrance_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_TRAINER_HILL_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

TrainerHill_Entrance_EventScript_Attendant::
	lock
	faceplayer
	trainerhill_inchallenge
	goto_if_eq VAR_RESULT, FALSE, TrainerHill_Entrance_EventScript_ThanksForPlaying
	msgbox TrainerHill_Entrance_Text_HopeYouGiveItYourBest, MSGBOX_DEFAULT
	goto TrainerHill_Entrance_EventScript_AttendantEnd

TrainerHill_Entrance_EventScript_ThanksForPlaying::
	msgbox TrainerHill_Entrance_Text_ThankYouForPlaying, MSGBOX_DEFAULT
TrainerHill_Entrance_EventScript_AttendantEnd::
	release
	end

TrainerHill_Entrance_EventScript_EntryTrigger::
	lockall
	applymovement LOCALID_PLAYER, TrainerHill_Entrance_Movement_PlayerFaceAttendant
	goto_if_unset FLAG_SYS_GAME_CLEAR, TrainerHill_Entrance_EventScript_Closed
	msgbox TrainerHill_Entrance_Text_WelcomeToTrainerHill, MSGBOX_DEFAULT
	trainerhill_getsaved
	call_if_eq VAR_RESULT, FALSE, TrainerHill_Entrance_EventScript_SaveGame
	trainerhill_allfloorsused
	goto_if_eq VAR_RESULT, TRUE, TrainerHill_Entrance_EventScript_AllFloorsUsed
	msgbox TrainerHill_Entrance_Text_TrainersUpToFloorX, MSGBOX_DEFAULT
	goto TrainerHill_Entrance_EventScript_AskChallengeTrainers

TrainerHill_Entrance_EventScript_AllFloorsUsed::
	msgbox TrainerHill_Entrance_Text_TrainersInEveryRoom, MSGBOX_DEFAULT
TrainerHill_Entrance_EventScript_AskChallengeTrainers::
	message TrainerHill_Entrance_Text_LikeToChallengeTrainers
	waitmessage
	multichoice 15, 6, MULTI_YESNOINFO, FALSE
	switch VAR_RESULT
	case 0, TrainerHill_Entrance_EventScript_ChooseChallenge
	case 1, TrainerHill_Entrance_EventScript_CancelEntry
	case 2, TrainerHill_Entrance_EventScript_Info
	case MULTI_B_PRESSED, TrainerHill_Entrance_EventScript_CancelEntry
	end

TrainerHill_Entrance_EventScript_Info::
	msgbox TrainerHill_Entrance_Text_ExplainTrainerHill, MSGBOX_DEFAULT
	goto TrainerHill_Entrance_EventScript_AskChallengeTrainers
	end

TrainerHill_Entrance_EventScript_ChooseChallenge::
	multichoice 13, 2, MULTI_TAG_MATCH_TYPE, FALSE
	switch VAR_RESULT
	case 4, TrainerHill_Entrance_EventScript_CancelEntry
	case MULTI_B_PRESSED, TrainerHill_Entrance_EventScript_CancelEntry
	trainerhill_setmode VAR_RESULT
	setvar VAR_TRAINER_HILL_IS_ACTIVE, 1
	setvar VAR_TEMP_5, 0
	special HealPlayerParty
	msgbox TrainerHill_Entrance_Text_TimeProgessGetSetGo, MSGBOX_DEFAULT
	trainerhill_start
	releaseall
	end

TrainerHill_Entrance_EventScript_CancelEntry::
	setvar VAR_TEMP_5, 0
	msgbox TrainerHill_Entrance_Text_PleaseVisitUsAgain, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, TrainerHill_Entrance_Movement_PushPlayerBackFromCounter
	waitmovement 0
	releaseall
	end

TrainerHill_Entrance_EventScript_SaveGame::
	msgbox TrainerHill_Entrance_Text_SaveGameBeforeEnter, MSGBOX_DEFAULT
	trainerhill_setsaved
	setvar VAR_TEMP_5, 1
	call Common_EventScript_SaveGame
	goto_if_eq VAR_RESULT, FALSE, TrainerHill_Entrance_EventScript_SaveFailed
	trainerhill_setsaved
	return

TrainerHill_Entrance_EventScript_SaveFailed::
	trainerhill_clearsaved
	goto TrainerHill_Entrance_EventScript_CancelEntry
	end

TrainerHill_Entrance_EventScript_Closed::
	msgbox TrainerHill_Entrance_Text_StillGettingReady, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, TrainerHill_Entrance_Movement_PushPlayerBackFromCounter
	waitmovement 0
	releaseall
	end

TrainerHill_Entrance_Movement_PlayerFaceAttendant:
	face_right
	step_end

TrainerHill_Entrance_Movement_PushPlayerBackFromCounter:
	walk_down
	step_end

/* Unused*/
TrainerHill_Entrance_Movement_FaceUp:
	face_up
	step_end

TrainerHill_Entrance_Movement_PlayerExitElevator:
	walk_down
	walk_down
	step_end

TrainerHill_Entrance_EventScript_Records::
	lockall
	fadescreen FADE_TO_BLACK
	special ShowTrainerHillRecords
	waitstate
	releaseall
	end

TrainerHill_Entrance_EventScript_Man::
	goto_if_unset FLAG_SYS_GAME_CLEAR, TrainerHill_Entrance_EventScript_ManTrainerHillClosed
	msgbox TrainerHill_Entrance_Text_WhatSortOfTrainersAreAhead, MSGBOX_NPC
	end

TrainerHill_Entrance_EventScript_ManTrainerHillClosed::
	msgbox TrainerHill_Entrance_Text_CantWaitToTestTheWaters, MSGBOX_NPC
	end

TrainerHill_Entrance_EventScript_Girl::
	goto_if_unset FLAG_SYS_GAME_CLEAR, TrainerHill_Entrance_EventScript_GirlTrainerHillClosed
	msgbox TrainerHill_Entrance_Text_FriendsTryingToReachTimeBoardTop, MSGBOX_NPC
	end

TrainerHill_Entrance_EventScript_GirlTrainerHillClosed::
	msgbox TrainerHill_Entrance_Text_DoYouKnowWhenTheyOpen, MSGBOX_NPC
	end

TrainerHill_Entrance_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	goto_if_set FLAG_SYS_GAME_CLEAR, TrainerHill_Entrance_EventScript_ExpandedPokemart
	pokemart TrainerHill_Entrance_Pokemart_Basic
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
TrainerHill_Entrance_Pokemart_Basic:
	.short ITEM_SUPER_POTION
	.short ITEM_ANTIDOTE
	.short ITEM_PARALYZE_HEAL
	.short ITEM_AWAKENING
	.short ITEM_X_SPEED
	.short ITEM_X_SPECIAL
	.short ITEM_X_ATTACK
	.short ITEM_X_DEFEND
	.short ITEM_DIRE_HIT
	.short ITEM_GUARD_SPEC
	.short ITEM_X_ACCURACY
	pokemartlistend

TrainerHill_Entrance_EventScript_ExpandedPokemart::
	pokemart TrainerHill_Entrance_Pokemart_Expanded
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
TrainerHill_Entrance_Pokemart_Expanded:
	.short ITEM_HYPER_POTION
	.short ITEM_MAX_POTION
	.short ITEM_FULL_RESTORE
	.short ITEM_FULL_HEAL
	.short ITEM_REVIVE
	.short ITEM_X_SPEED
	.short ITEM_X_SPECIAL
	.short ITEM_X_ATTACK
	.short ITEM_X_DEFEND
	.short ITEM_DIRE_HIT
	.short ITEM_GUARD_SPEC
	.short ITEM_X_ACCURACY
	pokemartlistend

TrainerHill_Entrance_Text_StillGettingReady:
	.string "Esta é a COLINA DOS TREINADORES, onde\n"
	.string "você curte batalhas em dupla com\l"
	.string "muitos TREINADORES.\p"
	.string "Infelizmente, ainda estamos\n"
	.string "preparando tudo. Volte depois!$"

TrainerHill_Entrance_Text_WelcomeToTrainerHill:
	.string "Boas-vindas!\p"
	.string "Esta é a COLINA DOS TREINADORES, onde\n"
	.string "você curte batalhas em dupla com\l"
	.string "muitos TREINADORES!$"

TrainerHill_Entrance_Text_SaveGameBeforeEnter:
	.string "É a sua primeira visita?\p"
	.string "Antes de entrar, salve o jogo,\n"
	.string "por favor.$"

TrainerHill_Entrance_Text_TrainersUpToFloorX:
	.string "Vejamos…\n"
	.string "Os TREINADORES daqui agora estão…\p"
	.string "Até o andar nº {STR_VAR_1}.$"

TrainerHill_Entrance_Text_TrainersInEveryRoom:
	.string "Vejamos…\n"
	.string "Os TREINADORES daqui agora estão…\p"
	.string "Parece que tem TREINADORES em\n"
	.string "todas as salas.$"

TrainerHill_Entrance_Text_LikeToChallengeTrainers:
	.string "Quer desafiar os TREINADORES que\n"
	.string "estão esperando?$"

TrainerHill_Entrance_Text_TimeProgessGetSetGo:
	.string "Vou cronometrar o seu tempo.\n"
	.string "Boa sorte!\p"
	.string "Preparar…\p"
	.string "Apontar…\p"
	.string "Já!$"

TrainerHill_Entrance_Text_PleaseVisitUsAgain:
	.string "Volte a nos visitar!$"

TrainerHill_Entrance_Text_TooBadTremendousEffort:
	.string "Que pena.\p"
	.string "Acho que você se esforçou muito\n"
	.string "nas batalhas.\p"
	.string "Volte e tente de novo!$"

TrainerHill_Entrance_Text_HopeYouGiveItYourBest:
	.string "Espero que dê o seu melhor.$"

TrainerHill_Entrance_Text_MovedReceptionHereForSwitch:
	.string "Quando os TREINADORES trocam de\n"
	.string "lugar, a movimentação é uma loucura.\p"
	.string "Pra evitar o tumulto, mudamos o\n"
	.string "balcão de recepção pra cá.\p"
	.string "Desculpe o transtorno.$"

TrainerHill_Entrance_Text_ThankYouForPlaying:
	.string "Obrigado por participar!$"

TrainerHill_Entrance_Text_ExplainTrainerHill:
	.string "Aqui na COLINA DOS TREINADORES,\n"
	.string "fazemos um evento chamado Contra\l"
	.string "o Tempo.\p"
	.string "É uma corrida que mede quanto você\n"
	.string "leva pra chegar deste balcão até o\l"
	.string "nosso dono, no terraço.\p"
	.string "Os melhores tempos ficam no Quadro\n"
	.string "de Tempos: dispute com os amigos.\p"
	.string "Nas batalhas com TREINADORES do\n"
	.string "caminho, não se ganha EXP. nem\l"
	.string "dinheiro.$"

/* Unused*/
TrainerHill_Entrance_Text_NeedAtLeastTwoPokemon:
	.string "Ah, desculpe, mas parece que você\n"
	.string "só tem um POKéMON.\p"
	.string "É preciso ter pelo menos dois\n"
	.string "POKéMON pra entrar no evento.$"

TrainerHill_Roof_Text_YouFinallyCameBravo:
	.string "Hum! Hum!\p"
	.string "Você finalmente veio!\n"
	.string "Sim, você chegou!\p"
	.string "Pera! Não fala!\n"
	.string "Já sei por que subiu até aqui\l"
	.string "por conta própria!\p"
	.string "Queria me ver, o dono da COLINA\n"
	.string "DOS TREINADORES, porque…\p"
	.string "Quer formar dupla comigo!\n"
	.string "Uau-hu!\p"
	.string "…Hã?\n"
	.string "Não é isso?\p"
	.string "Enfim, vi você subindo.\n"
	.string "Que batalhas maravilhosas! Bravo!$"

TrainerHill_Roof_Text_HaveTheMostMarvelousGift:
	.string "Pra alguém maravilhoso como você,\n"
	.string "tenho o presente mais maravilhoso!$"

TrainerHill_Roof_Text_FullUpBeBackLaterForThis:
	.string "Ah, não, está cheio de coisas!\n"
	.string "Volte depois pra pegar isto!$"

TrainerHill_Roof_Text_GotHereMarvelouslyQuickly:
	.string "Ah, pera aí! Por acaso você chegou\n"
	.string "aqui maravilhosamente rápido?\p"
	.string "Que esplêndido! Nem precisava ter\n"
	.string "tanta pressa pra me ver!\p"
	.string "Que delícia. Vou mandar atualizar o\n"
	.string "Quadro de Tempos da recepção!$"

TrainerHill_Roof_Text_YouWerentVeryQuick:
	.string "Mas, ah…\n"
	.string "Você demorou um pouco pra chegar.$"

TrainerHill_Roof_Text_ArriveZippierNextTime:
	.string "Talvez me agrade mais se você\n"
	.string "chegar mais ligeiro da próxima vez.\p"
	.string "Aí vou adorar formar dupla com\n"
	.string "você!\p"
	.string "Até a próxima, amigo!$"

/* Unused*/
TrainerHill_Roof_Text_BuiltTrainerHillToFindPartner:
	.string "Mandei construir a COLINA DOS\n"
	.string "TREINADORES por um único motivo!\p"
	.string "Achar o parceiro ideal pra formar\n"
	.string "dupla comigo!$"

TrainerHill_Entrance_Text_ChallengeTime:
	.string "{STR_VAR_1} min {STR_VAR_2},{STR_VAR_3} s$"

TrainerHill_Entrance_Text_WhatSortOfTrainersAreAhead:
	.string "Quem sabe que TREINADORES e POKéMON\n"
	.string "me esperam lá na frente?\p"
	.string "Só sei que vou tirar do caminho\n"
	.string "quem ficar na minha frente!$"

TrainerHill_Entrance_Text_CantWaitToTestTheWaters:
	.string "Ouvi dizer que TREINADORES fortes\n"
	.string "vêm de todo canto pra COLINA.\p"
	.string "Não vejo a hora de testar!\p"
	.string "Vou tirar do caminho quem ficar\n"
	.string "na minha frente!$"

TrainerHill_Entrance_Text_FriendsTryingToReachTimeBoardTop:
	.string "Está vendo o Quadro de Tempos ali?\p"
	.string "Eu e os meus amigos estamos vendo\n"
	.string "quem chega ao topo mais rápido.$"

TrainerHill_Entrance_Text_DoYouKnowWhenTheyOpen:
	.string "Sabe quando vão abrir este lugar?\p"
	.string "Estou esperando aqui pra ser o\n"
	.string "primeiro desafiante de todos!$"

TrainerHill_Elevator_Text_ReturnToReception:
	.string "Quer voltar ao balcão de\n"
	.string "recepção?$"
