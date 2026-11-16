BattleFrontier_Lounge3_MapScripts::
	.byte 0

	.set BET_AMOUNT_5,  5
	.set BET_AMOUNT_10, 10
	.set BET_AMOUNT_15, 15

BattleFrontier_Lounge3_EventScript_Gambler::
	lock
	faceplayer
	goto_if_set FLAG_MET_BATTLE_FRONTIER_GAMBLER, BattleFrontier_Lounge3_EventScript_AlreadyMetGambler
	call BattleFrontier_Lounge3_EventScript_CountSilverSymbols
	goto_if_le VAR_0x8004, 2, BattleFrontier_Lounge3_EventScript_NotEnoughSilverSymbols
	setflag FLAG_MET_BATTLE_FRONTIER_GAMBLER
	msgbox BattleFrontier_Lounge3_Text_YouLookToughExplainGambling, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge3_EventScript_AskToEnterChallenge
	end

BattleFrontier_Lounge3_EventScript_AskToEnterChallenge::
	special ShowFrontierGamblerLookingMessage
	waitmessage
	waitbuttonpress
	msgbox BattleFrontier_Lounge3_Text_HowAboutEnteringEventForMe, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, BattleFrontier_Lounge3_EventScript_DeclineChallenge
	msgbox BattleFrontier_Lounge3_Text_SpotMeSomeBattlePoints, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, BattleFrontier_Lounge3_EventScript_DeclineChallenge
	message BattleFrontier_Lounge3_Text_HowMuchCanYouSpot
	waitmessage
	special ShowBattlePointsWindow
	goto BattleFrontier_Lounge3_EventScript_ChooseBetAmount
	end

BattleFrontier_Lounge3_EventScript_ChooseBetAmount::
	multichoice 20, 4, MULTI_FRONTIER_GAMBLER_BET, FALSE
	copyvar VAR_FRONTIER_GAMBLER_AMOUNT_BET, VAR_RESULT
	switch VAR_RESULT
	case FRONTIER_GAMBLER_BET_5, BattleFrontier_Lounge3_EventScript_Bet5
	case FRONTIER_GAMBLER_BET_10, BattleFrontier_Lounge3_EventScript_Bet10
	case FRONTIER_GAMBLER_BET_15, BattleFrontier_Lounge3_EventScript_Bet15
	case FRONTIER_GAMBLER_BET_CANCEL, BattleFrontier_Lounge3_EventScript_CancelBet
	case MULTI_B_PRESSED, BattleFrontier_Lounge3_EventScript_CancelBet
	end

BattleFrontier_Lounge3_EventScript_Bet5::
	setvar VAR_0x8008, BET_AMOUNT_5
	goto BattleFrontier_Lounge3_EventScript_TryPlaceBet
	end

BattleFrontier_Lounge3_EventScript_Bet10::
	setvar VAR_0x8008, BET_AMOUNT_10
	goto BattleFrontier_Lounge3_EventScript_TryPlaceBet
	end

BattleFrontier_Lounge3_EventScript_Bet15::
	setvar VAR_0x8008, BET_AMOUNT_15
	goto BattleFrontier_Lounge3_EventScript_TryPlaceBet
	end

BattleFrontier_Lounge3_EventScript_TryPlaceBet::
	specialvar VAR_TEMP_1, GetFrontierBattlePoints
	goto_if_ge VAR_TEMP_1, VAR_0x8008, BattleFrontier_Lounge3_EventScript_PlaceBet
	msgbox BattleFrontier_Lounge3_Text_YouDontHaveEnoughPoints, MSGBOX_DEFAULT
	message BattleFrontier_Lounge3_Text_HowMuchCanYouSpot
	waitmessage
	goto BattleFrontier_Lounge3_EventScript_ChooseBetAmount
	end

