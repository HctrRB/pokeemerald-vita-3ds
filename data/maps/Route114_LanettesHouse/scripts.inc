Route114_LanettesHouse_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route114_LanettesHouse_OnTransition
	.byte 0

Route114_LanettesHouse_OnTransition:
	setflag FLAG_LANDMARK_LANETTES_HOUSE
	end

Route114_LanettesHouse_EventScript_Lanette::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_DOLL_LANETTE, Route114_LanettesHouse_EventScript_OfferAdvice
	setflag FLAG_SYS_PC_LANETTE
	msgbox Route114_LanettesHouse_Text_EverythingClutteredKeepThis, MSGBOX_DEFAULT
	givedecoration DECOR_LOTAD_DOLL
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowNoRoomForDecor
	setflag FLAG_RECEIVED_DOLL_LANETTE
	release
	end

Route114_LanettesHouse_EventScript_OfferAdvice::
	msgbox Route114_LanettesHouse_Text_OrganizeYourBoxes, MSGBOX_DEFAULT
	release
	end

Route114_LanettesHouse_EventScript_Notebook::
	lockall
	msgbox Route114_LanettesHouse_Text_ResearchNotesPage1, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Route114_LanettesHouse_EventScript_NotebookPage2
	msgbox Route114_LanettesHouse_Text_ClosedTheNotebook, MSGBOX_DEFAULT
	releaseall
	end

Route114_LanettesHouse_EventScript_NotebookPage2::
	msgbox Route114_LanettesHouse_Text_ResearchNotesPage2, MSGBOX_YESNO
	call_if_eq VAR_RESULT, YES, Route114_LanettesHouse_EventScript_NotebookPage3
	releaseall
	end

Route114_LanettesHouse_EventScript_NotebookPage3::
	msgbox Route114_LanettesHouse_Text_ResearchNotesPage3, MSGBOX_DEFAULT
	return

Route114_LanettesHouse_EventScript_PC::
	msgbox Route114_LanettesHouse_Text_EmailFromBill, MSGBOX_SIGN
	end

Route114_LanettesHouse_Text_EverythingClutteredKeepThis:
	.string "LANETTE: Oh! {PLAYER}{KUN}!\p"
	.string "Desculpa a bagunça…\n"
	.string "Quando me empolgo com a pesquisa,\l"
	.string "fica tudo assim…\p"
	.string "Que vergonha… Por favor, guarde\n"
	.string "segredo em troca disto.$"

Route114_LanettesHouse_Text_OrganizeYourBoxes:
	.string "Posso dar uma dica sobre o meu\n"
	.string "Sistema de Armazenamento POKéMON?\p"
	.string "Organize as suas CAIXAS pra saber\n"
	.string "que POKéMON estão em cada uma.$"

Route114_LanettesHouse_Text_ResearchNotesPage1:
	.string "São as anotações de pesquisa da\n"
	.string "LANETTE. Falam sobre as CAIXAS.\p"
	.string "Cada CAIXA deve guardar 30 POKéMON.\p"
	.string "Cada TREINADOR deve poder guardar\n"
	.string "420 POKéMON no sistema do PC.\p"
	.string "Continuar lendo?$"

Route114_LanettesHouse_Text_ResearchNotesPage2:
	.string "Adicionar um sistema de marcas pra\n"
	.string "facilitar a organização dos POKéMON.\p"
	.string "O nome e o papel de parede de cada\n"
	.string "CAIXA poderão ser mudados, pra\l"
	.string "agradar os POKéMON guardados.\p"
	.string "Continuar lendo?$"

Route114_LanettesHouse_Text_ResearchNotesPage3:
	.string "Ao guardar um POKéMON, ele deve ir\n"
	.string "pra última CAIXA consultada.\p"
	.string "Se essa CAIXA estiver cheia, o\n"
	.string "POKéMON vai pra CAIXA seguinte.\p"
	.string "Ou seja, quando uma CAIXA é aberta,\n"
	.string "ela vira automaticamente a CAIXA\l"
	.string "que recebe os POKéMON.$"

Route114_LanettesHouse_Text_ClosedTheNotebook:
	.string "{PLAYER} fechou o caderno.$"

Route114_LanettesHouse_Text_EmailFromBill:
	.string "Tem um e-mail de alguém no PC.\p"
	.string "“… … … … … … …\p"
	.string "“O seu Sistema de Armazenamento é\n"
	.string "mais prático que o meu.\p"
	.string "“Tem muitas funções fáceis de usar\n"
	.string "que deixam ele divertido e útil.\p"
	.string "“Tenho orgulho de ter participado\n"
	.string "do desenvolvimento dele.\p"
	.string "“Espero que você continue pesquisando\n"
	.string "Sistemas de Armazenamento.\p"
	.string "“Do BILL\n"
	.string "… … … … … … … …”$"

