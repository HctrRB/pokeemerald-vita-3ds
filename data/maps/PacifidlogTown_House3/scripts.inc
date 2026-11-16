PacifidlogTown_House3_MapScripts::
	.byte 0

PacifidlogTown_House3_EventScript_Trader::
	lock
	faceplayer
	goto_if_set FLAG_PACIFIDLOG_NPC_TRADE_COMPLETED, PacifidlogTown_House3_EventScript_TradeCompleted
	setvar VAR_0x8008, INGAME_TRADE_HORSEA
	copyvar VAR_0x8004, VAR_0x8008
	specialvar VAR_RESULT, GetInGameTradeSpeciesInfo
	copyvar VAR_0x8009, VAR_RESULT
	msgbox PacifidlogTown_House3_Text_WillingToTradeIt, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PacifidlogTown_House3_EventScript_DeclineTrade
	special ChoosePartyMon
	waitstate
	copyvar VAR_0x800A, VAR_0x8004
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, PacifidlogTown_House3_EventScript_DeclineTrade
	copyvar VAR_0x8005, VAR_0x800A
	specialvar VAR_RESULT, GetTradeSpecies
	copyvar VAR_0x800B, VAR_RESULT
	goto_if_ne VAR_RESULT, VAR_0x8009, PacifidlogTown_House3_EventScript_NotRequestedMon
	copyvar VAR_0x8004, VAR_0x8008
	copyvar VAR_0x8005, VAR_0x800A
	special CreateInGameTradePokemon
	special DoInGameTradeScene
	waitstate
	bufferspeciesname STR_VAR_1, VAR_0x8009
	msgbox PacifidlogTown_House3_Text_ItsSubtlyDifferentThankYou, MSGBOX_DEFAULT
	setflag FLAG_PACIFIDLOG_NPC_TRADE_COMPLETED
	release
	end

PacifidlogTown_House3_EventScript_DeclineTrade::
	msgbox PacifidlogTown_House3_Text_NotDesperateOrAnything, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House3_EventScript_NotRequestedMon::
	bufferspeciesname STR_VAR_1, VAR_0x8009
	msgbox PacifidlogTown_House3_Text_WontAcceptAnyLessThanRealMon, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House3_EventScript_TradeCompleted::
	msgbox PacifidlogTown_House3_Text_ReallyWantedToGetBagon, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House3_EventScript_Girl::
	msgbox PacifidlogTown_House3_Text_IsThatAPokedex, MSGBOX_NPC
	end

PacifidlogTown_House3_Text_WillingToTradeIt:
	.string "Olha só este {STR_VAR_2}!\p"
	.string "É o {STR_VAR_2} que peguei ontem pra\n"
	.string "comemorar o meu aniversário!\p"
	.string "Ah, já vi que você quer!\n"
	.string "Afinal, ele não tem preço!\p"
	.string "Vou te dizer uma coisa. Talvez eu\n"
	.string "troque ele por um {STR_VAR_1}.$"

PacifidlogTown_House3_Text_ItsSubtlyDifferentThankYou:
	.string "Ah, então isto é um {STR_VAR_1}?\p"
	.string "Parece um {STR_VAR_2}, mas é\n"
	.string "sutilmente diferente.\p"
	.string "Obrigado!$"

PacifidlogTown_House3_Text_WontAcceptAnyLessThanRealMon:
	.string "Não, não, não! Só aceito um\n"
	.string "{STR_VAR_1} de verdade!$"

PacifidlogTown_House3_Text_NotDesperateOrAnything:
	.string "Ah, então você não vai trocar?\p"
	.string "Tudo bem. Não estou desesperado\n"
	.string "pra trocar nem nada.$"

PacifidlogTown_House3_Text_ReallyWantedToGetBagon:
	.string "Sei que podia procurar um sozinho,\n"
	.string "mas…\p"
	.string "Mas eu queria muito um BAGON que\n"
	.string "outro TREINADOR pegou…$"

PacifidlogTown_House3_Text_IsThatAPokedex:
	.string "Isso é uma POKéDEX?\p"
	.string "Você conheceu um monte de POKéMON\n"
	.string "diferentes?\p"
	.string "Queria ser como você.$"

