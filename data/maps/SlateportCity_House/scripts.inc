SlateportCity_House_MapScripts::
	.byte 0

SlateportCity_House_EventScript_PokefanM::
	msgbox SlateportCity_House_Text_NatureToDoWithStatGains, MSGBOX_NPC
	end

SlateportCity_House_EventScript_Girl::
	msgbox SlateportCity_House_Text_MustBeGoingToBattleTent, MSGBOX_NPC
	end

SlateportCity_House_Text_NatureToDoWithStatGains:
	.string "O meu POKéMON tem natureza APRESSADA.\p"
	.string "Ele tem mais VELOCIDADE que os\n"
	.string "meus outros POKéMON.\p"
	.string "Talvez a natureza tenha a ver com o\n"
	.string "crescimento dos atributos dos POKéMON.$"

SlateportCity_House_Text_MustBeGoingToBattleTent:
	.string "Você treina POKéMON, não treina?\p"
	.string "Se veio à CIDADE DE SLATEPORT,\n"
	.string "deve estar indo à TENDA DE BATALHA.$"

