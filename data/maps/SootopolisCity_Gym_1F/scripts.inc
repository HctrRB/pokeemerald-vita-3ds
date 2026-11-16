SootopolisCity_Gym_1F_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, SootopolisCity_Gym_1F_OnFrame
	map_script MAP_SCRIPT_ON_RESUME, SootopolisCity_Gym_1F_OnResume
	map_script MAP_SCRIPT_ON_LOAD, SootopolisCity_Gym_1F_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, SootopolisCity_Gym_1F_OnTransition
	.byte 0

SootopolisCity_Gym_1F_OnTransition:
	setvar VAR_ICE_STEP_COUNT, 1
	end

SootopolisCity_Gym_1F_OnResume:
	setstepcallback STEP_CB_SOOTOPOLIS_ICE
	end

SootopolisCity_Gym_1F_OnLoad:
	call SootopolisCity_Gym_1F_EventScript_CheckSetStairMetatiles
	special SetSootopolisGymCrackedIceMetatiles
	end

SootopolisCity_Gym_1F_EventScript_CheckSetStairMetatiles::
	goto_if_lt VAR_ICE_STEP_COUNT, 8, SootopolisCity_Gym_1F_EventScript_StopCheckingStairs  /* All stairs ice*/
	goto_if_lt VAR_ICE_STEP_COUNT, 28, SootopolisCity_Gym_1F_EventScript_OpenFirstStairs
	goto_if_lt VAR_ICE_STEP_COUNT, 67, SootopolisCity_Gym_1F_EventScript_OpenFirstAndSecondStairs
	setmetatile 8, 4, METATILE_SootopolisGym_Stairs, FALSE
	setmetatile 8, 5, METATILE_SootopolisGym_Stairs, FALSE
SootopolisCity_Gym_1F_EventScript_OpenFirstAndSecondStairs::
	setmetatile 8, 10, METATILE_SootopolisGym_Stairs, FALSE
	setmetatile 8, 11, METATILE_SootopolisGym_Stairs, FALSE
SootopolisCity_Gym_1F_EventScript_OpenFirstStairs::
	setmetatile 8, 15, METATILE_SootopolisGym_Stairs, FALSE
	setmetatile 8, 16, METATILE_SootopolisGym_Stairs, FALSE
SootopolisCity_Gym_1F_EventScript_StopCheckingStairs::
	return

SootopolisCity_Gym_1F_OnFrame:
	map_script_2 VAR_ICE_STEP_COUNT, 8, SootopolisCity_Gym_1F_EventScript_UnlockFirstStairs
	map_script_2 VAR_ICE_STEP_COUNT, 28, SootopolisCity_Gym_1F_EventScript_UnlockSecondStairs
	map_script_2 VAR_ICE_STEP_COUNT, 67, SootopolisCity_Gym_1F_EventScript_UnlockThirdStairs
	map_script_2 VAR_ICE_STEP_COUNT, 0, SootopolisCity_Gym_1F_EventScript_FallThroughIce
	.short 0

SootopolisCity_Gym_1F_EventScript_UnlockFirstStairs::
	addvar VAR_ICE_STEP_COUNT, 1
	delay 40
	playse SE_ICE_STAIRS
	call SootopolisCity_Gym_1F_EventScript_CheckSetStairMetatiles
	special DrawWholeMapView
	end

SootopolisCity_Gym_1F_EventScript_UnlockSecondStairs::
	addvar VAR_ICE_STEP_COUNT, 1
	delay 40
	playse SE_ICE_STAIRS
	call SootopolisCity_Gym_1F_EventScript_CheckSetStairMetatiles
	special DrawWholeMapView
	end

SootopolisCity_Gym_1F_EventScript_UnlockThirdStairs::
	addvar VAR_ICE_STEP_COUNT, 1
	delay 40
	playse SE_ICE_STAIRS
	call SootopolisCity_Gym_1F_EventScript_CheckSetStairMetatiles
	special DrawWholeMapView
	end

