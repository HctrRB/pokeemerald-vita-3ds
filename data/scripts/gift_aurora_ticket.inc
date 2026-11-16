MysteryGiftScript_AuroraTicket::
	setvaddress MysteryGiftScript_AuroraTicket
	lock
	faceplayer
	vgoto_if_set FLAG_RECEIVED_AURORA_TICKET, AuroraTicket_Obtained
	vgoto_if_set FLAG_BATTLED_DEOXYS, AuroraTicket_Obtained
	checkitem ITEM_AURORA_TICKET
	vgoto_if_eq VAR_RESULT, TRUE, AuroraTicket_Obtained
	vmessage sText_AuroraTicketForYou
	waitmessage
	waitbuttonpress
	checkitemspace ITEM_AURORA_TICKET
	vgoto_if_eq VAR_RESULT, FALSE, AuroraTicket_NoBagSpace
	giveitem ITEM_AURORA_TICKET
	setflag FLAG_ENABLE_SHIP_BIRTH_ISLAND
	setflag FLAG_RECEIVED_AURORA_TICKET
	vmessage sText_AuroraTicketUseAtPort
	waitmessage
	waitbuttonpress
	release
	end

AuroraTicket_NoBagSpace:
	vmessage sText_AuroraTicketBagFull
	waitmessage
	waitbuttonpress
	release
	end

AuroraTicket_Obtained:
	vmessage sText_AuroraTicketThankYou
	waitmessage
	waitbuttonpress
	release
	end

sText_AuroraTicketForYou:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.\p"
	.string "Você deve ser {PLAYER}.\n"
	.string "Tem um bilhete aqui pra você.$"

sText_AuroraTicketUseAtPort:
	.string "Parece que é pra usar no porto\n"
	.string "da CIDADE DE LILYCOVE.\p"
	.string "Que tal ir lá descobrir\n"
	.string "do que se trata?$"

sText_AuroraTicketThankYou:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.$"

sText_AuroraTicketBagFull:
	.string "Ah, desculpe, {PLAYER}.\n"
	.string "O bolso de ITENS CHAVE tá cheio.\p"
	.string "Guarde alguma coisa no PC\n"
	.string "e volte pra buscar isto.$"
