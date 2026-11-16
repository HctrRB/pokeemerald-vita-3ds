MtChimney_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, MtChimney_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, MtChimney_OnTransition
	.byte 0

MtChimney_OnTransition:
	setvar VAR_JAGGED_PASS_ASH_WEATHER, 1
	end

MtChimney_OnResume:
	setstepcallback STEP_CB_ASH
	end

MtChimney_EventScript_Archie::
	lock
	faceplayer
	call_if_unset FLAG_EVIL_LEADER_PLEASE_STOP, MtChimney_EventScript_ArchieGoStopTeamMagma
	call_if_set FLAG_EVIL_LEADER_PLEASE_STOP, MtChimney_EventScript_ArchieBusyFighting
	closemessage
	applymovement LOCALID_MT_CHIMNEY_ARCHIE, Common_Movement_FaceOriginalDirection
	waitmovement 0
	setflag FLAG_EVIL_LEADER_PLEASE_STOP
	release
	end

MtChimney_EventScript_ArchieGoStopTeamMagma::
	msgbox MtChimney_Text_ArchieGoStopTeamMagma, MSGBOX_DEFAULT
	return

MtChimney_EventScript_ArchieBusyFighting::
	msgbox MtChimney_Text_ArchieIHaveMyHandsFull, MSGBOX_DEFAULT
	return

MtChimney_EventScript_Maxie::
	lockall
	playbgm MUS_ENCOUNTER_MAGMA, FALSE
	msgbox MtChimney_Text_MeteoriteWillActivateVolcano, MSGBOX_DEFAULT
	applymovement LOCALID_MT_CHIMNEY_MAXIE, Common_Movement_FacePlayer
	waitmovement 0
	playse SE_PIN
	applymovement LOCALID_MT_CHIMNEY_MAXIE, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_MT_CHIMNEY_MAXIE, Common_Movement_Delay48
	waitmovement 0
	msgbox MtChimney_Text_MaxieIntro, MSGBOX_DEFAULT
	trainerbattle_no_intro TRAINER_MAXIE_MT_CHIMNEY, MtChimney_Text_MaxieDefeat
	msgbox MtChimney_Text_MaxieYouHaventSeenLastOfMagma, MSGBOX_DEFAULT
	closemessage
	delay 30
	fadescreen FADE_TO_BLACK
	removeobject LOCALID_MT_CHIMNEY_MAXIE
	removeobject LOCALID_MT_CHIMNEY_MAGMA_GRUNT_1
	removeobject LOCALID_MT_CHIMNEY_TABITHA
	removeobject LOCALID_MT_CHIMNEY_MAGMA_GRUNT_2
	setflag FLAG_HIDE_MT_CHIMNEY_TEAM_MAGMA
	fadescreen FADE_FROM_BLACK
	setobjectxyperm LOCALID_MT_CHIMNEY_ARCHIE, 10, 12
	addobject LOCALID_MT_CHIMNEY_ARCHIE
	call_if_eq VAR_FACING, DIR_EAST, MtChimney_EventScript_ArchieApproachPlayerEast
	call_if_eq VAR_FACING, DIR_NORTH, MtChimney_EventScript_ArchieApproachPlayerNorth
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	msgbox MtChimney_Text_ArchieThankYou, MSGBOX_DEFAULT
	closemessage
	call_if_eq VAR_FACING, DIR_EAST, MtChimney_EventScript_ArchieExitEast
	call_if_eq VAR_FACING, DIR_NORTH, MtChimney_EventScript_ArchieExitNorth
	removeobject LOCALID_MT_CHIMNEY_ARCHIE
	setflag FLAG_HIDE_MT_CHIMNEY_TEAM_AQUA
	setflag FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY
	clearflag FLAG_HIDE_FALLARBOR_HOUSE_PROF_COZMO
	setflag FLAG_HIDE_METEOR_FALLS_1F_1R_COZMO
	clearflag FLAG_HIDE_MT_CHIMNEY_LAVA_COOKIE_LADY
	releaseall
	end

