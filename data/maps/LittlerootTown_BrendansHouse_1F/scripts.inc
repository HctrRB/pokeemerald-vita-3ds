LittlerootTown_BrendansHouse_1F_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, LittlerootTown_BrendansHouse_1F_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, LittlerootTown_BrendansHouse_1F_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, LittlerootTown_BrendansHouse_1F_OnFrame
	.byte 0

LittlerootTown_BrendansHouse_1F_OnLoad:
	call_if_lt VAR_LITTLEROOT_INTRO_STATE, 6, LittlerootTown_BrendansHouse_1F_EventScript_SetMovingBoxes
	call_if_set FLAG_RECEIVED_RUNNING_SHOES, LittlerootTown_BrendansHouse_1F_EventScript_CheckShowShoesManual
	end

LittlerootTown_BrendansHouse_1F_EventScript_SetMovingBoxes::
	setmetatile 5, 4, METATILE_BrendansMaysHouse_MovingBox_Open, TRUE
	setmetatile 5, 2, METATILE_BrendansMaysHouse_MovingBox_Closed, TRUE
	return

LittlerootTown_BrendansHouse_1F_EventScript_CheckShowShoesManual::
	checkplayergender
	goto_if_eq VAR_RESULT, MALE, LittlerootTown_BrendansHouse_1F_EventScript_ShowRunningShoesManual
	return

LittlerootTown_BrendansHouse_1F_EventScript_ShowRunningShoesManual::
	setmetatile 3, 7, METATILE_BrendansMaysHouse_BookOnTable, TRUE
	return

LittlerootTown_BrendansHouse_1F_OnTransition:
	call_if_eq VAR_LITTLEROOT_INTRO_STATE, 3, LittlerootTown_BrendansHouse_1F_EventScript_MoveMomToDoor
	call_if_eq VAR_LITTLEROOT_INTRO_STATE, 5, LittlerootTown_BrendansHouse_1F_EventScript_MoveMomToStairs
	call_if_eq VAR_LITTLEROOT_INTRO_STATE, 6, LittlerootTown_BrendansHouse_1F_EventScript_MoveMomToTV
	end

LittlerootTown_BrendansHouse_1F_EventScript_MoveMomToStairs::
	setobjectxyperm LOCALID_PLAYERS_HOUSE_1F_MOM, 8, 4
	setobjectmovementtype LOCALID_PLAYERS_HOUSE_1F_MOM, MOVEMENT_TYPE_FACE_UP
	return

LittlerootTown_BrendansHouse_1F_EventScript_MoveMomToTV::
	setobjectxyperm LOCALID_PLAYERS_HOUSE_1F_MOM, 4, 5
	setobjectmovementtype LOCALID_PLAYERS_HOUSE_1F_MOM, MOVEMENT_TYPE_FACE_UP
	return

LittlerootTown_BrendansHouse_1F_EventScript_MoveMomToDoor::
	setobjectxyperm LOCALID_PLAYERS_HOUSE_1F_MOM, 9, 8
	setobjectmovementtype LOCALID_PLAYERS_HOUSE_1F_MOM, MOVEMENT_TYPE_FACE_UP
	return

/* Many of the below scripts have no gender check because they assume youre in the correct house*/
/* The below SS Ticket script uses Mays house state by accident(?), but theyre both set identically after the intro*/
LittlerootTown_BrendansHouse_1F_OnFrame:
	map_script_2 VAR_LITTLEROOT_INTRO_STATE, 3, LittlerootTown_BrendansHouse_1F_EventScript_EnterHouseMovingIn
	map_script_2 VAR_LITTLEROOT_INTRO_STATE, 5, LittlerootTown_BrendansHouse_1F_EventScript_GoUpstairsToSetClock
	map_script_2 VAR_LITTLEROOT_INTRO_STATE, 6, LittlerootTown_BrendansHouse_1F_EventScript_PetalburgGymReport
	map_script_2 VAR_LITTLEROOT_HOUSES_STATE_MAY, 1, LittlerootTown_BrendansHouse_1F_EventScript_YoureNewNeighbor
	map_script_2 VAR_LITTLEROOT_HOUSES_STATE_MAY, 3, PlayersHouse_1F_EventScript_GetSSTicketAndSeeLatiTV
	.short 0

