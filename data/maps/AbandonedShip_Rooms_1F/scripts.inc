AbandonedShip_Rooms_1F_MapScripts::
	.byte 0

AbandonedShip_Rooms_1F_EventScript_Gentleman::
	msgbox AbandonedShip_Rooms_1F_Text_TakingALookAround, MSGBOX_NPC
	end

AbandonedShip_Rooms_1F_EventScript_Demetrius::
	trainerbattle_single TRAINER_DEMETRIUS, AbandonedShip_Rooms_1F_Text_DemetriusIntro, AbandonedShip_Rooms_1F_Text_DemetriusDefeat
	msgbox AbandonedShip_Rooms_1F_Text_DemetriusPostBattle, MSGBOX_AUTOCLOSE
	end

AbandonedShip_Rooms_1F_EventScript_Thalia::
	trainerbattle_single TRAINER_THALIA_1, AbandonedShip_Rooms_1F_Text_ThaliaIntro, AbandonedShip_Rooms_1F_Text_ThaliaDefeat, AbandonedShip_Rooms_1F_EventScript_RegisterThalia
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, AbandonedShip_Rooms_1F_EventScript_ThaliaRematch
	msgbox AbandonedShip_Rooms_1F_Text_ThaliaPostBattle, MSGBOX_DEFAULT
	release
	end

AbandonedShip_Rooms_1F_EventScript_RegisterThalia::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox AbandonedShip_Rooms_1F_Text_ThaliaRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_THALIA_1
	release
	end

AbandonedShip_Rooms_1F_EventScript_ThaliaRematch::
	trainerbattle_rematch TRAINER_THALIA_1, AbandonedShip_Rooms_1F_Text_ThaliaRematchIntro, AbandonedShip_Rooms_1F_Text_ThaliaRematchDefeat
	msgbox AbandonedShip_Rooms_1F_Text_ThaliaPostRematch, MSGBOX_AUTOCLOSE
	end

AbandonedShip_Rooms_1F_Text_TakingALookAround:
	.string "Navios assim são raros, então estou\n"
	.string "dando uma olhada.\p"
	.string "Humhum…\n"
	.string "Parece que há outras cabines…$"

AbandonedShip_Rooms_1F_Text_ThaliaIntro:
	.string "O que te trouxe até aqui? Que\n"
	.string "curiosidade, hein!$"

AbandonedShip_Rooms_1F_Text_ThaliaDefeat:
	.string "Além de curiosidade, tem força…$"

AbandonedShip_Rooms_1F_Text_ThaliaPostBattle:
	.string "O homem da cabine ao lado…\p"
	.string "Diz que só está passeando, mas\n"
	.string "sei não.$"

AbandonedShip_Rooms_1F_Text_ThaliaRegister:
	.string "Que TREINADOR forte!\n"
	.string "Vou te registrar de lembrança!$"

AbandonedShip_Rooms_1F_Text_ThaliaRematchIntro:
	.string "O que te fez voltar? Que curiosidade,\n"
	.string "hein!$"

AbandonedShip_Rooms_1F_Text_ThaliaRematchDefeat:
	.string "Forte demais, não?$"

AbandonedShip_Rooms_1F_Text_ThaliaPostRematch:
	.string "Tenho certeza de que aquele homem\n"
	.string "apronta alguma! Ele é muito suspeito!$"

AbandonedShip_Rooms_1F_Text_DemetriusIntro:
	.string "Aaah!\n"
	.string "Me acharam! …Hã?$"

AbandonedShip_Rooms_1F_Text_DemetriusDefeat:
	.string "Ah, você não é a minha mãe.$"

AbandonedShip_Rooms_1F_Text_DemetriusPostBattle:
	.string "Aprontei e a minha mãe vai brigar,\n"
	.string "então fugi. Não conta onde estou!$"

