MossdeepCity_Mart_MapScripts::
	.byte 0

MossdeepCity_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart MossdeepCity_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
MossdeepCity_Mart_Pokemart:
	.short ITEM_ULTRA_BALL
	.short ITEM_NET_BALL
	.short ITEM_DIVE_BALL
	.short ITEM_HYPER_POTION
	.short ITEM_FULL_HEAL
	.short ITEM_REVIVE
	.short ITEM_MAX_REPEL
	.short ITEM_X_ATTACK
	.short ITEM_X_DEFEND
	pokemartlistend

MossdeepCity_Mart_EventScript_Woman::
	msgbox MossdeepCity_Mart_Text_ReviveIsFantastic, MSGBOX_NPC
	end

MossdeepCity_Mart_EventScript_Boy::
	msgbox MossdeepCity_Mart_Text_MaxRepelLastsLongest, MSGBOX_NPC
	end

MossdeepCity_Mart_EventScript_Sailor::
	msgbox MossdeepCity_Mart_Text_NetAndDiveBallsRare, MSGBOX_NPC
	end

MossdeepCity_Mart_Text_ReviveIsFantastic:
	.string "O REVIVER é fantástico!\p"
	.string "Dê pra um POKéMON desmaiado, e ele\n"
	.string "levanta.\p"
	.string "Mas cuidado: o REVIVER não recupera\n"
	.string "os PP gastos dos golpes.$"

MossdeepCity_Mart_Text_MaxRepelLastsLongest:
	.string "O REPELENTE MÁXIMO afasta todos os\n"
	.string "POKéMON fracos.\p"
	.string "De todos os REPELENTES, é o que\n"
	.string "dura mais.$"

MossdeepCity_Mart_Text_NetAndDiveBallsRare:
	.string "A BOLA TELA e a BOLA MERGULHO são\n"
	.string "POKé BOLAS raras, só de MOSSDEEP.\p"
	.string "A BOLA TELA funciona bem contra\n"
	.string "POKéMON de INSETO e de ÁGUA.\p"
	.string "A BOLA MERGULHO é melhor com\n"
	.string "POKéMON do fundo do mar.$"

