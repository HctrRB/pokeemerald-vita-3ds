FortreeCity_Mart_MapScripts::
	.byte 0

FortreeCity_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart FortreeCity_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
FortreeCity_Mart_Pokemart:
	.short ITEM_GREAT_BALL
	.short ITEM_ULTRA_BALL
	.short ITEM_SUPER_POTION
	.short ITEM_HYPER_POTION
	.short ITEM_ANTIDOTE
	.short ITEM_PARALYZE_HEAL
	.short ITEM_AWAKENING
	.short ITEM_REVIVE
	.short ITEM_SUPER_REPEL
	.short ITEM_WOOD_MAIL
	pokemartlistend

FortreeCity_Mart_EventScript_Woman::
	msgbox FortreeCity_Mart_Text_SuperRepelBetter, MSGBOX_NPC
	end

FortreeCity_Mart_EventScript_Girl::
	msgbox FortreeCity_Mart_Text_StockUpOnItems, MSGBOX_NPC
	end

FortreeCity_Mart_EventScript_Boy::
	msgbox FortreeCity_Mart_Text_RareCandyMakesMonGrow, MSGBOX_NPC
	end

FortreeCity_Mart_Text_SuperRepelBetter:
	.string "O SUPER REPELENTE dura bastante e\n"
	.string "resolve o problema.\p"
	.string "É bem melhor que um REPELENTE\n"
	.string "comum.$"

FortreeCity_Mart_Text_StockUpOnItems:
	.string "Eu sempre compro mais itens do que\n"
	.string "sei que vou precisar.\p"
	.string "Nunca se sabe o que pode acontecer.\n"
	.string "Melhor prevenir que remediar!$"

FortreeCity_Mart_Text_RareCandyMakesMonGrow:
	.string "Um DOCE RARO faz um POKéMON subir\n"
	.string "um nível na hora.$"

