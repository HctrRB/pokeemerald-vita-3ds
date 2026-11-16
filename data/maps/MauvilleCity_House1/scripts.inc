MauvilleCity_House1_MapScripts::
	.byte 0

MauvilleCity_House1_EventScript_RockSmashDude::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_HM_ROCK_SMASH, MauvilleCity_House1_EventScript_ReceivedRockSmash
	msgbox MauvilleCity_House1_Text_ImRockSmashDudeTakeThis, MSGBOX_DEFAULT
	giveitem ITEM_HM_ROCK_SMASH
	setflag FLAG_RECEIVED_HM_ROCK_SMASH
	setflag FLAG_HIDE_ROUTE_111_ROCK_SMASH_TIP_GUY
	msgbox MauvilleCity_House1_Text_ExplainRockSmash, MSGBOX_DEFAULT
	release
	end

MauvilleCity_House1_EventScript_ReceivedRockSmash::
	msgbox MauvilleCity_House1_Text_MonCanFlyOutOfSmashedRock, MSGBOX_DEFAULT
	release
	end

MauvilleCity_House1_Text_ImRockSmashDudeTakeThis:
	.string "Uhuu!\p"
	.string "Ouvi dizer que me chamam de HOMEM\n"
	.string "DO ESMAGA PEDRA, mas acho isso meio\l"
	.string "humilhante.\p"
	.string "Acho que mereço mais respeito, tipo,\n"
	.string "CARA DO ESMAGA PEDRA.\p"
	.string "Uhuu!\p"
	.string "Enfim, os seus POKéMON parecem bem\n"
	.string "fortes.\p"
	.string "Gostei!\n"
	.string "Tome, pegue esta MÁQUINA OCULTA!$"

MauvilleCity_House1_Text_ExplainRockSmash:
	.string "Essa MO tem ESMAGA PEDRA.\p"
	.string "Se você topar com pedras enormes\n"
	.string "bloqueando o caminho…\p"
	.string "Bom, use esse golpe de MO e esmague\n"
	.string "elas pra fora do caminho!\p"
	.string "Isso aí! Esmague as pedras, eu digo!\n"
	.string "Uhuu!$"

MauvilleCity_House1_Text_MonCanFlyOutOfSmashedRock:
	.string "Ah, é, se você esmaga uma pedra, um\n"
	.string "POKéMON pode sair voando de lá.\p"
	.string "Uhuu!$"

