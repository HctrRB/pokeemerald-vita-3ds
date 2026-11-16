AquaHideout_1F_MapScripts::
	.byte 0

/* The below two entrance guards give hints about what to do to progress the story*/
AquaHideout_1F_EventScript_HideoutEntranceGrunt1::
	lock
	faceplayer
	goto_if_set FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT, AquaHideout_1F_EventScript_SlateportHint1
	goto_if_set FLAG_RECEIVED_RED_OR_BLUE_ORB, AquaHideout_1F_EventScript_MagmaHideoutHint1
	msgbox AquaHideout_1F_Text_OurBossIsSnatchingSomething, MSGBOX_DEFAULT
	release
	end

AquaHideout_1F_EventScript_MagmaHideoutHint1::
	msgbox AquaHideout_1F_Text_WhereMightMagmaHideoutBe, MSGBOX_DEFAULT
	release
	end

AquaHideout_1F_EventScript_SlateportHint1::
	msgbox AquaHideout_1F_Text_BossWentToJackASubmarine, MSGBOX_DEFAULT
	release
	end

AquaHideout_1F_EventScript_HideoutEntranceGrunt2::
	lock
	faceplayer
	goto_if_set FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT, AquaHideout_1F_EventScript_SlateportHint2
	goto_if_set FLAG_RECEIVED_RED_OR_BLUE_ORB, AquaHideout_1F_EventScript_MagmaHideoutHint2
	msgbox AquaHideout_1F_Text_BossIsOnRoute122, MSGBOX_DEFAULT
	release
	end

AquaHideout_1F_EventScript_MagmaHideoutHint2::
	msgbox AquaHideout_1F_Text_TeamMagmaAtMtChimney, MSGBOX_DEFAULT
	release
	end

AquaHideout_1F_EventScript_SlateportHint2::
	msgbox AquaHideout_1F_Text_BossIsInSlateportCity, MSGBOX_DEFAULT
	release
	end

AquaHideout_1F_EventScript_Grunt1::
	trainerbattle_single TRAINER_GRUNT_AQUA_HIDEOUT_1, AquaHideout_1F_Text_Grunt1Intro, AquaHideout_1F_Text_Grunt1Defeat, AquaHideout_1F_EventScript_Grunt1Defeated
	msgbox AquaHideout_1F_Text_Grunt1PostBattle, MSGBOX_AUTOCLOSE
	end

AquaHideout_1F_EventScript_Grunt1Defeated::
	msgbox AquaHideout_1F_Text_Grunt1PostBattle, MSGBOX_DEFAULT
	release
	end

AquaHideout_1F_Text_OurBossIsSnatchingSomething:
	.string "Quê? Quê? O que quer com a\n"
	.string "EQUIPE AQUA?\p"
	.string "O nosso CHEFE não está! Foi pegar\n"
	.string "uma coisa importante!\p"
	.string "… …\n"
	.string "Aonde ele foi?\p"
	.string "Wahaha! Acha mesmo que eu ia contar\n"
	.string "uma coisa tão importante?$"

AquaHideout_1F_Text_WhereMightMagmaHideoutBe:
	.string "Quê? Quê?\n"
	.string "Você é CAPANGA da EQUIPE MAGMA?\p"
	.string "Ouvi dizer que a EQUIPE MAGMA quer\n"
	.string "despertar um POKéMON incrível no\l"
	.string "ESCONDERIJO deles.\p"
	.string "Mas onde será o ESCONDERIJO deles?$"

AquaHideout_1F_Text_BossWentToJackASubmarine:
	.string "Quê? Quê? O que quer com a\n"
	.string "EQUIPE AQUA?\p"
	.string "O nosso CHEFE não está!\n"
	.string "Foi roubar um submarino!\p"
	.string "… …\n"
	.string "Aonde ele foi?\p"
	.string "Wahaha! Acha mesmo que eu ia contar\n"
	.string "uma coisa tão importante?$"

AquaHideout_1F_Text_BossIsOnRoute122:
	.string "Quê? Quê? O que quer com a\n"
	.string "EQUIPE AQUA?\p"
	.string "O nosso CHEFE não está! Foi pro\n"
	.string "MONTE PYRE, na ROTA 122!\p"
	.string "… …\n"
	.string "Por que ele foi?\p"
	.string "Wahaha! Acha mesmo que eu ia contar\n"
	.string "uma coisa tão importante?$"

AquaHideout_1F_Text_TeamMagmaAtMtChimney:
	.string "Quê? Quê?\n"
	.string "Você é CAPANGA da EQUIPE MAGMA?\p"
	.string "Ouvi dizer que a EQUIPE MAGMA está\n"
	.string "atrás de um POKéMON incrível no\l"
	.string "MONTE CHIMNEY.\p"
	.string "Mas como será esse POKéMON?$"

AquaHideout_1F_Text_BossIsInSlateportCity:
	.string "Quê? Quê? O que quer com a\n"
	.string "EQUIPE AQUA?\p"
	.string "O nosso CHEFE não está!\n"
	.string "Foi pra CIDADE DE SLATEPORT!\p"
	.string "… …\n"
	.string "Por que ele foi?\p"
	.string "Wahaha! Acha mesmo que eu ia contar\n"
	.string "uma coisa tão importante?$"

AquaHideout_1F_Text_Grunt1Intro:
	.string "Aiaiai!\n"
	.string "Figura suspeita à vista!$"

AquaHideout_1F_Text_Grunt1Defeat:
	.string "Grrrrr…\n"
	.string "Perdi!$"

AquaHideout_1F_Text_Grunt1PostBattle:
	.string "Perdi pela EQUIPE, mas fiz a minha\n"
	.string "parte…$"

