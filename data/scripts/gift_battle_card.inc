MysteryGiftScript_BattleCard::
	setvaddress MysteryGiftScript_BattleCard
	vgoto_if_set FLAG_MYSTERY_GIFT_DONE, MysteryGiftScript_BattleCardInfo
	setorcopyvar VAR_RESULT, GET_CARD_BATTLES_WON
	specialvar VAR_0x8008, GetMysteryGiftCardStat
	vgoto_if_ne VAR_0x8008, REQUIRED_CARD_BATTLES, MysteryGiftScript_BattleCardInfo
	lock
	faceplayer
	vmessage sText_MysteryGiftBattleCountCard_WonPrize
	waitmessage
	waitbuttonpress
	giveitem ITEM_POTION
	release
	setflag FLAG_MYSTERY_GIFT_DONE
	end

MysteryGiftScript_BattleCardInfo:
	lock
	faceplayer
	vmessage sText_MysteryGiftBattleCountCard
	waitmessage
	waitbuttonpress
	release
	end

sText_MysteryGiftBattleCountCard:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.\p"
	.string "O seu CARTÃO DE BATALHAS guarda\n"
	.string "o placar das batalhas contra\l"
	.string "TREINADORES com o mesmo CARTÃO.\p"
	.string "Procure e enfrente TREINADORES\n"
	.string "que tenham o mesmo CARTÃO.\p"
	.string "Dá pra ver o ranking geral\n"
	.string "nas NOTÍCIAS.\p"
	.string "Experimente!$"

sText_MysteryGiftBattleCountCard_WonPrize:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.\p"
	.string "Parabéns!\p"
	.string "Você ganhou um prêmio por\n"
	.string "vencer três batalhas!\p"
	.string "Tomara que isso te anime a\n"
	.string "batalhar ainda mais.$"
