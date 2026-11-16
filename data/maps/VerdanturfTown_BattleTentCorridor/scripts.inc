VerdanturfTown_BattleTentCorridor_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, VerdanturfTown_BattleTentCorridor_OnFrame
	.byte 0

VerdanturfTown_BattleTentCorridor_OnFrame:
	map_script_2 VAR_TEMP_0, 0, VerdanturfTown_BattleTentCorridor_EventScript_EnterCorridor
	.short 0

VerdanturfTown_BattleTentCorridor_EventScript_EnterCorridor::
	lockall
	setvar VAR_TEMP_0, 1
	applymovement LOCALID_VERDANTURF_TENT_CORRIDOR_ATTENDANT, VerdanturfTown_BattleTentCorridor_Movement_WalkToDoor
	applymovement LOCALID_PLAYER, VerdanturfTown_BattleTentCorridor_Movement_WalkToDoor
	waitmovement 0
	opendoor 2, 1
	waitdooranim
	applymovement LOCALID_VERDANTURF_TENT_CORRIDOR_ATTENDANT, VerdanturfTown_BattleTentCorridor_Movement_AttendantEnterDoor
	applymovement LOCALID_PLAYER, VerdanturfTown_BattleTentCorridor_Movement_PlayerEnterDoor
	waitmovement 0
	closedoor 2, 1
	waitdooranim
	setvar VAR_0x8006, 0
	warp MAP_VERDANTURF_TOWN_BATTLE_TENT_BATTLE_ROOM, 6, 5
	waitstate
	releaseall
	end

VerdanturfTown_BattleTentCorridor_Movement_WalkToDoor:
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

VerdanturfTown_BattleTentCorridor_Movement_PlayerEnterDoor:
	walk_up
VerdanturfTown_BattleTentCorridor_Movement_AttendantEnterDoor:
	walk_up
	set_invisible
	step_end


/* Leftover text from when this was a Contest Hall in R/S*/
/* Unused*/
VerdanturfTown_ContestHall_Text_WhichContestYouEntering:
	.string "Em qual CONCURSO você vai entrar?\n"
	.string "Quer um conselho?\p"
	.string "Em qualquer CONCURSO, por exemplo um\n"
	.string "CONCURSO DE FOFURA, acho que não\l"
	.string "julgam só a fofura na primeira etapa.\p"
	.string "Você precisa achar jeitos de criar\n"
	.string "os POKéMON cada vez melhor.$"

/* Unused*/
VerdanturfTown_ContestHall_Text_RaisedMonToBeCute:
	.string "Criei o meu POKéMON pra ser fofo.\p"
	.string "Descobri que dá pra pôr POKéMON\n"
	.string "num CONCURSO de fofura!\p"
	.string "Que bom que criei o meu POKéMON\n"
	.string "com tanto carinho…$"

/* Unused*/
VerdanturfTown_ContestHall_Text_MyMonRules:
	.string "O meu POKéMON é o máximo!\p"
	.string "É estiloso, forte e ainda bonito,\n"
	.string "fofo e esperto. É completo!\p"
	.string "Bem que eu podia ganhar em todos\n"
	.string "os CONCURSOS.$"

/* Unused*/
VerdanturfTown_ContestHall_Text_NormalRankStage:
	.string "CONCURSOS POKéMON\n"
	.string "PALCO DO NÍVEL NORMAL!$"

