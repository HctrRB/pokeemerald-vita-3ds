MagmaHideout_4F_MapScripts::
	.byte 0

MagmaHideout_4F_EventScript_Maxie::
	lockall
	playbgm MUS_ENCOUNTER_MAGMA, FALSE
	msgbox MagmaHideout_4F_Text_MaxieAwakenGroudon, MSGBOX_DEFAULT
	closemessage
	delay 20
	setvar VAR_RESULT, 1
	playse SE_M_DETECT
	dofieldeffectsparkle 18, 42, 0
	waitfieldeffect FLDEFF_SPARKLE
	setvar VAR_RESULT, 1
	playfanfare MUS_AWAKEN_LEGEND
	playse SE_ORB
	special DoOrbEffect
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	delay 150
	removeobject LOCALID_MAGMA_HIDEOUT_4F_GROUDON_SLEEPING
	addobject LOCALID_MAGMA_HIDEOUT_4F_GROUDON
	waitstate
	delay 60
	applymovement LOCALID_MAGMA_HIDEOUT_4F_GROUDON, MagmaHideout_4F_Movement_GroudonApproach
	waitmovement 0
	special FadeOutOrbEffect
	waitstate
	setvar VAR_0x8004, 1  /* vertical pan*/
	setvar VAR_0x8005, 1  /* horizontal pan*/
	setvar VAR_0x8006, 8  /* num shakes*/
	setvar VAR_0x8007, 5  /* shake delay*/
	special ShakeCamera
	waitstate
	applymovement LOCALID_MAGMA_HIDEOUT_4F_GROUDON, MagmaHideout_4F_Movement_GroudonExit
	waitmovement 0
	removeobject LOCALID_MAGMA_HIDEOUT_4F_GROUDON
	delay 4
	setvar VAR_0x8004, 2  /* vertical pan*/
	setvar VAR_0x8005, 2  /* horizontal pan*/
	setvar VAR_0x8006, 8  /* num shakes*/
	setvar VAR_0x8007, 5  /* shake delay*/
	special ShakeCamera
	waitstate
	delay 30
	applymovement LOCALID_MAGMA_HIDEOUT_4F_MAXIE, MagmaHideout_4F_Movement_MaxieLookAround
	waitmovement 0
	msgbox MagmaHideout_4F_Text_MaxieGroudonWhatsWrong, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	delay 30
	applymovement LOCALID_MAGMA_HIDEOUT_4F_MAXIE, Common_Movement_FacePlayer
	waitmovement 0
	msgbox MagmaHideout_4F_Text_MaxieOhItWasYou, MSGBOX_DEFAULT
	closemessage
	trainerbattle_no_intro TRAINER_MAXIE_MAGMA_HIDEOUT, MagmaHideout_4F_Text_MaxieDefeat
	msgbox MagmaHideout_4F_Text_MaxieImGoingAfterGroudon, MSGBOX_DEFAULT
	closemessage
	clearflag FLAG_HIDE_SLATEPORT_CITY_CAPTAIN_STERN
	clearflag FLAG_HIDE_SLATEPORT_CITY_GABBY_AND_TY
	setvar VAR_SLATEPORT_CITY_STATE, 1
	setflag FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT
	setvar VAR_SLATEPORT_HARBOR_STATE, 1
	fadescreen FADE_TO_BLACK
	removeobject LOCALID_MAGMA_HIDEOUT_4F_MAXIE
	removeobject LOCALID_MAGMA_HIDEOUT_4F_GRUNT_1
	removeobject LOCALID_MAGMA_HIDEOUT_4F_GRUNT_2
	removeobject LOCALID_MAGMA_HIDEOUT_4F_GRUNT_3
	removeobject LOCALID_MAGMA_HIDEOUT_4F_TABITHA
	setflag FLAG_HIDE_MAGMA_HIDEOUT_GRUNTS
	fadescreen FADE_FROM_BLACK
	releaseall
	end

MagmaHideout_4F_Movement_GroudonApproach:
	delay_16
	delay_16
	walk_slow_down
	delay_16
	delay_16
	delay_16
	walk_slow_down
	delay_16
	delay_16
	delay_16
	step_end

MagmaHideout_4F_Movement_GroudonExit:
	slide_up
	slide_up
	step_end

MagmaHideout_4F_Movement_MaxieLookAround:
	face_left
	delay_16
	face_right
	delay_16
	face_left
	delay_16
	face_right
	delay_16
	face_up
	delay_16
	delay_16
	step_end