MtChimney_EventScript_ArchieApproachPlayerEast::
	applymovement LOCALID_MT_CHIMNEY_ARCHIE, MtChimney_Movement_ArchieApproachPlayerEast
	waitmovement 0
	return

MtChimney_EventScript_ArchieApproachPlayerNorth::
	applymovement LOCALID_MT_CHIMNEY_ARCHIE, MtChimney_Movement_ArchieApproachPlayerNorth
	waitmovement 0
	return

MtChimney_EventScript_ArchieExitEast::
	applymovement LOCALID_MT_CHIMNEY_ARCHIE, MtChimney_Movement_ArchieExitEast
	waitmovement 0
	return

MtChimney_EventScript_ArchieExitNorth::
	applymovement LOCALID_MT_CHIMNEY_ARCHIE, MtChimney_Movement_ArchieExitNorth
	waitmovement 0
	return

MtChimney_EventScript_LavaCookieLady::
	lock
	faceplayer
	showmoneybox 0, 0
	msgbox MtChimney_Text_LavaCookiesJust200, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MtChimney_EventScript_DeclineLavaCookie
	checkmoney 200
	goto_if_eq VAR_RESULT, FALSE, MtChimney_EventScript_NotEnoughMoney
	msgbox MtChimney_Text_ThankYouDear, MSGBOX_DEFAULT
	checkitemspace ITEM_LAVA_COOKIE
	call_if_eq VAR_RESULT, TRUE, MtChimney_EventScript_RemoveMoney
	giveitem ITEM_LAVA_COOKIE
	goto_if_eq VAR_RESULT, FALSE, MtChimney_EventScript_BagIsFull
	hidemoneybox
	release
	end

MtChimney_EventScript_BagIsFull::
	msgbox gText_TooBadBagIsFull, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

MtChimney_EventScript_RemoveMoney::
	removemoney 200
	updatemoneybox
	return

MtChimney_EventScript_DeclineLavaCookie::
	msgbox MtChimney_Text_OhFineThen, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

MtChimney_EventScript_NotEnoughMoney::
	msgbox MtChimney_Text_YouveNotGotTheMoney, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

MtChimney_Movement_ArchieApproachPlayerEast:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_right
	step_end

MtChimney_Movement_ArchieExitEast:
	walk_left
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

MtChimney_Movement_ArchieApproachPlayerNorth:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_right
	walk_right
	step_end

MtChimney_Movement_ArchieExitNorth:
	walk_left
	walk_left
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

MtChimney_Movement_Unused1:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_down
	step_end

MtChimney_Movement_Unused2:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_down
	step_end

MtChimney_Movement_Unused3:
	walk_right
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

MtChimney_Movement_Unused4:
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_left
	walk_left
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_down
	step_end

MtChimney_Movement_Unused5:
	delay_16
	delay_16
	delay_16
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_down
	step_end

MtChimney_Movement_Unused6:
	delay_16
	walk_left
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

MtChimney_Movement_Unused7:
	delay_16
	walk_left
	walk_left
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_down
	step_end

MtChimney_Movement_Unused8:
	delay_16
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_in_place_faster_down
	step_end

MtChimney_Movement_Unused9:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

MtChimney_Movement_Unused10:
	walk_down
	step_end

MtChimney_Movement_Unused11:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

MtChimney_Movement_Unused12:
	delay_16
	delay_16
	walk_in_place_faster_left
	delay_16
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

MtChimney_Movement_Unused13:
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	walk_in_place_faster_left
	delay_16
	walk_down
	walk_down
	step_end

MtChimney_Movement_Unused14:
	lock_facing_direction
	walk_fast_left
	unlock_facing_direction
	face_right
	step_end

MtChimney_Movement_Unused15:
	walk_left
	walk_in_place_faster_right
	delay_16
	step_end

