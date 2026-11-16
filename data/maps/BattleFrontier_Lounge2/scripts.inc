BattleFrontier_Lounge2_MapScripts::
	.byte 0

/* This NPC gives hints about a random facility or battle mode.*/
/* For battle modes he gives generic advice*/
/* For facilities, depending on how far the player has progressed he will say either*/
/*    The name of the Frontier Brain there*/
/*    The type and description of the 3 pokemon they use in their silver battle*/
/*    The type and description of the 3 pokemon they use in their gold battle*/
BattleFrontier_Lounge2_EventScript_FrontierManiac::
	lock
	faceplayer
	goto_if_set FLAG_MET_BATTLE_FRONTIER_MANIAC, BattleFrontier_Lounge2_EventScript_AlreadyMetManiac
	setflag FLAG_MET_BATTLE_FRONTIER_MANIAC
	msgbox BattleFrontier_Lounge2_Text_FrontierManiacIntro, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge2_EventScript_GiveAdvice
	end

BattleFrontier_Lounge2_EventScript_AlreadyMetManiac::
	msgbox BattleFrontier_Lounge2_Text_SwingByForTheLatestWord, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge2_EventScript_GiveAdvice
	end

BattleFrontier_Lounge2_EventScript_GiveAdvice::
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_TOWER_SINGLES, BattleFrontier_Lounge2_EventScript_BufferSingle
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_TOWER_DOUBLES, BattleFrontier_Lounge2_EventScript_BufferDouble
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_TOWER_MULTIS, BattleFrontier_Lounge2_EventScript_BufferMulti
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_TOWER_LINK, BattleFrontier_Lounge2_EventScript_BufferMultiLink
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_DOME, BattleFrontier_Lounge2_EventScript_BufferBattleDome
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_FACTORY, BattleFrontier_Lounge2_EventScript_BufferBattleFactory
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_PALACE, BattleFrontier_Lounge2_EventScript_BufferBattlePalace
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_ARENA, BattleFrontier_Lounge2_EventScript_BufferBattleArena
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_PIKE, BattleFrontier_Lounge2_EventScript_BufferBattlePike
	call_if_eq VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_PYRAMID, BattleFrontier_Lounge2_EventScript_BufferBattlePyramid
@ <= FRONTIER_MANIAC_TOWER_LINK is any Battle Tower mode
	call_if_le VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_TOWER_LINK, BattleFrontier_Lounge2_EventScript_BattleTowerNews
@ >= FRONTIER_MANIAC_DOME is any facility other than Battle Tower
	call_if_ge VAR_FRONTIER_MANIAC_FACILITY, FRONTIER_MANIAC_DOME, BattleFrontier_Lounge2_EventScript_FacilityNews
	special ShowFrontierManiacMessage
	waitmessage
	waitbuttonpress
	release
	end

BattleFrontier_Lounge2_EventScript_BattleTowerNews::
	msgbox BattleFrontier_Lounge2_Text_BattleTowerIsHottest, MSGBOX_DEFAULT
	return

BattleFrontier_Lounge2_EventScript_FacilityNews::
	msgbox BattleFrontier_Lounge2_Text_FacilityIsHottest, MSGBOX_DEFAULT
	return

BattleFrontier_Lounge2_EventScript_BufferSingle::
	bufferstdstring STR_VAR_1, STDSTRING_SINGLE
	return

BattleFrontier_Lounge2_EventScript_BufferDouble::
	bufferstdstring STR_VAR_1, STDSTRING_DOUBLE
	return

BattleFrontier_Lounge2_EventScript_BufferMulti::
	bufferstdstring STR_VAR_1, STDSTRING_MULTI
	return

BattleFrontier_Lounge2_EventScript_BufferMultiLink::
	bufferstdstring STR_VAR_1, STDSTRING_MULTI_LINK
	return

BattleFrontier_Lounge2_EventScript_BufferBattleDome::
	bufferstdstring STR_VAR_1, STDSTRING_BATTLE_DOME
	return

BattleFrontier_Lounge2_EventScript_BufferBattleFactory::
	bufferstdstring STR_VAR_1, STDSTRING_BATTLE_FACTORY
	return

BattleFrontier_Lounge2_EventScript_BufferBattlePalace::
	bufferstdstring STR_VAR_1, STDSTRING_BATTLE_PALACE
	return

BattleFrontier_Lounge2_EventScript_BufferBattleArena::
	bufferstdstring STR_VAR_1, STDSTRING_BATTLE_ARENA
	return

BattleFrontier_Lounge2_EventScript_BufferBattlePike::
	bufferstdstring STR_VAR_1, STDSTRING_BATTLE_PIKE
	return

BattleFrontier_Lounge2_EventScript_BufferBattlePyramid::
	bufferstdstring STR_VAR_1, STDSTRING_BATTLE_PYRAMID
	return

