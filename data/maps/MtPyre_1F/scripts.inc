MtPyre_1F_MapScripts::
	.byte 0

MtPyre_1F_EventScript_CleanseTagWoman::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_CLEANSE_TAG, MtPyre_1F_EventScript_ReceivedCleanseTag
	msgbox MtPyre_1F_Text_TakeThisForYourOwnGood, MSGBOX_DEFAULT
	giveitem ITEM_CLEANSE_TAG
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_CLEANSE_TAG
	release
	end

MtPyre_1F_EventScript_ReceivedCleanseTag::
	msgbox MtPyre_1F_Text_ExplainCleanseTag, MSGBOX_DEFAULT
	release
	end

MtPyre_1F_EventScript_PokefanF::
	msgbox MtPyre_1F_Text_ComeToPayRespects, MSGBOX_NPC
	end

MtPyre_1F_EventScript_Man::
	msgbox MtPyre_1F_Text_RestingPlaceOfZigzagoon, MSGBOX_NPC
	end

MtPyre_1F_Text_TakeThisForYourOwnGood:
	.string "Todo tipo de ser vagueia pelas\n"
	.string "encostas do MONTE PYRE…\p"
	.string "Não se sabe o que pode acontecer.\n"
	.string "Leve isto. É para o seu bem.$"

MtPyre_1F_Text_ExplainCleanseTag:
	.string "Deixe um POKéMON segurar esse\n"
	.string "TALISMÃ PURO.\p"
	.string "Ele ajuda a afastar POKéMON\n"
	.string "selvagens.$"

MtPyre_1F_Text_ComeToPayRespects:
	.string "Veio prestar homenagem aos espíritos\n"
	.string "dos POKéMON que partiram?\p"
	.string "Você deve gostar muito dos seus\n"
	.string "POKéMON.$"

MtPyre_1F_Text_RestingPlaceOfZigzagoon:
	.string "Aqui descansa o meu ZIGZAGOON.\n"
	.string "Eu gostava tanto dele…$"

