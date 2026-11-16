Route114_FossilManiacsTunnel_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route114_FossilManiacsTunnel_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route114_FossilManiacsTunnel_OnLoad
	.byte 0

Route114_FossilManiacsTunnel_OnTransition:
	call_if_set FLAG_SYS_GAME_CLEAR, Route114_FossilManiacsTunnel_EventScript_MoveFossilManiac
	end

Route114_FossilManiacsTunnel_EventScript_MoveFossilManiac::
	setobjectxyperm LOCALID_FOSSIL_MANIAC, 6, 5
	setobjectmovementtype LOCALID_FOSSIL_MANIAC, MOVEMENT_TYPE_FACE_DOWN
	return

Route114_FossilManiacsTunnel_OnLoad:
	call_if_unset FLAG_SYS_GAME_CLEAR, Route114_FossilManiacsTunnel_EventScript_CloseDesertUnderpass
	end

Route114_FossilManiacsTunnel_EventScript_CloseDesertUnderpass::
	setmetatile 6, 1, METATILE_Fallarbor_RedRockWall, TRUE
	setmetatile 6, 2, METATILE_Fallarbor_RedRockWall, TRUE
	return

Route114_FossilManiacsTunnel_EventScript_ManiacMentionCaveIn::
	lockall
	applymovement LOCALID_FOSSIL_MANIAC, Common_Movement_WalkInPlaceFasterUp
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	msgbox Route114_FossilManiacsTunnel_Text_NotSafeThatWay, MSGBOX_DEFAULT
	setvar VAR_FOSSIL_MANIAC_STATE, 2
	releaseall
	end

Route114_FossilManiacsTunnel_EventScript_FossilManiac::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_REVIVED_FOSSIL_MON, Route114_FossilManiacsTunnel_EventScript_PlayerRevivedFossil
	checkitem ITEM_ROOT_FOSSIL
	goto_if_eq VAR_RESULT, TRUE, Route114_FossilManiacsTunnel_EventScript_PlayerHasFossil
	checkitem ITEM_CLAW_FOSSIL
	goto_if_eq VAR_RESULT, TRUE, Route114_FossilManiacsTunnel_EventScript_PlayerHasFossil
	msgbox Route114_FossilManiacsTunnel_Text_LookInDesertForFossils, MSGBOX_DEFAULT
	release
	end

Route114_FossilManiacsTunnel_EventScript_PlayerHasFossil::
	msgbox Route114_FossilManiacsTunnel_Text_DevonCorpRevivingFossils, MSGBOX_DEFAULT
	release
	end

Route114_FossilManiacsTunnel_EventScript_PlayerRevivedFossil::
	msgbox Route114_FossilManiacsTunnel_Text_FossilsAreWonderful, MSGBOX_DEFAULT
	release
	end

Route114_FossilManiacsTunnel_Text_LookInDesertForFossils:
	.string "Eu sou o MANÍACO POR FÓSSEIS…\n"
	.string "Um cara legal que ama FÓSSEIS…\p"
	.string "Quer um FÓSSIL?\p"
	.string "Mas os FÓSSEIS daqui são todos\n"
	.string "meus… Nenhum pra você…\p"
	.string "Se não aguentar ficar sem um FÓSSIL,\n"
	.string "procure num deserto com pedras\l"
	.string "e areia que podem esconder\l"
	.string "FÓSSEIS…$"

Route114_FossilManiacsTunnel_Text_DevonCorpRevivingFossils:
	.string "Você achou um FÓSSIL, não achou?\n"
	.string "Que legal… Que sonho…\p"
	.string "O que vai fazer com esse FÓSSIL?\p"
	.string "Pelo que ouvi, a DEVON está\n"
	.string "pesquisando como reviver POKéMON\l"
	.string "de FÓSSEIS…\p"
	.string "Eu amo os meus FÓSSEIS, então\n"
	.string "nunca faria uma coisa dessas…$"

Route114_FossilManiacsTunnel_Text_FossilsAreWonderful:
	.string "FÓSSEIS são tão… Maravilhosos…\n"
	.string "Que sonho…$"

Route114_FossilManiacsTunnel_Text_NotSafeThatWay:
	.string "Ah…\n"
	.string "Por ali não é seguro…\p"
	.string "Eu estava cavando, sabe…\n"
	.string "Quando a parede inteira desabou…\p"
	.string "Acho que agora tem uma caverna\n"
	.string "enorme lá embaixo…\p"
	.string "Mas deixei quieto, porque acho que\n"
	.string "não tem FÓSSIL nenhum lá…$"

