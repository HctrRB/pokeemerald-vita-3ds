BattleFrontier_OutsideWest_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, BattleFrontier_OutsideWest_OnTransition
	.byte 0

BattleFrontier_OutsideWest_OnTransition:
	setvar VAR_BRAVO_TRAINER_BATTLE_TOWER_ON, 0
	setflag FLAG_HIDE_BATTLE_TOWER_REPORTER
	end

BattleFrontier_OutsideWest_EventScript_FerryAttendant::
	lock
	faceplayer
	msgbox BattleFrontier_OutsideWest_Text_MayISeeYourTicket, MSGBOX_DEFAULT
	checkitem ITEM_SS_TICKET
	goto_if_eq VAR_RESULT, FALSE, BattleFrontier_OutsideWest_EventScript_NoSSTicket
	message BattleFrontier_OutsideWest_Text_WhereWouldYouLikeToGo
	waitmessage
	goto BattleFrontier_OutsideWest_EventScript_ChooseFerryDestination
	end

BattleFrontier_OutsideWest_EventScript_ChooseFerryDestination::
	multichoicedefault 18, 6, MULTI_SSTIDAL_BATTLE_FRONTIER, 2, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_OutsideWest_EventScript_FerryToSlateport
	case 1, BattleFrontier_OutsideWest_EventScript_FerryToLilycove
	case 2, BattleFrontier_OutsideWest_EventScript_CancelFerrySelect
	case MULTI_B_PRESSED, BattleFrontier_OutsideWest_EventScript_CancelFerrySelect
	end

BattleFrontier_OutsideWest_EventScript_NoSSTicket::
	msgbox BattleFrontier_OutsideWest_Text_MustHaveTicketToBoard, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideWest_EventScript_FerryToSlateport::
	msgbox BattleFrontier_OutsideWest_Text_SlateportItIs, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, BattleFrontier_OutsideWest_EventScript_ChooseNewFerryDestination
	msgbox BattleFrontier_OutsideWest_Text_PleaseBoardFerry, MSGBOX_DEFAULT
	call BattleFrontier_OutsideWest_EventScript_BoardFerry
	warp MAP_SLATEPORT_CITY_HARBOR, 8, 11
	waitstate
	release
	end

BattleFrontier_OutsideWest_EventScript_FerryToLilycove::
	msgbox BattleFrontier_OutsideWest_Text_LilycoveItIs, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, BattleFrontier_OutsideWest_EventScript_ChooseNewFerryDestination
	msgbox BattleFrontier_OutsideWest_Text_PleaseBoardFerry, MSGBOX_DEFAULT
	call BattleFrontier_OutsideWest_EventScript_BoardFerry
	warp MAP_LILYCOVE_CITY_HARBOR, 8, 11
	waitstate
	release
	end

BattleFrontier_OutsideWest_EventScript_ChooseNewFerryDestination::
	message BattleFrontier_OutsideWest_Text_ThenWhereWouldYouLikeToGo
	waitmessage
	goto BattleFrontier_OutsideWest_EventScript_ChooseFerryDestination
	end

BattleFrontier_OutsideWest_EventScript_BoardFerry::
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	delay 30
	hideobjectat LOCALID_FRONTIER_FERRY_ATTENDANT, MAP_BATTLE_FRONTIER_OUTSIDE_WEST
	setvar VAR_0x8004, LOCALID_FRONTIER_SS_TIDAL
	call Common_EventScript_FerryDepartIsland
	return

BattleFrontier_OutsideWest_EventScript_CancelFerrySelect::
	msgbox BattleFrontier_OutsideWest_Text_SailWithUsAnotherTime, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideWest_EventScript_BattleDomeSign::
	msgbox BattleFrontier_OutsideWest_Text_BattleDomeSign, MSGBOX_SIGN
	end

BattleFrontier_OutsideWest_EventScript_BattleFactorySign::
	msgbox BattleFrontier_OutsideWest_Text_BattleFactorySign, MSGBOX_SIGN
	end

BattleFrontier_OutsideWest_EventScript_BattlePikeSign::
	msgbox BattleFrontier_OutsideWest_Text_BattlePikeSign, MSGBOX_SIGN
	end

/* Unused. Of note, Battle Tower is in the East section in the final release*/
BattleFrontier_OutsideWest_EventScript_UnusedNPC1::
	msgbox BattleFrontier_OutsideWest_Text_ThisIsBattleTower, MSGBOX_NPC
	end

