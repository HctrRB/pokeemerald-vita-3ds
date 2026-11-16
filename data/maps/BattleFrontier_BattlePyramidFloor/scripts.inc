BattleFrontier_BattlePyramidFloor_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, BattleFrontier_BattlePyramidFloor_OnResume
	map_script MAP_SCRIPT_ON_FRAME_TABLE, BattleFrontier_BattlePyramidFloor_OnFrame
	map_script MAP_SCRIPT_ON_TRANSITION, BattleFrontier_BattlePyramidFloor_OnTransition
	.byte 0

BattleFrontier_BattlePyramidFloor_OnFrame:
	map_script_2 VAR_TEMP_D, 1, BattleFrontier_BattlePyramidFloor_EventScript_UpdateLight
	map_script_2 VAR_TEMP_PLAYING_PYRAMID_MUSIC, 0, BattleFrontier_BattlePyramidFloor_EventScript_PlayPyramidMusic
	map_script_2 VAR_TEMP_F, 1, BattleFrontier_BattlePyramidFloor_EventScript_ShowMapName
	.short 0

BattleFrontier_BattlePyramidFloor_EventScript_UpdateLight::
	lockall
	/* pyramid_updatelight, cant use macro because it straddles the loop*/
	setvar VAR_0x8004, BATTLE_PYRAMID_FUNC_UPDATE_LIGHT
	setvar VAR_0x8005, 4
	setvar VAR_0x8006, PYRAMID_LIGHT_INCR_RADIUS
	setvar VAR_0x8007, SE_SAVE
	setvar VAR_RESULT, 0
BattleFrontier_BattlePyramidFloor_EventScript_UpdateLightLoop::
	special CallBattlePyramidFunction
	delay 2
	goto_if_ne VAR_RESULT, 2, BattleFrontier_BattlePyramidFloor_EventScript_UpdateLightLoop
	setvar VAR_TEMP_D, 0
	releaseall
	end

BattleFrontier_BattlePyramidFloor_EventScript_ShowMapName::
	special ShowMapNamePopup
	setvar VAR_TEMP_F, 0
	end

BattleFrontier_BattlePyramidFloor_EventScript_PlayPyramidMusic::
	playbgm MUS_B_PYRAMID, FALSE
	setvar VAR_TEMP_PLAYING_PYRAMID_MUSIC, 1
	end

BattleFrontier_BattlePyramidFloor_OnResume:
	pyramid_setfloorpal
	frontier_getstatus
	switch VAR_TEMP_CHALLENGE_STATUS
	case 0, BattleFrontier_BattlePyramidFloor_EventScript_ReadyChallenge
	case CHALLENGE_STATUS_SAVING, BattleFrontier_BattlePyramid_EventScript_WarpToLobby
	case CHALLENGE_STATUS_PAUSED, BattleFrontier_BattlePyramidFloor_EventScript_ReadyChallenge
	frontier_get FRONTIER_DATA_BATTLE_OUTCOME
	goto_if_eq VAR_RESULT, B_OUTCOME_RAN, BattleFrontier_BattlePyramidFloor_EventScript_ResetParty
	goto_if_eq VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED, BattleFrontier_BattlePyramidFloor_EventScript_ResetParty
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattlePyramidFloor_EventScript_ResetParty
	goto_if_eq VAR_RESULT, B_OUTCOME_LOST, BattleFrontier_BattlePyramid_EventScript_WarpToLobbyLost
	goto_if_eq VAR_RESULT, B_OUTCOME_DREW, BattleFrontier_BattlePyramid_EventScript_WarpToLobbyLost
	goto_if_eq VAR_RESULT, B_OUTCOME_FORFEITED, BattleFrontier_BattlePyramid_EventScript_WarpToLobbyLost
	frontier_isbattletype BATTLE_TYPE_TRAINER  /* VAR_RESULT seems to be ignored here*/
	setvar VAR_TEMP_D, 1
BattleFrontier_BattlePyramidFloor_EventScript_ResetParty::
	pyramid_resetparty
	end

BattleFrontier_BattlePyramid_EventScript_WarpToLobbyLost::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_LOST
	pyramid_set PYRAMID_DATA_TRAINER_FLAGS, 255
BattleFrontier_BattlePyramid_EventScript_WarpToLobby::
	pyramid_updatelight 0, PYRAMID_LIGHT_SET_RADIUS
	pyramid_clearhelditems
	special HealPlayerParty
	warpsilent MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_LOBBY, 7, 13
	waitstate
	end

