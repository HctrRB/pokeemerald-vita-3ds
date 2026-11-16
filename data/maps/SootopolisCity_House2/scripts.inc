SootopolisCity_House2_MapScripts::
	.byte 0

SootopolisCity_House2_EventScript_ExpertF::
	lock
	faceplayer
	msgbox SootopolisCity_House2_Text_DidYouKnowAboutMtPyreOrbs, MSGBOX_YESNO
	call_if_eq VAR_RESULT, YES, SootopolisCity_House2_EventScript_KnowAboutOrbs
	call_if_eq VAR_RESULT, NO, SootopolisCity_House2_EventScript_DontKnowAboutOrbs
	release
	end

SootopolisCity_House2_EventScript_KnowAboutOrbs::
	msgbox SootopolisCity_House2_Text_YesTwoOrbsSideBySide, MSGBOX_DEFAULT
	return

SootopolisCity_House2_EventScript_DontKnowAboutOrbs::
	msgbox SootopolisCity_House2_Text_OughtToVisitAndSee, MSGBOX_DEFAULT
	return

SootopolisCity_House2_Text_DidYouKnowAboutMtPyreOrbs:
	.string "O MONTE PYRE…\p"
	.string "No topo, há dois orbes lado a lado.\n"
	.string "Você sabia?$"

SootopolisCity_House2_Text_YesTwoOrbsSideBySide:
	.string "Sim, dois orbes lado a lado…\p"
	.string "Ver os dois juntos…\n"
	.string "De algum jeito, acalma…$"

SootopolisCity_House2_Text_OughtToVisitAndSee:
	.string "É mesmo?\n"
	.string "Talvez devesse ir lá ver…$"

