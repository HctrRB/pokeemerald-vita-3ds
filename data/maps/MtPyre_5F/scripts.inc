MtPyre_5F_MapScripts::
	.byte 0

/* Seems like the scripts for the 4F and 5F were swapped*/

MtPyre_4F_EventScript_Tasha::
	trainerbattle_single TRAINER_TASHA, MtPyre_4F_Text_TashaIntro, MtPyre_4F_Text_TashaDefeat
	msgbox MtPyre_4F_Text_TashaPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_4F_Text_TashaIntro:
	.string "Eu adoro tudo o que é horripilante…\n"
	.string "É como uma doença…\p"
	.string "Quando estou aqui…\n"
	.string "Tremo de medo…$"

MtPyre_4F_Text_TashaDefeat:
	.string "Perder, eu não gosto…$"

MtPyre_4F_Text_TashaPostBattle:
	.string "Quero ver coisas medonhas…\n"
	.string "Não consigo ir embora…\p"
	.string "Fique…\n"
	.string "Não quer ficar comigo?$"