SootopolisCity_Gym_1F_EventScript_FallThroughIce::
	lockall
	delay 20
	applymovement LOCALID_PLAYER, SootopolisCity_Gym_1F_Movement_FallThroughIce
	waitmovement 0
	playse SE_FALL
	delay 60
	warphole MAP_SOOTOPOLIS_CITY_GYM_B1F
	waitstate
	end

SootopolisCity_Gym_1F_Movement_FallThroughIce:
	set_invisible
	step_end

SootopolisCity_Gym_1F_EventScript_Juan::
	trainerbattle_single TRAINER_JUAN_1, SootopolisCity_Gym_1F_Text_JuanIntro, SootopolisCity_Gym_1F_Text_JuanDefeat, SootopolisCity_Gym_1F_EventScript_JuanDefeated, NO_MUSIC
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, SootopolisCity_Gym_1F_EventScript_JuanRematch
	goto_if_unset FLAG_RECEIVED_TM_WATER_PULSE, SootopolisCity_Gym_1F_EventScript_GiveWaterPulse2
	goto_if_unset FLAG_BADGE06_GET, SootopolisCity_Gym_1F_EventScript_GoGetFortreeBadge
	msgbox SootopolisCity_Gym_1F_Text_JuanPostBattle, MSGBOX_DEFAULT
	release
	end

SootopolisCity_Gym_1F_EventScript_JuanDefeated::
	message SootopolisCity_Gym_1F_Text_ReceivedRainBadge
	waitmessage
	call Common_EventScript_PlayGymBadgeFanfare
	msgbox SootopolisCity_Gym_1F_Text_ExplainRainBadgeTakeThis, MSGBOX_DEFAULT
	setflag FLAG_DEFEATED_SOOTOPOLIS_GYM
	setflag FLAG_BADGE08_GET
	setflag FLAG_HIDE_SOOTOPOLIS_CITY_RESIDENTS
	setflag FLAG_HIDE_SOOTOPOLIS_CITY_STEVEN
	setflag FLAG_HIDE_SOOTOPOLIS_CITY_WALLACE
	setvar VAR_SOOTOPOLIS_CITY_STATE, 6
	clearflag FLAG_HIDE_SOOTOPOLIS_CITY_MAN_1
	setvar VAR_0x8008, 8
	call Common_EventScript_SetGymTrainers
	call SootopolisCity_Gym_1F_EventScript_GiveWaterPulse
	closemessage
	delay 30
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox SootopolisCity_Gym_1F_Text_RegisteredJuan, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	delay 30
	setflag FLAG_ENABLE_JUAN_MATCH_CALL
	release
	end

SootopolisCity_Gym_1F_EventScript_GiveWaterPulse::
	giveitem ITEM_TM_WATER_PULSE
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_BagIsFull
	msgbox SootopolisCity_Gym_1F_Text_ExplainWaterPulse, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_WATER_PULSE
	return

SootopolisCity_Gym_1F_EventScript_GiveWaterPulse2::
	giveitem ITEM_TM_WATER_PULSE
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	msgbox SootopolisCity_Gym_1F_Text_ExplainWaterPulse, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_WATER_PULSE
	release
	end

SootopolisCity_Gym_1F_EventScript_GoGetFortreeBadge::
	msgbox SootopolisCity_Gym_1F_Text_GoGetFortreeBadge, MSGBOX_DEFAULT
	release
	end

SootopolisCity_Gym_1F_EventScript_JuanRematch::
	trainerbattle_rematch_double TRAINER_JUAN_1, SootopolisCity_Gym_1F_Text_JuanPreRematch, SootopolisCity_Gym_1F_Text_JuanRematchDefeat, SootopolisCity_Gym_1F_Text_JuanRematchNeedTwoMons
	msgbox SootopolisCity_Gym_1F_Text_JuanPostRematch, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_1F_EventScript_GymGuide::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_SOOTOPOLIS_GYM, SootopolisCity_Gym_1F_EventScript_GymGuidePostVictory
	msgbox SootopolisCity_Gym_1F_Text_GymGuideAdvice, MSGBOX_DEFAULT
	release
	end

SootopolisCity_Gym_1F_EventScript_GymGuidePostVictory::
	msgbox SootopolisCity_Gym_1F_Text_GymGuidePostVictory, MSGBOX_DEFAULT
	release
	end

