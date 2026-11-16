SootopolisCity_House3_MapScripts::
	.byte 0

SootopolisCity_House3_EventScript_Woman::
	lock
	faceplayer
	msgbox SootopolisCity_House3_Text_JuanHasManyFansDoYou, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, SootopolisCity_House3_EventScript_HaveFans
	msgbox SootopolisCity_House3_Text_LonesomeTryWorkingHarder, MSGBOX_DEFAULT
	release
	end

SootopolisCity_House3_EventScript_HaveFans::
	msgbox SootopolisCity_House3_Text_YouMustBePrettyStrong, MSGBOX_DEFAULT
	release
	end

SootopolisCity_House3_EventScript_Girl::
	msgbox SootopolisCity_House3_Text_TrainerFanClubWasWild, MSGBOX_NPC
	end

SootopolisCity_House3_Text_JuanHasManyFansDoYou:
	.string "Você é TREINADOR POKéMON, não é?\p"
	.string "O JUAN de SOOTOPOLIS tem muitos fãs.\n"
	.string "Mais até que o aluno dele, o WALLACE!\p"
	.string "Você tem algum?$"

SootopolisCity_House3_Text_YouMustBePrettyStrong:
	.string "Ah, então você deve ser bem forte.$"

SootopolisCity_House3_Text_LonesomeTryWorkingHarder:
	.string "Ai, ai…\n"
	.string "Que solidão.\p"
	.string "Se esforce mais um pouco pra ganhar\n"
	.string "uns fãs.$"

SootopolisCity_House3_Text_TrainerFanClubWasWild:
	.string "Tem fãs fiéis que vêm até de fora\n"
	.string "de HOENN.\p"
	.string "Foi uma loucura quando fui ao\n"
	.string "FÃ-CLUBE DOS TREINADORES em LILYCOVE.$"

