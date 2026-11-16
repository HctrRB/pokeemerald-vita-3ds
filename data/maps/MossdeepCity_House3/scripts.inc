MossdeepCity_House3_MapScripts::
	.byte 0

MossdeepCity_House3_EventScript_SuperRodFisherman::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_SUPER_ROD, MossdeepCity_House3_EventScript_ReceivedSuperRod
	msgbox MossdeepCity_House3_Text_YouWantSuperRod, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MossdeepCity_House3_EventScript_DeclineSuperRod
	msgbox MossdeepCity_House3_Text_SuperRodIsSuper, MSGBOX_DEFAULT
	giveitem ITEM_SUPER_ROD
	setflag FLAG_RECEIVED_SUPER_ROD
	msgbox MossdeepCity_House3_Text_TryDroppingRodInWater, MSGBOX_DEFAULT
	release
	end

MossdeepCity_House3_EventScript_ReceivedSuperRod::
	msgbox MossdeepCity_House3_Text_GoAfterSeafloorPokemon, MSGBOX_DEFAULT
	release
	end

MossdeepCity_House3_EventScript_DeclineSuperRod::
	msgbox MossdeepCity_House3_Text_DontYouLikeToFish, MSGBOX_DEFAULT
	release
	end

MossdeepCity_House3_Text_YouWantSuperRod:
	.string "E aí, TREINADOR!\n"
	.string "A SUPERVARA é super mesmo!\p"
	.string "Fale o que quiser, mas esta belezinha\n"
	.string "pesca POKéMON do fundo do mar!\p"
	.string "E aí?\n"
	.string "Você quer, não quer?$"

MossdeepCity_House3_Text_SuperRodIsSuper:
	.string "Isso, isso!\n"
	.string "Afinal, a SUPERVARA é super mesmo!$"

MossdeepCity_House3_Text_TryDroppingRodInWater:
	.string "Onde tiver água, jogue a VARA e veja\n"
	.string "o que morde!$"

MossdeepCity_House3_Text_DontYouLikeToFish:
	.string "Hã?\n"
	.string "Você não gosta de pescar?$"

MossdeepCity_House3_Text_GoAfterSeafloorPokemon:
	.string "Vá atrás dos POKéMON do fundo do\n"
	.string "mar com a sua SUPERVARA.$"

