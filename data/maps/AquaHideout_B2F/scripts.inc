AquaHideout_B2F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, AquaHideout_B2F_OnTransition
	.byte 0

AquaHideout_B2F_OnTransition:
	call_if_set FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE, AquaHideout_B2F_EventScript_PreventMattNoticing
	end

AquaHideout_B2F_EventScript_PreventMattNoticing::
	setvar VAR_TEMP_1, 1
	return

AquaHideout_B2F_EventScript_MattNoticePlayer::
	lockall
	setvar VAR_0x8008, LOCALID_AQUA_HIDEOUT_MATT
	playse SE_PIN
	applymovement VAR_0x8008, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement VAR_0x8008, Common_Movement_FacePlayer
	waitmovement 0
	setvar VAR_TEMP_1, 1
	releaseall
	end

AquaHideout_B2F_EventScript_Matt::
	trainerbattle_single TRAINER_MATT, AquaHideout_B2F_Text_MattIntro, AquaHideout_B2F_Text_MattDefeat, AquaHideout_B2F_EventScript_SubmarineEscape
	msgbox AquaHideout_B2F_Text_MattPostBattle, MSGBOX_DEFAULT
	release
	end

AquaHideout_B2F_EventScript_SubmarineEscape::
	setvar VAR_0x8008, LOCALID_AQUA_HIDEOUT_MATT
	setvar VAR_0x8009, LOCALID_AQUA_HIDEOUT_SUBMARINE
	applymovement VAR_0x8008, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	delay 20
	applymovement VAR_0x8008, Common_Movement_FacePlayer
	waitmovement 0
	msgbox AquaHideout_B2F_Text_OurBossGotThroughHisPreparations, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_0x8008, Common_Movement_WalkInPlaceFasterLeft
	applymovement VAR_0x8009, AquaHideout_B2F_Movement_SumbarineDepartLeft
	waitmovement 0
	removeobject VAR_0x8009
	delay 20
	applymovement VAR_0x8008, Common_Movement_FacePlayer
	waitmovement 0
	msgbox AquaHideout_B2F_Text_MattPostBattle, MSGBOX_DEFAULT
	setflag FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE
	setflag FLAG_HIDE_LILYCOVE_CITY_AQUA_GRUNTS
	release
	end

AquaHideout_B2F_Movement_SumbarineDepartLeft:
	walk_left
	walk_left
	walk_left
	walk_left
	step_end

/* Unused*/
AquaHideout_B2F_Movement_SumbarineDepartRight:
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

AquaHideout_B2F_EventScript_Grunt4::
	trainerbattle_single TRAINER_GRUNT_AQUA_HIDEOUT_4, AquaHideout_B2F_Text_Grunt4Intro, AquaHideout_B2F_Text_Grunt4Defeat, AquaHideout_B2F_EventScript_Grunt4Defeated
	msgbox AquaHideout_B2F_Text_Grunt4PostBattle, MSGBOX_AUTOCLOSE
	end

AquaHideout_B2F_EventScript_Grunt4Defeated::
	msgbox AquaHideout_B2F_Text_Grunt4PostBattle, MSGBOX_DEFAULT
	release
	end

AquaHideout_B2F_EventScript_Grunt6::
	trainerbattle_single TRAINER_GRUNT_AQUA_HIDEOUT_6, AquaHideout_B2F_Text_Grunt6Intro, AquaHideout_B2F_Text_Grunt6Defeat
	msgbox AquaHideout_B2F_Text_Grunt6PostBattle, MSGBOX_AUTOCLOSE
	end

AquaHideout_B2F_EventScript_Grunt8::
	trainerbattle_single TRAINER_GRUNT_AQUA_HIDEOUT_8, AquaHideout_B2F_Text_Grunt8Intro, AquaHideout_B2F_Text_Grunt8Defeat
	msgbox AquaHideout_B2F_Text_Grunt8PostBattle, MSGBOX_AUTOCLOSE
	end

AquaHideout_B2F_Text_MattIntro:
	.string "Hehehe…\p"
	.string "Já chegou aqui, é?\n"
	.string "Subestimamos você!\p"
	.string "Mas acabou!\p"
	.string "Sou bem superior aos CAPANGAS que\n"
	.string "você viu até agora.\p"
	.string "Não estou enrolando pra ganhar tempo.\n"
	.string "Vou te esmagar!$"

AquaHideout_B2F_Text_MattDefeat:
	.string "Hehehe…\n"
	.string "Então eu também perdi…$"

AquaHideout_B2F_Text_OurBossGotThroughHisPreparations:
	.string "Hehehe!\p"
	.string "Enquanto eu brincava com você, o\n"
	.string "nosso CHEFE terminou os preparativos!$"

AquaHideout_B2F_Text_MattPostBattle:
	.string "Hehehe!\p"
	.string "O nosso CHEFE já partiu pra uma\n"
	.string "caverna no fundo do mar!\p"
	.string "Se for atrás dele, é melhor procurar\n"
	.string "no mar imenso depois de LILYCOVE.\p"
	.string "Mas será que vai achar?\n"
	.string "Hehehe!$"

AquaHideout_B2F_Text_Grunt4Intro:
	.string "Wahahah, cansei de esperar!\n"
	.string "Você também me deve uma batalha!$"

AquaHideout_B2F_Text_Grunt4Defeat:
	.string "Cansado de esperar…\n"
	.string "E derrotado, tonto…$"

AquaHideout_B2F_Text_Grunt4PostBattle:
	.string "CHEFE…\n"
	.string "Isso já basta?$"

AquaHideout_B2F_Text_Grunt6Intro:
	.string "Os painéis de teletransporte, o\n"
	.string "orgulho do ESCONDERIJO!\p"
	.string "Você não faz ideia de onde está,\n"
	.string "não é?\p"
	.string "Deixar o inimigo confuso e cansado,\n"
	.string "e aí atacar! Esse é o plano!$"

AquaHideout_B2F_Text_Grunt6Defeat:
	.string "Qual é a sua?\n"
	.string "Você nem se cansou!$"

AquaHideout_B2F_Text_Grunt6PostBattle:
	.string "Falando nisso… Não lembro onde\n"
	.string "guardei a BOLA MESTRA.\p"
	.string "Se eu não vigiar ela, o CHEFE vai\n"
	.string "me dar uma bronca…$"

AquaHideout_B2F_Text_Grunt8Intro:
	.string "Quando entrei na EQUIPE AQUA, a\n"
	.string "primeira coisa que tive que aprender\l"
	.string "foi como os painéis se ligavam.$"

AquaHideout_B2F_Text_Grunt8Defeat:
	.string "Estava ocupado demais pensando\n"
	.string "nos painéis…$"

AquaHideout_B2F_Text_Grunt8PostBattle:
	.string "Vou ter que aprender a batalhar\n"
	.string "melhor…$"