BattleFrontier_Lounge2_EventScript_Maniac1::
	lock
	msgbox BattleFrontier_Lounge2_Text_NewsGatheringPower, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Lounge2_EventScript_Maniac2::
	lock
	msgbox BattleFrontier_Lounge2_Text_AmazingPowersOfObservation, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Lounge2_EventScript_Maniac3::
	lock
	msgbox BattleFrontier_Lounge2_Text_AmazingPowerOfPersuasion, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Lounge2_EventScript_TriathleteF::
	msgbox BattleFrontier_Lounge2_Text_ThisPlaceIsScaringMe, MSGBOX_NPC
	end

BattleFrontier_Lounge2_Text_FrontierManiacIntro::
	.string "E aí! Em notícias da FRONTEIRA DE\n"
	.string "BATALHA, sou o nº 1.\p"
	.string "Pode me chamar de MANÍACO DA\n"
	.string "FRONTEIRA.\p"
	.string "Só confirmando: você é TREINADOR,\n"
	.string "não é?\p"
	.string "Vou contar com prazer as notícias\n"
	.string "mais quentes da FRONTEIRA.$"

BattleFrontier_Lounge2_Text_SwingByForTheLatestWord::
	.string "E aí! Veio me arrancar as últimas\n"
	.string "novidades? Tá bom, então!$"

/* Unused*/
BattleFrontier_Lounge2_Text_MyInformationsBeenUsefulRight::
	.string "E aí? E aí? E aí?\p"
	.string "As minhas informações foram muito\n"
	.string "úteis, né?$"

BattleFrontier_Lounge2_Text_FacilityIsHottest::
	.string "Vejamos…\p"
	.string "Parece que o lugar do momento é\n"
	.string "{STR_VAR_1}.$"

BattleFrontier_Lounge2_Text_BattleTowerIsHottest::
	.string "Vejamos…\p"
	.string "Parece que o mais quente é a\n"
	.string "TORRE DE BATALHA: {STR_VAR_1}.$"

BattleFrontier_Lounge2_Text_SalonMaidenIsThere::
	.string "Aposto que você não sabia!\p"
	.string "Um dos TREINADORES de elite que o\n"
	.string "SCOTT chama de CÉREBROS DA FRONTEIRA\l"
	.string "está lá.\p"
	.string "Quem comanda o lugar é uma\n"
	.string "TREINADORA misteriosa, a DAMA DO\l"
	.string "SALÃO.$"

BattleFrontier_Lounge2_Text_SalonMaidenSilverMons::
	.string "Já batalhou com a DAMA DO SALÃO?\p"
	.string "Quando ela está medindo o oponente,\n"
	.string "parece que usa estes POKéMON:\p"
	.string "um POKéMON PSI de PSÍQUICO,\n"
	.string "um POKéMON VULCÃO de FOGO\l"
	.string "e um POKéMON DORMINHOCO de NORMAL.$"

BattleFrontier_Lounge2_Text_SalonMaidenGoldMons::
	.string "Já batalhou com a DAMA DO SALÃO\n"
	.string "falando sério?\p"
	.string "Quando ela batalha com tudo,\n"
	.string "parece que usa estes POKéMON:\p"
	.string "um POKéMON EON de DRAGÃO e PSÍQUICO,\n"
	.string "um POKéMON TROVÃO de ELÉTRICO\l"
	.string "e um POKéMON DORMINHOCO de NORMAL.$"

BattleFrontier_Lounge2_Text_DomeAceIsThere::
	.string "Aposto que você não sabia!\p"
	.string "Um dos TREINADORES de elite que o\n"
	.string "SCOTT chama de CÉREBROS DA FRONTEIRA\l"
	.string "está lá.\p"
	.string "Quem comanda o lugar é um\n"
	.string "TREINADOR exibido, o ÁS DA CÚPULA.$"

BattleFrontier_Lounge2_Text_DomeAceSilverMons::
	.string "Já batalhou com o ÁS DA CÚPULA?\p"
	.string "Quando pega leve com o oponente,\n"
	.string "ele usa estes três POKéMON:\p"
	.string "um POKéMON DRAGÃO de DRAGÃO e\l"
	.string "VOADOR, um POKéMON PEIXE-LAMA de\l"
	.string "ÁGUA e TERRA e um POKéMON CHAMA\l"
	.string "de FOGO e VOADOR.$"

BattleFrontier_Lounge2_Text_DomeAceGoldMons::
	.string "Já batalhou com o ÁS DA CÚPULA\n"
	.string "falando sério?\p"
	.string "Quando mostra a sua estratégia,\n"
	.string "ele usa estes três POKéMON:\p"
	.string "um POKéMON EON de DRAGÃO e VOADOR,\n"
	.string "um POKéMON PEIXE-LAMA de ÁGUA e\l"
	.string "TERRA e um POKéMON PERNA FERRO\l"
	.string "de AÇO e PSÍQUICO.$"

