CaveOfOrigin_B1F_MapScripts::
	.byte 0

CaveOfOrigin_B1F_EventScript_Wallace::
	lock
	faceplayer
	msgbox CaveOfOrigin_B1F_Text_WallaceStory, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_CAVE_OF_ORIGIN_WALLACE, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	delay 60
	playse SE_PIN
	applymovement LOCALID_CAVE_OF_ORIGIN_WALLACE, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_CAVE_OF_ORIGIN_WALLACE, Common_Movement_Delay48
	waitmovement 0
	delay 30
	applymovement LOCALID_CAVE_OF_ORIGIN_WALLACE, Common_Movement_FacePlayer
	waitmovement 0
	message CaveOfOrigin_B1F_Text_WhereIsRayquaza
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_WheresRayquaza::
	multichoice 0, 0, MULTI_WHERES_RAYQUAZA, FALSE
	switch VAR_RESULT
	case 0, CaveOfOrigin_B1F_EventScript_AtCaveOfOrigin
	case 1, CaveOfOrigin_B1F_EventScript_AtMtPyre
	case 2, CaveOfOrigin_B1F_EventScript_AtSkyPillar
	case 3, CaveOfOrigin_B1F_EventScript_DontRemember
	goto CaveOfOrigin_B1F_EventScript_DontRemember
	end

CaveOfOrigin_B1F_EventScript_AtCaveOfOrigin::
	message CaveOfOrigin_B1F_Text_ButWereInCaveOfOrigin
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_AtMtPyre::
	message CaveOfOrigin_B1F_Text_OldLadyDidntMentionThat
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_DontRemember::
	message CaveOfOrigin_B1F_Text_CantYouRememberSomehow
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_AtSkyPillar::
	msgbox CaveOfOrigin_B1F_Text_WellHeadToSkyPillar, MSGBOX_DEFAULT
	closemessage
	playse SE_EXIT
	fadescreenspeed FADE_TO_BLACK, 4
	setflag FLAG_WALLACE_GOES_TO_SKY_PILLAR
	setvar VAR_SOOTOPOLIS_CITY_STATE, 3
	removeobject LOCALID_CAVE_OF_ORIGIN_WALLACE
	clearflag FLAG_HIDE_SKY_PILLAR_WALLACE
	fadescreen FADE_FROM_BLACK
	release
	end

CaveOfOrigin_B1F_Text_WallaceStory:
	.string "Ah, então você é {PLAYER}{KUN}?\n"
	.string "Ouvi falar das suas façanhas.\p"
	.string "O meu nome é WALLACE.\p"
	.string "Eu era o LÍDER do GINÁSIO de\n"
	.string "SOOTOPOLIS, mas surgiu um assunto.\p"
	.string "Então confiei o GINÁSIO ao meu\n"
	.string "mestre, o JUAN.\p"
	.string "… … … … … …\n"
	.string "… … … … … …\p"
	.string "GROUDON e KYOGRE, os dois POKéMON\n"
	.string "que estão causando este caos, são\l"
	.string "considerados POKéMON superantigos.\p"
	.string "Mas não existem só dois POKéMON\n"
	.string "superantigos.\p"
	.string "Há mais um em algum lugar.\p"
	.string "Em algum lugar, existe um POKéMON\n"
	.string "superantigo chamado RAYQUAZA.\p"
	.string "Dizem que foi o RAYQUAZA que acalmou\n"
	.string "os dois combatentes num passado\l"
	.string "distante.\p"
	.string "Mas nem eu faço ideia de onde\n"
	.string "o RAYQUAZA esteja…$"

CaveOfOrigin_B1F_Text_WhereIsRayquaza:
	.string "WALLACE: {PLAYER}{KUN}, por acaso sabe\n"
	.string "onde o RAYQUAZA está agora?\p"
	.string "Se souber, por favor, me diga.$"

CaveOfOrigin_B1F_Text_ButWereInCaveOfOrigin:
	.string "WALLACE: A CAVERNA DA ORIGEM?\p"
	.string "Mas é aqui mesmo!\n"
	.string "Preciso de algo melhor que isso!\p"
	.string "Por favor, pense onde o RAYQUAZA\n"
	.string "pode estar agora.$"

CaveOfOrigin_B1F_Text_OldLadyDidntMentionThat:
	.string "WALLACE: O MONTE PYRE?\p"
	.string "Mas, quando encontrei a senhora de\n"
	.string "lá agora há pouco, ela não disse nada.\p"
	.string "Duvido muito que a senhora\n"
	.string "escondesse algo de mim…\p"
	.string "{PLAYER}{KUN}, pode pensar nisso com\n"
	.string "mais calma pra mim?$"

CaveOfOrigin_B1F_Text_CantYouRememberSomehow:
	.string "WALLACE: Hã? Não se lembra?\n"
	.string "Hum… Isso é um problema…\p"
	.string "Não consegue se lembrar de algum\n"
	.string "jeito?$"

CaveOfOrigin_B1F_Text_WellHeadToSkyPillar:
	.string "WALLACE: O PILAR CELESTE?\p"
	.string "É isso!\n"
	.string "Só pode ser o PILAR CELESTE!\p"
	.string "{PLAYER}{KUN}, não há tempo a perder!\n"
	.string "Vamos já pro PILAR CELESTE!$"