LittlerootTown_BrendansHouse_1F_EventScript_GoUpstairsToSetClock::
	lockall
	msgbox PlayersHouse_1F_Text_GoSetTheClock, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, LittlerootTown_BrendansHouse_1F_Movement_PushTowardStairs
	applymovement LOCALID_PLAYERS_HOUSE_1F_MOM, LittlerootTown_BrendansHouse_1F_Movement_PushTowardStairs
	waitmovement 0
	warp MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F, 7, 1
	waitstate
	releaseall
	end

LittlerootTown_BrendansHouse_1F_Movement_PushTowardStairs:
	walk_up
	step_end

LittlerootTown_BrendansHouse_1F_EventScript_EnterHouseMovingIn::
	lockall
	setvar VAR_0x8004, LOCALID_PLAYERS_HOUSE_1F_MOM
	setvar VAR_0x8005, MALE
	goto PlayersHouse_1F_EventScript_EnterHouseMovingIn
	end

LittlerootTown_BrendansHouse_1F_EventScript_PetalburgGymReport::
	lockall
	setvar VAR_0x8004, MALE
	setvar VAR_0x8005, LOCALID_PLAYERS_HOUSE_1F_MOM
	goto PlayersHouse_1F_EventScript_PetalburgGymReportMale
	end

LittlerootTown_BrendansHouse_1F_EventScript_YoureNewNeighbor::
	lockall
	playse SE_PIN
	applymovement LOCALID_RIVALS_HOUSE_1F_MOM, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_RIVALS_HOUSE_1F_MOM, Common_Movement_Delay48
	waitmovement 0
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	applymovement LOCALID_RIVALS_HOUSE_1F_MOM, LittlerootTown_BrendansHouse_1F_Movement_RivalMomApproach
	waitmovement 0
	special GetRivalSonDaughterString
	msgbox RivalsHouse_1F_Text_OhYoureTheNewNeighbor, MSGBOX_DEFAULT
	setflag FLAG_MET_RIVAL_MOM
	setvar VAR_LITTLEROOT_HOUSES_STATE_MAY, 2
	releaseall
	end

LittlerootTown_BrendansHouse_1F_Movement_RivalMomApproach:
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

LittlerootTown_BrendansHouse_1F_EventScript_GoSeeRoom::
	lockall
	setvar VAR_0x8004, LOCALID_PLAYERS_HOUSE_1F_MOM
	setvar VAR_0x8005, MALE
	applymovement VAR_0x8004, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	goto PlayersHouse_1F_EventScript_MomGoSeeRoom
	end

LittlerootTown_BrendansHouse_1F_EventScript_MeetRival0::
	lockall
	setvar VAR_0x8008, 0
	goto LittlerootTown_BrendansHouse_1F_EventScript_MeetRival
	end

LittlerootTown_BrendansHouse_1F_EventScript_MeetRival1::
	lockall
	setvar VAR_0x8008, 1
	goto LittlerootTown_BrendansHouse_1F_EventScript_MeetRival
	end

LittlerootTown_BrendansHouse_1F_EventScript_MeetRival2::
	lockall
	setvar VAR_0x8008, 2
	goto LittlerootTown_BrendansHouse_1F_EventScript_MeetRival
	end