/* Unused*/
BattleFrontier_OutsideWest_EventScript_UnusedNPC2::
	msgbox BattleFrontier_OutsideWest_Text_CantFindBattleTower, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_Boy1::
	msgbox BattleFrontier_OutsideWest_Text_BestOutOfAllMyFriends, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_Fisherman2::
	lock
	faceplayer
	message BattleFrontier_OutsideWest_Text_GotSeasickOnWayHere
	waitmessage
	applymovement LOCALID_FRONTIER_FISHERMAN_2, Common_Movement_FaceAwayPlayer
	waitmovement 0
	waitbuttonpress
	release
	end

BattleFrontier_OutsideWest_EventScript_Man1::
	msgbox BattleFrontier_OutsideWest_Text_OnlyToughTrainersBroughtHere, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_Maniac1::
	lock
	goto BattleFrontier_OutsideWest_EventScript_FactoryChallengersTalk
	end

BattleFrontier_OutsideWest_EventScript_Maniac2::
	lock
	goto BattleFrontier_OutsideWest_EventScript_FactoryChallengersTalk
	end

BattleFrontier_OutsideWest_EventScript_FactoryChallengersTalk::
	applymovement LOCALID_FRONTIER_MANIAC_1, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	msgbox BattleFrontier_OutsideWest_Text_SureWeCanChallengeWithNoMons, MSGBOX_DEFAULT
	applymovement LOCALID_FRONTIER_MANIAC_2, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	msgbox BattleFrontier_OutsideWest_Text_BigGuySaidIllLendYouMons, MSGBOX_DEFAULT
	closemessage
	delay 25
	applymovement LOCALID_FRONTIER_MANIAC_1, Common_Movement_WalkInPlaceFasterUp
	applymovement LOCALID_FRONTIER_MANIAC_2, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	release
	end

BattleFrontier_OutsideWest_EventScript_Camper::
	lock
	faceplayer
	delay 20
	call_if_eq VAR_FACING, DIR_NORTH, BattleFrontier_OutsideWest_EventScript_CamperFaceFactory
	call_if_eq VAR_FACING, DIR_SOUTH, BattleFrontier_OutsideWest_EventScript_CamperAlreadyFacingFactory
	call_if_eq VAR_FACING, DIR_WEST, BattleFrontier_OutsideWest_EventScript_CamperFaceFactory
	call_if_eq VAR_FACING, DIR_EAST, BattleFrontier_OutsideWest_EventScript_CamperFaceFactory
	msgbox BattleFrontier_OutsideWest_Text_WhosRaisingThoseRentalMons, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideWest_EventScript_CamperFaceFactory::
	applymovement LOCALID_FRONTIER_CAMPER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	return

BattleFrontier_OutsideWest_EventScript_CamperAlreadyFacingFactory::
	return

BattleFrontier_OutsideWest_EventScript_Girl::
	lock
	faceplayer
	message BattleFrontier_OutsideWest_Text_ScaredOfPikeBecauseSeviper
	waitmessage
	call_if_eq VAR_FACING, DIR_NORTH, BattleFrontier_OutsideWest_EventScript_GirlShudderNorth
	call_if_eq VAR_FACING, DIR_SOUTH, BattleFrontier_OutsideWest_EventScript_GirlShudderSouth
	call_if_eq VAR_FACING, DIR_WEST, BattleFrontier_OutsideWest_EventScript_GirlShudderWest
	call_if_eq VAR_FACING, DIR_EAST, BattleFrontier_OutsideWest_EventScript_GirlShudderEast
	waitbuttonpress
	release
	end

BattleFrontier_OutsideWest_EventScript_GirlShudderNorth::
	applymovement LOCALID_FRONTIER_GIRL, BattleFrontier_OutsideWest_Movement_GirlShudderNorth
	waitmovement 0
	return

BattleFrontier_OutsideWest_EventScript_GirlShudderSouth::
	applymovement LOCALID_FRONTIER_GIRL, BattleFrontier_OutsideWest_Movement_GirlShudderSouth
	waitmovement 0
	return

BattleFrontier_OutsideWest_EventScript_GirlShudderWest::
	applymovement LOCALID_FRONTIER_GIRL, BattleFrontier_OutsideWest_Movement_GirlShudderWest
	waitmovement 0
	return

BattleFrontier_OutsideWest_EventScript_GirlShudderEast::
	applymovement LOCALID_FRONTIER_GIRL, BattleFrontier_OutsideWest_Movement_GirlShudderEast
	waitmovement 0
	return

BattleFrontier_OutsideWest_Movement_GirlShudderNorth:
	walk_in_place_faster_down
	walk_in_place_faster_down
	step_end

BattleFrontier_OutsideWest_Movement_GirlShudderSouth:
	walk_in_place_faster_up
	walk_in_place_faster_up
	step_end

