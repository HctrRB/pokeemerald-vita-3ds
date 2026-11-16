AbandonedShip_Rooms2_1F_MapScripts::
	.byte 0

AbandonedShip_Rooms2_1F_EventScript_Dan::
	trainerbattle_double TRAINER_KIRA_AND_DAN_1, AbandonedShip_Rooms2_1F_Text_DanIntro, AbandonedShip_Rooms2_1F_Text_DanDefeat, AbandonedShip_Rooms2_1F_Text_DanNotEnoughMons, AbandonedShip_Rooms2_1F_EventScript_RegisterDan
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, AbandonedShip_Rooms2_1F_EventScript_DanRematch
	msgbox AbandonedShip_Rooms2_1F_Text_DanPostBattle, MSGBOX_DEFAULT
	release
	end

AbandonedShip_Rooms2_1F_EventScript_RegisterDan::
	msgbox AbandonedShip_Rooms2_1F_Text_KiraRegister, MSGBOX_DEFAULT  /* Kira speaks for both when registering KiraAndDan*/
	register_matchcall TRAINER_KIRA_AND_DAN_1
	release
	end

AbandonedShip_Rooms2_1F_EventScript_DanRematch::
	trainerbattle_rematch_double TRAINER_KIRA_AND_DAN_1, AbandonedShip_Rooms2_1F_Text_DanRematchIntro, AbandonedShip_Rooms2_1F_Text_DanRematchDefeat, AbandonedShip_Rooms2_1F_Text_DanRematchNotEnoughMons
	msgbox AbandonedShip_Rooms2_1F_Text_DanPostRematch, MSGBOX_AUTOCLOSE
	end

AbandonedShip_Rooms2_1F_EventScript_Kira::
	trainerbattle_double TRAINER_KIRA_AND_DAN_1, AbandonedShip_Rooms2_1F_Text_KiraIntro, AbandonedShip_Rooms2_1F_Text_KiraDefeat, AbandonedShip_Rooms2_1F_Text_KiraNotEnoughMons, AbandonedShip_Rooms2_1F_EventScript_RegisterKira
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, AbandonedShip_Rooms2_1F_EventScript_KiraRematch
	msgbox AbandonedShip_Rooms2_1F_Text_KiraPostBattle, MSGBOX_DEFAULT
	release
	end

AbandonedShip_Rooms2_1F_EventScript_RegisterKira::
	msgbox AbandonedShip_Rooms2_1F_Text_KiraRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_KIRA_AND_DAN_1
	release
	end

AbandonedShip_Rooms2_1F_EventScript_KiraRematch::
	trainerbattle_rematch_double TRAINER_KIRA_AND_DAN_1, AbandonedShip_Rooms2_1F_Text_KiraRematchIntro, AbandonedShip_Rooms2_1F_Text_KiraRematchDefeat, AbandonedShip_Rooms2_1F_Text_KiraRematchNotEnoughMons
	msgbox AbandonedShip_Rooms2_1F_Text_KiraPostRematch, MSGBOX_AUTOCLOSE
	end

AbandonedShip_Rooms2_1F_EventScript_Jani::
	trainerbattle_single TRAINER_JANI, AbandonedShip_Rooms2_1F_Text_JaniIntro, AbandonedShip_Rooms2_1F_Text_JaniDefeat
	msgbox AbandonedShip_Rooms2_1F_Text_JaniPostBattle, MSGBOX_AUTOCLOSE
	end

AbandonedShip_Rooms2_1F_EventScript_Garrison::
	trainerbattle_single TRAINER_GARRISON, AbandonedShip_Rooms2_1F_Text_GarrisonIntro, AbandonedShip_Rooms2_1F_Text_GarrisonDefeat
	msgbox AbandonedShip_Rooms2_1F_Text_GarrisonPostBattle, MSGBOX_AUTOCLOSE
	end

AbandonedShip_Rooms2_1F_Text_DanIntro:
	.string "DAN: Procurando tesouros, achamos\n"
	.string "um TREINADOR!$"

AbandonedShip_Rooms2_1F_Text_DanDefeat:
	.string "DAN: Nem juntos conseguimos ganhar…$"