BattleFrontier_Lounge3_EventScript_PlaceBet::
	copyvar VAR_0x8004, VAR_0x8008
	special TakeFrontierBattlePoints
	setvar VAR_FRONTIER_GAMBLER_STATE, FRONTIER_GAMBLER_PLACED_BET
	special UpdateBattlePointsWindow
	playse SE_SHOP
	msgbox BattleFrontier_Lounge3_Text_ThanksOffYouGo, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge3_EventScript_FinishBet
	end

BattleFrontier_Lounge3_EventScript_FinishBet::
	special ShowFrontierGamblerGoMessage
	waitmessage
	waitbuttonpress
	special CloseBattlePointsWindow
	release
	end

BattleFrontier_Lounge3_EventScript_CountSilverSymbols::
	setvar VAR_0x8004, 0
	call_if_set FLAG_SYS_TOWER_SILVER, BattleFrontier_Lounge3_EventScript_AddSilverSymbolCount
	call_if_set FLAG_SYS_DOME_SILVER, BattleFrontier_Lounge3_EventScript_AddSilverSymbolCount
	call_if_set FLAG_SYS_PALACE_SILVER, BattleFrontier_Lounge3_EventScript_AddSilverSymbolCount
	call_if_set FLAG_SYS_ARENA_SILVER, BattleFrontier_Lounge3_EventScript_AddSilverSymbolCount
	call_if_set FLAG_SYS_FACTORY_SILVER, BattleFrontier_Lounge3_EventScript_AddSilverSymbolCount
	call_if_set FLAG_SYS_PIKE_SILVER, BattleFrontier_Lounge3_EventScript_AddSilverSymbolCount
	call_if_set FLAG_SYS_PYRAMID_SILVER, BattleFrontier_Lounge3_EventScript_AddSilverSymbolCount
	return

BattleFrontier_Lounge3_EventScript_AddSilverSymbolCount::
	addvar VAR_0x8004, 1
	return

BattleFrontier_Lounge3_EventScript_NotEnoughSilverSymbols::
	msgbox BattleFrontier_Lounge3_Text_CantYouSeeWereBusyHere, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge3_EventScript_FaceOriginalDirection
	end

BattleFrontier_Lounge3_EventScript_AlreadyMetGambler::
	msgbox BattleFrontier_Lounge3_Text_Oh, MSGBOX_DEFAULT
	goto_if_ge VAR_FRONTIER_GAMBLER_STATE, FRONTIER_GAMBLER_PLACED_BET, BattleFrontier_Lounge3_EventScript_CheckBetResults
	goto BattleFrontier_Lounge3_EventScript_AskToEnterChallenge
	end

BattleFrontier_Lounge3_EventScript_CheckBetResults::
	goto_if_eq VAR_FRONTIER_GAMBLER_STATE, FRONTIER_GAMBLER_PLACED_BET, BattleFrontier_Lounge3_EventScript_ChallengeNotAttempted
	goto_if_eq VAR_FRONTIER_GAMBLER_STATE, FRONTIER_GAMBLER_WON, BattleFrontier_Lounge3_EventScript_WonChallenge
	goto BattleFrontier_Lounge3_EventScript_LostChallenge
	end

BattleFrontier_Lounge3_EventScript_WonChallenge::
	msgbox BattleFrontier_Lounge3_Text_HelloChampHeresYourPoints, MSGBOX_DEFAULT
	call_if_eq VAR_FRONTIER_GAMBLER_AMOUNT_BET, FRONTIER_GAMBLER_BET_5, BattleFrontier_Lounge3_EventScript_RewardBet5
	call_if_eq VAR_FRONTIER_GAMBLER_AMOUNT_BET, FRONTIER_GAMBLER_BET_10, BattleFrontier_Lounge3_EventScript_RewardBet10
	call_if_eq VAR_FRONTIER_GAMBLER_AMOUNT_BET, FRONTIER_GAMBLER_BET_15, BattleFrontier_Lounge3_EventScript_RewardBet15
	msgbox BattleFrontier_Lounge3_Text_ObtainedBattlePoints, MSGBOX_GETPOINTS
	special GiveFrontierBattlePoints
	msgbox BattleFrontier_Lounge3_Text_ThinkOfMeForAnotherChallenge, MSGBOX_DEFAULT
	setvar VAR_FRONTIER_GAMBLER_STATE, FRONTIER_GAMBLER_WAITING
	release
	end

