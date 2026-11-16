MtPyre_6F_MapScripts::
	.byte 0

MtPyre_6F_EventScript_Valerie::
	trainerbattle_single TRAINER_VALERIE_1, MtPyre_6F_Text_ValerieIntro, MtPyre_6F_Text_ValerieDefeat, MtPyre_6F_EventScript_RegisterValerie
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, MtPyre_6F_EventScript_RematchValerie
	msgbox MtPyre_6F_Text_ValeriePostBattle, MSGBOX_DEFAULT
	release
	end

MtPyre_6F_EventScript_RegisterValerie::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox MtPyre_6F_Text_ValerieRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_VALERIE_1
	release
	end

MtPyre_6F_EventScript_RematchValerie::
	trainerbattle_rematch TRAINER_VALERIE_1, MtPyre_6F_Text_ValerieRematchIntro, MtPyre_6F_Text_ValerieRematchDefeat
	msgbox MtPyre_6F_Text_ValeriePostRematch, MSGBOX_AUTOCLOSE
	end

MtPyre_6F_EventScript_Cedric::
	trainerbattle_single TRAINER_CEDRIC, MtPyre_6F_Text_CedricIntro, MtPyre_6F_Text_CedricDefeat
	msgbox MtPyre_6F_Text_CedricPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_6F_Text_ValerieIntro:
	.string "Quando estou aqui…\n"
	.string "Um poder estranho flui em mim…$"

MtPyre_6F_Text_ValerieDefeat:
	.string "O poder está se esvaindo…$"

MtPyre_6F_Text_ValeriePostBattle:
	.string "Talvez o poder venha dos espíritos\n"
	.string "dos POKéMON que dormem inquietos aqui…$"

MtPyre_6F_Text_ValerieRegister:
	.string "Fufufu… Perdi a batalha, mas…\n"
	.string "Tenho esta pequena habilidade…\p"
	.string "Sem nem encostar no seu POKéNAV…\n"
	.string "Hiá!$"

MtPyre_6F_Text_ValerieRematchIntro:
	.string "Atrás de você…\n"
	.string "O que é aquilo…$"

MtPyre_6F_Text_ValerieRematchDefeat:
	.string "Alguma coisa sumiu…$"

MtPyre_6F_Text_ValeriePostRematch:
	.string "Os POKéMON que descansam aqui…\n"
	.string "Às vezes, eles brincam…$"

MtPyre_6F_Text_CedricIntro:
	.string "Você se perdeu?\n"
	.string "Não tema, pois eu estou aqui!$"

MtPyre_6F_Text_CedricDefeat:
	.string "Você não tinha se perdido?$"

MtPyre_6F_Text_CedricPostBattle:
	.string "Achei que um TREINADOR perdido ia\n"
	.string "entrar em pânico e ser fácil de vencer.\p"
	.string "Isso é sujo, e não vou tentar de novo…$"

