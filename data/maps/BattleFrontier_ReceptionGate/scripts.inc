BattleFrontier_ReceptionGate_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, BattleFrontier_ReceptionGate_OnFrame
	map_script MAP_SCRIPT_ON_TRANSITION, BattleFrontier_ReceptionGate_OnTransition
	.byte 0

BattleFrontier_ReceptionGate_OnTransition:
	setflag FLAG_LANDMARK_BATTLE_FRONTIER
	end

BattleFrontier_ReceptionGate_OnFrame:
	map_script_2 VAR_HAS_ENTERED_BATTLE_FRONTIER, 0, BattleFrontier_ReceptionGate_EventScript_FirstTimeEntering
	.short 0

BattleFrontier_ReceptionGate_EventScript_FirstTimeEntering::
	lockall
	setvar VAR_HAS_ENTERED_BATTLE_FRONTIER, 1
	playse SE_PIN
	applymovement LOCALID_FRONTIER_RECEPTION_GREETER, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_FRONTIER_RECEPTION_GREETER, Common_Movement_Delay48
	waitmovement 0
	msgbox BattleFrontier_ReceptionGate_Text_FirstTimeHereThisWay, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, BattleFrontier_ReceptionGate_Movement_PlayerApproachCounter
	waitmovement 0
	goto BattleFrontier_ReceptionGate_EventScript_ScottScene
	end

BattleFrontier_ReceptionGate_EventScript_ScottScene::
	msgbox BattleFrontier_ReceptionGate_Text_WelcomeToBattleFrontier, MSGBOX_DEFAULT
	msgbox BattleFrontier_ReceptionGate_Text_IssueFrontierPass, MSGBOX_DEFAULT
	playfanfare MUS_OBTAIN_ITEM
	message BattleFrontier_ReceptionGate_Text_ObtainedFrontierPass
	waitfanfare
	waitmessage
	msgbox BattleFrontier_ReceptionGate_Text_PlacedTrainerCardInFrontierPass, MSGBOX_DEFAULT
	setflag FLAG_SYS_FRONTIER_PASS
	msgbox BattleFrontier_ReceptionGate_Text_EnjoyBattleFrontier, MSGBOX_DEFAULT
	msgbox BattleFrontier_ReceptionGate_Text_IfItIsntPlayerYouCame, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_FRONTIER_RECEPTION_GREETER, Common_Movement_WalkInPlaceFasterUp
	applymovement LOCALID_FRONTIER_RECEPTION_GUIDE, Common_Movement_WalkInPlaceFasterUp
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	playse SE_PIN
	applymovement LOCALID_FRONTIER_RECEPTION_GREETER, Common_Movement_ExclamationMark
	applymovement LOCALID_FRONTIER_RECEPTION_GUIDE, Common_Movement_ExclamationMark
	applymovement LOCALID_PLAYER, Common_Movement_ExclamationMark
	waitmovement 0
	msgbox BattleFrontier_ReceptionGate_Text_OhMrScottGoodDay, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_FRONTIER_RECEPTION_GREETER, BattleFrontier_ReceptionGate_Movement_GreeterFaceScott
	applymovement LOCALID_FRONTIER_RECEPTION_GUIDE, BattleFrontier_ReceptionGate_Movement_FacilityGuideFaceScott
	applymovement LOCALID_PLAYER, BattleFrontier_ReceptionGate_Movement_PlayerFaceScott
	applymovement LOCALID_FRONTIER_RECEPTION_SCOTT, BattleFrontier_ReceptionGate_Movement_ScottEnter
	waitmovement 0
	msgbox BattleFrontier_ReceptionGate_Text_ScottGreatToSeeYouHere, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_FRONTIER_RECEPTION_SCOTT, BattleFrontier_ReceptionGate_Movement_ScottExit
	waitmovement 0
	removeobject LOCALID_FRONTIER_RECEPTION_SCOTT
	releaseall
	end

BattleFrontier_ReceptionGate_Movement_PlayerApproachCounter:
	walk_up
	walk_up
	walk_left
	walk_left
	step_end

BattleFrontier_ReceptionGate_Movement_PlayerFaceScott:
	delay_16
	delay_16
	delay_16
	delay_8
	walk_in_place_faster_right
	step_end

/* Unused*/
BattleFrontier_ReceptionGate_Movement_WalkDown:
	walk_down
	walk_down
	step_end

