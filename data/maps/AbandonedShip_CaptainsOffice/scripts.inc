AbandonedShip_CaptainsOffice_MapScripts:: /* 82387E1*/
	.byte 0

AbandonedShip_CaptainsOffice_EventScript_CaptSternAide:: /* 82387E2*/
	lock
	faceplayer
	goto_if_set FLAG_EXCHANGED_SCANNER, AbandonedShip_CaptainsOffice_EventScript_ThisIsSSCactus
	checkitem ITEM_SCANNER
	goto_if_eq VAR_RESULT, TRUE, AbandonedShip_CaptainsOffice_EventScript_CanYouDeliverScanner
	goto_if_set FLAG_ITEM_ABANDONED_SHIP_HIDDEN_FLOOR_ROOM_2_SCANNER, AbandonedShip_CaptainsOffice_EventScript_ThisIsSSCactus
	msgbox AbandonedShip_CaptainsOffice_Text_NoSuccessFindingScanner, MSGBOX_DEFAULT
	release
	end

AbandonedShip_CaptainsOffice_EventScript_CanYouDeliverScanner::
	msgbox AbandonedShip_CaptainsOffice_Text_OhCanYouDeliverScanner, MSGBOX_DEFAULT
	release
	end

AbandonedShip_CaptainsOffice_EventScript_ThisIsSSCactus::
	msgbox AbandonedShip_CaptainsOffice_Text_ThisIsSSCactus, MSGBOX_DEFAULT
	release
	end

AbandonedShip_CaptainsOffice_Text_NoSuccessFindingScanner:
	.string "Estou investigando este navio a pedido\n"
	.string "do CAP. STERN.\p"
	.string "Ele também me pediu pra achar um\n"
	.string "SCANNER, mas não tive sorte…$"

AbandonedShip_CaptainsOffice_Text_OhCanYouDeliverScanner:
	.string "Oh! É um SCANNER!\p"
	.string "Escuta, pode entregar isso pro\n"
	.string "CAP. STERN?\p"
	.string "Quero investigar este navio mais\n"
	.string "um pouco.$"

AbandonedShip_CaptainsOffice_Text_ThisIsSSCactus:
	.string "Este navio se chama S.S. CACTUS.\n"
	.string "Parece ser de outra época.$"

