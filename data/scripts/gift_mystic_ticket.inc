MysteryGiftScript_MysticTicket::
	setvaddress MysteryGiftScript_MysticTicket
	lock
	faceplayer
	vgoto_if_set FLAG_RECEIVED_MYSTIC_TICKET, MysticTicket_Obtained
	vgoto_if_set FLAG_CAUGHT_LUGIA, MysticTicket_Obtained
	vgoto_if_set FLAG_CAUGHT_HO_OH, MysticTicket_Obtained
	checkitem ITEM_MYSTIC_TICKET
	vgoto_if_eq VAR_RESULT, TRUE, MysticTicket_Obtained
	vmessage sText_MysticTicketForYou
	waitmessage
	waitbuttonpress
	checkitemspace ITEM_MYSTIC_TICKET
	vgoto_if_eq VAR_RESULT, FALSE, MysticTicket_NoBagSpace
	giveitem ITEM_MYSTIC_TICKET
	setflag FLAG_ENABLE_SHIP_NAVEL_ROCK
	setflag FLAG_RECEIVED_MYSTIC_TICKET
	vmessage sText_MysticTicketUseAtPort
	waitmessage
	waitbuttonpress
	release
	end

MysticTicket_NoBagSpace:
	vmessage sText_MysticTicketBagFull
	waitmessage
	waitbuttonpress
	release
	end

MysticTicket_Obtained:
	vmessage sText_MysticTicketThankYou
	waitmessage
	waitbuttonpress
	release
	end

sText_MysticTicketForYou:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.\p"
	.string "Você deve ser {PLAYER}.\n"
	.string "Tem um bilhete aqui pra você.$"

sText_MysticTicketUseAtPort:
	.string "Parece que é pra usar no porto\n"
	.string "da CIDADE DE LILYCOVE.\p"
	.string "Que tal ir lá descobrir\n"
	.string "do que se trata?$"

sText_MysticTicketThankYou:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.$"

sText_MysticTicketBagFull:
	.string "Ah, desculpe, {PLAYER}.\n"
	.string "O bolso de ITENS CHAVE tá cheio.\p"
	.string "Guarde alguma coisa no PC\n"
	.string "e volte pra buscar isto.$"
