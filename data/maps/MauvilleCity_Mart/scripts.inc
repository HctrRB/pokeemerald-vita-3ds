MauvilleCity_Mart_MapScripts::
	.byte 0

MauvilleCity_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart MauvilleCity_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
MauvilleCity_Mart_Pokemart:
	.short ITEM_POKE_BALL
	.short ITEM_GREAT_BALL
	.short ITEM_SUPER_POTION
	.short ITEM_ANTIDOTE
	.short ITEM_PARALYZE_HEAL
	.short ITEM_AWAKENING
	.short ITEM_X_SPEED
	.short ITEM_X_ATTACK
	.short ITEM_X_DEFEND
	.short ITEM_GUARD_SPEC
	.short ITEM_DIRE_HIT
	.short ITEM_X_ACCURACY
	pokemartlistend

MauvilleCity_Mart_EventScript_ExpertM::
	msgbox MauvilleCity_Mart_Text_ItemsToTemporarilyElevateStats, MSGBOX_NPC
	end

MauvilleCity_Mart_EventScript_Man::
	msgbox MauvilleCity_Mart_Text_DecisionsDetermineBattle, MSGBOX_NPC
	end

MauvilleCity_Mart_Text_ItemsToTemporarilyElevateStats:
	.string "Tem itens que aumentam os atributos\n"
	.string "dos POKéMON por um tempo.\p"
	.string "Os que eu sei que se usam na batalha\n"
	.string "são ATAQUE X e DEFESA X…\p"
	.string "Acho que existem outros\n"
	.string "parecidos.$"

MauvilleCity_Mart_Text_DecisionsDetermineBattle:
	.string "Usar um golpe, ou usar um item\n"
	.string "no lugar…\p"
	.string "Acho que são as decisões de quem treina\n"
	.string "que decidem como a batalha termina.$"

