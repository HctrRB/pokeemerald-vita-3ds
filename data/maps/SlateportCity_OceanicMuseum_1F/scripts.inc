SlateportCity_OceanicMuseum_1F_MapScripts::
	.byte 0

SlateportCity_OceanicMuseum_1F_EventScript_EntranceAttendant::
	msgbox SlateportCity_OceanicMuseum_1F_Text_PleaseEnjoyYourself, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFeeLeft::
	lockall
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFee
	end

SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFeeRight::
	lockall
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFee
	end

SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFee::
	showmoneybox 0, 0
	msgbox SlateportCity_OceanicMuseum_1F_Text_WouldYouLikeToEnter, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, SlateportCity_OceanicMuseum_1F_EventScript_CheckMoneyForFee
	closemessage
	hidemoneybox
	applymovement LOCALID_PLAYER, SlateportCity_OceanicMuseum_1F_Movement_PushPlayerBackFromCounter
	waitmovement 0
	releaseall
	end

SlateportCity_OceanicMuseum_1F_EventScript_CheckMoneyForFee::
	checkmoney 50
	goto_if_eq VAR_RESULT, FALSE, SlateportCity_OceanicMuseum_1F_EventScript_NotEnoughMoney
	playse SE_SHOP
	removemoney 50
	updatemoneybox
	msgbox SlateportCity_OceanicMuseum_1F_Text_PleaseEnjoyYourself, MSGBOX_DEFAULT
	setvar VAR_SLATEPORT_MUSEUM_1F_STATE, 1
	hidemoneybox
	releaseall
	end

SlateportCity_OceanicMuseum_1F_EventScript_NotEnoughMoney::
	goto_if_unset FLAG_DELIVERED_DEVON_GOODS, SlateportCity_OceanicMuseum_1F_EventScript_AllowEntranceAnyway
	msgbox SlateportCity_OceanicMuseum_1F_Text_NotEnoughMoney, MSGBOX_DEFAULT
	closemessage
	hidemoneybox
	applymovement LOCALID_PLAYER, SlateportCity_OceanicMuseum_1F_Movement_PushPlayerBackFromCounter
	waitmovement 0
	releaseall
	end

SlateportCity_OceanicMuseum_1F_EventScript_AllowEntranceAnyway::
	msgbox SlateportCity_OceanicMuseum_1F_Text_CatchUpWithYourGroup, MSGBOX_DEFAULT
	setvar VAR_SLATEPORT_MUSEUM_1F_STATE, 1
	hidemoneybox
	releaseall
	end

SlateportCity_OceanicMuseum_1F_Movement_PushPlayerBackFromCounter:
	walk_down
	step_end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt1::
	msgbox SlateportCity_OceanicMuseum_1F_Text_AquaExistForGoodOfAll, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt2::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OurBossIsntHere, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt3::
	msgbox SlateportCity_OceanicMuseum_1F_Text_WouldStuffHereMakeMeRich, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt4::
	msgbox SlateportCity_OceanicMuseum_1F_Text_CanLearnForNefariousDeeds, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt5::
	msgbox SlateportCity_OceanicMuseum_1F_Text_RustboroBungled, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt6::
	msgbox SlateportCity_OceanicMuseum_1F_Text_DidntHaveMoney, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_WhirlpoolExperiment::
	msgbox SlateportCity_OceanicMuseum_1F_Text_WhirlpoolExperiment, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_WaterfallExperiment::
	msgbox SlateportCity_OceanicMuseum_1F_Text_WaterfallExperiment, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_OceanSoilDisplay::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OceanSoilDisplay, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_BeachSandDisplay::
	msgbox SlateportCity_OceanicMuseum_1F_Text_BeachSandDisplay, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_OceanicMinifact1::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact1, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_OceanicMinifact2::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact2, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_OceanicMinifact3::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact3, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_FossilDisplay::
	msgbox SlateportCity_OceanicMuseum_1F_Text_FossilDisplay, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_DepthMeasuringMachine::
	msgbox SlateportCity_OceanicMuseum_1F_Text_DepthMeasuringMachine, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumPatron1::
	msgbox SlateportCity_OceanicMuseum_1F_Text_LearnAboutSeaForBattling, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumPatron2::
	msgbox SlateportCity_OceanicMuseum_1F_Text_SternIsRoleModel, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumPatron3::
	msgbox SlateportCity_OceanicMuseum_1F_Text_MustBePokemonWeDontKnow, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumPatron4::
	msgbox SlateportCity_OceanicMuseum_1F_Text_WantSeaPokemon, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGrunt::
	lock
	faceplayer
	delay 8
	playse SE_PIN
	applymovement LOCALID_OCEANIC_MUSEUM_FAMILIAR_GRUNT, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_OCEANIC_MUSEUM_FAMILIAR_GRUNT, Common_Movement_Delay48
	waitmovement 0
	msgbox SlateportCity_OceanicMuseum_1F_Text_RememberMeTakeThis, MSGBOX_DEFAULT
	giveitem ITEM_TM_THIEF
	goto_if_eq VAR_RESULT, 0, SlateportCity_OceanicMuseum_1F_EventScript_NoRoomForThief
	setflag FLAG_RECEIVED_TM_THIEF
	msgbox SlateportCity_OceanicMuseum_1F_Text_HopeINeverSeeYouAgain, MSGBOX_DEFAULT
	closemessage
	goto_if_eq VAR_FACING, DIR_NORTH, SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitNorth
	goto_if_eq VAR_FACING, DIR_SOUTH, SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitSouth
	goto_if_eq VAR_FACING, DIR_WEST, SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitWestEast
	goto_if_eq VAR_FACING, DIR_EAST, SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitWestEast
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitNorth::
	applymovement LOCALID_PLAYER, SlateportCity_OceanicMuseum_1F_Movement_PlayerWatchGruntExitNorth
	applymovement LOCALID_OCEANIC_MUSEUM_FAMILIAR_GRUNT, SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExitNorth
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExited
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitSouth::
	applymovement LOCALID_OCEANIC_MUSEUM_FAMILIAR_GRUNT, SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExit
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExited
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitWestEast::
	applymovement LOCALID_PLAYER, SlateportCity_OceanicMuseum_1F_Movement_PlayerWatchGruntExitWestEast
	applymovement LOCALID_OCEANIC_MUSEUM_FAMILIAR_GRUNT, SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExit
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExited
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExited::
	setflag FLAG_HIDE_SLATEPORT_CITY_OCEANIC_MUSEUM_FAMILIAR_AQUA_GRUNT
	playse SE_EXIT
	removeobject LOCALID_OCEANIC_MUSEUM_FAMILIAR_GRUNT
	release
	end