LittlerootTown_BrendansHouse_1F_EventScript_MeetRival::
	playse SE_EXIT
	delay 10
	addobject LOCALID_RIVALS_HOUSE_1F_RIVAL
	delay 30
	playse SE_PIN
	applymovement LOCALID_RIVALS_HOUSE_1F_RIVAL, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_RIVALS_HOUSE_1F_RIVAL, Common_Movement_Delay48
	waitmovement 0
	call_if_ne VAR_0x8008, 1, LittlerootTown_BrendansHouse_1F_EventScript_PlayerFaceBrendan
	playbgm MUS_ENCOUNTER_BRENDAN, TRUE
	call_if_eq VAR_0x8008, 0, LittlerootTown_BrendansHouse_1F_EventScript_BrendanApproachPlayer0
	call_if_eq VAR_0x8008, 1, LittlerootTown_BrendansHouse_1F_EventScript_BrendanApproachPlayer1
	call_if_eq VAR_0x8008, 2, LittlerootTown_BrendansHouse_1F_EventScript_BrendanApproachPlayer2
	msgbox RivalsHouse_1F_Text_BrendanWhoAreYou, MSGBOX_DEFAULT
	closemessage
	call_if_eq VAR_0x8008, 0, LittlerootTown_BrendansHouse_1F_EventScript_BrendanGoUpstairs0
	call_if_eq VAR_0x8008, 1, LittlerootTown_BrendansHouse_1F_EventScript_BrendanGoUpstairs1
	call_if_eq VAR_0x8008, 2, LittlerootTown_BrendansHouse_1F_EventScript_BrendanGoUpstairs2
	playse SE_EXIT
	removeobject LOCALID_RIVALS_HOUSE_1F_RIVAL
	setflag FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_BRENDAN
	setflag FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F_POKE_BALL
	clearflag FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_RIVAL_BEDROOM
	delay 30
	setvar VAR_LITTLEROOT_RIVAL_STATE, 3
	setvar VAR_LITTLEROOT_TOWN_STATE, 1
	savebgm MUS_DUMMY
	fadedefaultbgm
	releaseall
	end

LittlerootTown_BrendansHouse_1F_EventScript_PlayerFaceBrendan::
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	return

LittlerootTown_BrendansHouse_1F_EventScript_BrendanApproachPlayer0::
	applymovement LOCALID_RIVALS_HOUSE_1F_RIVAL, LittlerootTown_BrendansHouse_1F_Movement_BrendanApproachPlayer0
	waitmovement 0
	return

LittlerootTown_BrendansHouse_1F_EventScript_BrendanApproachPlayer1::
	applymovement LOCALID_RIVALS_HOUSE_1F_RIVAL, LittlerootTown_BrendansHouse_1F_Movement_BrendanApproachPlayer1
	waitmovement 0
	return

LittlerootTown_BrendansHouse_1F_EventScript_BrendanApproachPlayer2::
	applymovement LOCALID_RIVALS_HOUSE_1F_RIVAL, LittlerootTown_BrendansHouse_1F_Movement_BrendanApproachPlayer2
	waitmovement 0
	return

LittlerootTown_BrendansHouse_1F_Movement_BrendanApproachPlayer0:
	walk_in_place_faster_left
	walk_left
	walk_in_place_faster_up
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

LittlerootTown_BrendansHouse_1F_Movement_BrendanApproachPlayer1:
	walk_up
	walk_up
	walk_up
	step_end

LittlerootTown_BrendansHouse_1F_Movement_BrendanApproachPlayer2:
	walk_in_place_faster_right
	walk_right
	walk_in_place_faster_up
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

LittlerootTown_BrendansHouse_1F_EventScript_BrendanGoUpstairs0::
	applymovement LOCALID_PLAYER, LittlerootTown_BrendansHouse_1F_Movement_PlayerWatchBrendanExit0
	applymovement LOCALID_RIVALS_HOUSE_1F_RIVAL, LittlerootTown_BrendansHouse_1F_Movement_BrendanGoUpstairs0
	waitmovement 0
	return

LittlerootTown_BrendansHouse_1F_EventScript_BrendanGoUpstairs1::
	applymovement LOCALID_PLAYER, LittlerootTown_BrendansHouse_1F_Movement_PlayerWatchBrendanExit1
	applymovement LOCALID_RIVALS_HOUSE_1F_RIVAL, LittlerootTown_BrendansHouse_1F_Movement_BrendanGoUpstairs1
	waitmovement 0
	return

LittlerootTown_BrendansHouse_1F_EventScript_BrendanGoUpstairs2::
	applymovement LOCALID_PLAYER, LittlerootTown_BrendansHouse_1F_Movement_PlayerWatchBrendanExit2
	applymovement LOCALID_RIVALS_HOUSE_1F_RIVAL, LittlerootTown_BrendansHouse_1F_Movement_BrendanGoUpstairs2
	waitmovement 0
	return

LittlerootTown_BrendansHouse_1F_Movement_PlayerWatchBrendanExit0:
	delay_16
	delay_8
	walk_in_place_faster_right
	step_end

