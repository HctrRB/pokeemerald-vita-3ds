DewfordTown_House2_MapScripts::
	.byte 0

DewfordTown_House2_EventScript_Man::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_SILK_SCARF, DewfordTown_House2_EventScript_ExplainSilkScarf
	msgbox DewfordTown_House2_Text_WantYouToHaveSilkScarf, MSGBOX_DEFAULT
	giveitem ITEM_SILK_SCARF
	goto_if_eq VAR_RESULT, FALSE, DewfordTown_House2_EventScript_NoRoomForScarf
	setflag FLAG_RECEIVED_SILK_SCARF
	release
	end

DewfordTown_House2_EventScript_NoRoomForScarf::
	msgbox DewfordTown_House2_Text_NoRoom, MSGBOX_DEFAULT
	release
	end

DewfordTown_House2_EventScript_ExplainSilkScarf::
	msgbox DewfordTown_House2_Text_ExplainSilkScarf, MSGBOX_DEFAULT
	release
	end

DewfordTown_House2_EventScript_Boy::
	msgbox DewfordTown_House2_Text_BrawlySoCool, MSGBOX_NPC
	end

DewfordTown_House2_Text_WantYouToHaveSilkScarf:
	.string "Encha os olhos com isto!\p"
	.string "É um LENÇO DE SEDA. É o que há de\n"
	.string "mais moderno na moda, viu!\p"
	.string "Ah, estou vendo seus olhos brilhando!\n"
	.string "Você aprecia o meu estilo deslumbrante!\p"
	.string "Ah, que amor!\n"
	.string "Tome. Quero que fique com ele!$"

DewfordTown_House2_Text_NoRoom:
	.string "Ah, não tem espaço?\p"
	.string "Então, escute bem, este LENÇO é\n"
	.string "indispensável! Eu venderia todos\l"
	.string "os meus itens pra ter um!$"

DewfordTown_House2_Text_ExplainSilkScarf:
	.string "O LENÇO DE SEDA aumenta a força dos\n"
	.string "golpes do tipo NORMAL.\p"
	.string "É um LENÇO maravilhoso que combina\n"
	.string "com quase todo POKéMON!$"

DewfordTown_House2_Text_BrawlySoCool:
	.string "Nossa, você atravessou o mar só pra\n"
	.string "visitar DEWFORD?\p"
	.string "Será que veio por causa do que\n"
	.string "ouviu falar do BRAWLY?\p"
	.string "Ele é tão legal…\n"
	.string "Todo mundo é fã dele.$"