BattleFrontier_BattlePyramidFloor_EventScript_ReadyChallenge::
	pyramid_save CHALLENGE_STATUS_SAVING
	special SavePlayerParty
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	pyramid_settrainers
	frontier_setpartyorder FRONTIER_PARTY_SIZE
	setvar VAR_TEMP_F, 1
	end

BattleFrontier_BattlePyramidFloor_OnTransition:
	call BattleFrontier_BattlePyramidFloor_EventScript_SetLightRadius
	setvar VAR_TEMP_F, 1
	end

BattleFrontier_BattlePyramidFloor_EventScript_SetLightRadius::
	pyramid_updatelight 32, PYRAMID_LIGHT_SET_RADIUS
	return

BattlePyramid_WarpToNextFloor::
	call BattleFrontier_EventScript_IncrementWinStreak
	frontier_get FRONTIER_DATA_BATTLE_NUM  /* Floor number*/
	addvar VAR_RESULT, 1
	frontier_set FRONTIER_DATA_BATTLE_NUM, VAR_RESULT
	goto_if_eq VAR_RESULT, 7, BattlePyramid_WarpToTop
	pyramid_seedfloor
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	setvar VAR_RESULT, 0
	warp MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR, 1, 1
	waitstate
	end

BattlePyramid_WarpToTop::
	warp MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_TOP, 17, 17
	waitstate
	end

/* TRAINER_PHILLIP is used as a placeholder*/
BattlePyramid_TrainerBattle::
	trainerbattle TRAINER_BATTLE_PYRAMID, TRAINER_PHILLIP, LOCALID_NONE, BattleFacility_TrainerBattle_PlaceholderText, BattleFacility_TrainerBattle_PlaceholderText
	pyramid_showhint
	waitmessage
	waitbuttonpress
	closemessage
	releaseall
	end

BattlePyramid_FindItemBall::
	pyramid_setitem
	callstd STD_FIND_ITEM
	goto_if_eq VAR_0x8007, 0, BattlePyramid_FindItemBallEnd
	pyramid_hideitem
BattlePyramid_FindItemBallEnd::
	end

BattlePyramid_Retire::
	goto BattleFrontier_BattlePyramid_EventScript_WarpToLobbyLost

/* Also used by Trainer Hill*/
BattleFacility_TrainerBattle_PlaceholderText:
	.string "Esta é uma mensagem de exemplo.$"

gText_BattlePyramidConfirmRest::
	.string "A sua missão na PIRÂMIDE DE BATALHA\n"
	.string "será salva pra você parar por agora.\l"
	.string "Tudo bem?$"

gText_BattlePyramidConfirmRetire::
	.string "Tem certeza de que quer abandonar\n"
	.string "a missão na PIRÂMIDE?$"

BattlePyramid_Text_ExitHintUp1::
	.string "A saída deste andar fica\n"
	.string "naquela direção: {UP_ARROW}$"

BattlePyramid_Text_ExitHintLeft1::
	.string "A saída deste andar fica\n"
	.string "naquela direção: {LEFT_ARROW}$"

BattlePyramid_Text_ExitHintRight1::
	.string "A saída deste andar fica\n"
	.string "naquela direção: {RIGHT_ARROW}$"

BattlePyramid_Text_ExitHintDown1::
	.string "A saída deste andar fica\n"
	.string "naquela direção: {DOWN_ARROW}$"

BattlePyramid_Text_ExitHintUp2::
	.string "A saída deste andar fica\n"
	.string "na direção {UP_ARROW}.$"

BattlePyramid_Text_ExitHintLeft2::
	.string "A saída deste andar fica\n"
	.string "na direção {LEFT_ARROW}.$"

BattlePyramid_Text_ExitHintRight2::
	.string "A saída deste andar fica\n"
	.string "na direção {RIGHT_ARROW}.$"

BattlePyramid_Text_ExitHintDown2::
	.string "A saída deste andar fica\n"
	.string "na direção {DOWN_ARROW}.$"

BattlePyramid_Text_ExitHintUp3::
	.string "A saída é\n"
	.string "pra lá {UP_ARROW}.$"

BattlePyramid_Text_ExitHintLeft3::
	.string "A saída é\n"
	.string "pra lá {LEFT_ARROW}.$"