BattleFrontier_ReceptionGate_Movement_ScottEnter:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	step_end

BattleFrontier_ReceptionGate_Movement_ScottExit:
	walk_right
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

BattleFrontier_ReceptionGate_Movement_GreeterFaceScott:
	delay_16
	delay_16
	delay_16
	delay_8
	walk_in_place_faster_right
	step_end

BattleFrontier_ReceptionGate_Movement_FacilityGuideFaceScott:
	delay_16
	delay_16
	delay_16
	delay_8
	walk_in_place_faster_left
	step_end

BattleFrontier_ReceptionGate_EventScript_Greeter::
	lock
	faceplayer
	msgbox BattleFrontier_ReceptionGate_Text_WelcomeToBattleFrontier, MSGBOX_DEFAULT
	msgbox BattleFrontier_ReceptionGate_Text_EnjoyBattleFrontier, MSGBOX_DEFAULT
	release
	end

BattleFrontier_ReceptionGate_EventScript_FacilityGuide::
	lock
	faceplayer
	msgbox BattleFrontier_ReceptionGate_Text_YourGuideToFacilities, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout::
	message BattleFrontier_ReceptionGate_Text_LearnAboutWhich2
	waitmessage
	setvar VAR_0x8004, SCROLL_MULTI_BF_RECEPTIONIST
	special ShowScrollableMultichoice
	waitstate
	switch VAR_RESULT
	case 0, BattleFrontier_ReceptionGate_EventScript_BattleTower
	case 1, BattleFrontier_ReceptionGate_EventScript_BattleDome
	case 2, BattleFrontier_ReceptionGate_EventScript_BattlePalace
	case 3, BattleFrontier_ReceptionGate_EventScript_BattleArena
	case 4, BattleFrontier_ReceptionGate_EventScript_BattleFactory
	case 5, BattleFrontier_ReceptionGate_EventScript_BattlePike
	case 6, BattleFrontier_ReceptionGate_EventScript_BattlePyramid
	case 7, BattleFrontier_ReceptionGate_EventScript_RankingHall
	case 8, BattleFrontier_ReceptionGate_EventScript_ExchangeCorner
	case 9, BattleFrontier_ReceptionGate_EventScript_ExitFacilityGuide
	case MULTI_B_PRESSED, BattleFrontier_ReceptionGate_EventScript_ExitFacilityGuide
	end

BattleFrontier_ReceptionGate_EventScript_BattleTower::
	msgbox BattleFrontier_ReceptionGate_Text_BattleTowerInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_BattleDome::
	msgbox BattleFrontier_ReceptionGate_Text_BattleDomeInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_BattlePalace::
	msgbox BattleFrontier_ReceptionGate_Text_BattlePalaceInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_BattleArena::
	msgbox BattleFrontier_ReceptionGate_Text_BattleArenaInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_BattleFactory::
	msgbox BattleFrontier_ReceptionGate_Text_BattleFactoryInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_BattlePike::
	msgbox BattleFrontier_ReceptionGate_Text_BattlePikeInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_BattlePyramid::
	msgbox BattleFrontier_ReceptionGate_Text_BattlePyramidInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_RankingHall::
	msgbox BattleFrontier_ReceptionGate_Text_RankingHallInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_ExchangeCorner::
	msgbox BattleFrontier_ReceptionGate_Text_ExchangeCornerInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFacilityToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_ExitFacilityGuide::
	msgbox BattleFrontier_ReceptionGate_Text_EnjoyBattleFrontier, MSGBOX_DEFAULT
	release
	end

BattleFrontier_ReceptionGate_EventScript_RulesGuide::
	lock
	faceplayer
	msgbox BattleFrontier_ReceptionGate_Text_YourGuideToRules, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseRuleToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_ChooseRuleToLearnAbout::
	message BattleFrontier_ReceptionGate_Text_LearnAboutWhat
	waitmessage
	multichoice 15, 0, MULTI_FRONTIER_RULES, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_ReceptionGate_EventScript_LevelMode
	case 1, BattleFrontier_ReceptionGate_EventScript_Level50
	case 2, BattleFrontier_ReceptionGate_EventScript_OpenLevel
	case 3, BattleFrontier_ReceptionGate_EventScript_MonEntry
	case 4, BattleFrontier_ReceptionGate_EventScript_HoldItems
	case 5, BattleFrontier_ReceptionGate_EventScript_ExitRulesGuide
	case MULTI_B_PRESSED, BattleFrontier_ReceptionGate_EventScript_ExitRulesGuide
	end

