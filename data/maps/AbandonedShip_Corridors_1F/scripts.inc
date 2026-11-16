AbandonedShip_Corridors_1F_MapScripts::
	.byte 0

AbandonedShip_Corridors_1F_EventScript_Youngster::
	msgbox AbandonedShip_Corridors_1F_Text_IsntItFunHere, MSGBOX_NPC
	end

AbandonedShip_Corridors_1F_EventScript_Charlie::
	trainerbattle_single TRAINER_CHARLIE, AbandonedShip_Corridors_1F_Text_CharlieIntro, AbandonedShip_Corridors_1F_Text_CharlieDefeat
	msgbox AbandonedShip_Corridors_1F_Text_CharliePostBattle, MSGBOX_AUTOCLOSE
	end

AbandonedShip_Corridors_1F_Text_CharlieIntro:
	.string "Qual é a graça de eu trazer a minha\n"
	.string "boia pro navio?$"

AbandonedShip_Corridors_1F_Text_CharlieDefeat:
	.string "Uau, você me atropelou!$"

AbandonedShip_Corridors_1F_Text_CharliePostBattle:
	.string "Não é fácil jogar POKé BOLAS\n"
	.string "segurando uma boia!$"

AbandonedShip_Corridors_1F_Text_IsntItFunHere:
	.string "Não é divertido aqui?\n"
	.string "Só de estar aqui fico empolgado!$"