LittlerootTown_BrendansHouse_1F_Movement_PlayerWatchBrendanExit1:
	delay_16
	delay_8
	walk_in_place_faster_right
	delay_16
	delay_16
	walk_in_place_faster_up
	step_end

LittlerootTown_BrendansHouse_1F_Movement_PlayerWatchBrendanExit2:
	delay_16
	delay_8
	walk_in_place_faster_left
	step_end

LittlerootTown_BrendansHouse_1F_Movement_BrendanGoUpstairs0:
	walk_in_place_faster_right
	walk_right
	walk_in_place_faster_up
	walk_up
	walk_up
	step_end

LittlerootTown_BrendansHouse_1F_Movement_BrendanGoUpstairs1:
	walk_in_place_faster_right
	walk_right
	walk_in_place_faster_up
	walk_up
	walk_up
	walk_in_place_faster_left
	walk_left
	walk_in_place_faster_up
	walk_up
	step_end

LittlerootTown_BrendansHouse_1F_Movement_BrendanGoUpstairs2:
	walk_in_place_faster_left
	walk_left
	walk_in_place_faster_up
	walk_up
	walk_up
	step_end

PlayersHouse_1F_Text_IsntItNiceInHere:
	.string "MAMÃE: Viu só, {PLAYER}?\n"
	.string "Aqui dentro também é bom, não é?$"

PlayersHouse_1F_Text_MoversPokemonGoSetClock:
	.string "Os POKéMON da mudança fazem todo o\n"
	.string "trabalho de trazer as coisas e limpar.\l"
	.string "É tão prático!\p"
	.string "{PLAYER}, o seu quarto é lá em cima.\n"
	.string "Vá dar uma olhada, meu bem!\p"
	.string "O PAPAI comprou um relógio novo para\n"
	.string "marcar a nossa mudança.\l"
	.string "Não se esqueça de acertá-lo!$"

PlayersHouse_1F_Text_ArentYouInterestedInRoom:
	.string "MAMÃE: E então, {PLAYER}?\p"
	.string "Não quer ver o seu próprio\n"
	.string "quarto?$"

PlayersHouse_1F_Text_GoSetTheClock:
	.string "MAMÃE: {PLAYER}.\p"
	.string "Vá acertar o relógio do seu quarto.$"

PlayersHouse_1F_Text_OhComeQuickly:
	.string "MAMÃE: Oh! {PLAYER}, {PLAYER}!\n"
	.string "Rápido! Venha depressa!$"

PlayersHouse_1F_Text_MaybeDadWillBeOn:
	.string "MAMÃE: Olhe! É o GINÁSIO DE PETALBURG!\n"
	.string "Talvez o PAPAI apareça!$"

PlayersHouse_1F_Text_ItsOverWeMissedHim:
	.string "MAMÃE: Ah… Acabou.\p"
	.string "Acho que o PAPAI apareceu, mas não\n"
	.string "deu tempo de ver. Que pena.$"

PlayersHouse_1F_Text_GoIntroduceYourselfNextDoor:
	.string "Ah, sim.\n"
	.string "Um amigo do PAPAI mora na cidade.\p"
	.string "O nome dele é PROF. BIRCH.\p"
	.string "Ele mora bem aqui ao lado, então\n"
	.string "vá até lá se apresentar.$"

PlayersHouse_1F_Text_SeeYouHoney:
	.string "MAMÃE: Até mais, meu amor!$"

PlayersHouse_1F_Text_DidYouMeetProfBirch:
	.string "MAMÃE: Você já se apresentou ao\n"
	.string "PROF. BIRCH?$"

PlayersHouse_1F_Text_YouShouldRestABit:
	.string "MAMÃE: Como você está, {PLAYER}?\n"
	.string "Você parece meio sem energia.\p"
	.string "Acho que deveria descansar um pouco.$"

PlayersHouse_1F_Text_TakeCareHoney:
	.string "MAMÃE: Se cuide, meu amor!$"

PlayersHouse_1F_Text_GotDadsBadgeHeresSomethingFromMom:
	.string "MAMÃE: Oh? O PAPAI te deu essa\n"
	.string "INSÍGNIA?\p"
	.string "Então aqui vai algo da MAMÃE!$"