BattleFrontier_ReceptionGate_EventScript_LevelMode::
	msgbox BattleFrontier_ReceptionGate_Text_LevelModeInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseRuleToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_Level50::
	msgbox BattleFrontier_ReceptionGate_Text_Level50Info, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseRuleToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_OpenLevel::
	msgbox BattleFrontier_ReceptionGate_Text_OpenLevelInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseRuleToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_MonEntry::
	msgbox BattleFrontier_ReceptionGate_Text_MonEntryInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseRuleToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_HoldItems::
	msgbox BattleFrontier_ReceptionGate_Text_HoldItemsInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseRuleToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_ExitRulesGuide::
	msgbox BattleFrontier_ReceptionGate_Text_EnjoyBattleFrontier, MSGBOX_DEFAULT
	release
	end

BattleFrontier_ReceptionGate_EventScript_FrontierPassGuide::
	lock
	faceplayer
	msgbox BattleFrontier_ReceptionGate_Text_YourGuideToFrontierPass, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFrontierPassInfoToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_ChooseFrontierPassInfoToLearnAbout::
	message BattleFrontier_ReceptionGate_Text_LearnAboutWhich1
	waitmessage
	multichoice 16, 4, MULTI_FRONTIER_PASS_INFO, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_ReceptionGate_EventScript_Symbols
	case 1, BattleFrontier_ReceptionGate_EventScript_RecordBattle
	case 2, BattleFrontier_ReceptionGate_EventScript_BattlePoints
	case 3, BattleFrontier_ReceptionGate_EventScript_ExitFrontierPassGuide
	case MULTI_B_PRESSED, BattleFrontier_ReceptionGate_EventScript_ExitFrontierPassGuide
	end

BattleFrontier_ReceptionGate_EventScript_Symbols::
	msgbox BattleFrontier_ReceptionGate_Text_SymbolsInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFrontierPassInfoToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_RecordBattle::
	msgbox BattleFrontier_ReceptionGate_Text_RecordedBattleInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFrontierPassInfoToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_BattlePoints::
	msgbox BattleFrontier_ReceptionGate_Text_BattlePointsInfo, MSGBOX_DEFAULT
	goto BattleFrontier_ReceptionGate_EventScript_ChooseFrontierPassInfoToLearnAbout
	end

BattleFrontier_ReceptionGate_EventScript_ExitFrontierPassGuide::
	msgbox BattleFrontier_ReceptionGate_Text_EnjoyBattleFrontier, MSGBOX_DEFAULT
	release
	end

BattleFrontier_ReceptionGate_Text_FirstTimeHereThisWay:
	.string "É a sua primeira vez aqui?\n"
	.string "Por aqui, por favor!$"

BattleFrontier_ReceptionGate_Text_WelcomeToBattleFrontier:
	.string "A linha de frente das batalhas POKéMON!\n"
	.string "Boas-vindas à FRONTEIRA DE BATALHA!$"

BattleFrontier_ReceptionGate_Text_IssueFrontierPass:
	.string "Pra quem vem pela primeira vez,\n"
	.string "entregamos o PASSE DA FRONTEIRA.\p"
	.string "Ele serve em todas as instalações\n"
	.string "da FRONTEIRA DE BATALHA.\p"
	.string "Aqui está!$"

BattleFrontier_ReceptionGate_Text_ObtainedFrontierPass:
	.string "{PLAYER} recebeu\n"
	.string "o PASSE DA FRONTEIRA.$"

BattleFrontier_ReceptionGate_Text_PlacedTrainerCardInFrontierPass:
	.string "{PLAYER} colocou o CARTÃO DE TREINADOR\n"
	.string "no PASSE DA FRONTEIRA.$"

BattleFrontier_ReceptionGate_Text_EnjoyBattleFrontier:
	.string "Aproveite tudo o que a FRONTEIRA\n"
	.string "DE BATALHA tem a oferecer!$"

BattleFrontier_ReceptionGate_Text_IfItIsntPlayerYouCame:
	.string "???: Ora, se não é {PLAYER}{KUN}!\n"
	.string "Você veio!$"

