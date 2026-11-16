RustboroCity_CuttersHouse_MapScripts::
	.byte 0

RustboroCity_CuttersHouse_EventScript_Cutter::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_HM_CUT, RustboroCity_CuttersHouse_EventScript_ExplainCut
	msgbox RustboroCity_CuttersHouse_Text_YouCanPutThisHMToGoodUse, MSGBOX_DEFAULT
	giveitem ITEM_HM_CUT
	setflag FLAG_RECEIVED_HM_CUT
	msgbox RustboroCity_CuttersHouse_Text_ExplainCut, MSGBOX_DEFAULT
	release
	end

RustboroCity_CuttersHouse_EventScript_ExplainCut::
	msgbox RustboroCity_CuttersHouse_Text_ExplainCut, MSGBOX_DEFAULT
	release
	end

RustboroCity_CuttersHouse_EventScript_Lass::
	msgbox RustboroCity_CuttersHouse_Text_DadHelpedClearLandOfTrees, MSGBOX_NPC
	end

RustboroCity_CuttersHouse_Text_YouCanPutThisHMToGoodUse:
	.string "Essa cara de determinação…\n"
	.string "Esse jeito ágil de se mexer…\l"
	.string "E os seus POKéMON bem treinados…\p"
	.string "Dá pra ver que você treina POKéMON\n"
	.string "com talento!\p"
	.string "Não, espera, não diga nada.\n"
	.string "Só de olhar eu já sei.\p"
	.string "Tenho certeza de que você vai saber\n"
	.string "usar bem esta MÁQUINA OCULTA.\p"
	.string "Nada de modéstia ou vergonha.\n"
	.string "Vai, pode pegar!$"

RustboroCity_CuttersHouse_Text_ExplainCut:
	.string "Essa MÁQUINA OCULTA, ou MO, tem\n"
	.string "o golpe CORTAR.\p"
	.string "Um golpe de MO pode ser usado pelos\n"
	.string "POKéMON fora das batalhas.\p"
	.string "Qualquer POKéMON que aprendeu CORTAR\n"
	.string "derruba árvores finas, se você já\l"
	.string "tiver a INSÍGNIA DA PEDRA.\p"
	.string "E, diferente de uma MT, uma MO pode\n"
	.string "ser usada mais de uma vez.$"

RustboroCity_CuttersHouse_Text_DadHelpedClearLandOfTrees:
	.string "Quando estavam aumentando a cidade de\n"
	.string "RUSTBORO, meu pai ajudou.\p"
	.string "Ele fez os POKéMON dele usarem CORTAR\n"
	.string "pra limpar as árvores do terreno.$"