BattleFrontier_OutsideWest_Movement_GirlShudderWest:
	walk_in_place_faster_right
	walk_in_place_faster_right
	step_end

BattleFrontier_OutsideWest_Movement_GirlShudderEast:
	walk_in_place_faster_left
	walk_in_place_faster_left
	step_end

BattleFrontier_OutsideWest_EventScript_Woman2::
	lock
	faceplayer
	msgbox BattleFrontier_OutsideWest_Text_LetsPlayRockPaperScissors, MSGBOX_DEFAULT
	random 2
	goto_if_eq VAR_RESULT, 1, BattleFrontier_OutsideWest_EventScript_WomanWonRockPaperScissors
	goto BattleFrontier_OutsideWest_EventScript_WomanLostRockPaperScissors
	end

BattleFrontier_OutsideWest_EventScript_WomanWonRockPaperScissors::
	msgbox BattleFrontier_OutsideWest_Text_WonIllTakePikeChallenge, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideWest_EventScript_WomanLostRockPaperScissors::
	msgbox BattleFrontier_OutsideWest_Text_LostIllPutOffPikeChallenge, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideWest_EventScript_Fisherman1::
	msgbox BattleFrontier_OutsideWest_Text_ChooseFishingOverBattling, MSGBOX_NPC
	end

/* Unused*/
BattleFrontier_OutsideWest_EventScript_UnusedNPC3::
	msgbox BattleFrontier_OutsideWest_Text_DomeIsHereGrandpa, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_Gentleman::
	msgbox BattleFrontier_OutsideWest_Text_YoureOffToChallengeDome, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_Lass::
	msgbox BattleFrontier_OutsideWest_Text_KeepBattlingUntilIGetSymbol, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_ExpertM::
	msgbox BattleFrontier_OutsideWest_Text_WontLetGentlemenBeatMe, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_Man2::
	msgbox BattleFrontier_OutsideWest_Text_NothingHereNotLongAgo, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_Woman1::
	msgbox BattleFrontier_OutsideWest_Text_FinallyArrivedAtFrontier, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_FatMan1::
	msgbox BattleFrontier_OutsideWest_Text_SquareFilledWithToughPeople, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_FatMan2::
	msgbox BattleFrontier_OutsideWest_Text_MetOlderGirlAtPike, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_Woman3::
	lock
	msgbox BattleFrontier_OutsideWest_Text_LastTimeOurEyesMet, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideWest_EventScript_Boy2::
	lock
	msgbox BattleFrontier_OutsideWest_Text_DomeAceLookedBecauseOfMyCheering, MSGBOX_DEFAULT
	release
	end

BattleFrontier_OutsideWest_EventScript_OldMan::
	msgbox BattleFrontier_OutsideWest_Text_DomeAceIsMine, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_EventScript_Man4::
	lock
	faceplayer
	msgbox BattleFrontier_OutsideWest_Text_FansOverThereUsedToBeTrainers, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_FRONTIER_MAN_4, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

BattleFrontier_OutsideWest_EventScript_PokefanF::
	msgbox BattleFrontier_OutsideWest_Text_MonWithLongTailInFrontier, MSGBOX_NPC
	end

BattleFrontier_OutsideWest_Text_BattleDomeSign:
	.string "Esta é a CÚPULA DE BATALHA!\n"
	.string "O seu caminho pro estrelato!$"

BattleFrontier_OutsideWest_Text_BattleFactorySign:
	.string "Esta é a FÁBRICA DE BATALHA!\n"
	.string "Em busca dos POKéMON mais fortes!$"

BattleFrontier_OutsideWest_Text_BattlePikeSign:
	.string "Este é o PICO DE BATALHA!\n"
	.string "Escolha um de três caminhos!$"

BattleFrontier_OutsideWest_Text_ThisIsBattleTower:
	.string "Esta é a TORRE DE BATALHA.\p"
	.string "TREINADORES trazem os seus melhores\n"
	.string "POKéMON pra batalhas eliminatórias.$"

BattleFrontier_OutsideWest_Text_MayISeeYourTicket:
	.string "Olá, veio pegar a balsa?\n"
	.string "Posso ver o seu BILHETE?$"

BattleFrontier_OutsideWest_Text_MustHaveTicketToBoard:
	.string "{PLAYER} não tem o BILHETE…\p"
	.string "Sinto muito.\p"
	.string "É preciso ter um BILHETE pra\n"
	.string "embarcar na balsa.$"