BattleFrontier_ReceptionGate_Text_OhMrScottGoodDay:
	.string "GUIA: Oh! SR. SCOTT!\n"
	.string "Bom dia, senhor!$"

BattleFrontier_ReceptionGate_Text_ScottGreatToSeeYouHere:
	.string "SCOTT: Que ótimo te ver aqui,\n"
	.string "de verdade!\p"
	.string "Espero que explore tudo com calma.\p"
	.string "E, claro, espero que você sinta a\n"
	.string "essência pura da batalha.\p"
	.string "Também tenho os meus aposentos\n"
	.string "aqui, então apareça se tiver tempo.$"

BattleFrontier_ReceptionGate_Text_YourGuideToFacilities:
	.string "Sou o guia das instalações da\n"
	.string "FRONTEIRA DE BATALHA.$"

BattleFrontier_ReceptionGate_Text_LearnAboutWhich2:
	.string "Sobre qual você quer saber?$"

BattleFrontier_ReceptionGate_Text_BattleTowerInfo:
	.string "É a torre gigante, considerada o\n"
	.string "símbolo da FRONTEIRA DE BATALHA.\p"
	.string "Há quatro tipos de SALAS DE BATALHA\n"
	.string "na torre: BATALHAS SIMPLES, DUPLAS,\l"
	.string "MÚLTIPLAS e MÚLTIPLAS POR LINK.$"

BattleFrontier_ReceptionGate_Text_BattleDomeInfo:
	.string "A CÚPULA DE BATALHA é o prédio\n"
	.string "grande em forma de ovo gigante.\p"
	.string "Nela acontecem os eventos chamados\n"
	.string "Torneios de Batalha.\p"
	.string "Os Torneios de Batalha têm duas\n"
	.string "modalidades: BATALHAS SIMPLES\l"
	.string "e DUPLAS.$"

BattleFrontier_ReceptionGate_Text_BattlePalaceInfo:
	.string "O PALÁCIO DE BATALHA é o prédio\n"
	.string "vermelho à direita da FRONTEIRA.\p"
	.string "Há dois tipos de SALÕES DE BATALHA,\n"
	.string "para BATALHAS SIMPLES e DUPLAS.$"

BattleFrontier_ReceptionGate_Text_BattleArenaInfo:
	.string "A ARENA DE BATALHA é o prédio estilo\n"
	.string "dojô no centro-direita da\l"
	.string "FRONTEIRA DE BATALHA.\p"
	.string "Nela acontece o evento chamado\n"
	.string "Torneio de Nocaute.$"

BattleFrontier_ReceptionGate_Text_BattleFactoryInfo:
	.string "A FÁBRICA DE BATALHA é o prédio\n"
	.string "grande mais perto daqui.\p"
	.string "Nela acontece o evento chamado\n"
	.string "Troca de Batalha.\p"
	.string "A Troca de Batalha tem duas\n"
	.string "modalidades: BATALHAS SIMPLES\l"
	.string "e DUPLAS.$"

BattleFrontier_ReceptionGate_Text_BattlePikeInfo:
	.string "O PICO DE BATALHA é o prédio em\n"
	.string "forma de POKéMON no centro-esquerda\l"
	.string "da FRONTEIRA DE BATALHA.\p"
	.string "Nele acontece o evento chamado\n"
	.string "Escolha de Batalha.$"

BattleFrontier_ReceptionGate_Text_BattlePyramidInfo:
	.string "A PIRÂMIDE DE BATALHA é a pirâmide\n"
	.string "enorme.\p"
	.string "Nela acontece o evento chamado\n"
	.string "Missão de Batalha.$"

BattleFrontier_ReceptionGate_Text_RankingHallInfo:
	.string "O SALÃO DO RANKING fica perto da\n"
	.string "TORRE DE BATALHA.\p"
	.string "Lá dá pra ver os recordes fantásticos\n"
	.string "deixados pelos TREINADORES que\l"
	.string "encararam os vários desafios da\l"
	.string "FRONTEIRA DE BATALHA.$"

BattleFrontier_ReceptionGate_Text_ExchangeCornerInfo:
	.string "A CENTRAL DE TROCAS fica perto da\n"
	.string "TORRE DE BATALHA.\p"
	.string "Lá, os Pontos de Batalha ganhos na\n"
	.string "FRONTEIRA podem ser trocados por\l"
	.string "prêmios fabulosos.$"

