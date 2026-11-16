FallarborTown_Mart_MapScripts::
	.byte 0

FallarborTown_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart FallarborTown_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
FallarborTown_Mart_Pokemart:
	.short ITEM_GREAT_BALL
	.short ITEM_SUPER_POTION
	.short ITEM_ANTIDOTE
	.short ITEM_PARALYZE_HEAL
	.short ITEM_ESCAPE_ROPE
	.short ITEM_SUPER_REPEL
	.short ITEM_X_SPECIAL
	.short ITEM_X_SPEED
	.short ITEM_X_ATTACK
	.short ITEM_X_DEFEND
	.short ITEM_DIRE_HIT
	.short ITEM_GUARD_SPEC
	pokemartlistend

FallarborTown_Mart_EventScript_Woman::
	msgbox FallarborTown_Mart_Text_DecidingSkittyEvolve, MSGBOX_NPC
	end

FallarborTown_Mart_EventScript_PokefanM::
	msgbox FallarborTown_Mart_Text_SellNuggetIFound, MSGBOX_NPC
	end

FallarborTown_Mart_EventScript_Skitty::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_SKITTY, CRY_MODE_NORMAL
	msgbox FallarborTown_Mart_Text_Skitty, MSGBOX_DEFAULT
	waitmoncry
	release
	end

FallarborTown_Mart_Text_DecidingSkittyEvolve:
	.string "Não consigo decidir se faço o meu\n"
	.string "SKITTY evoluir ou não.\p"
	.string "É só usar esta PEDRA DA LUA, mas é\n"
	.string "tão difícil decidir…\p"
	.string "Se ele evoluir, vai ficar muito mais\n"
	.string "forte.\p"
	.string "Mas também vai ficar tão diferente.$"

FallarborTown_Mart_Text_Skitty:
	.string "SKITTY: Miau?$"

FallarborTown_Mart_Text_SellNuggetIFound:
	.string "Esta PEPITA que achei aqui…\n"
	.string "Acho que vou ter que vender, já\l"
	.string "que não serve pra mais nada.$"