SootopolisCity_Gym_1F_EventScript_LeftGymStatue::
	lockall
	goto_if_set FLAG_BADGE08_GET, SootopolisCity_Gym_1F_EventScript_GymStatueCertified
	goto SootopolisCity_Gym_1F_EventScript_GymStatue
	end

SootopolisCity_Gym_1F_EventScript_RightGymStatue::
	lockall
	goto_if_set FLAG_BADGE08_GET, SootopolisCity_Gym_1F_EventScript_GymStatueCertified
	goto SootopolisCity_Gym_1F_EventScript_GymStatue
	end

SootopolisCity_Gym_1F_EventScript_GymStatueCertified::
	msgbox SootopolisCity_Gym_1F_Text_GymStatueCertified, MSGBOX_DEFAULT
	releaseall
	end

SootopolisCity_Gym_1F_EventScript_GymStatue::
	msgbox SootopolisCity_Gym_1F_Text_GymStatue, MSGBOX_DEFAULT
	releaseall
	end

SootopolisCity_Gym_1F_Text_GymGuideAdvice:
	.string "E aí! Como vai, {PLAYER},\n"
	.string "futura estrela da LIGA?\p"
	.string "O JUAN, LÍDER do GINÁSIO de\n"
	.string "SOOTOPOLIS, é mestre do tipo ÁGUA.\p"
	.string "E, pra chegar até o JUAN, um chão\n"
	.string "de gelo vai atrapalhar o caminho…\p"
	.string "Escute, desculpe, mas é só esse o\n"
	.string "conselho que tenho pra você.\p"
	.string "Daqui em diante, é com você!$"

SootopolisCity_Gym_1F_Text_GymGuidePostVictory:
	.string "Uau! Você venceu até o JUAN, que\n"
	.string "diziam ser o melhor de HOENN!\p"
	.string "Certo! Confira o seu CARTÃO DE\n"
	.string "TREINADOR.\p"
	.string "Se tiver todas as INSÍGNIAS, já\n"
	.string "pode encarar a LIGA POKéMON!$"

SootopolisCity_Gym_1F_Text_JuanIntro:
	.string "Deixe-me perguntar.\n"
	.string "Você sabia?\l"
	.string "Ah, não devo fazer mistério.\p"
	.string "Fui eu que ensinei ao WALLACE tudo\n"
	.string "o que há pra saber sobre POKéMON.\p"
	.string "Um dia, deixei o cargo de LÍDER\n"
	.string "do GINÁSIO.\p"
	.string "No meu lugar, confiei o GINÁSIO\n"
	.string "ao WALLACE.\p"
	.string "Porém, surgiu um motivo forte para\n"
	.string "o meu retorno.\p"
	.string "Ah, mas chega de conversa.\n"
	.string "Vamos começar a batalha?\p"
	.string "Por favor, seja testemunha da\n"
	.string "nossa arte.\p"
	.string "Uma grande ilusão de água esculpida\n"
	.string "pelos POKéMON e por mim!$"

/* NOTE: This defeat text actually causes a buffer overflow. It's about 50 bytes too long for*/
/* the gDisplayedStringBattle buffer that it's put into, and it stomps all over the gBattleTextBuffs*/
/* after, as well as the otherwise unused array after that. One wonders if that's the reason for*/
/* the existence of that unused array of ints.*/
SootopolisCity_Gym_1F_Text_JuanDefeat:
	.string "Ahahaha, excelente!\n"
	.string "Muito bem, a vitória é sua.\p"
	.string "Em você, sinto o brilho de um\n"
	.string "talento que vai superar tudo.\p"
	.string "Porém, comparado a mim ou mesmo\n"
	.string "ao WALLACE, falta elegância.\p"
	.string "Talvez eu devesse te emprestar\n"
	.string "a minha roupa?\p"
	.string "… … … … … …\n"
	.string "… … … … … …\p"
	.string "Hahaha, é só brincadeira!\p"
	.string "Em vez das minhas roupas, vou te\n"
	.string "recompensar com isto, a INSÍGNIA\l"
	.string "DA CHUVA!$"