BattlePyramid_Text_ExitHintRight3::
	.string "A saída é\n"
	.string "pra lá {RIGHT_ARROW}.$"

BattlePyramid_Text_ExitHintDown3::
	.string "A saída é\n"
	.string "pra lá {DOWN_ARROW}.$"

BattlePyramid_Text_ExitHintUp4::
	.string "Neste andar, a saída fica em algum\n"
	.string "lugar na direção {UP_ARROW}.$"

BattlePyramid_Text_ExitHintLeft4::
	.string "Neste andar, a saída fica em algum\n"
	.string "lugar na direção {LEFT_ARROW}.$"

BattlePyramid_Text_ExitHintRight4::
	.string "Neste andar, a saída fica em algum\n"
	.string "lugar na direção {RIGHT_ARROW}.$"

BattlePyramid_Text_ExitHintDown4::
	.string "Neste andar, a saída fica em algum\n"
	.string "lugar na direção {DOWN_ARROW}.$"

BattlePyramid_Text_ExitHintUp5::
	.string "A saída?\n"
	.string "É pra lá {UP_ARROW}.$"

BattlePyramid_Text_ExitHintLeft5::
	.string "A saída?\n"
	.string "É pra lá {LEFT_ARROW}.$"

BattlePyramid_Text_ExitHintRight5::
	.string "A saída?\n"
	.string "É pra lá {RIGHT_ARROW}.$"

BattlePyramid_Text_ExitHintDown5::
	.string "A saída?\n"
	.string "É pra lá {DOWN_ARROW}.$"

BattlePyramid_Text_ExitHintUp6::
	.string "Por acaso, a saída fica\n"
	.string "na direção {UP_ARROW}.$"

BattlePyramid_Text_ExitHintLeft6::
	.string "Por acaso, a saída fica\n"
	.string "na direção {LEFT_ARROW}.$"

BattlePyramid_Text_ExitHintRight6::
	.string "Por acaso, a saída fica\n"
	.string "na direção {RIGHT_ARROW}.$"

BattlePyramid_Text_ExitHintDown6::
	.string "Por acaso, a saída fica\n"
	.string "na direção {DOWN_ARROW}.$"

BattlePyramid_Text_EightItemsRemaining1::
	.string "Procurando itens?\p"
	.string "Ainda faltam oito itens pra\n"
	.string "achar.$"

BattlePyramid_Text_SevenItemsRemaining1::
	.string "Procurando itens?\p"
	.string "Ainda faltam sete itens pra\n"
	.string "achar.$"

BattlePyramid_Text_SixItemsRemaining1::
	.string "Procurando itens?\p"
	.string "Ainda faltam seis itens pra\n"
	.string "achar.$"

BattlePyramid_Text_FiveItemsRemaining1::
	.string "Procurando itens?\p"
	.string "Ainda faltam cinco itens pra\n"
	.string "achar.$"

BattlePyramid_Text_FourItemsRemaining1::
	.string "Procurando itens?\p"
	.string "Ainda faltam quatro itens pra\n"
	.string "achar.$"

BattlePyramid_Text_ThreeItemsRemaining1::
	.string "Procurando itens?\p"
	.string "Ainda faltam três itens pra\n"
	.string "achar.$"

BattlePyramid_Text_TwoItemsRemaining1::
	.string "Procurando itens?\p"
	.string "Ainda faltam dois itens pra\n"
	.string "achar.$"

BattlePyramid_Text_OneItemRemaining1::
	.string "Procurando itens?\p"
	.string "Falta achar só um item.$"

BattlePyramid_Text_ZeroItemsRemaining1::
	.string "Procurando itens?\p"
	.string "Não sobrou nada pra achar!$"

BattlePyramid_Text_EightItemsRemaining2::
	.string "Como você ganhou, vou contar\n"
	.string "um segredinho!\p"
	.string "Tem oito itens jogados\n"
	.string "por aqui.$"

BattlePyramid_Text_SevenItemsRemaining2::
	.string "Como você ganhou, vou contar\n"
	.string "um segredinho!\p"
	.string "Ainda tem sete itens jogados\n"
	.string "por aqui.$"

BattlePyramid_Text_SixItemsRemaining2::
	.string "Como você ganhou, vou contar\n"
	.string "um segredinho!\p"
	.string "Ainda tem seis itens jogados\n"
	.string "por aqui.$"