BattleFrontier_Lounge2_Text_FactoryHeadIsThere::
	.string "Aposto que você não sabia!\p"
	.string "Um dos TREINADORES de elite que o\n"
	.string "SCOTT chama de CÉREBROS DA FRONTEIRA\l"
	.string "está lá.\p"
	.string "Quem comanda o lugar é um\n"
	.string "TREINADOR esquisito, o CHEFE DA\l"
	.string "FÁBRICA.$"

BattleFrontier_Lounge2_Text_FactoryHeadSilverMons::
	.string "Já batalhou com o CHEFE DA FÁBRICA?\p"
	.string "Deixa eu pensar… Quando vai batalhar,\n"
	.string "ele só pega três POKéMON de aluguel.\p"
	.string "Ele batalha praticamente nas mesmas\n"
	.string "condições que você.$"

BattleFrontier_Lounge2_Text_FactoryHeadGoldMons::
	.string "Já batalhou com o CHEFE DA FÁBRICA\n"
	.string "falando sério?\p"
	.string "Mesmo quando batalha a sério, ele\n"
	.string "ainda pega três POKéMON de aluguel.\p"
	.string "Ele batalha praticamente nas mesmas\n"
	.string "condições que você.$"

BattleFrontier_Lounge2_Text_PikeQueenIsThere::
	.string "Aposto que você não sabia!\p"
	.string "Um dos TREINADORES de elite que o\n"
	.string "SCOTT chama de CÉREBROS DA FRONTEIRA\l"
	.string "está lá.\p"
	.string "Quem comanda o lugar é uma\n"
	.string "TREINADORA assustadora, a RAINHA\l"
	.string "DO PICO.$"

BattleFrontier_Lounge2_Text_PikeQueenSilverMons::
	.string "Já batalhou com a RAINHA DO PICO?\p"
	.string "Quando está de bom humor, dizem que\n"
	.string "ela usa estes três POKéMON:\p"
	.string "um POKéMON COBRA PRESA de VENENO,\n"
	.string "um POKéMON FUNGO de INSETO e PEDRA\l"
	.string "e um POKéMON TERNO de ÁGUA.$"

BattleFrontier_Lounge2_Text_PikeQueenGoldMons::
	.string "Já batalhou com a RAINHA DO PICO\n"
	.string "falando sério?\p"
	.string "Quando está irritada de verdade,\n"
	.string "dizem que ela usa estes POKéMON:\p"
	.string "um POKéMON COBRA PRESA de VENENO,\n"
	.string "um POKéMON COBRA FERRO de AÇO e\l"
	.string "TERRA e um POKéMON ATROZ de ÁGUA\l"
	.string "e VOADOR.$"

BattleFrontier_Lounge2_Text_ArenaTycoonIsThere::
	.string "Aposto que você não sabia!\p"
	.string "Um dos TREINADORES de elite que o\n"
	.string "SCOTT chama de CÉREBROS DA FRONTEIRA\l"
	.string "está lá.\p"
	.string "Quem comanda o lugar é uma\n"
	.string "TREINADORA fofa, a MAGNATA DA ARENA.$"

BattleFrontier_Lounge2_Text_ArenaTycoonSilverMons::
	.string "Já batalhou com a MAGNATA DA ARENA?\p"
	.string "Quando avalia o oponente, parece\n"
	.string "que ela usa estes POKéMON:\p"
	.string "um POKéMON MONOCHIFRE de INSETO e\l"
	.string "LUTADOR, um POKéMON LUAR de\l"
	.string "SOMBRIO e um POKéMON CASCA VAZIA\l"
	.string "de INSETO e FANTASMA.$"

BattleFrontier_Lounge2_Text_ArenaTycoonGoldMons::
	.string "Já batalhou com a MAGNATA DA ARENA\n"
	.string "falando sério?\p"
	.string "Quando batalha pra valer, parece que\n"
	.string "ela usa estes POKéMON:\p"
	.string "um POKéMON LUAR de SOMBRIO, um\n"
	.string "POKéMON SOMBRA de FANTASMA e VENENO\l"
	.string "e um POKéMON COGUMELO de PLANTA\l"
	.string "e LUTADOR.$"

BattleFrontier_Lounge2_Text_PalaceMavenIsThere::
	.string "Aposto que você não sabia!\p"
	.string "Um dos TREINADORES de elite que o\n"
	.string "SCOTT chama de CÉREBROS DA FRONTEIRA\l"
	.string "está lá.\p"
	.string "Quem comanda o lugar é um\n"
	.string "TREINADOR sinistro, o SÁBIO DO\l"
	.string "PALÁCIO.$"