BattleFrontier_Lounge3_EventScript_LostChallenge::
	msgbox BattleFrontier_Lounge3_Text_NiceTryCantReturnPoints, MSGBOX_DEFAULT
	setvar VAR_FRONTIER_GAMBLER_STATE, FRONTIER_GAMBLER_WAITING
	release
	end

BattleFrontier_Lounge3_EventScript_RewardBet5::
	buffernumberstring STR_VAR_1, (BET_AMOUNT_5 * 2)
	setvar VAR_0x8004, (BET_AMOUNT_5 * 2)
	return

BattleFrontier_Lounge3_EventScript_RewardBet10::
	buffernumberstring STR_VAR_1, (BET_AMOUNT_10 * 2)
	setvar VAR_0x8004, (BET_AMOUNT_10 * 2)
	return

BattleFrontier_Lounge3_EventScript_RewardBet15::
	buffernumberstring STR_VAR_1, (BET_AMOUNT_15 * 2)
	setvar VAR_0x8004, (BET_AMOUNT_15 * 2)
	return

BattleFrontier_Lounge3_EventScript_ChallengeNotAttempted::
	special ShowFrontierGamblerGoMessage
	waitmessage
	waitbuttonpress
	release
	end

BattleFrontier_Lounge3_EventScript_DeclineChallenge::
	msgbox BattleFrontier_Lounge3_Text_NotInterested, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Lounge3_EventScript_CancelBet::
	special CloseBattlePointsWindow
	goto BattleFrontier_Lounge3_EventScript_DeclineChallenge
	end

BattleFrontier_Lounge3_EventScript_Man::
	msgbox BattleFrontier_Lounge3_Text_ShouldBeTakingChallenges, MSGBOX_NPC
	end

BattleFrontier_Lounge3_EventScript_Woman::
	lock
	faceplayer
	msgbox BattleFrontier_Lounge3_Text_BackedWrongTrainer, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge3_EventScript_FaceOriginalDirection
	end

BattleFrontier_Lounge3_EventScript_PokefanF::
	lock
	faceplayer
	msgbox BattleFrontier_Lounge3_Text_KnowWinnerWhenISeeOne, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge3_EventScript_FaceOriginalDirection
	end

BattleFrontier_Lounge3_EventScript_FatMan::
	lock
	faceplayer
	msgbox BattleFrontier_Lounge3_Text_TrainerGoodButRattled, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge3_EventScript_FaceOriginalDirection
	end

BattleFrontier_Lounge3_EventScript_FaceOriginalDirection::
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

BattleFrontier_Lounge3_Text_CantYouSeeWereBusyHere::
	.string "…O que você quer?\p"
	.string "Não vê que a gente está meio\n"
	.string "ocupado? Não dá pra esperar?$"

BattleFrontier_Lounge3_Text_YouLookToughExplainGambling::
	.string "…Hã?\n"
	.string "Você tem cara de TREINADOR forte.\p"
	.string "Hehe…\n"
	.string "Escuta, tenho uma proposta.\p"
	.string "Temos um grupinho aqui e fazemos\n"
	.string "um joguinho com o que acontece\l"
	.string "na FRONTEIRA DE BATALHA.\p"
	.string "As regras são bem simples.\p"
	.string "Primeiro, escolhemos uma das\n"
	.string "instalações da FRONTEIRA.\p"
	.string "Depois, cada um escolhe um TREINADOR\n"
	.string "que vai encarar o desafio de lá e\l"
	.string "aposta Pontos de Batalha.\p"
	.string "Quem apostou no TREINADOR com o\n"
	.string "melhor resultado leva todos os\l"
	.string "Pontos de Batalha da aposta.\p"
	.string "Simples, né?\n"
	.string "Então, enfim…$"