BattlePyramid_Text_FiveItemsRemaining2::
	.string "Como você ganhou, vou contar\n"
	.string "um segredinho!\p"
	.string "Ainda tem cinco itens jogados\n"
	.string "por aqui.$"

BattlePyramid_Text_FourItemsRemaining2::
	.string "Como você ganhou, vou contar\n"
	.string "um segredinho!\p"
	.string "Ainda tem quatro itens jogados\n"
	.string "por aqui.$"

BattlePyramid_Text_ThreeItemsRemaining2::
	.string "Como você ganhou, vou contar\n"
	.string "um segredinho!\p"
	.string "Ainda tem três itens jogados\n"
	.string "por aqui.$"

BattlePyramid_Text_TwoItemsRemaining2::
	.string "Como você ganhou, vou contar\n"
	.string "um segredinho!\p"
	.string "Ainda tem dois itens jogados\n"
	.string "por aqui.$"

BattlePyramid_Text_OneItemRemaining2::
	.string "Como você ganhou, vou contar\n"
	.string "um segredinho!\p"
	.string "Só tem um item jogado\n"
	.string "por aqui.$"

BattlePyramid_Text_ZeroItemsRemaining2::
	.string "Como você ganhou, vou contar\n"
	.string "um segredinho!\p"
	.string "Não sobrou nenhum item\n"
	.string "por aqui.$"

BattlePyramid_Text_EightItemsRemaining3::
	.string "Como vão os seus itens?\p"
	.string "Acho que ainda faltam oito itens\n"
	.string "pra achar neste andar.$"

BattlePyramid_Text_SevenItemsRemaining3::
	.string "Como vão os seus itens?\p"
	.string "Acho que ainda faltam sete itens\n"
	.string "pra achar neste andar.$"

BattlePyramid_Text_SixItemsRemaining3::
	.string "Como vão os seus itens?\p"
	.string "Acho que ainda faltam seis itens\n"
	.string "pra achar neste andar.$"

BattlePyramid_Text_FiveItemsRemaining3::
	.string "Como vão os seus itens?\p"
	.string "Acho que ainda faltam cinco itens\n"
	.string "pra achar neste andar.$"

BattlePyramid_Text_FourItemsRemaining3::
	.string "Como vão os seus itens?\p"
	.string "Acho que ainda faltam quatro itens\n"
	.string "pra achar neste andar.$"

BattlePyramid_Text_ThreeItemsRemaining3::
	.string "Como vão os seus itens?\p"
	.string "Acho que ainda faltam três itens\n"
	.string "pra achar neste andar.$"

BattlePyramid_Text_TwoItemsRemaining3::
	.string "Como vão os seus itens?\p"
	.string "Acho que ainda faltam dois itens\n"
	.string "pra achar neste andar.$"

BattlePyramid_Text_OneItemRemaining3::
	.string "Como vão os seus itens?\p"
	.string "Acho que só falta um item pra\n"
	.string "achar neste andar.$"

BattlePyramid_Text_ZeroItemsRemaining3::
	.string "Como vão os seus itens?\p"
	.string "Acho que não tem mais itens pra\n"
	.string "achar neste andar. Se cuida!$"

BattlePyramid_Text_EightItemsRemaining4::
	.string "Você é forte, então merece\n"
	.string "uma dica!\p"
	.string "Parece que tem mais oito itens\n"
	.string "no chão.$"

BattlePyramid_Text_SevenItemsRemaining4::
	.string "Você é forte, então merece\n"
	.string "uma dica!\p"
	.string "Parece que tem mais sete itens\n"
	.string "no chão.$"

BattlePyramid_Text_SixItemsRemaining4::
	.string "Você é forte, então merece\n"
	.string "uma dica!\p"
	.string "Parece que tem mais seis itens\n"
	.string "no chão.$"

BattlePyramid_Text_FiveItemsRemaining4::
	.string "Você é forte, então merece\n"
	.string "uma dica!\p"
	.string "Parece que tem mais cinco itens\n"
	.string "no chão.$"

BattlePyramid_Text_FourItemsRemaining4::
	.string "Você é forte, então merece\n"
	.string "uma dica!\p"
	.string "Parece que tem mais quatro itens\n"
	.string "no chão.$"

