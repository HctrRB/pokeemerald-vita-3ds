JaggedPass_MapScripts::
	map_script MAP_SCRIPT_ON_RESUME, JaggedPass_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, JaggedPass_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, JaggedPass_OnLoad
	.byte 0

JaggedPass_OnResume:
	setstepcallback STEP_CB_ASH
	call_if_eq VAR_JAGGED_PASS_STATE, 0, JaggedPass_EventScript_CheckHasMagmaEmblem
	end

JaggedPass_EventScript_CheckHasMagmaEmblem::
	checkitem ITEM_MAGMA_EMBLEM
	goto_if_eq VAR_RESULT, TRUE, JaggedPass_EventScript_SetReadyToOpenHideout
	return

JaggedPass_EventScript_SetReadyToOpenHideout::
	setvar VAR_JAGGED_PASS_STATE, 1
	return

JaggedPass_OnTransition:
	call_if_eq VAR_JAGGED_PASS_ASH_WEATHER, 1, JaggedPass_EventScript_SetWeatherAsh
	end

JaggedPass_EventScript_SetWeatherAsh::
	setweather WEATHER_VOLCANIC_ASH
	doweather
	return

JaggedPass_OnLoad:
	goto_if_le VAR_JAGGED_PASS_STATE, 1, JaggedPass_EventScript_ConcealHideoutEntrance
	end

JaggedPass_EventScript_ConcealHideoutEntrance::
	setmetatile 16, 17, METATILE_Lavaridge_RockWall, TRUE
	setmetatile 16, 18, METATILE_Lavaridge_RockWall, TRUE
	end

JaggedPass_EventScript_OpenMagmaHideout::
	lockall
	setvar VAR_0x8004, 1  /* vertical pan*/
	setvar VAR_0x8005, 1  /* horizontal pan*/
	setvar VAR_0x8006, 8  /* num shakes*/
	setvar VAR_0x8007, 5  /* shake delay*/
	special ShakeCamera
	waitstate
	msgbox JaggedPass_Text_BoulderShakingInResponseToEmblem, MSGBOX_DEFAULT
	closemessage
	setvar VAR_0x8004, 1   /* vertical pan*/
	setvar VAR_0x8005, 1   /* horizontal pan*/
	setvar VAR_0x8006, 16  /* num shakes*/
	setvar VAR_0x8007, 3   /* shake delay*/
	special ShakeCamera
	waitstate
	playse SE_EFFECTIVE
	setmetatile 16, 17, METATILE_Lavaridge_CaveEntrance_Top, TRUE
	setmetatile 16, 18, METATILE_Lavaridge_CaveEntrance_Bottom, FALSE
	special DrawWholeMapView
	delay 30
	setvar VAR_JAGGED_PASS_STATE, 2
	waitse
	releaseall
	end

JaggedPass_EventScript_MagmaHideoutGuard::
	lockall
	goto_if_set FLAG_BEAT_MAGMA_GRUNT_JAGGED_PASS, JaggedPass_EventScript_GuardDefeated
	waitse
	playse SE_PIN
	applymovement LOCALID_MAGMA_HIDEOUT_GUARD, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_MAGMA_HIDEOUT_GUARD, Common_Movement_Delay48
	waitmovement 0
	applymovement LOCALID_MAGMA_HIDEOUT_GUARD, Common_Movement_FacePlayer
	waitmovement 0
	msgbox JaggedPass_Text_GruntIntro, MSGBOX_DEFAULT
	closemessage
	trainerbattle_no_intro TRAINER_GRUNT_JAGGED_PASS, JaggedPass_Text_GruntDefeat
	setflag FLAG_BEAT_MAGMA_GRUNT_JAGGED_PASS
	applymovement LOCALID_MAGMA_HIDEOUT_GUARD, Common_Movement_FaceOriginalDirection
	waitmovement 0
	releaseall
	end

JaggedPass_EventScript_GuardDefeated::
	applymovement LOCALID_MAGMA_HIDEOUT_GUARD, Common_Movement_FacePlayer
	waitmovement 0
	msgbox JaggedPass_Text_GoWhereverYouWant, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_MAGMA_HIDEOUT_GUARD, Common_Movement_FaceOriginalDirection
	waitmovement 0
	releaseall
	end

JaggedPass_EventScript_Eric::
	trainerbattle_single TRAINER_ERIC, JaggedPass_Text_EricIntro, JaggedPass_Text_EricDefeat
	msgbox JaggedPass_Text_EricPostBattle, MSGBOX_AUTOCLOSE
	end

JaggedPass_EventScript_Diana::
	trainerbattle_single TRAINER_DIANA_1, JaggedPass_Text_DianaIntro, JaggedPass_Text_DianaDefeat, JaggedPass_EventScript_RegisterDiana
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, JaggedPass_EventScript_DianaRematch
	msgbox JaggedPass_Text_DianaPostBattle, MSGBOX_DEFAULT
	release
	end

JaggedPass_EventScript_RegisterDiana::
	special PlayerFaceTrainerAfterBattle
	msgbox JaggedPass_Text_DianaRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_DIANA_1
	release
	end

JaggedPass_EventScript_DianaRematch::
	trainerbattle_rematch TRAINER_DIANA_1, JaggedPass_Text_DianaRematchIntro, JaggedPass_Text_DianaRematchDefeat
	msgbox JaggedPass_Text_DianaPostRematch, MSGBOX_AUTOCLOSE
	end

JaggedPass_EventScript_Ethan::
	trainerbattle_single TRAINER_ETHAN_1, JaggedPass_Text_EthanIntro, JaggedPass_Text_EthanDefeat, JaggedPass_EventScript_RegisterEthan
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, JaggedPass_EventScript_EthanRematch
	msgbox JaggedPass_Text_EthanPostBattle, MSGBOX_DEFAULT
	release
	end