MtChimney_EventScript_BusyAquaGrunt1::
	msgbox MtChimney_Text_MagmaOutnumbersUs, MSGBOX_SIGN
	end

MtChimney_EventScript_BusyAquaGrunt2::
	msgbox MtChimney_Text_LessHabitatForWaterPokemon, MSGBOX_SIGN
	end

MtChimney_EventScript_BusyAquaGrunt3::
	msgbox MtChimney_Text_MagmasNameSimilar, MSGBOX_SIGN
	end

MtChimney_EventScript_Tabitha::
	trainerbattle_single TRAINER_TABITHA_MT_CHIMNEY, MtChimney_Text_TabithaIntro, MtChimney_Text_TabithaDefeat
	msgbox MtChimney_Text_TabithaPostBattle, MSGBOX_AUTOCLOSE
	end

MtChimney_EventScript_Grunt2::
	trainerbattle_single TRAINER_GRUNT_MT_CHIMNEY_2, MtChimney_Text_Grunt2Intro, MtChimney_Text_Grunt2Defeat
	msgbox MtChimney_Text_Grunt2PostBattle, MSGBOX_AUTOCLOSE
	end

MtChimney_EventScript_BusyMagmaGrunt1::
	msgbox MtChimney_Text_TeamAquaAlwaysMessingWithPlans, MSGBOX_SIGN
	end

MtChimney_EventScript_BusyMagmaGrunt2::
	msgbox MtChimney_Text_MeteoritesPackAmazingPower, MSGBOX_SIGN
	end

MtChimney_EventScript_BusyMagmaGrunt3::
	msgbox MtChimney_Text_YouBetterNotMessWithUs, MSGBOX_SIGN
	end

MtChimney_EventScript_BusyMagmaGrunt4::
	msgbox MtChimney_Text_AquasNameSimilar, MSGBOX_SIGN
	end

MtChimney_EventScript_BusyMagmaGrunt5::
	msgbox MtChimney_Text_DouseThemInFire, MSGBOX_SIGN
	end

MtChimney_EventScript_BusyMagmaGrunt6::
	msgbox MtChimney_Text_KeepMakingMoreLand, MSGBOX_SIGN
	end

MtChimney_EventScript_MagmaPoochyena::
	msgbox MtChimney_Text_Bufoh, MSGBOX_SIGN
	end

MtChimney_EventScript_AquaPoochyena::
	msgbox MtChimney_Text_Bushaa, MSGBOX_SIGN
	end

MtChimney_EventScript_MeteoriteMachine::
	lockall
	goto_if_unset FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY, MtChimney_EventScript_MachineOn
	goto_if_set FLAG_RECEIVED_METEORITE, MtChimney_EventScript_MachineOff
	msgbox MtChimney_Text_RemoveTheMeteorite, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MtChimney_EventScript_LeaveMeteoriteAlone
	msgbox MtChimney_Text_PlayerRemovedMeteorite, MSGBOX_DEFAULT
	giveitem ITEM_METEORITE
	setflag FLAG_RECEIVED_METEORITE
	releaseall
	end

MtChimney_EventScript_LeaveMeteoriteAlone::
	msgbox MtChimney_Text_PlayerLeftMeteorite, MSGBOX_DEFAULT
	releaseall
	end

MtChimney_EventScript_MachineOff::
	msgbox MtChimney_Text_MachineMakesNoResponse, MSGBOX_DEFAULT
	releaseall
	end

MtChimney_EventScript_MachineOn::
	msgbox MtChimney_Text_MetoriteFittedOnMachine, MSGBOX_DEFAULT
	releaseall
	end

MtChimney_EventScript_RouteSign::
	msgbox MtChimney_Text_RouteSign, MSGBOX_SIGN
	end

MtChimney_EventScript_Shelby::
	trainerbattle_single TRAINER_SHELBY_1, MtChimney_Text_ShelbyIntro, MtChimney_Text_ShelbyDefeat, MtChimney_EventScript_DefeatedShelby
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, MtChimney_EventScript_RematchShelby
	msgbox MtChimney_Text_ShelbyPostBattle, MSGBOX_DEFAULT
	release
	end

