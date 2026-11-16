FortreeCity_DecorationShop_MapScripts::
	.byte 0

FortreeCity_DecorationShop_EventScript_PokefanM::
	msgbox FortreeCity_DecorationShop_Text_MerchandiseSentToPC, MSGBOX_NPC
	end

FortreeCity_DecorationShop_EventScript_Girl::
	msgbox FortreeCity_DecorationShop_Text_BuyingDeskForDolls, MSGBOX_NPC
	end

FortreeCity_DecorationShop_EventScript_ClerkDesks::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemartdecoration FortreeCity_DecorationShop_PokemartDecor_Desks
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
FortreeCity_DecorationShop_PokemartDecor_Desks:
	.short DECOR_SMALL_DESK
	.short DECOR_POKEMON_DESK
	.short DECOR_HEAVY_DESK
	.short DECOR_RAGGED_DESK
	.short DECOR_COMFORT_DESK
	.short DECOR_BRICK_DESK
	.short DECOR_CAMP_DESK
	.short DECOR_HARD_DESK
	pokemartlistend

FortreeCity_DecorationShop_EventScript_ClerkChairs::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemartdecoration FortreeCity_DecorationShop_PokemartDecor_Chairs
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
FortreeCity_DecorationShop_PokemartDecor_Chairs:
	.short DECOR_SMALL_CHAIR
	.short DECOR_POKEMON_CHAIR
	.short DECOR_HEAVY_CHAIR
	.short DECOR_RAGGED_CHAIR
	.short DECOR_COMFORT_CHAIR
	.short DECOR_BRICK_CHAIR
	.short DECOR_CAMP_CHAIR
	.short DECOR_HARD_CHAIR
	pokemartlistend

FortreeCity_DecorationShop_Text_MerchandiseSentToPC:
	.string "O que você compra aqui vai direto\n"
	.string "pro seu PC.\p"
	.string "Incrível! Queria que me entregassem\n"
	.string "em casa desse jeito também.$"

FortreeCity_DecorationShop_Text_BuyingDeskForDolls:
	.string "Vou comprar uma mesa bonita e botar\n"
	.string "as minhas BONECAS fofas em cima.\p"
	.string "Senão, quando eu decorar a minha\n"
	.string "BASE SECRETA, as BONECAS vão se\l"
	.string "sujar ou pegar farpas.$"