BattlePyramid_Text_ThreeItemsRemaining4::
	.string "Você é forte, então merece\n"
	.string "uma dica!\p"
	.string "Parece que tem mais três itens\n"
	.string "no chão.$"

BattlePyramid_Text_TwoItemsRemaining4::
	.string "Você é forte, então merece\n"
	.string "uma dica!\p"
	.string "Parece que tem mais dois itens\n"
	.string "no chão.$"

BattlePyramid_Text_OneItemRemaining4::
	.string "Você é forte, então merece\n"
	.string "uma dica!\p"
	.string "Parece que só tem mais um item\n"
	.string "no chão.$"

BattlePyramid_Text_ZeroItemsRemaining4::
	.string "Você é forte, então merece\n"
	.string "uma dica!\p"
	.string "Parece que não tem mais itens\n"
	.string "no chão.$"

BattlePyramid_Text_EightItemsRemaining5::
	.string "Neste andar da PIRÂMIDE, dizem\n"
	.string "que há oito itens…$"

BattlePyramid_Text_SevenItemsRemaining5::
	.string "Neste andar da PIRÂMIDE, dizem\n"
	.string "que há sete itens…$"

BattlePyramid_Text_SixItemsRemaining5::
	.string "Neste andar da PIRÂMIDE, dizem\n"
	.string "que há seis itens…$"

BattlePyramid_Text_FiveItemsRemaining5::
	.string "Neste andar da PIRÂMIDE, dizem\n"
	.string "que há cinco itens…$"

BattlePyramid_Text_FourItemsRemaining5::
	.string "Neste andar da PIRÂMIDE, dizem\n"
	.string "que há quatro itens…$"

BattlePyramid_Text_ThreeItemsRemaining5::
	.string "Neste andar da PIRÂMIDE, dizem\n"
	.string "que há três itens…$"

BattlePyramid_Text_TwoItemsRemaining5::
	.string "Neste andar da PIRÂMIDE, dizem\n"
	.string "que há dois itens…$"

BattlePyramid_Text_OneItemRemaining5::
	.string "Neste andar da PIRÂMIDE, dizem\n"
	.string "que há um item…$"

BattlePyramid_Text_ZeroItemsRemaining5::
	.string "Neste andar da PIRÂMIDE, dizem\n"
	.string "que não há itens…$"

BattlePyramid_Text_EightItemsRemaining6::
	.string "Já juntou algum item?\p"
	.string "Acho que há mais oito itens\n"
	.string "neste andar.$"

BattlePyramid_Text_SevenItemsRemaining6::
	.string "Já juntou algum item?\p"
	.string "Acho que há mais sete itens\n"
	.string "neste andar.$"

BattlePyramid_Text_SixItemsRemaining6::
	.string "Já juntou algum item?\p"
	.string "Acho que há mais seis itens\n"
	.string "neste andar.$"

BattlePyramid_Text_FiveItemsRemaining6::
	.string "Já juntou algum item?\p"
	.string "Acho que há mais cinco itens\n"
	.string "neste andar.$"

BattlePyramid_Text_FourItemsRemaining6::
	.string "Já juntou algum item?\p"
	.string "Acho que há mais quatro itens\n"
	.string "neste andar.$"

BattlePyramid_Text_ThreeItemsRemaining6::
	.string "Já juntou algum item?\p"
	.string "Acho que há mais três itens\n"
	.string "neste andar.$"

BattlePyramid_Text_TwoItemsRemaining6::
	.string "Já juntou algum item?\p"
	.string "Acho que há mais dois itens\n"
	.string "neste andar.$"

BattlePyramid_Text_OneItemRemaining6::
	.string "Já juntou algum item?\p"
	.string "Acho que há mais um item\n"
	.string "neste andar.$"

BattlePyramid_Text_ZeroItemsRemaining6::
	.string "Já juntou algum item?\p"
	.string "Acho que não há mais itens\n"
	.string "neste andar.$"

BattlePyramid_Text_SevenTrainersRemaining1::
	.string "Você foi demais!\p"
	.string "Mas ainda tem sete TREINADORES\n"
	.string "fortes além de mim!$"

BattlePyramid_Text_SixTrainersRemaining1::
	.string "Você foi demais!\p"
	.string "Mas ainda tem seis TREINADORES\n"
	.string "fortes além de mim!$"

