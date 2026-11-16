GraniteCave_1F_MapScripts::
	.byte 0

GraniteCave_1F_EventScript_Hiker::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_HM_FLASH, GraniteCave_1F_EventScript_ReceivedFlash
	msgbox GraniteCave_1F_Text_GetsDarkAheadHereYouGo, MSGBOX_DEFAULT
	giveitem ITEM_HM_FLASH
	setflag FLAG_RECEIVED_HM_FLASH
	msgbox GraniteCave_1F_Text_ExplainFlash, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_EventScript_ReceivedFlash::
	msgbox GraniteCave_1F_Text_ExplainFlash, MSGBOX_DEFAULT
	release
	end

GraniteCave_1F_Text_GetsDarkAheadHereYouGo:
	.string "Ei, você.\n"
	.string "Mais pra frente fica muito escuro.\l"
	.string "Vai ser difícil explorar.\p"
	.string "Aquele cara que passou aqui antes…\n"
	.string "STEVEN, acho que era isso.\p"
	.string "Ele sabia usar CLARÃO, então deve\n"
	.string "estar bem, mas…\p"
	.string "Bom, pra nós, ALPINISTAS, ajudar quem\n"
	.string "a gente encontra é o nosso lema.\p"
	.string "Tome, vou passar isto pra você.$"

GraniteCave_1F_Text_ExplainFlash:
	.string "Ensine o golpe oculto CLARÃO a um\n"
	.string "POKéMON e use.\p"
	.string "Ilumina até a escuridão total\n"
	.string "das cavernas.\p"
	.string "Mas, pra usar, você precisa da INSÍGNIA\n"
	.string "do GINÁSIO POKéMON de DEWFORD.$"

