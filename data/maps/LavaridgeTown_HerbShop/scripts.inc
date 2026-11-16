LavaridgeTown_HerbShop_MapScripts::
	.byte 0

LavaridgeTown_HerbShop_EventScript_Clerk::
	lock
	faceplayer
	message LavaridgeTown_HerbShop_Text_WelcomeToHerbShop
	waitmessage
	pokemart LavaridgeTown_HerbShop_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
LavaridgeTown_HerbShop_Pokemart:
	.short ITEM_ENERGY_POWDER
	.short ITEM_ENERGY_ROOT
	.short ITEM_HEAL_POWDER
	.short ITEM_REVIVAL_HERB
	pokemartlistend

LavaridgeTown_HerbShop_EventScript_ExpertM::
	msgbox LavaridgeTown_HerbShop_Text_HerbalMedicineWorksButMonWillDislike, MSGBOX_NPC
	end

LavaridgeTown_HerbShop_EventScript_OldMan::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_CHARCOAL, LavaridgeTown_HerbShop_EventScript_ExplainCharcoal
	msgbox LavaridgeTown_HerbShop_Text_YouveComeToLookAtHerbalMedicine, MSGBOX_DEFAULT
	giveitem ITEM_CHARCOAL
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_CHARCOAL
	release
	end

LavaridgeTown_HerbShop_EventScript_ExplainCharcoal::
	msgbox LavaridgeTown_HerbShop_Text_ExplainCharcoal, MSGBOX_DEFAULT
	release
	end

LavaridgeTown_HerbShop_Text_WelcomeToHerbShop:
	.string "Bem-vindo à LOJA DE ERVAS, remédios\n"
	.string "eficazes e baratinhos!$"

LavaridgeTown_HerbShop_Text_YouveComeToLookAtHerbalMedicine:
	.string "Veio ver os remédios de ervas de\n"
	.string "LAVARIDGE?\p"
	.string "Isso é muito louvável.\p"
	.string "Gostei de você! Tome isto!$"

LavaridgeTown_HerbShop_Text_ExplainCharcoal:
	.string "Esse CARVÃO que te dei é usado pra\n"
	.string "fazer remédios de ervas.\p"
	.string "E faz maravilhas quando um POKéMON\n"
	.string "segura ele.\p"
	.string "Aumenta o poder dos golpes de FOGO.$"

LavaridgeTown_HerbShop_Text_HerbalMedicineWorksButMonWillDislike:
	.string "Remédio de erva funciona que é uma\n"
	.string "beleza. Mas o seu POKéMON vai ficar\l"
	.string "chateado com você. Deve ser amargo!$"

