MtPyre_4F_MapScripts::
	.byte 0

/* Seems like the scripts for the 4F and 5F were swapped*/

MtPyre_5F_EventScript_Atsushi::
	trainerbattle_single TRAINER_ATSUSHI, MtPyre_5F_Text_AtsushiIntro, MtPyre_5F_Text_AtsushiDefeat
	msgbox MtPyre_5F_Text_AtsushiPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_5F_Text_AtsushiIntro:
	.string "Mestre…\n"
	.string "Por favor, veja o meu progresso!$"

MtPyre_5F_Text_AtsushiDefeat:
	.string "Mestre…\n"
	.string "Por favor, me perdoe!$"

MtPyre_5F_Text_AtsushiPostBattle:
	.string "Até eu melhorar, o meu mestre, que\n"
	.string "descansa aqui, nunca terá paz…$"