BattleFrontier_OutsideWest_Text_WhereWouldYouLikeToGo:
	.string "{PLAYER} mostrou o BILHETE.\p"
	.string "Perfeito! É só isso!\p"
	.string "E pra onde você quer ir?$"

BattleFrontier_OutsideWest_Text_SlateportItIs:
	.string "Pra CIDADE DE SLATEPORT, então!$"

BattleFrontier_OutsideWest_Text_LilycoveItIs:
	.string "Pra CIDADE DE LILYCOVE, então!$"

BattleFrontier_OutsideWest_Text_SailWithUsAnotherTime:
	.string "Viaje com a gente outra hora!$"

BattleFrontier_OutsideWest_Text_PleaseBoardFerry:
	.string "Embarque na balsa e aguarde a\n"
	.string "partida, por favor.$"

BattleFrontier_OutsideWest_Text_ThenWhereWouldYouLikeToGo:
	.string "Então, pra onde você quer ir?$"

BattleFrontier_OutsideWest_Text_BestOutOfAllMyFriends:
	.string "Sou o melhor de todos os meus amigos.\n"
	.string "Mas aqui…\l"
	.string "Não sirvo pra nada!$"

BattleFrontier_OutsideWest_Text_CantFindBattleTower:
	.string "Quero ir à TORRE DE BATALHA, mas não\n"
	.string "acho, mesmo com o mapa da FRONTEIRA.\p"
	.string "Este lugar é grande demais!$"

BattleFrontier_OutsideWest_Text_GotSeasickOnWayHere:
	.string "Eu queria encarar um desafio assim\n"
	.string "que chegasse.\p"
	.string "Mas no caminho fiquei enjoado…\n"
	.string "Urrrgh…$"

BattleFrontier_OutsideWest_Text_OnlyToughTrainersBroughtHere:
	.string "Não é qualquer um que vem aqui,\n"
	.string "sabia?\p"
	.string "Só trazem TREINADORES reconhecidos\n"
	.string "como fortes.\p"
	.string "Por isso muitos TREINADORES nem\n"
	.string "conhecem a FRONTEIRA DE BATALHA.$"

BattleFrontier_OutsideWest_Text_SureWeCanChallengeWithNoMons:
	.string "Ei, mano…\p"
	.string "Tem certeza que dá pra encarar os\n"
	.string "desafios mesmo sem ter POKéMON?$"

BattleFrontier_OutsideWest_Text_BigGuySaidIllLendYouMons:
	.string "Hã…\n"
	.string "Tenho certeza que vai dar certo.\p"
	.string "Acho…\p"
	.string "Mas lembra daquele grandão\n"
	.string "assustador? Ele disse: “Eu empresto\l"
	.string "POKéMON pra vocês!”$"

BattleFrontier_OutsideWest_Text_WhosRaisingThoseRentalMons:
	.string "Ali é a FÁBRICA DE BATALHA.\n"
	.string "Dá pra alugar POKéMON fortes lá.\p"
	.string "Mas fico pensando.\n"
	.string "Quem treina esses POKéMON de aluguel?$"

BattleFrontier_OutsideWest_Text_ScaredOfPikeBecauseSeviper:
	.string "Tenho medo de entrar no PICO DE\n"
	.string "BATALHA por causa do SEVIPER…\p"
	.string "M-mas vim até aqui, então vou tentar\n"
	.string "conquistar tudo!\l"
	.string "…Brrr…$"

BattleFrontier_OutsideWest_Text_LetsPlayRockPaperScissors:
	.string "Vamos jogar pedra, papel e tesoura!\n"
	.string "Jo-ken-pô!\p"
	.string "… … … … … …$"

BattleFrontier_OutsideWest_Text_WonIllTakePikeChallenge:
	.string "Oba! Ganhei!\n"
	.string "Vou encarar o desafio do PICO!$"

BattleFrontier_OutsideWest_Text_LostIllPutOffPikeChallenge:
	.string "Ah, não…\n"
	.string "Perdi.\p"
	.string "Acho que não estou com sorte hoje.\n"
	.string "Vou deixar o desafio do PICO DE\l"
	.string "BATALHA pra amanhã.$"

BattleFrontier_OutsideWest_Text_ChooseFishingOverBattling:
	.string "Acho que sou a única pessoa aqui que,\n"
	.string "sei lá por quê, prefere pescar a\l"
	.string "batalhar.\p"
	.string "Hã? Não dá pra pescar nada aqui?\n"
	.string "Que decepção…$"

BattleFrontier_OutsideWest_Text_KeepBattlingUntilIGetSymbol:
	.string "Hoje vou batalhar sem parar, custe o\n"
	.string "que custar, até ganhar um Símbolo.$"