MtChimney_EventScript_DefeatedShelby::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox MtChimney_Text_ShelbyRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_SHELBY_1
	release
	end

MtChimney_EventScript_RematchShelby::
	trainerbattle_rematch TRAINER_SHELBY_1, MtChimney_Text_ShelbyRematchIntro, MtChimney_Text_ShelbyRematchDefeat
	msgbox MtChimney_Text_ShelbyPostRematch, MSGBOX_AUTOCLOSE
	end

MtChimney_EventScript_Melissa::
	trainerbattle_single TRAINER_MELISSA, MtChimney_Text_MelissaIntro, MtChimney_Text_MelissaDefeat
	msgbox MtChimney_Text_MelissaPostBattle, MSGBOX_AUTOCLOSE
	end

MtChimney_EventScript_Sheila::
	trainerbattle_single TRAINER_SHEILA, MtChimney_Text_SheilaIntro, MtChimney_Text_SheilaDefeat
	msgbox MtChimney_Text_SheilaPostBattle, MSGBOX_AUTOCLOSE
	end

MtChimney_EventScript_Shirley::
	trainerbattle_single TRAINER_SHIRLEY, MtChimney_Text_ShirleyIntro, MtChimney_Text_ShirleyDefeat
	msgbox MtChimney_Text_ShirleyPostBattle, MSGBOX_AUTOCLOSE
	end

MtChimney_EventScript_Grunt1::
	trainerbattle_single TRAINER_GRUNT_MT_CHIMNEY_1, MtChimney_Text_Grunt1Intro, MtChimney_Text_Grunt1Defeat
	msgbox MtChimney_Text_Grunt1PostBattle, MSGBOX_AUTOCLOSE
	end

MtChimney_EventScript_Sawyer::
	trainerbattle_single TRAINER_SAWYER_1, MtChimney_Text_SawyerIntro, MtChimney_Text_SawyerDefeat, MtChimney_EventScript_SawyerDefeated
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, MtChimney_EventScript_SawyerRematch
	msgbox MtChimney_Text_SawyerPostBattle, MSGBOX_DEFAULT
	release
	end

MtChimney_EventScript_SawyerDefeated::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox MtChimney_Text_SawyerRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_SAWYER_1
	release
	end

MtChimney_EventScript_SawyerRematch::
	trainerbattle_rematch TRAINER_SAWYER_1, MtChimney_Text_SawyerRematchIntro, MtChimney_Text_SawyerRematchDefeat
	msgbox MtChimney_Text_SawyerPostRematch, MSGBOX_AUTOCLOSE
	end

MtChimney_Text_MeteoriteWillActivateVolcano:
	.string "O poder contido no METEORITO…\p"
	.string "Amplificado por esta máquina, ele vai\n"
	.string "intensificar na hora a atividade\l"
	.string "vulcânica do MONTE CHIMNEY…\p"
	.string "A energia vai crescer lá no fundo\n"
	.string "do vulcão e…\l"
	.string "Fufufu…$"

MtChimney_Text_MaxieIntro:
	.string "MAXIE: Hum?\n"
	.string "Quem é você?\p"
	.string "… … … … … …\n"
	.string "… … … … … …\p"
	.string "Ouvi o ARCHIE reclamando de uma\n"
	.string "criança que se mete nos assuntos\l"
	.string "da EQUIPE AQUA.\p"
	.string "Deve ser você.\p"
	.string "Hunf! Então quer atrapalhar a gente,\n"
	.string "a EQUIPE MAGMA?\p"
	.string "Então escute.\n"
	.string "Há muito tempo, os seres vivos\l"
	.string "usavam a terra para viver e crescer.\p"
	.string "Por isso, a terra é o mais importante!\n"
	.string "Ela é o berço de tudo!\p"
	.string "Por isso, a EQUIPE MAGMA se dedica\n"
	.string "a expandir a terra firme.\p"
	.string "Para o progresso da humanidade e\n"
	.string "dos POKéMON!\p"
	.string "E, para isso, precisamos do poder\n"
	.string "do que dorme nesta montanha…\p"
	.string "Oh! Você não precisava saber\n"
	.string "tanto assim.\p"
	.string "Mas não importa!\n"
	.string "Vou te mostrar o preço de se meter\l"
	.string "nos nossos grandes planos!$"