SootopolisCity_Gym_1F_Text_ReceivedRainBadge:
	.string "{PLAYER} recebeu a INSÍGNIA DA CHUVA\n"
	.string "do JUAN.$"

SootopolisCity_Gym_1F_Text_ExplainRainBadgeTakeThis:
	.string "Com a INSÍGNIA DA CHUVA, todos os\n"
	.string "seus POKéMON vão obedecer a todos\l"
	.string "os seus comandos.\p"
	.string "A INSÍGNIA DA CHUVA também permite\n"
	.string "usar o golpe de MO CACHOEIRA pra\l"
	.string "subir paredes de água.\p"
	.string "E, pra nunca esquecer a batalha\n"
	.string "que tivemos, leve isto…$"

SootopolisCity_Gym_1F_Text_ExplainWaterPulse:
	.string "A MÁQUINA TÉCNICA que te entreguei\n"
	.string "contém PULSO D'ÁGUA.\p"
	.string "Às vezes, ele confunde o alvo com\n"
	.string "ondas ultrassônicas.\p"
	.string "… … … … … …$"

SootopolisCity_Gym_1F_Text_RegisteredJuan:
	.string "O LÍDER JUAN foi registrado\n"
	.string "no POKéNAV.$"

SootopolisCity_Gym_1F_Text_JuanPostBattle:
	.string "Os TREINADORES que reuniram todas as\n"
	.string "INSÍGNIAS de HOENN devem seguir pro\l"
	.string "destino final.\p"
	.string "A LIGA POKéMON.\p"
	.string "Viaje até o extremo leste de HOENN,\n"
	.string "até a ilha EVER GRANDE.\p"
	.string "Lá, você vai encontrar a LIGA\n"
	.string "POKéMON.$"

SootopolisCity_Gym_1F_Text_GoGetFortreeBadge:
	.string "Só falta uma INSÍGNIA de HOENN.\p"
	.string "Se quiser desafiar a LIGA POKéMON,\n"
	.string "precisa da última INSÍGNIA, do\l"
	.string "GINÁSIO de FORTREE.$"

SootopolisCity_Gym_1F_Text_GymStatue:
	.string "GINÁSIO POKéMON DE SOOTOPOLIS$"

SootopolisCity_Gym_1F_Text_GymStatueCertified:
	.string "GINÁSIO POKéMON DE SOOTOPOLIS\p"
	.string "TREINADORES APROVADOS POR JUAN:\n"
	.string "{PLAYER}$"

SootopolisCity_Gym_1F_Text_JuanPreRematch:
	.string "JUAN: Ah, este GINÁSIO tinha voltado\n"
	.string "à serenidade de sempre…\p"
	.string "Mas o nosso jovem tufão voltou pra\n"
	.string "nos testar de novo!\p"
	.string "Pois bem, com certeza!\p"
	.string "Vou adorar dançar com você quantas\n"
	.string "vezes quiser!$"

SootopolisCity_Gym_1F_Text_JuanRematchDefeat:
	.string "Ahahaha, a vitória é sua!\n"
	.string "Você me venceu de novo!$"

SootopolisCity_Gym_1F_Text_JuanPostRematch:
	.string "JUAN: Se eu te pedisse pra ser meu\n"
	.string "aprendiz, você recusaria, decerto.\p"
	.string "Queria te dar o meu casaco de\n"
	.string "presente.\p"
	.string "Mas, de novo, você recusaria.\n"
	.string "Imagino que sim.\p"
	.string "E isso é sinal certo\n"
	.string "de nobreza!$"

SootopolisCity_Gym_1F_Text_JuanRematchNeedTwoMons:
	.string "JUAN: Ah, este GINÁSIO tinha voltado\n"
	.string "à serenidade de sempre…\p"
	.string "Mas o nosso jovem tufão voltou pra\n"
	.string "nos testar de novo!\p"
	.string "Pois bem, com certeza!\n"
	.string "Vou adorar dançar com você…\p"
	.string "Ah, não, não, não.\n"
	.string "Você só tem um POKéMON.\p"
	.string "Por favor, volte com dois POKéMON,\n"
	.string "talvez mais.$"

