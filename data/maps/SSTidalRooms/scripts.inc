SSTidalRooms_MapScripts::
	.byte 0

SSTidalRooms_EventScript_SnatchGiver::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_TM_SNATCH, SSTidalRooms_EventScript_ExplainSnatch
	msgbox SSTidalRooms_Text_NotSuspiciousTakeThis, MSGBOX_DEFAULT
	giveitem ITEM_TM_SNATCH
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM_SNATCH
	msgbox SSTidalRooms_Text_ExplainSnatch, MSGBOX_DEFAULT
	release
	end

SSTidalRooms_EventScript_ExplainSnatch::
	msgbox SSTidalRooms_Text_ExplainSnatch, MSGBOX_DEFAULT
	release
	end

SSTidalRooms_EventScript_Bed::
	lockall
	msgbox SSTidalRooms_Text_TakeRestOnBed, MSGBOX_DEFAULT
	closemessage
	call Common_EventScript_OutOfCenterPartyHeal
	call SSTidalRooms_EventScript_ProgessCruiseAfterBed
	releaseall
	end

SSTidalRooms_EventScript_Colton::
	trainerbattle_single TRAINER_COLTON, SSTidalRooms_Text_ColtonIntro, SSTidalRooms_Text_ColtonDefeat
	msgbox SSTidalRooms_Text_ColtonPostBattle, MSGBOX_AUTOCLOSE
	end

SSTidalRooms_EventScript_Micah::
	trainerbattle_single TRAINER_MICAH, SSTidalRooms_Text_MicahIntro, SSTidalRooms_Text_MicahDefeat
	msgbox SSTidalRooms_Text_MicahPostBattle, MSGBOX_AUTOCLOSE
	end

SSTidalRooms_EventScript_Thomas::
	trainerbattle_single TRAINER_THOMAS, SSTidalRooms_Text_ThomasIntro, SSTidalRooms_Text_ThomasDefeat
	msgbox SSTidalRooms_Text_ThomasPostBattle, MSGBOX_AUTOCLOSE
	end

SSTidalRooms_EventScript_Jed::
	trainerbattle_double TRAINER_LEA_AND_JED, SSTidalRooms_Text_JedIntro, SSTidalRooms_Text_JedDefeat, SSTidalRooms_Text_JedNotEnoughMons
	msgbox SSTidalRooms_Text_JedPostBattle, MSGBOX_AUTOCLOSE
	end

SSTidalRooms_EventScript_Lea::
	trainerbattle_double TRAINER_LEA_AND_JED, SSTidalRooms_Text_LeaIntro, SSTidalRooms_Text_LeaDefeat, SSTidalRooms_Text_LeaNotEnoughMons
	msgbox SSTidalRooms_Text_LeaPostBattle, MSGBOX_AUTOCLOSE
	end

SSTidalRooms_EventScript_Garret::
	trainerbattle_single TRAINER_GARRET, SSTidalRooms_Text_GarretIntro, SSTidalRooms_Text_GarretDefeat
	msgbox SSTidalRooms_Text_GarretPostBattle, MSGBOX_AUTOCLOSE
	end

SSTidalRooms_EventScript_Naomi::
	trainerbattle_single TRAINER_NAOMI, SSTidalRooms_Text_NaomiIntro, SSTidalRooms_Text_NaomiDefeat
	msgbox SSTidalRooms_Text_NaomiPostBattle, MSGBOX_AUTOCLOSE
	end

SSTidalRooms_Text_TakeRestOnBed:
	.string "Tem uma cama…\n"
	.string "Vamos descansar.$"

SSTidalRooms_Text_ColtonIntro:
	.string "Vou muito à CIDADE DE LILYCOVE.\p"
	.string "Gosto de assistir aos CONCURSOS,\n"
	.string "sabe?$"

SSTidalRooms_Text_ColtonDefeat:
	.string "Que batalha agradável!$"