MtChimney_Text_MaxieDefeat:
	.string "O quê?!\p"
	.string "Eu, MAXIE, fui pego de surpresa?!$"

MtChimney_Text_MaxieYouHaventSeenLastOfMagma:
	.string "MAXIE: Mas chega.\n"
	.string "Desta vez, eu recuo.\p"
	.string "Só não pense que viu a EQUIPE MAGMA\n"
	.string "pela última vez.\p"
	.string "Fufufu…\p"
	.string "Mesmo sem o METEORITO, se a gente\n"
	.string "conseguir aquele ORBE… Fufufu…$"

MtChimney_Text_TabithaIntro:
	.string "Hehehe!\p"
	.string "Então você veio até aqui!\p"
	.string "Mas chegou tarde!\n"
	.string "Já entreguei ao CHEFE o METEORITO\l"
	.string "da CATARATA DOS METEOROS!$"

MtChimney_Text_TabithaDefeat:
	.string "Hehehe…\p"
	.string "Perdi, mas, se o nosso líder\n"
	.string "despertar aquela coisa…$"

MtChimney_Text_TabithaPostBattle:
	.string "CHEFE, rápido!\n"
	.string "Use a energia do METEORITO!\p"
	.string "Hehehe…$"

MtChimney_Text_Grunt2Intro:
	.string "A EQUIPE MAGMA trabalha duro pelo\n"
	.string "bem de todo mundo.\p"
	.string "Tipo, se o poder daquela coisa criar\n"
	.string "mais terra, vai ter mais lugar pra\l"
	.string "morar.\p"
	.string "Todo mundo vai ficar feliz!$"

MtChimney_Text_Grunt2Defeat:
	.string "Hã?\n"
	.string "Como assim, eu perdi?$"

MtChimney_Text_Grunt2PostBattle:
	.string "O nosso CHEFE diz: “Isso vai deixar\n"
	.string "todo mundo feliz.”\p"
	.string "Então por que todo mundo vive\n"
	.string "atrapalhando a gente?$"

MtChimney_Text_Grunt1Intro:
	.string "Com mais terra, eu ia conseguir uma\n"
	.string "casa grandona só pra mim!\p"
	.string "Vou construir em cima da lava dura!$"

MtChimney_Text_Grunt1Defeat:
	.string "O meu sonho da casa grandona…$"

MtChimney_Text_Grunt1PostBattle:
	.string "Criança como você devia estar\n"
	.string "brincando nas ondas!$"

MtChimney_Text_TeamAquaAlwaysMessingWithPlans:
	.string "Essa EQUIPE AQUA chata…\n"
	.string "Sempre estraga os nossos planos!$"

MtChimney_Text_MeteoritesPackAmazingPower:
	.string "METEORITOS têm um poder incrível!$"

MtChimney_Text_YouBetterNotMessWithUs:
	.string "É melhor não mexer com a gente!\p"
	.string "Queremos despertar aquela coisa\n"
	.string "pro bem de todo mundo!$"

MtChimney_Text_AquasNameSimilar:
	.string "Nós somos a EQUIPE MAGMA!\p"
	.string "Eles são a EQUIPE AQUA!\p"
	.string "Me irrita demais eles usarem um\n"
	.string "nome parecido com o nosso!$"

MtChimney_Text_DouseThemInFire:
	.string "É isso aí!\n"
	.string "Taca fogo neles!$"