MagmaHideout_4F_EventScript_Grunt11::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_11, MagmaHideout_4F_Text_Grunt11Intro, MagmaHideout_4F_Text_Grunt11Defeat
	msgbox MagmaHideout_4F_Text_Grunt11PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_4F_EventScript_Grunt12::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_12, MagmaHideout_4F_Text_Grunt12Intro, MagmaHideout_4F_Text_Grunt12Defeat
	msgbox MagmaHideout_4F_Text_Grunt12PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_4F_EventScript_Grunt13::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_13, MagmaHideout_4F_Text_Grunt13Intro, MagmaHideout_4F_Text_Grunt13Defeat
	msgbox MagmaHideout_4F_Text_Grunt13PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_4F_EventScript_Tabitha::
	trainerbattle_single TRAINER_TABITHA_MAGMA_HIDEOUT, MagmaHideout_4F_Text_TabithaIntro, MagmaHideout_4F_Text_TabithaDefeat
	msgbox MagmaHideout_4F_Text_TabithaPostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_4F_Text_Grunt11Intro:
	.string "Eu também quero ver o GROUDON, mas\n"
	.string "nem o rabo dele me deixam ver…\p"
	.string "Isso me deixa muito frustrado.\p"
	.string "Ah, não!\n"
	.string "Falei do GROUDON!$"

MagmaHideout_4F_Text_Grunt11Defeat:
	.string "Acho que não dá pra vencer sem\n"
	.string "ter a cabeça fria…$"

MagmaHideout_4F_Text_Grunt11PostBattle:
	.string "Será que o GROUDON tem rabo?$"

MagmaHideout_4F_Text_Grunt12Intro:
	.string "Fuhahaha!\n"
	.string "Logo! Muito em breve!\l"
	.string "O nosso grande objetivo vai se cumprir!$"

MagmaHideout_4F_Text_Grunt12Defeat:
	.string "Grrr…\n"
	.string "Cheguei tão longe pra isto?$"

MagmaHideout_4F_Text_Grunt12PostBattle:
	.string "Senhor MAXIE!\n"
	.string "Um intruso está indo até aí!$"

MagmaHideout_4F_Text_Grunt13Intro:
	.string "Ainda não acabou!\n"
	.string "Não vai passar por mim tão fácil!$"

MagmaHideout_4F_Text_Grunt13Defeat:
	.string "Fui tão fácil de derrubar assim?$"

MagmaHideout_4F_Text_Grunt13PostBattle:
	.string "V-vamos, mais uma batalha…$"

MagmaHideout_4F_Text_TabithaIntro:
	.string "Hehehe!\n"
	.string "Já que chegou até aqui, eu conto!\p"
	.string "Isso mesmo!\n"
	.string "Lá na frente, o GROUDON dorme!\p"
	.string "O MAXIE foi até o GROUDON agora\n"
	.string "mesmo!\p"
	.string "Ele vai despertar já, já!\n"
	.string "Hehe! Hehehe!$"

MagmaHideout_4F_Text_TabithaDefeat:
	.string "Derrubado de novo…\n"
	.string "Hehe…$"

MagmaHideout_4F_Text_TabithaPostBattle:
	.string "…E, enquanto você perdia tempo\n"
	.string "comigo, o MAXIE já deve ter\l"
	.string "despertado o GROUDON…$"

MagmaHideout_4F_Text_MaxieAwakenGroudon:
	.string "MAXIE: GROUDON…\p"
	.string "Nada conseguia te despertar do seu\n"
	.string "sono banhado em magma…\p"
	.string "Este ORBE AZUL é o que você buscava.\n"
	.string "Não é?\p"
	.string "Eu te trouxe o ORBE AZUL.\n"
	.string "Que o brilho dele te desperte!\p"
	.string "E me mostre…\n"
	.string "Me mostre todo o seu poder!$"

MagmaHideout_4F_Text_MaxieGroudonWhatsWrong:
	.string "MAXIE: GROUDON!\n"
	.string "O que houve?\p"
	.string "O ORBE AZUL não era a chave?\p"
	.string "GROUDON!\n"
	.string "Aonde você foi…$"

MagmaHideout_4F_Text_MaxieOhItWasYou:
	.string "MAXIE: Ah, então foi você?\p"
	.string "Vi você bisbilhotando por aí sem\n"
	.string "ser convidado…\p"
	.string "Agora entendi!\n"
	.string "Você deve ter feito alguma trapaça!$"

MagmaHideout_4F_Text_MaxieDefeat:
	.string "Como você lida tão bem com\n"
	.string "POKéMON?$"

MagmaHideout_4F_Text_MaxieImGoingAfterGroudon:
	.string "MAXIE: Deve haver um motivo para o\n"
	.string "GROUDON ter fugido…\p"
	.string "É isso que você quer dizer,\n"
	.string "não é?\p"
	.string "… … … … … …\n"
	.string "… … … … … …\p"
	.string "Hunf…\n"
	.string "Acha que eu não sabia?\p"
	.string "Sem o GROUDON, não preciso mais\n"
	.string "deste vulcão maldito.\p"
	.string "Vou atrás do GROUDON. É aqui que\n"
	.string "nos separamos!$"