PlayersHouse_1F_Text_DontPushYourselfTooHard:
	.string "Não exagere, meu bem.\n"
	.string "Você sempre pode voltar para casa.\p"
	.string "Vá em frente, meu amor!$"

PlayersHouse_1F_Text_IsThatAPokenav:
	.string "MAMÃE: O que é isso? Um POKéNAV?\n"
	.string "Alguém da DEVON te deu?\p"
	.string "Então, meu amor, que tal registrar\n"
	.string "a sua MAMÃE?\p"
	.string "… … …$"

PlayersHouse_1F_Text_RegisteredMom:
	.string "MAMÃE registrada\n"
	.string "no POKéNAV.$"

PlayersHouse_1F_Text_Vigoroth1:
	.string "Fugiiiiih!$"

PlayersHouse_1F_Text_Vigoroth2:
	.string "Huggoh, uggo uggo…$"

PlayersHouse_1F_Text_ReportFromPetalburgGym:
	.string "REPÓRTER: …Esta foi a nossa reportagem\n"
	.string "em frente ao GINÁSIO DE PETALBURG.$"

PlayersHouse_1F_Text_TheresAMovieOnTV:
	.string "Está passando um filme na TV.\p"
	.string "Dois homens dançam sobre o teclado\n"
	.string "de um piano enorme.\p"
	.string "Melhor ir andando!$"

PlayersHouse_1F_Text_RunningShoesManual:
	.string "É o manual de instruções do\n"
	.string "TÊNIS DE CORRER.\p"
	.string "“Aperte o Botão B para correr usando\n"
	.string "o seu TÊNIS DE CORRER.\p"
	.string "“Amarre o seu TÊNIS DE CORRER e saia\n"
	.string "pela estrada a toda velocidade!”$"

PlayersHouse_1F_Text_TicketFromBrineyCameForYou:
	.string "PAPAI: Hum?\p"
	.string "Ora, é {PLAYER}!\p"
	.string "Faz tempo que não te vejo, mas você\n"
	.string "parece… mais forte, de algum jeito.\p"
	.string "É a impressão que eu tenho.\n"
	.string "Mas o seu velho ainda não se rendeu!\p"
	.string "Ah, sim, tenho algo para você.\n"
	.string "Isto chegou para você, de alguém\l"
	.string "chamado SR. BRINEY.$"

PlayersHouse_1F_Text_PortsInSlateportLilycove:
	.string "PAPAI: Hum, um BILHETE de balsa?\p"
	.string "Se não me engano, há portos de balsa\n"
	.string "em SLATEPORT e LILYCOVE.$"

PlayersHouse_1F_Text_BetterGetBackToGym:
	.string "É melhor eu voltar ao GINÁSIO DE\n"
	.string "PETALBURG.\p"
	.string "MAMÃE, obrigado por cuidar da casa\n"
	.string "enquanto eu estou fora.$"

PlayersHouse_1F_Text_DadShouldStayLonger:
	.string "MAMÃE: Esse seu PAPAI…\p"
	.string "Ele volta para casa depois de tanto\n"
	.string "tempo, mas só fala de POKéMON.\p"
	.string "Ele devia relaxar e ficar mais um pouco.$"

PlayersHouse_1F_Text_IsThatABreakingStory:
	.string "MAMÃE: É uma notícia urgente?$"

PlayersHouse_1F_Text_LatiEmergencyNewsFlash:
	.string "Interrompemos a programação para\n"
	.string "um plantão de notícias!\p"
	.string "Em vários lugares de HOENN, houve\n"
	.string "relatos de um POKéMON… BZZT…\l"
	.string "voando pelos céus.\p"
	.string "A identidade deste POKéMON ainda\n"
	.string "é desconhecida.\p"
	.string "Voltamos agora à nossa programação\n"
	.string "normal de filmes.$"

PlayersHouse_1F_Text_WhatColorDidTheySay:
	.string "MAMÃE: {PLAYER}, você ouviu aquilo?\p"
	.string "De que cor o apresentador disse\n"
	.string "que era aquele POKéMON?$"

PlayersHouse_1F_Text_StillUnknownPokemon:
	.string "MAMÃE: Ora, quem diria!\n"
	.string "Ainda existem POKéMON desconhecidos.$"