MtChimney_Text_KeepMakingMoreLand:
	.string "Vamos continuar criando mais terra!$"

MtChimney_Text_Bufoh:
	.string "Bufoh!$"

MtChimney_Text_ArchieGoStopTeamMagma:
	.string "ARCHIE: Grr, {PLAYER}!\n"
	.string "Eu devia saber que você ia aparecer!\p"
	.string "Veja com os seus olhos o que os\n"
	.string "fanáticos da EQUIPE MAGMA aprontam!\p"
	.string "Querem injetar no vulcão o poder\n"
	.string "do METEORITO roubado!\p"
	.string "Uma coisa dessas vai fazer o\n"
	.string "vulcão entrar em erupção!$"

MtChimney_Text_ArchieIHaveMyHandsFull:
	.string "ARCHIE: Grrr…\p"
	.string "Quero parar o MAXIE, mas não\n"
	.string "consigo!\p"
	.string "Não com três oponentes ao mesmo\n"
	.string "tempo pra enfrentar!$"

MtChimney_Text_ArchieThankYou:
	.string "ARCHIE: {PLAYER}!\n"
	.string "Obrigado!\p"
	.string "Com a sua ajuda, frustramos o plano\n"
	.string "destrutivo da EQUIPE MAGMA!\p"
	.string "Mas… Você…\n"
	.string "De que lado você está?\p"
	.string "Ah, tanto faz.\p"
	.string "Vamos ficar de olho e continuar\n"
	.string "atrás da EQUIPE MAGMA.\p"
	.string "{PLAYER}, a gente se vê de novo!$"

MtChimney_Text_MagmaOutnumbersUs:
	.string "Droga… A EQUIPE MAGMA tem mais gente!\n"
	.string "Não dá pra segurar!$"

MtChimney_Text_LessHabitatForWaterPokemon:
	.string "Se expandirem a terra, vai ter menos\n"
	.string "lugar pros POKéMON de ÁGUA!$"

MtChimney_Text_MagmasNameSimilar:
	.string "Nós somos a EQUIPE AQUA!\p"
	.string "Eles são a EQUIPE MAGMA!\p"
	.string "Me dá raiva eles usarem um nome\n"
	.string "tão confuso!$"

MtChimney_Text_Bushaa:
	.string "Bushaa!$"

MtChimney_Text_LavaCookiesJust200:
	.string "O BISCOITO LAVA é a especialidade\n"
	.string "do MONTE CHIMNEY.\p"
	.string "Experimente um. Só ¥200.$"

MtChimney_Text_ThankYouDear:
	.string "Obrigada, querida criança!$"

MtChimney_Text_YouveNotGotTheMoney:
	.string "Ai, ai. Sem dinheiro, não dá pra\n"
	.string "comprar nada.$"

MtChimney_Text_OhFineThen:
	.string "Ah, tudo bem, então.$"

MtChimney_Text_MetoriteFittedOnMachine:
	.string "Um METEORITO está encaixado numa\n"
	.string "máquina misteriosa…\p"
	.string "Parece que a máquina está guardando\n"
	.string "energia no METEORITO.$"

MtChimney_Text_RemoveTheMeteorite:
	.string "Um METEORITO está encaixado numa\n"
	.string "máquina misteriosa…\p"
	.string "Quer tirar o METEORITO?$"

MtChimney_Text_PlayerRemovedMeteorite:
	.string "{PLAYER} tirou o METEORITO da\n"
	.string "máquina misteriosa.$"

MtChimney_Text_PlayerLeftMeteorite:
	.string "{PLAYER} deixou o METEORITO onde\n"
	.string "estava.$"

MtChimney_Text_MachineMakesNoResponse:
	.string "Esta máquina misteriosa…\n"
	.string "Não responde a nada.$"

MtChimney_Text_RouteSign:
	.string "{DOWN_ARROW} TRILHA ESCARPADA\n"
	.string "CIDADE DE LAVARIDGE À FRENTE$"