SlateportCity_OceanicMuseum_1F_EventScript_NoRoomForThief::
	msgbox SlateportCity_OceanicMuseum_1F_Text_YouHaveToTakeThis, MSGBOX_DEFAULT
	release
	end

SlateportCity_OceanicMuseum_1F_Movement_PlayerWatchGruntExitNorth:
	delay_16
	delay_8
	delay_4
	walk_in_place_faster_down
	step_end

SlateportCity_OceanicMuseum_1F_Movement_PlayerWatchGruntExitWestEast:
	delay_16
	walk_in_place_faster_down
	step_end

SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExit:
	face_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	delay_8
	step_end

SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExitNorth:
	walk_fast_right
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	delay_8
	step_end

SlateportCity_OceanicMuseum_1F_Text_WouldYouLikeToEnter:
	.string "Boas-vindas ao MUSEU OCEÂNICO.\p"
	.string "A entrada custa ¥50.\n"
	.string "Quer entrar?$"

SlateportCity_OceanicMuseum_1F_Text_PleaseEnjoyYourself:
	.string "Aproveite a visita.$"

SlateportCity_OceanicMuseum_1F_Text_NotEnoughMoney:
	.string "Hã? Desculpe, mas parece que você não\n"
	.string "tem dinheiro suficiente.$"

SlateportCity_OceanicMuseum_1F_Text_CatchUpWithYourGroup:
	.string "Hã? Você é daquele grupo que\n"
	.string "entrou agora há pouco?\p"
	.string "Só falta você.\n"
	.string "É melhor ir alcançar o pessoal!$"

SlateportCity_OceanicMuseum_1F_Text_AquaExistForGoodOfAll:
	.string "Nós, da EQUIPE AQUA, existimos pelo\n"
	.string "bem de todos!$"

SlateportCity_OceanicMuseum_1F_Text_OurBossIsntHere:
	.string "Mandaram a gente se reunir aqui,\n"
	.string "e a gente veio, mas…\p"
	.string "O nosso CHEFE, a peça principal,\n"
	.string "não está aqui.$"

SlateportCity_OceanicMuseum_1F_Text_WouldStuffHereMakeMeRich:
	.string "Se eu roubasse as coisas daqui,\n"
	.string "ficaria rico?$"

SlateportCity_OceanicMuseum_1F_Text_CanLearnForNefariousDeeds:
	.string "O que eu aprender aqui, posso usar\n"
	.string "nas minhas maldades…$"

SlateportCity_OceanicMuseum_1F_Text_RustboroBungled:
	.string "Se os nossos capangas não tivessem\n"
	.string "feito besteira em RUSTBORO, a gente\l"
	.string "nem estaria aqui!$"

SlateportCity_OceanicMuseum_1F_Text_DidntHaveMoney:
	.string "Eu não tinha ¥50, então demorei\n"
	.string "pra passar pela recepcionista.$"

SlateportCity_OceanicMuseum_1F_Text_LearnAboutSeaForBattling:
	.string "Quero aprender sobre o mar e usar\n"
	.string "isso nas batalhas.$"