BattleFrontier_Lounge2_Text_PalaceMavenSilverMons::
	.string "Já batalhou com o SÁBIO DO PALÁCIO?\p"
	.string "Quando testa o espírito do oponente,\n"
	.string "dizem que ele usa estes POKéMON:\p"
	.string "um POKéMON MORCEGO de VENENO e\l"
	.string "VOADOR, um POKéMON PREGUIÇA de\l"
	.string "NORMAL e um POKéMON TRANSPORTE\l"
	.string "de ÁGUA e GELO.$"

BattleFrontier_Lounge2_Text_PalaceMavenGoldMons::
	.string "Já batalhou com o SÁBIO DO PALÁCIO\n"
	.string "falando sério?\p"
	.string "Quando põe toda a força na batalha,\n"
	.string "dizem que ele usa estes POKéMON:\p"
	.string "um POKéMON LENDÁRIO de FOGO,\n"
	.string "um POKéMON PREGUIÇA de NORMAL\l"
	.string "e um POKéMON AURORA de ÁGUA.$"

BattleFrontier_Lounge2_Text_PyramidKingIsThere::
	.string "Aposto que você não sabia!\p"
	.string "Um dos TREINADORES de elite que o\n"
	.string "SCOTT chama de CÉREBROS DA FRONTEIRA\l"
	.string "está lá.\p"
	.string "Quem comanda o lugar é um\n"
	.string "TREINADOR esquentado, o REI DA\l"
	.string "PIRÂMIDE.$"

BattleFrontier_Lounge2_Text_PyramidKingSilverMons::
	.string "Já batalhou com o REI DA PIRÂMIDE?\p"
	.string "Quando testa a força do oponente,\n"
	.string "parece que ele usa estes POKéMON:\p"
	.string "um POKéMON PICO ROCHA de PEDRA,\n"
	.string "um POKéMON ICEBERG de GELO\l"
	.string "e um POKéMON FERRO de AÇO.$"

BattleFrontier_Lounge2_Text_PyramidKingGoldMons::
	.string "Já batalhou com o REI DA PIRÂMIDE\n"
	.string "falando sério?\p"
	.string "Quando está cheio de força, parece\n"
	.string "que ele usa estes POKéMON:\p"
	.string "um POKéMON CONGELANTE de GELO e\l"
	.string "VOADOR, um POKéMON ELÉTRICO de\l"
	.string "ELÉTRICO e VOADOR e um POKéMON\l"
	.string "CHAMA de FOGO e VOADOR.$"

BattleFrontier_Lounge2_Text_DoubleBattleAdvice1::
	.string "Claro, tem vários lugares com\n"
	.string "BATALHAS DUPLAS.\p"
	.string "Mas as SALAS DE BATALHA DUPLA da\n"
	.string "TORRE DE BATALHA são, tipo, o básico!\p"
	.string "É lá que você aprende como funcionam\n"
	.string "as BATALHAS DUPLAS aqui na\l"
	.string "FRONTEIRA DE BATALHA.$"

BattleFrontier_Lounge2_Text_DoubleBattleAdvice2::
	.string "Cuidado nas batalhas daqui.\p"
	.string "Dizem que tem TREINADORES com\n"
	.string "estratégias feitas só pras\l"
	.string "BATALHAS DUPLAS.$"

BattleFrontier_Lounge2_Text_DoubleBattleAdvice3::
	.string "Quando estiver confiante e à vontade\n"
	.string "nas BATALHAS DUPLAS daqui, pense em\l"
	.string "desafiar outros lugares com\l"
	.string "BATALHAS DUPLAS.$"

BattleFrontier_Lounge2_Text_MultiBattleAdvice::
	.string "Todo tipo de TREINADOR se reúne no\n"
	.string "SALÃO DE BATALHA.\p"
	.string "Pense só: você pode encontrar\n"
	.string "amigos ou seguidores!\l"
	.string "Olhe com atenção!$"

BattleFrontier_Lounge2_Text_LinkMultiBattleAdvice::
	.string "Se estiver com um amigo, vá pra\n"
	.string "SALA DE BATALHA MÚLTIPLA POR LINK.\p"
	.string "Jogando com um amigo forte, espere\n"
	.string "oponentes duros!$"

BattleFrontier_Lounge2_Text_NewsGatheringPower::
	.string "Que poder de apurar notícias!\n"
	.string "O meu mestre é único!$"

BattleFrontier_Lounge2_Text_AmazingPowersOfObservation::
	.string "Que poder de observação!\n"
	.string "O meu mestre é único!$"

BattleFrontier_Lounge2_Text_AmazingPowerOfPersuasion::
	.string "Que poder de persuasão!\n"
	.string "O meu mestre é único!$"

BattleFrontier_Lounge2_Text_ThisPlaceIsScaringMe::
	.string "…Que lugar é este?\n"
	.string "Está me dando medo…$"