BattleFrontier_Lounge3_Text_ChallengeBattleTowerSingle::
	.string "Procuro um TREINADOR que vá desafiar\n"
	.string "as SALAS DE BATALHA SIMPLES da\l"
	.string "TORRE DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattleTowerDouble::
	.string "Procuro um TREINADOR que vá desafiar\n"
	.string "as SALAS DE BATALHA DUPLA da\l"
	.string "TORRE DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattleTowerMulti::
	.string "Procuro um TREINADOR que vá desafiar\n"
	.string "as SALAS DE BATALHA MÚLTIPLA da\l"
	.string "TORRE DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattleDomeSingle::
	.string "Procuro um TREINADOR que vá entrar\n"
	.string "no Torneio de BATALHA SIMPLES da\l"
	.string "CÚPULA DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattleDomeDouble::
	.string "Procuro um TREINADOR que vá entrar\n"
	.string "no Torneio de BATALHA DUPLA da\l"
	.string "CÚPULA DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattleFactorySingle::
	.string "Procuro um TREINADOR que vá entrar\n"
	.string "na Troca de Batalha Simples da\l"
	.string "FÁBRICA DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattleFactoryDouble::
	.string "Procuro um TREINADOR que vá entrar\n"
	.string "na Troca de Batalha Dupla da\l"
	.string "FÁBRICA DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattlePalaceSingle::
	.string "Procuro um TREINADOR que vá desafiar\n"
	.string "os SALÕES DE BATALHA SIMPLES do\l"
	.string "PALÁCIO DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattlePalaceDouble::
	.string "Procuro um TREINADOR que vá desafiar\n"
	.string "os SALÕES DE BATALHA DUPLA do\l"
	.string "PALÁCIO DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattleArena::
	.string "Procuro um TREINADOR que vá entrar\n"
	.string "no Torneio de Nocaute da\l"
	.string "ARENA DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattlePike::
	.string "Procuro um TREINADOR que vá entrar\n"
	.string "na Escolha de Batalha do\l"
	.string "PICO DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_ChallengeBattlePyramid::
	.string "Procuro um TREINADOR que vá entrar\n"
	.string "na Missão de Batalha da\l"
	.string "PIRÂMIDE DE BATALHA.\p"
	.string "Mas até agora não vi nenhum\n"
	.string "TREINADOR com cara de vencedor.$"

BattleFrontier_Lounge3_Text_HowAboutEnteringEventForMe::
	.string "Você também vai sair ganhando.\n"
	.string "Então, que tal?\l"
	.string "Que tal entrar nesse evento por mim?$"

BattleFrontier_Lounge3_Text_SpotMeSomeBattlePoints::
	.string "Perfeito.\n"
	.string "Então, hã… Que tal me adiantar uns\l"
	.string "Pontos de Batalha seus?\p"
	.string "Confia, depois eu te recompenso.$"

BattleFrontier_Lounge3_Text_HowMuchCanYouSpot::
	.string "Ótimo, ótimo!\n"
	.string "Quanto você pode me adiantar?$"

BattleFrontier_Lounge3_Text_YouDontHaveEnoughPoints::
	.string "Ah, não, não, não!\n"
	.string "Você não tem Pontos de Batalha\l"
	.string "suficientes!\p"
	.string "Não fique de brincadeira, fazendo\n"
	.string "todo mundo perder tempo!$"

BattleFrontier_Lounge3_Text_ThanksOffYouGo::
	.string "Hehe! Valeu mesmo!\n"
	.string "Então, pode ir!$"

