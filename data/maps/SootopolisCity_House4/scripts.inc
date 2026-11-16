SootopolisCity_House4_MapScripts::
	.byte 0

SootopolisCity_House4_EventScript_Man::
	msgbox SootopolisCity_House4_Text_AncientTreasuresWaitingInSea, MSGBOX_NPC
	end

SootopolisCity_House4_EventScript_Woman::
	msgbox SootopolisCity_House4_Text_StrollUnderwaterWithPokemon, MSGBOX_NPC
	end

SootopolisCity_House4_EventScript_Azumarill::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_AZUMARILL, CRY_MODE_NORMAL
	msgbox SootopolisCity_House4_Text_Azumarill, MSGBOX_DEFAULT
	waitmoncry
	release
	end

SootopolisCity_House4_Text_AncientTreasuresWaitingInSea:
	.string "Escuta, que vou te contar uma coisa\n"
	.string "boa.\p"
	.string "Dizem que tem uma ruína antiga no\n"
	.string "mar aqui perto.\p"
	.string "Pode ter tesouros lá embaixo\n"
	.string "esperando pra serem achados.$"

SootopolisCity_House4_Text_StrollUnderwaterWithPokemon:
	.string "Tesouros antigos…\p"
	.string "Seria bom se existissem, mas, mesmo\n"
	.string "que não, seria tão lindo dar um\l"
	.string "passeio debaixo d'água com os\l"
	.string "meus POKéMON.$"

SootopolisCity_House4_Text_Azumarill:
	.string "AZUMARILL: Maruriii.$"