MtChimney_Text_ShelbyIntro:
	.string "Fui às fontes termais e renovei\n"
	.string "esses ossos cansados.\l"
	.string "Agora estou me sentindo forte!$"

MtChimney_Text_ShelbyDefeat:
	.string "Minha nossa.\n"
	.string "Você é demais!$"

MtChimney_Text_ShelbyPostBattle:
	.string "Ora, ora, perdi. Agora não posso\n"
	.string "mais me chamar de ESPECIALISTA, né?$"

MtChimney_Text_ShelbyRegister:
	.string "Obrigada, criança. Foi divertido, como\n"
	.string "se eu batalhasse com um neto meu.\p"
	.string "Volte para uma revanche, por favor.$"

MtChimney_Text_ShelbyRematchIntro:
	.string "Se o seu coração bater junto com o\n"
	.string "dos seus POKéMON, você vai\l"
	.string "conseguir grandes coisas.$"

MtChimney_Text_ShelbyRematchDefeat:
	.string "Minha nossa.\n"
	.string "Você é demais!$"

MtChimney_Text_ShelbyPostRematch:
	.string "Talvez o seu coração já seja um só\n"
	.string "com o dos seus POKéMON.$"

MtChimney_Text_MelissaIntro:
	.string "Tô pegando fogo, benzinho.\n"
	.string "Não aguento! Preciso batalhar!$"

MtChimney_Text_MelissaDefeat:
	.string "Uh, que batalha escaldante!$"

MtChimney_Text_MelissaPostBattle:
	.string "O calor do MONTE CHIMNEY me\n"
	.string "aquece, benzinho!$"

MtChimney_Text_SheilaIntro:
	.string "Finalmente cheguei ao MONTE CHIMNEY.\n"
	.string "Quero botar meus POKéMON pra lutar!$"

MtChimney_Text_SheilaDefeat:
	.string "O seu jeito de batalhar…\n"
	.string "Parece uma erupção do MONTE CHIMNEY!$"

MtChimney_Text_SheilaPostBattle:
	.string "Como eu disse, finalmente cheguei ao\n"
	.string "MONTE CHIMNEY. Seria uma pena só\l"
	.string "fazer um pouco de turismo…\p"
	.string "Quero batalhar e comprar BISCOITOS\n"
	.string "de lembrança.$"

MtChimney_Text_ShirleyIntro:
	.string "Desde que tomei banho nas fontes\n"
	.string "termais, estou ótima!\l"
	.string "Com certeza vou ganhar!$"

MtChimney_Text_ShirleyDefeat:
	.string "Ai!\n"
	.string "Saí da água e peguei um friozinho.$"

MtChimney_Text_ShirleyPostBattle:
	.string "Vou ter que dar outro mergulho nas\n"
	.string "fontes termais. Quer vir junto?\p"
	.string "Brincadeira!$"

MtChimney_Text_SawyerIntro:
	.string "Que montanha boa! Cheia de gente\n"
	.string "quente pra fazer companhia!$"

MtChimney_Text_SawyerDefeat:
	.string "Ora, você também é um fogo só!$"

MtChimney_Text_SawyerPostBattle:
	.string "Acho que preciso de um mergulho\n"
	.string "nas FONTES TERMAIS de LAVARIDGE!$"

MtChimney_Text_SawyerRegister:
	.string "Gosto de foguinhos como você.\n"
	.string "Vou te registrar no meu POKéNAV.$"

MtChimney_Text_SawyerRematchIntro:
	.string "Por aqui, só tem gente quente à minha\n"
	.string "volta. Não vou perder!$"

MtChimney_Text_SawyerRematchDefeat:
	.string "Puxa, você continua sendo o\n"
	.string "mesmo fogo de antes!$"

MtChimney_Text_SawyerPostRematch:
	.string "Na verdade, aqui é quente mesmo.\n"
	.string "Estou com roupa demais pra cá.$"

