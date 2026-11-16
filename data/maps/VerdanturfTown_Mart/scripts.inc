VerdanturfTown_Mart_MapScripts::
	.byte 0

VerdanturfTown_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart VerdanturfTown_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
VerdanturfTown_Mart_Pokemart:
	.short ITEM_GREAT_BALL
	.short ITEM_NEST_BALL
	.short ITEM_SUPER_POTION
	.short ITEM_ANTIDOTE
	.short ITEM_PARALYZE_HEAL
	.short ITEM_AWAKENING
	.short ITEM_BURN_HEAL
	.short ITEM_ICE_HEAL
	.short ITEM_REPEL
	.short ITEM_X_SPECIAL
	.short ITEM_FLUFFY_TAIL
	pokemartlistend

VerdanturfTown_Mart_EventScript_Boy::
	msgbox VerdanturfTown_Mart_Text_XSpecialIsCrucial, MSGBOX_NPC
	end

VerdanturfTown_Mart_EventScript_ExpertF::
	msgbox VerdanturfTown_Mart_Text_NoStrategyGuidesForBattleTent, MSGBOX_NPC
	end

VerdanturfTown_Mart_EventScript_Lass::
	msgbox VerdanturfTown_Mart_Text_NestBallOnWeakenedPokemon, MSGBOX_NPC
	end

VerdanturfTown_Mart_Text_XSpecialIsCrucial:
	.string "Em qualquer batalha POKéMON, o\n"
	.string "ESPECIAL X é fundamental.\p"
	.string "Aumenta a força de alguns golpes,\n"
	.string "mesmo que só por uma batalha.$"

VerdanturfTown_Mart_Text_NoStrategyGuidesForBattleTent:
	.string "Parece que não vendem guias de\n"
	.string "estratégia pra TENDA DE BATALHA…\p"
	.string "Acho que cada um tem que contar\n"
	.string "com a própria cabeça, afinal…$"

VerdanturfTown_Mart_Text_NestBallOnWeakenedPokemon:
	.string "A BOLA DE NINHO funciona melhor em\n"
	.string "POKéMON enfraquecidos.\p"
	.string "Só dá pra comprar em\n"
	.string "VERDANTURF.$"