BattleFrontier_Lounge3_Text_GetToBattleTowerSingle::
	.string "Corre pras SALAS DE BATALHA\n"
	.string "SIMPLES da TORRE DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattleTowerDouble::
	.string "Corre pras SALAS DE BATALHA\n"
	.string "DUPLA da TORRE DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattleTowerMulti::
	.string "Corre pras SALAS DE BATALHA\n"
	.string "MÚLTIPLA da TORRE DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattleDomeSingle::
	.string "Corre pro Torneio de BATALHA\n"
	.string "SIMPLES da CÚPULA DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattleDomeDouble::
	.string "Corre pro Torneio de BATALHA\n"
	.string "DUPLA da CÚPULA DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattleFactorySingle::
	.string "Corre pra Troca de Batalha\n"
	.string "Simples da FÁBRICA DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattleFactoryDouble::
	.string "Corre pra Troca de Batalha\n"
	.string "Dupla da FÁBRICA DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattlePalaceSingle::
	.string "Corre pros SALÕES DE BATALHA\n"
	.string "SIMPLES do PALÁCIO DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattlePalaceDouble::
	.string "Corre pros SALÕES DE BATALHA\n"
	.string "DUPLA do PALÁCIO DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattleArena::
	.string "Corre pro Torneio de Nocaute\n"
	.string "da ARENA DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattlePike::
	.string "Corre pra Escolha de Batalha\n"
	.string "do PICO DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_GetToBattlePyramid::
	.string "Corre pra Missão de Batalha\n"
	.string "da PIRÂMIDE DE BATALHA!\p"
	.string "Tem que ganhar!\n"
	.string "Não desperdice a chance!\l"
	.string "O futuro de nós dois depende de você!$"

BattleFrontier_Lounge3_Text_NiceTryCantReturnPoints::
	.string "Ah, é você…\n"
	.string "Boa tentativa…\p"
	.string "Detesto dizer, mas não posso\n"
	.string "devolver os seus Pontos de Batalha…\p"
	.string "Que isso nos motive a tentar mais\n"
	.string "da próxima vez!$"

BattleFrontier_Lounge3_Text_HelloChampHeresYourPoints::
	.string "Ah, sim!\n"
	.string "Olá, fera!\p"
	.string "Eu sabia que você conseguia!\n"
	.string "Eu sabia!\l"
	.string "Ganhamos os dois!\p"
	.string "Vou devolver os seus Pontos de\n"
	.string "Batalha e, claro, um extra meu!$"

BattleFrontier_Lounge3_Text_ObtainedBattlePoints::
	.string "{PLAYER} recebeu\n"
	.string "{STR_VAR_1} Pontos de Batalha.$"

BattleFrontier_Lounge3_Text_ThinkOfMeForAnotherChallenge::
	.string "Se topar outro desafio, por favor,\n"
	.string "lembre de mim!$"

BattleFrontier_Lounge3_Text_NotInterested::
	.string "Não quer?! Não tenha tanto medo de\n"
	.string "arriscar!$"

BattleFrontier_Lounge3_Text_Oh::
	.string "Ah…$"

BattleFrontier_Lounge3_Text_BackedWrongTrainer::
	.string "Apostei no TREINADOR errado de novo!\p"
	.string "Talvez eu devesse só batalhar\n"
	.string "normalmente como todo mundo…$"

BattleFrontier_Lounge3_Text_TrainerGoodButRattled::
	.string "Aquele TREINADOR…\p"
	.string "É bom, mas fica nervoso fácil demais\n"
	.string "pra aguentar a CÚPULA DE BATALHA…$"

BattleFrontier_Lounge3_Text_KnowWinnerWhenISeeOne::
	.string "Hihi!\n"
	.string "Eu reconheço um vencedor de longe!$"

BattleFrontier_Lounge3_Text_ShouldBeTakingChallenges::
	.string "Aqueles TREINADORES…\n"
	.string "O que estão fazendo?\l"
	.string "Deviam estar encarando desafios.$"
