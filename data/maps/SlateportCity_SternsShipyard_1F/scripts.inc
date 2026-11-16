SlateportCity_SternsShipyard_1F_MapScripts::
	.byte 0

SlateportCity_SternsShipyard_1F_EventScript_Dock::
	lockall
	goto_if_set FLAG_SYS_GAME_CLEAR, SlateportCity_SternsShipyard_1F_EventScript_FerryReady
	goto_if_set FLAG_BADGE07_GET, SlateportCity_SternsShipyard_1F_EventScript_BrineyJoined
	goto_if_set FLAG_DELIVERED_DEVON_GOODS, SlateportCity_SternsShipyard_1F_EventScript_NeedVeteran
	goto_if_set FLAG_DOCK_REJECTED_DEVON_GOODS, SlateportCity_SternsShipyard_1F_EventScript_GoFindStern
	msgbox SlateportCity_SternsShipyard_1F_Text_CantMakeHeadsOrTails, MSGBOX_DEFAULT
	applymovement LOCALID_DOCK, Common_Movement_FacePlayer
	waitmovement 0
	msgbox SlateportCity_SternsShipyard_1F_Text_MeetDockDeliverToStern, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_DOCK, Common_Movement_FaceOriginalDirection
	waitmovement 0
	setflag FLAG_DOCK_REJECTED_DEVON_GOODS
	setflag FLAG_HIDE_SLATEPORT_CITY_TEAM_AQUA
	releaseall
	end

SlateportCity_SternsShipyard_1F_EventScript_FerryReady::
	applymovement LOCALID_DOCK, Common_Movement_FacePlayer
	waitmovement 0
	msgbox SlateportCity_SternsShipyard_1F_Text_FerryIsReady, MSGBOX_DEFAULT
	releaseall
	end

SlateportCity_SternsShipyard_1F_EventScript_BrineyJoined::
	applymovement LOCALID_DOCK, Common_Movement_FacePlayer
	waitmovement 0
	msgbox SlateportCity_SternsShipyard_1F_Text_BrineyJoinedUs, MSGBOX_DEFAULT
	releaseall
	end

SlateportCity_SternsShipyard_1F_EventScript_GoFindStern::
	applymovement LOCALID_DOCK, Common_Movement_FacePlayer
	waitmovement 0
	msgbox SlateportCity_SternsShipyard_1F_Text_CouldYouFindStern, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_DOCK, Common_Movement_FaceOriginalDirection
	waitmovement 0
	releaseall
	end

SlateportCity_SternsShipyard_1F_EventScript_NeedVeteran::
	applymovement LOCALID_DOCK, Common_Movement_FacePlayer
	waitmovement 0
	msgbox SlateportCity_SternsShipyard_1F_Text_CouldUseAdviceFromVeteran, MSGBOX_DEFAULT
	releaseall
	end

SlateportCity_SternsShipyard_1F_EventScript_Scientist1::
	msgbox SlateportCity_SternsShipyard_1F_Text_SeaIsLikeLivingThing, MSGBOX_NPC
	end

SlateportCity_SternsShipyard_1F_EventScript_Scientist2::
	msgbox SlateportCity_SternsShipyard_1F_Text_GetSeasickEasily, MSGBOX_NPC
	end

SlateportCity_SternsShipyard_1F_EventScript_Briney::
	msgbox SlateportCity_SternsShipyard_1F_Text_DecidedToHelpDock, MSGBOX_NPC
	end

SlateportCity_SternsShipyard_1F_Text_CantMakeHeadsOrTails:
	.string "Hum… Se isto vai aqui, e aquilo\n"
	.string "vai ali…\p"
	.string "Então onde vai esta coisa?\n"
	.string "E aquele troço ali?\p"
	.string "Aaargh! Não estou entendendo\n"
	.string "nada disto!$"

SlateportCity_SternsShipyard_1F_Text_MeetDockDeliverToStern:
	.string "Hm?\n"
	.string "Oi, eu sou o DOCK.\p"
	.string "O CAP. STERN me encomendou o\n"
	.string "projeto de uma balsa.\p"
	.string "Ah! Isso aí…\n"
	.string "São as PEÇAS DEVON?\p"
	.string "Mas, hmm…\n"
	.string "Assim não dá…\p"
	.string "O CAP. STERN saiu pra algum lugar.\n"
	.string "Disse que tinha um trabalho a fazer.\p"
	.string "Você pode ir achar o CAP. STERN\n"
	.string "e entregar isso a ele?$"

SlateportCity_SternsShipyard_1F_Text_CouldYouFindStern:
	.string "DOCK: Aonde será que o CAP. STERN\n"
	.string "foi?\p"
	.string "Você pode achar o CAP. STERN e\n"
	.string "entregar esse pacote a ele?$"

SlateportCity_SternsShipyard_1F_Text_CouldUseAdviceFromVeteran:
	.string "DOCK: Construir navios é uma arte.\p"
	.string "Muita coisa não se resolve só\n"
	.string "fazendo contas.\p"
	.string "Eu precisava muito do conselho de um\n"
	.string "veterano que conheça o mar…$"

SlateportCity_SternsShipyard_1F_Text_BrineyJoinedUs:
	.string "DOCK: Oi! O SR. BRINEY veio nos\n"
	.string "ajudar.\p"
	.string "Graças ao velho marinheiro, a balsa\n"
	.string "está ficando pronta aos poucos.$"

SlateportCity_SternsShipyard_1F_Text_FerryIsReady:
	.string "DOCK: A balsa finalmente está pronta!\p"
	.string "A nova S.S. TIDAL é mesmo uma\n"
	.string "maravilha da tecnologia!\p"
	.string "Mas tenho a sensação de que dá pra\n"
	.string "fazer algo ainda melhor.\p"
	.string "Sabe, a tecnologia nunca para\n"
	.string "de avançar.$"

SlateportCity_SternsShipyard_1F_Text_DecidedToHelpDock:
	.string "SR. BRINEY: Ah, {PLAYER}{KUN}!\n"
	.string "Quanto tempo!\p"
	.string "É, desde que te conheci, este velho lobo\n"
	.string "do mar anda cheio de energia!\p"
	.string "Então resolvi ajudar o DOCK a fazer\n"
	.string "uma balsa.\p"
	.string "Afinal, uma balsa leva muita\n"
	.string "gente.\p"
	.string "Mas, sabe, esse DOCK é mesmo\n"
	.string "especial.\p"
	.string "Com o talento dele pra tecnologia e\n"
	.string "a minha experiência, com certeza a\l"
	.string "gente constrói um baita navio!$"

SlateportCity_SternsShipyard_1F_Text_SeaIsLikeLivingThing:
	.string "As estações, o tempo, onde a lua\n"
	.string "fica no céu…\p"
	.string "Essas e outras coisas fazem\n"
	.string "o mar mudar de cara.\p"
	.string "É isso aí!\n"
	.string "O mar é como um ser vivo!$"

SlateportCity_SternsShipyard_1F_Text_GetSeasickEasily:
	.string "Eu enjoo no mar muito fácil.\n"
	.string "Então eu ajudo aqui, em vez disso.$"