SlateportCity_OceanicMuseum_1F_Text_SternIsRoleModel:
	.string "Eu fico toda boba quando\n"
	.string "vejo o mar!\p"
	.string "Pra mim, o CAP. STERN é o maior\n"
	.string "exemplo de todos!$"

SlateportCity_OceanicMuseum_1F_Text_MustBePokemonWeDontKnow:
	.string "O mar é imenso e sem fim, e\n"
	.string "infinitamente profundo…\p"
	.string "Deve haver muitos POKéMON que\n"
	.string "a gente nem conhece.$"

SlateportCity_OceanicMuseum_1F_Text_WantSeaPokemon:
	.string "Quero um POKéMON do mar.\p"
	.string "Acho que deve ser fresquinho e\n"
	.string "gostoso de abraçar.$"

SlateportCity_OceanicMuseum_1F_Text_RememberMeTakeThis:
	.string "Aaaaai!\n"
	.string "O que você está fazendo aqui?\p"
	.string "Eu? Sou o membro da EQUIPE AQUA que\n"
	.string "você derrotou, lembra?\l"
	.string "Lá no TÚNEL RUSTURF?\p"
	.string "Tome, pega isto!\n"
	.string "Você tem que me perdoar!$"

SlateportCity_OceanicMuseum_1F_Text_HopeINeverSeeYouAgain:
	.string "Essa MT combina mais com você\n"
	.string "do que comigo.\p"
	.string "Espero nunca mais te ver!\n"
	.string "Uahahaha!$"

SlateportCity_OceanicMuseum_1F_Text_YouHaveToTakeThis:
	.string "Ah, qual é!\n"
	.string "Você tem que pegar isto e me deixar ir!$"

SlateportCity_OceanicMuseum_1F_Text_WhirlpoolExperiment:
	.string "Um líquido azul gira dentro de\n"
	.string "um recipiente de vidro.\p"
	.string "“Experimento para criar um\n"
	.string "REDEMOINHO artificial com o vento.”$"

SlateportCity_OceanicMuseum_1F_Text_WaterfallExperiment:
	.string "Uma bola vermelha sobe e desce\n"
	.string "dentro de um recipiente de vidro.\p"
	.string "“Experimento que simula uma CACHOEIRA\n"
	.string "com a flutuação da bola.”$"

SlateportCity_OceanicMuseum_1F_Text_OceanSoilDisplay:
	.string "É uma amostra de solo do fundo\n"
	.string "do mar.\p"
	.string "“Ao longo de muitos anos, os restos\n"
	.string "de seres vivos se depositam no fundo\l"
	.string "do mar, formando camadas.\p"
	.string "“Analisando essas camadas, o passado\n"
	.string "remoto é revelado.”$"

SlateportCity_OceanicMuseum_1F_Text_BeachSandDisplay:
	.string "É uma amostra de areia da praia.\p"
	.string "“As pedras das montanhas são levadas\n"
	.string "pelos rios, onde se lascam e se\l"
	.string "desgastam.\p"
	.string "“Elas viram grãos e acabam como\n"
	.string "areia nas praias.”$"

SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact1:
	.string "“CURIOSIDADE OCEÂNICA 1\n"
	.string "Por que a água do mar é azul?\p"
	.string "“A luz é formada por muitas cores.\p"
	.string "“Quando a luz atravessa a água, a\n"
	.string "maioria das cores se perde.\p"
	.string "“Mas a luz azul mantém a cor,\n"
	.string "e o mar parece azul.”$"

SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact2:
	.string "“CURIOSIDADE OCEÂNICA 2\n"
	.string "Por que o mar é salgado?\p"
	.string "“A água do mar tem sal dissolvido,\n"
	.string "em forma de íons de sódio e cloro.\p"
	.string "“Esses íons saem das rochas e\n"
	.string "são levados ao mar pela chuva.\p"
	.string "“A concentração de sal dissolvido\n"
	.string "deixa o mar salgado.”$"

SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact3:
	.string "“CURIOSIDADE OCEÂNICA 3\n"
	.string "Qual é maior? O mar ou a terra?\p"
	.string "“O mar cobre cerca de 70% do\n"
	.string "planeta, e o resto é terra.\p"
	.string "“Então o mar tem mais que o dobro\n"
	.string "do tamanho da terra.”$"

SlateportCity_OceanicMuseum_1F_Text_FossilDisplay:
	.string "É um fóssil com ondulações.\p"
	.string "“O solo do fundo do mar é varrido\n"
	.string "pela maré.\p"
	.string "“A água em movimento marca o solo\n"
	.string "com pequenas cristas e vales.\p"
	.string "“Se esse solo vira fóssil, chama-se\n"
	.string "marca de ondulação.”$"

SlateportCity_OceanicMuseum_1F_Text_DepthMeasuringMachine:
	.string "Uma máquina estranha gira dentro\n"
	.string "de uma cúpula de vidro.\p"
	.string "Talvez seja pra medir a profundidade\n"
	.string "de alguma coisa…$"

