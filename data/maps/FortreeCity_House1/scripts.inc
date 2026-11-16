FortreeCity_House1_MapScripts::
	.byte 0

FortreeCity_House1_EventScript_Trader::
	lock
	faceplayer
	goto_if_set FLAG_FORTREE_NPC_TRADE_COMPLETED, FortreeCity_House1_EventScript_TradeCompleted
	setvar VAR_0x8008, INGAME_TRADE_PLUSLE
	copyvar VAR_0x8004, VAR_0x8008
	specialvar VAR_RESULT, GetInGameTradeSpeciesInfo
	copyvar VAR_0x8009, VAR_RESULT
	msgbox FortreeCity_House1_Text_YouWillTradeWontYou, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, FortreeCity_House1_EventScript_DeclineTrade
	special ChoosePartyMon
	waitstate
	copyvar VAR_0x800A, VAR_0x8004
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, FortreeCity_House1_EventScript_DeclineTrade
	copyvar VAR_0x8005, VAR_0x800A
	specialvar VAR_RESULT, GetTradeSpecies
	copyvar VAR_0x800B, VAR_RESULT
	goto_if_ne VAR_RESULT, VAR_0x8009, FortreeCity_House1_EventScript_NotRequestedMon
	copyvar VAR_0x8004, VAR_0x8008
	copyvar VAR_0x8005, VAR_0x800A
	special CreateInGameTradePokemon
	special DoInGameTradeScene
	waitstate
	bufferspeciesname STR_VAR_1, VAR_0x8009
	msgbox FortreeCity_House1_Text_MonYouTakeCare, MSGBOX_DEFAULT
	setflag FLAG_FORTREE_NPC_TRADE_COMPLETED
	release
	end

FortreeCity_House1_EventScript_DeclineTrade::
	msgbox FortreeCity_House1_Text_YouWontTradeMe, MSGBOX_DEFAULT
	release
	end

FortreeCity_House1_EventScript_NotRequestedMon::
	bufferspeciesname STR_VAR_1, VAR_0x8009
	msgbox FortreeCity_House1_Text_ThisIsntAMon, MSGBOX_DEFAULT
	release
	end

FortreeCity_House1_EventScript_TradeCompleted::
	msgbox FortreeCity_House1_Text_GoingToMakeVolbeatStrong, MSGBOX_DEFAULT
	release
	end

FortreeCity_House1_EventScript_ExpertF::
	msgbox FortreeCity_House1_Text_TradingMemoriesWithOthers, MSGBOX_NPC
	end

FortreeCity_House1_EventScript_Zigzagoon::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZIGZAGOON, CRY_MODE_NORMAL
	msgbox FortreeCity_House1_Text_Zigzagoon, MSGBOX_DEFAULT
	waitmoncry
	release
	end

FortreeCity_House1_Text_YouWillTradeWontYou:
	.string "Uooooaaar! Eu preciso!\n"
	.string "Tenho que ter um {STR_VAR_1}!\l"
	.string "Faço qualquer coisa!\p"
	.string "…Hã… Você ouviu?\n"
	.string "O meu grito do fundo do coração?\p"
	.string "Depois disso, você vai trocar o seu\n"
	.string "{STR_VAR_1} pelo meu {STR_VAR_2},\l"
	.string "não vai?$"

FortreeCity_House1_Text_MonYouTakeCare:
	.string "Isso aí, beleza!\p"
	.string "{STR_VAR_1}, bem-vindo!\n"
	.string "{STR_VAR_2}, se cuida!$"

FortreeCity_House1_Text_ThisIsntAMon:
	.string "Hã, não, acho que não.\n"
	.string "Isso não é um {STR_VAR_1}.$"

FortreeCity_House1_Text_YouWontTradeMe:
	.string "Não? Não vai trocar comigo?\n"
	.string "Mesmo depois de eu abrir o coração?$"

FortreeCity_House1_Text_GoingToMakeVolbeatStrong:
	.string "De agora em diante, vou deixar o\n"
	.string "VOLBEAT superforte!\p"
	.string "Espero que você faça o mesmo com\n"
	.string "o PLUSLE!$"

FortreeCity_House1_Text_TradingMemoriesWithOthers:
	.string "Trocar POKéMON com os outros…\p"
	.string "É como trocar as suas próprias\n"
	.string "lembranças com outras pessoas.$"

FortreeCity_House1_Text_Zigzagoon:
	.string "ZIGZAGOON: Gumomoh?$"