JaggedPass_EventScript_RegisterEthan::
	special PlayerFaceTrainerAfterBattle
	msgbox JaggedPass_Text_EthanRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_ETHAN_1
	release
	end

JaggedPass_EventScript_EthanRematch::
	trainerbattle_rematch TRAINER_ETHAN_1, JaggedPass_Text_EthanRematchIntro, JaggedPass_Text_EthanRematchDefeat
	msgbox JaggedPass_Text_EthanPostRematch, MSGBOX_AUTOCLOSE
	end

JaggedPass_EventScript_Julio::
	trainerbattle_single TRAINER_JULIO, JaggedPass_Text_JulioIntro, JaggedPass_Text_JulioDefeat
	msgbox JaggedPass_Text_JulioPostBattle, MSGBOX_AUTOCLOSE
	end

JaggedPass_EventScript_Autumn::
	trainerbattle_single TRAINER_AUTUMN, JaggedPass_Text_AutumnIntro, JaggedPass_Text_AutumnDefeat
	msgbox JaggedPass_Text_AutumnPostBattle, MSGBOX_AUTOCLOSE
	end

JaggedPass_Text_EricIntro:
	.string "A TRILHA ESCARPADA do MONTE CHIMNEY…\p"
	.string "Isso é tudo o que eu sempre quis\n"
	.string "numa montanha.\p"
	.string "Esse chão todo irregular…\n"
	.string "Mexe com a minha alma!$"

JaggedPass_Text_EricDefeat:
	.string "Perder me deixou amargo!$"

JaggedPass_Text_EricPostBattle:
	.string "É, perdi na batalha POKéMON…\p"
	.string "Mas, no amor pelas montanhas,\n"
	.string "quem ganha sou eu!$"

JaggedPass_Text_DianaIntro:
	.string "Isto aqui não é uma caminhada à toa.\n"
	.string "Não serve pra piquenique.$"

JaggedPass_Text_DianaDefeat:
	.string "Ah, não!\n"
	.string "O chão é irregular demais…$"

JaggedPass_Text_DianaPostBattle:
	.string "Sabia?\p"
	.string "Tem gente esperta que sobe esta trilha\n"
	.string "horrível de bicicleta.$"

JaggedPass_Text_DianaRegister:
	.string "Você vai voltar por aqui?\n"
	.string "Se voltar, quero uma revanche.$"

JaggedPass_Text_DianaRematchIntro:
	.string "Piquenique é bom em qualquer lugar.\n"
	.string "Igualzinho aos POKéMON!$"

JaggedPass_Text_DianaRematchDefeat:
	.string "Só perdi porque o chão é irregular\n"
	.string "demais!$"

JaggedPass_Text_DianaPostRematch:
	.string "Vou esquecer a derrota e curtir\n"
	.string "esta trilha cheia de buracos.$"

JaggedPass_Text_EthanIntro:
	.string "É difícil andar na TRILHA ESCARPADA.\n"
	.string "É um bom lugar pra treinar.$"

JaggedPass_Text_EthanDefeat:
	.string "Acabou enquanto a gente ainda\n"
	.string "procurava onde pisar…$"

JaggedPass_Text_EthanPostBattle:
	.string "Se eu tivesse uma BICICLETA ACRO,\n"
	.string "dava pra pular os degraus.$"

JaggedPass_Text_EthanRegister:
	.string "Quando eu me acostumar com esse chão,\n"
	.string "vou ganhar com certeza!\p"
	.string "Pode me registrar no seu POKéNAV?$"

JaggedPass_Text_EthanRematchIntro:
	.string "Já me acostumei com esse chão.\n"
	.string "Agora até canto enquanto subo.$"

JaggedPass_Text_EthanRematchDefeat:
	.string "Ainda não é fácil batalhar neste\n"
	.string "chão irregular…$"

JaggedPass_Text_EthanPostRematch:
	.string "Eu devia pegar uma BICICLETA ACRO\n"
	.string "com o RYDEL, em MAUVILLE…$"

JaggedPass_Text_GruntIntro:
	.string "Uau!\n"
	.string "O que você está fazendo aqui?\p"
	.string "O que eu estou fazendo num lugar\n"
	.string "desses?\p"
	.string "E o que você tem a ver com isso?$"

JaggedPass_Text_GruntDefeat:
	.string "Argh…\p"
	.string "Eu devia ter me enfiado logo no\n"
	.string "nosso ESCONDERIJO…$"

JaggedPass_Text_GoWhereverYouWant:
	.string "Tá bom, tá booom!\n"
	.string "Admito: você é forte!\p"
	.string "Não se preocupe comigo.\n"
	.string "Vá aonde quiser!$"

JaggedPass_Text_BoulderShakingInResponseToEmblem:
	.string "Oh! Esta pedra está tremendo por\n"
	.string "causa do EMBLEMA MAGMA!$"

JaggedPass_Text_JulioIntro:
	.string "Aiii! Dá muito medo descer a\n"
	.string "montanha de uma vez só!$"

JaggedPass_Text_JulioDefeat:
	.string "Parece que estou caindo aos pedaços…$"

JaggedPass_Text_JulioPostBattle:
	.string "A bicicleta pulou tanto que estou\n"
	.string "com o traseiro doendo…$"

JaggedPass_Text_AutumnIntro:
	.string "Subo este morro todo dia.\n"
	.string "Confio na minha força!$"

JaggedPass_Text_AutumnDefeat:
	.string "Hmm…\n"
	.string "O que deu errado?$"

JaggedPass_Text_AutumnPostBattle:
	.string "O que é aquela rocha estranha saindo\n"
	.string "do chão, um pouco mais pra cima?$"