BattlePyramid_Text_FiveTrainersRemaining1::
	.string "Você foi demais!\p"
	.string "Mas ainda tem cinco TREINADORES\n"
	.string "fortes além de mim!$"

BattlePyramid_Text_FourTrainersRemaining1::
	.string "Você foi demais!\p"
	.string "Mas ainda tem quatro TREINADORES\n"
	.string "fortes além de mim!$"

BattlePyramid_Text_ThreeTrainersRemaining1::
	.string "Você foi demais!\p"
	.string "Mas ainda tem três TREINADORES\n"
	.string "fortes além de mim!$"

BattlePyramid_Text_TwoTrainersRemaining1::
	.string "Você foi demais!\p"
	.string "Mas ainda tem dois TREINADORES\n"
	.string "fortes além de mim!$"

BattlePyramid_Text_OneTrainersRemaining1::
	.string "Você foi demais!\p"
	.string "Mas ainda tem um TREINADOR\n"
	.string "forte além de mim!$"

BattlePyramid_Text_ZeroTrainersRemaining1::
	.string "Você foi demais!\p"
	.string "Não sobrou ninguém que\n"
	.string "consiga te vencer!$"

BattlePyramid_Text_SevenTrainersRemaining2::
	.string "Que raiva!\p"
	.string "Mas ainda faltam sete TREINADORES!\n"
	.string "Alguém vai te dar uma lição!$"

BattlePyramid_Text_SixTrainersRemaining2::
	.string "Que raiva!\p"
	.string "Mas ainda faltam seis TREINADORES!\n"
	.string "Alguém vai te dar uma lição!$"

BattlePyramid_Text_FiveTrainersRemaining2::
	.string "Que raiva!\p"
	.string "Mas ainda faltam cinco TREINADORES!\n"
	.string "Alguém vai te dar uma lição!$"

BattlePyramid_Text_FourTrainersRemaining2::
	.string "Que raiva!\p"
	.string "Mas ainda faltam quatro TREINADORES!\n"
	.string "Alguém vai te dar uma lição!$"

BattlePyramid_Text_ThreeTrainersRemaining2::
	.string "Que raiva!\p"
	.string "Mas ainda faltam três TREINADORES!\n"
	.string "Alguém vai te dar uma lição!$"

BattlePyramid_Text_TwoTrainersRemaining2::
	.string "Que raiva!\p"
	.string "Mas ainda faltam dois TREINADORES!\n"
	.string "Alguém vai te dar uma lição!$"

BattlePyramid_Text_OneTrainersRemaining2::
	.string "Que raiva!\p"
	.string "Mas ainda falta um TREINADOR!\n"
	.string "Você vai levar uma lição!$"

BattlePyramid_Text_ZeroTrainersRemaining2::
	.string "Que raiva!\p"
	.string "Mas não sobrou nenhum TREINADOR\n"
	.string "pra te encarar!$"

BattlePyramid_Text_SevenTrainersRemaining3::
	.string "Bem impressionante!\p"
	.string "Mas ainda faltam sete TREINADORES\n"
	.string "neste andar. Consegue vencer todos?$"

BattlePyramid_Text_SixTrainersRemaining3::
	.string "Bem impressionante!\p"
	.string "Mas ainda faltam seis TREINADORES\n"
	.string "neste andar. Consegue vencer todos?$"

BattlePyramid_Text_FiveTrainersRemaining3::
	.string "Bem impressionante!\p"
	.string "Mas ainda faltam cinco TREINADORES\n"
	.string "neste andar. Consegue vencer todos?$"

BattlePyramid_Text_FourTrainersRemaining3::
	.string "Bem impressionante!\p"
	.string "Mas ainda faltam quatro TREINADORES\n"
	.string "neste andar. Consegue vencer todos?$"

BattlePyramid_Text_ThreeTrainersRemaining3::
	.string "Bem impressionante!\p"
	.string "Mas ainda faltam três TREINADORES\n"
	.string "neste andar. Consegue vencer todos?$"

BattlePyramid_Text_TwoTrainersRemaining3::
	.string "Bem impressionante!\p"
	.string "Mas ainda faltam dois TREINADORES\n"
	.string "neste andar. Consegue vencer os dois?$"

BattlePyramid_Text_OneTrainersRemaining3::
	.string "Bem impressionante!\p"
	.string "Mas ainda falta um TREINADOR\n"
	.string "neste andar. Consegue vencer?$"