AbandonedShip_Rooms2_1F_Text_DanPostBattle:
	.string "DAN: Não achamos nenhum tesouro…\n"
	.string "Será que alguém já pegou?$"

AbandonedShip_Rooms2_1F_Text_DanNotEnoughMons:
	.string "DAN: Você nem tem dois POKéMON.\n"
	.string "Assim não dá pra vencer a gente.$"

AbandonedShip_Rooms2_1F_Text_KiraIntro:
	.string "KIRA: Hã?\n"
	.string "A gente procurava tesouros.\l"
	.string "Mas achou foi um TREINADOR!$"

AbandonedShip_Rooms2_1F_Text_KiraDefeat:
	.string "KIRA: Uh, que força!$"

AbandonedShip_Rooms2_1F_Text_KiraPostBattle:
	.string "KIRA: Onde estarão os tesouros?\p"
	.string "Já decidi o que vou comprar quando\n"
	.string "a gente achar!$"

AbandonedShip_Rooms2_1F_Text_KiraNotEnoughMons:
	.string "KIRA: Ah, você não tem dois POKéMON?\n"
	.string "Vamos ter que batalhar outra hora!$"

AbandonedShip_Rooms2_1F_Text_KiraRegister:
	.string "KIRA: Ah, que raiva!\n"
	.string "Vou te registrar por isso!$"

AbandonedShip_Rooms2_1F_Text_DanRematchIntro:
	.string "DAN: Passamos esse tempo todo\n"
	.string "procurando tesouros.\p"
	.string "Os nossos POKéMON ficaram mais fortes\n"
	.string "também. Vamos te mostrar, tá?$"

AbandonedShip_Rooms2_1F_Text_DanRematchDefeat:
	.string "DAN: Forte como sempre!$"

AbandonedShip_Rooms2_1F_Text_DanPostRematch:
	.string "DAN: Não achamos tesouro, perdemos\n"
	.string "nas batalhas…\p"
	.string "Quero ir pra casa… Mas, se eu falar\n"
	.string "isso, ela fica uma fera comigo…$"

AbandonedShip_Rooms2_1F_Text_DanRematchNotEnoughMons:
	.string "DAN: Você nem tem dois POKéMON.\n"
	.string "Assim não dá pra vencer a gente.$"

AbandonedShip_Rooms2_1F_Text_KiraRematchIntro:
	.string "KIRA: Hã? Nos encontramos de novo!\p"
	.string "Igual à gente, você ainda não desistiu\n"
	.string "de procurar tesouros, né?\p"
	.string "Que tal quem perder desistir da\n"
	.string "busca?$"

AbandonedShip_Rooms2_1F_Text_KiraRematchDefeat:
	.string "KIRA: Ah, perdemos de novo…$"

AbandonedShip_Rooms2_1F_Text_KiraPostRematch:
	.string "KIRA: Não vamos embora até treinar mais\n"
	.string "os POKéMON e achar os tesouros!$"

AbandonedShip_Rooms2_1F_Text_KiraRematchNotEnoughMons:
	.string "KIRA: Ah, você não tem dois POKéMON?\n"
	.string "Vamos ter que batalhar outra hora!$"

AbandonedShip_Rooms2_1F_Text_JaniIntro:
	.string "Não sou boa de nado,\n"
	.string "mas sou boa de batalha!$"

AbandonedShip_Rooms2_1F_Text_JaniDefeat:
	.string "Ops.\n"
	.string "Não deu muito certo.$"

AbandonedShip_Rooms2_1F_Text_JaniPostBattle:
	.string "Andar descalça por este navio é\n"
	.string "meio nojento.$"

AbandonedShip_Rooms2_1F_Text_GarrisonIntro:
	.string "Força e compaixão…\n"
	.string "Esses são os tesouros de um TREINADOR!$"

AbandonedShip_Rooms2_1F_Text_GarrisonDefeat:
	.string "Ah, tem algo em você que brilha.$"

AbandonedShip_Rooms2_1F_Text_GarrisonPostBattle:
	.string "Numa cabine a bordo, vi uma\n"
	.string "coisa brilhar.$"