SSTidalRooms_Text_ColtonPostBattle:
	.string "Fico tão animado imaginando que\n"
	.string "POKéMON vou ver no próximo CONCURSO.\l"
	.string "A expectativa é uma emoção!$"

SSTidalRooms_Text_MicahIntro:
	.string "Os seus amigos são fortes?$"

SSTidalRooms_Text_MicahDefeat:
	.string "Os seus amigos são fortes mesmo.$"

SSTidalRooms_Text_MicahPostBattle:
	.string "Amigos não precisam ser humanos.\n"
	.string "Pra mim, os POKéMON são amigos\l"
	.string "preciosos!$"

SSTidalRooms_Text_ThomasIntro:
	.string "Criança…\n"
	.string "Você bateu na porta?$"

SSTidalRooms_Text_ThomasDefeat:
	.string "Uma derrota deve ser aceita sem\n"
	.string "pressa nem pânico.$"

SSTidalRooms_Text_ThomasPostBattle:
	.string "Nunca perder a compostura é o código\n"
	.string "de conduta do CAVALHEIRO.$"

SSTidalRooms_Text_JedIntro:
	.string "JED: Fico meio tímido, mas…\n"
	.string "Vamos mostrar o poder do nosso amor!$"

SSTidalRooms_Text_JedDefeat:
	.string "JED: Ai…$"

SSTidalRooms_Text_JedPostBattle:
	.string "JED: É a primeira vez que o poder do\n"
	.string "nosso amor não venceu!\l"
	.string "Você deve ser um TREINADOR incrível!$"

SSTidalRooms_Text_JedNotEnoughMons:
	.string "JED: Você só tem um POKéMON?\n"
	.string "Não é solitário demais?$"

SSTidalRooms_Text_LeaIntro:
	.string "LEA: Me sinto meio boba, mas…\n"
	.string "Vamos mostrar o poder do nosso amor!$"

SSTidalRooms_Text_LeaDefeat:
	.string "LEA: Ah, poxa!$"

SSTidalRooms_Text_LeaPostBattle:
	.string "LEA: Não acredito!\n"
	.string "O poder do nosso amor falhou…\l"
	.string "Você deve ser um TREINADOR incrível!$"

SSTidalRooms_Text_LeaNotEnoughMons:
	.string "LEA: Eu queria batalhar…\n"
	.string "Mas você nem tem dois POKéMON…$"

SSTidalRooms_Text_GarretIntro:
	.string "Ah, chegou na hora certa.\p"
	.string "Estou entediado, sabe.\n"
	.string "Você pode me divertir.$"

SSTidalRooms_Text_GarretDefeat:
	.string "…Está bom assim.$"

SSTidalRooms_Text_GarretPostBattle:
	.string "Talvez eu peça pro papai comprar um\n"
	.string "iate pra mim.\l"
	.string "Um iate pra mim e pros POKéMON!$"

SSTidalRooms_Text_NaomiIntro:
	.string "Ah, que TREINADOR adorável.\n"
	.string "Aceita um chá?\l"
	.string "Ou prefere batalhar?$"

SSTidalRooms_Text_NaomiDefeat:
	.string "Entendo.\n"
	.string "Você gosta de agito.$"

SSTidalRooms_Text_NaomiPostBattle:
	.string "Um cruzeiro pelo mundo num navio de\n"
	.string "luxo tem os seus encantos, devo dizer…\p"
	.string "Mas admito que viajar por HOENN de\n"
	.string "balsa tem lá o seu charme.$"

SSTidalRooms_Text_NotSuspiciousTakeThis:
	.string "Hã… Oi! E-eu não estou suspeito!\n"
	.string "Hã… Pode ficar com isto! De graça!\p"
	.string "É… Juro que não ROUBEI de ninguém!\n"
	.string "Eu nunca faria isso!\l"
	.string "Está limpo! Pode usar!$"

SSTidalRooms_Text_ExplainSnatch:
	.string "ARREBATADOR rouba os efeitos bons de\n"
	.string "certos golpes antes que o oponente\l"
	.string "ou o aliado possa usá-los.$"