BattlePyramid_Text_ZeroTrainersRemaining3::
	.string "Bem impressionante!\p"
	.string "Você passou por todos os\n"
	.string "TREINADORES deste andar.$"

BattlePyramid_Text_SevenTrainersRemaining4::
	.string "Talvez você vença os sete\n"
	.string "TREINADORES que faltam neste andar.$"

BattlePyramid_Text_SixTrainersRemaining4::
	.string "Talvez você vença os seis\n"
	.string "TREINADORES que faltam neste andar.$"

BattlePyramid_Text_FiveTrainersRemaining4::
	.string "Talvez você vença os cinco\n"
	.string "TREINADORES que faltam neste andar.$"

BattlePyramid_Text_FourTrainersRemaining4::
	.string "Talvez você vença os quatro\n"
	.string "TREINADORES que faltam neste andar.$"

BattlePyramid_Text_ThreeTrainersRemaining4::
	.string "Talvez você vença os três\n"
	.string "TREINADORES que faltam neste andar.$"

BattlePyramid_Text_TwoTrainersRemaining4::
	.string "Talvez você vença os dois\n"
	.string "TREINADORES que faltam neste andar.$"

BattlePyramid_Text_OneTrainersRemaining4::
	.string "Talvez você complete a limpa com\n"
	.string "o TREINADOR que falta neste andar.$"

BattlePyramid_Text_ZeroTrainersRemaining4::
	.string "Não sobrou ninguém que\n"
	.string "consiga te vencer agora…$"

BattlePyramid_Text_SevenTrainersRemaining5::
	.string "Talvez você tenha o que é preciso\n"
	.string "pra vencer os sete TREINADORES que\l"
	.string "restam.$"

BattlePyramid_Text_SixTrainersRemaining5::
	.string "Talvez você tenha o que é preciso\n"
	.string "pra vencer os seis TREINADORES que\l"
	.string "restam.$"

BattlePyramid_Text_FiveTrainersRemaining5::
	.string "Talvez você tenha o que é preciso\n"
	.string "pra vencer os cinco TREINADORES que\l"
	.string "restam.$"

BattlePyramid_Text_FourTrainersRemaining5::
	.string "Talvez você tenha o que é preciso\n"
	.string "pra vencer os quatro TREINADORES que\l"
	.string "restam.$"

BattlePyramid_Text_ThreeTrainersRemaining5::
	.string "Talvez você tenha o que é preciso\n"
	.string "pra vencer os três TREINADORES que\l"
	.string "restam.$"

BattlePyramid_Text_TwoTrainersRemaining5::
	.string "Talvez você tenha o que é preciso\n"
	.string "pra vencer os dois TREINADORES que\l"
	.string "restam.$"

BattlePyramid_Text_OneTrainersRemaining5::
	.string "Talvez você tenha o que é preciso\n"
	.string "pra vencer o último TREINADOR.$"

BattlePyramid_Text_ZeroTrainersRemaining5::
	.string "A sua habilidade é impecável.\p"
	.string "Não há mais TREINADORES aqui com\n"
	.string "chance de te vencer.$"

BattlePyramid_Text_SevenTrainersRemaining6::
	.string "Consegue continuar vencendo os\n"
	.string "sete TREINADORES que restam?$"

BattlePyramid_Text_SixTrainersRemaining6::
	.string "Consegue continuar vencendo os\n"
	.string "seis TREINADORES que restam?$"

BattlePyramid_Text_FiveTrainersRemaining6::
	.string "Consegue continuar vencendo os\n"
	.string "cinco TREINADORES que restam?$"

BattlePyramid_Text_FourTrainersRemaining6::
	.string "Consegue continuar vencendo os\n"
	.string "quatro TREINADORES que restam?$"

BattlePyramid_Text_ThreeTrainersRemaining6::
	.string "Consegue continuar vencendo os\n"
	.string "três TREINADORES que restam?$"

BattlePyramid_Text_TwoTrainersRemaining6::
	.string "Consegue continuar vencendo os\n"
	.string "dois TREINADORES que restam?$"

BattlePyramid_Text_OneTrainersRemaining6::
	.string "Consegue vencer o último\n"
	.string "TREINADOR que resta?$"

BattlePyramid_Text_ZeroTrainersRemaining6::
	.string "Não sobrou nenhum TREINADOR que\n"
	.string "possa te encarar agora…$"