BattleFrontier_OutsideWest_Text_YoureOffToChallengeDome:
	.string "Ah? Vai desafiar a CÚPULA DE\n"
	.string "BATALHA?\p"
	.string "Desejo toda a sorte.\n"
	.string "Que nós dois subamos vencendo e nos\l"
	.string "encontremos nos desafios.$"

BattleFrontier_OutsideWest_Text_DomeIsHereGrandpa:
	.string "Vovô, aqui!\n"
	.string "A CÚPULA DE BATALHA é aqui!\l"
	.string "Vai lá, vovô!$"

BattleFrontier_OutsideWest_Text_WontLetGentlemenBeatMe:
	.string "Ah, então esta é a CÚPULA DE\n"
	.string "BATALHA? Não vou perder pros\l"
	.string "CAVALHEIROS!\p"
	.string "Mas onde é a entrada?$"

BattleFrontier_OutsideWest_Text_NothingHereNotLongAgo:
	.string "Até pouco tempo atrás, não tinha\n"
	.string "nada aqui.\p"
	.string "Mas olha este lugar agora! Incrível!\n"
	.string "Vou trazer a minha mãe pra ver.$"

BattleFrontier_OutsideWest_Text_FinallyArrivedAtFrontier:
	.string "Finalmente cheguei à FRONTEIRA DE\n"
	.string "BATALHA!\p"
	.string "Com a minha beleza e o meu talento,\n"
	.string "vou chamar atenção, com certeza!$"

BattleFrontier_OutsideWest_Text_SquareFilledWithToughPeople:
	.string "Nhac, nhac…\p"
	.string "Parece que esta praça está cheia\n"
	.string "de gente forte.\p"
	.string "Nhac, nhac…$"

BattleFrontier_OutsideWest_Text_MetOlderGirlAtPike:
	.string "Crec, nhac…\p"
	.string "Um tempo atrás, conheci uma moça\n"
	.string "mais velha no PICO DE BATALHA.\p"
	.string "Ela tinha uns olhos assustadores,\n"
	.string "bem mal-humorados.\p"
	.string "Crec, nhac…\p"
	.string "Ela ficou furiosa: “Quer parar de\n"
	.string "comer?!” E aí me deu uma surra.\p"
	.string "Foi de arrepiar, pode acreditar.\p"
	.string "Crec, nhac…$"

BattleFrontier_OutsideWest_Text_LastTimeOurEyesMet:
	.string "Hã? Você se ouve falando?\n"
	.string "Isso não é nada!\p"
	.string "Da última vez que torci por ele, os\n"
	.string "nossos olhares se cruzaram e\l"
	.string "saiu faísca!$"

BattleFrontier_OutsideWest_Text_DomeAceLookedBecauseOfMyCheering:
	.string "Quêêê?!\n"
	.string "Ah, tá!\p"
	.string "Isso só aconteceu porque eu estava\n"
	.string "sentada do seu lado!\p"
	.string "O ÁS DA CÚPULA só olhou pra cá\n"
	.string "porque eu torci alto demais!$"

BattleFrontier_OutsideWest_Text_DomeAceIsMine:
	.string "Ah, silêncio!\n"
	.string "Menos barulho!\p"
	.string "Vocês, fãs de ocasião, deviam torcer\n"
	.string "pra algum LÍDER de GINÁSIO!\p"
	.string "O ÁS DA CÚPULA é meu!\n"
	.string "O meu único ídolo!$"

BattleFrontier_OutsideWest_Text_FansOverThereUsedToBeTrainers:
	.string "Aquelas pessoas discutindo ali…\p"
	.string "É difícil acreditar, mas já foram\n"
	.string "TREINADORAS, e das boas.\p"
	.string "Eram fortíssimas e famosas também.\p"
	.string "Mas foram arrasadas por um certo\n"
	.string "TREINADOR num torneio.\p"
	.string "Apanharam tanto que se apaixonaram\n"
	.string "pelo TREINADOR.\p"
	.string "Agora são só fãs dele. Vão torcer\n"
	.string "pelo ídolo na CÚPULA DE BATALHA\l"
	.string "todo dia.$"

BattleFrontier_OutsideWest_Text_MonWithLongTailInFrontier:
	.string "Ouvi um boato de que alguém viu um\n"
	.string "POKéMON de rabo muito comprido em\l"
	.string "algum lugar da FRONTEIRA.\p"
	.string "Deve ser uma graça. Tenho certeza…\p"
	.string "Ah, os desafios podem esperar. Tenho\n"
	.string "que achar essa criaturinha!$"

