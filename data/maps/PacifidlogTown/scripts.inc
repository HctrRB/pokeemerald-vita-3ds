PacifidlogTown_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, PacifidlogTown_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, PacifidlogTown_OnResume
	.byte 0

PacifidlogTown_OnTransition:
	setflag FLAG_VISITED_PACIFIDLOG_TOWN
	end

PacifidlogTown_OnResume:
	setstepcallback STEP_CB_PACIFIDLOG_BRIDGE
	end

PacifidlogTown_EventScript_NinjaBoy::
	msgbox PacifidlogTown_Text_NeatHousesOnWater, MSGBOX_NPC
	end

PacifidlogTown_EventScript_Girl::
	msgbox PacifidlogTown_Text_FastRunningCurrent, MSGBOX_NPC
	end

PacifidlogTown_EventScript_Fisherman::
	msgbox PacifidlogTown_Text_SkyPillarTooScary, MSGBOX_NPC
	end

PacifidlogTown_EventScript_TownSign::
	msgbox PacifidlogTown_Text_TownSign, MSGBOX_SIGN
	end

PacifidlogTown_Text_FastRunningCurrent:
	.string "O mar entre PACIFIDLOG e SLATEPORT\n"
	.string "tem uma correnteza forte.\p"
	.string "Se for SURFAR, a correnteza pode\n"
	.string "te levar pra outro lugar.$"

PacifidlogTown_Text_NeatHousesOnWater:
	.string "Olha, não é legal?\n"
	.string "As casas ficam em cima da água!\p"
	.string "Eu nasci aqui!$"

PacifidlogTown_Text_SkyPillarTooScary:
	.string "O PILAR CELESTE?\p"
	.string "…Ah, deve ser aquela torre altíssima\n"
	.string "um pouco mais adiante.\p"
	.string "Se fosse eu, não subiria. Dá medo\n"
	.string "demais ficar tão alto.\p"
	.string "A vida ao nível do mar em PACIFIDLOG\n"
	.string "me serve muito bem.$"

PacifidlogTown_Text_TownSign:
	.string "CIDADE DE PACIFIDLOG\p"
	.string "“Onde o sol da manhã sorri\n"
	.string "sobre as águas.”$"

