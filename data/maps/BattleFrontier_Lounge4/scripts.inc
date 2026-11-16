BattleFrontier_Lounge4_MapScripts::
	.byte 0

BattleFrontier_Lounge4_EventScript_Woman::
	msgbox BattleFrontier_Lounge4_Text_WonderIfInterviewsAiring, MSGBOX_NPC
	end

BattleFrontier_Lounge4_EventScript_Cook::
	msgbox BattleFrontier_Lounge4_Text_IfIOpenedRestaurantHere, MSGBOX_NPC
	end

BattleFrontier_Lounge4_EventScript_Man::
	msgbox BattleFrontier_Lounge4_Text_NeedBreatherAfterBattles, MSGBOX_NPC
	end

BattleFrontier_Lounge4_Text_WonderIfInterviewsAiring:
	.string "Será que hoje vão passar entrevistas\n"
	.string "com TREINADORES fortes?$"

BattleFrontier_Lounge4_Text_IfIOpenedRestaurantHere:
	.string "Se eu abrisse um restaurante aqui,\n"
	.string "ia ganhar dinheiro, com certeza.$"

BattleFrontier_Lounge4_Text_NeedBreatherAfterBattles:
	.string "Ufa…\p"
	.string "Preciso de um respiro depois de\n"
	.string "umas batalhas intensas…\p"
	.string "Mas, mesmo agora, não paro de\n"
	.string "planejar estratégias e combos.$"