BattleFrontier_ReceptionGate_Text_YourGuideToRules:
	.string "Sou o guia das regras básicas comuns\n"
	.string "a todos os desafios das instalações\l"
	.string "da FRONTEIRA DE BATALHA.$"

BattleFrontier_ReceptionGate_Text_LearnAboutWhat:
	.string "Sobre o que você quer saber?$"

BattleFrontier_ReceptionGate_Text_LevelModeInfo:
	.string "Todos os desafios das instalações\n"
	.string "da FRONTEIRA têm duas modalidades:\l"
	.string "Nível 50 e Nível Livre.$"

BattleFrontier_ReceptionGate_Text_Level50Info:
	.string "A modalidade Nível 50 aceita POKéMON\n"
	.string "até o nível 50, inclusive.\p"
	.string "Mas lembre que nenhum TREINADOR que\n"
	.string "você enfrentar terá POKéMON abaixo\l"
	.string "do nível 50.\p"
	.string "Esta é a modalidade de entrada nas\n"
	.string "batalhas da FRONTEIRA DE BATALHA.\p"
	.string "Pra começar, recomendamos que\n"
	.string "encare esta modalidade.$"

BattleFrontier_ReceptionGate_Text_OpenLevelInfo:
	.string "A modalidade Nível Livre não tem\n"
	.string "limite de nível pros POKéMON que\l"
	.string "entram nos desafios.\p"
	.string "O nível dos oponentes se ajusta ao\n"
	.string "nível dos seus POKéMON.\p"
	.string "Mas nenhum TREINADOR que você\n"
	.string "enfrentar terá POKéMON abaixo do\l"
	.string "nível 60.$"

BattleFrontier_ReceptionGate_Text_MonEntryInfo:
	.string "Quase todo tipo de POKéMON pode\n"
	.string "encarar os desafios das instalações.\p"
	.string "Mas OVOS e certos tipos de POKéMON\n"
	.string "não são permitidos.\p"
	.string "O número de POKéMON exigido depende\n"
	.string "da instalação.\p"
	.string "Em todas as instalações, porém,\n"
	.string "não são permitidos dois ou mais\l"
	.string "POKéMON da mesma espécie.$"

BattleFrontier_ReceptionGate_Text_HoldItemsInfo:
	.string "Ao entrar num desafio da FRONTEIRA\n"
	.string "DE BATALHA, os POKéMON não podem\l"
	.string "segurar o mesmo tipo de item.\p"
	.string "Confira se todos os POKéMON do\n"
	.string "desafio estão segurando itens\l"
	.string "diferentes.$"

BattleFrontier_ReceptionGate_Text_YourGuideToFrontierPass:
	.string "Sou o guia do PASSE DA FRONTEIRA.$"

BattleFrontier_ReceptionGate_Text_LearnAboutWhich1:
	.string "Sobre qual você quer saber?$"

BattleFrontier_ReceptionGate_Text_SymbolsInfo:
	.string "Há sete instalações na FRONTEIRA\n"
	.string "DE BATALHA.\p"
	.string "TREINADORES que se destacam podem\n"
	.string "ganhar símbolos de cada instalação.\p"
	.string "Pra ganhar um símbolo, pode ser\n"
	.string "preciso vencer TREINADORES em\l"
	.string "sequência ou vencer eventos várias\l"
	.string "vezes.\p"
	.string "Não é nada fácil ganhar símbolos.\n"
	.string "Boa sorte!$"

BattleFrontier_ReceptionGate_Text_RecordedBattleInfo:
	.string "Dá pra gravar uma batalha no seu\n"
	.string "PASSE DA FRONTEIRA.\p"
	.string "Pode ser uma batalha com um ou\n"
	.string "mais amigos.\p"
	.string "Ou uma batalha feita na FRONTEIRA DE\n"
	.string "BATALHA, menos as do PICO DE BATALHA\l"
	.string "e da PIRÂMIDE DE BATALHA.\p"
	.string "Você escolhe gravar a batalha no\n"
	.string "final dela.$"

BattleFrontier_ReceptionGate_Text_BattlePointsInfo:
	.string "Os Pontos de Batalha são prêmios\n"
	.string "para TREINADORES que se destacaram\l"
	.string "nas batalhas da FRONTEIRA.\p"
	.string "Dá pra trocar os Pontos de Batalha\n"
	.string "por prêmios na CENTRAL DE TROCAS.$"

