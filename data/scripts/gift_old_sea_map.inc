MysteryGiftScript_OldSeaMap::
	setvaddress MysteryGiftScript_OldSeaMap
	lock
	faceplayer
	vgoto_if_set FLAG_RECEIVED_OLD_SEA_MAP, OldSeaMap_Obtained
	vgoto_if_set FLAG_CAUGHT_MEW, OldSeaMap_Obtained
	checkitem ITEM_OLD_SEA_MAP
	vgoto_if_eq VAR_RESULT, TRUE, OldSeaMap_Obtained
	vmessage sText_MysteryGiftOldSeaMapForYou
	waitmessage
	waitbuttonpress
	checkitemspace ITEM_OLD_SEA_MAP
	vgoto_if_eq VAR_RESULT, FALSE, OldSeaMap_NoBagSpace
	giveitem ITEM_OLD_SEA_MAP
	setflag FLAG_ENABLE_SHIP_FARAWAY_ISLAND
	setflag FLAG_RECEIVED_OLD_SEA_MAP
	vmessage sText_MysteryGiftOldSeaMapUseAtPort
	waitmessage
	waitbuttonpress
	release
	end

OldSeaMap_NoBagSpace:
	vmessage sText_MysteryGiftOldSeaMapBagFull
	waitmessage
	waitbuttonpress
	release
	end

OldSeaMap_Obtained:
	vmessage sText_MysteryGiftOldSeaMapThankYou
	waitmessage
	waitbuttonpress
	release
	end

sText_MysteryGiftOldSeaMapForYou:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.\p"
	.string "Só pra confirmar: você é\n"
	.string "{PLAYER}?\p"
	.string "A gente recebeu este MAPA\n"
	.string "ANTIGO endereçado a você.$"

sText_MysteryGiftOldSeaMapUseAtPort:
	.string "Parece que é pra usar no porto\n"
	.string "da CIDADE DE LILYCOVE.\p"
	.string "Que tal ir lá descobrir\n"
	.string "do que se trata?$"

sText_MysteryGiftOldSeaMapThankYou:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.$"

sText_MysteryGiftOldSeaMapBagFull:
	.string "Ah, desculpe, {PLAYER}.\n"
	.string "O bolso de ITENS CHAVE tá cheio.\p"
	.string "Guarde alguma coisa no PC\n"
	.string "e volte pra buscar isto.$"
