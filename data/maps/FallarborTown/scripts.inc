FallarborTown_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, FallarborTown_OnTransition
	.byte 0

FallarborTown_OnTransition:
	setflag FLAG_VISITED_FALLARBOR_TOWN
	setvar VAR_CONTEST_HALL_STATE, 0
	clearflag FLAG_CONTEST_SKETCH_CREATED
	end

FallarborTown_EventScript_ExpertM::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY, FallarborTown_EventScript_ExpertMNormal
	msgbox FallarborTown_Text_ShadyCharactersCozmosHome, MSGBOX_DEFAULT
	release
	end

FallarborTown_EventScript_ExpertMNormal::
	msgbox FallarborTown_Text_RegionKnownForMeteors, MSGBOX_DEFAULT
	release
	end

FallarborTown_EventScript_Girl::
	msgbox FallarborTown_Text_MyPreciousAzurill, MSGBOX_NPC
	end

FallarborTown_EventScript_Gentleman::
	msgbox FallarborTown_Text_HaveYouChallengedFlannery, MSGBOX_NPC
	end

FallarborTown_EventScript_Azurill::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_AZURILL, CRY_MODE_NORMAL
	msgbox FallarborTown_Text_Azurill, MSGBOX_DEFAULT
	waitmoncry
	release
	end

FallarborTown_EventScript_BattleTentSign::
	msgbox FallarborTown_Text_BattleTentSign, MSGBOX_SIGN
	end

FallarborTown_EventScript_TownSign::
	msgbox FallarborTown_Text_TownSign, MSGBOX_SIGN
	end

FallarborTown_EventScript_MoveTutorSign::
	msgbox FallarborTown_Text_MoveTutorSign, MSGBOX_SIGN
	end

FallarborTown_Text_ShadyCharactersCozmosHome:
	.string "Tem alguma coisa acontecendo,\n"
	.string "e eu não estou gostando!\p"
	.string "Vi uns tipos suspeitos entrando e\n"
	.string "saindo da casa do PROF. COZMO…$"

FallarborTown_Text_RegionKnownForMeteors:
	.string "Esta região é famosa pelos meteoros\n"
	.string "desde os tempos antigos.\p"
	.string "Dizem que a CATARATA DOS METEOROS foi\n"
	.string "cavada pela queda de um meteorito.$"

FallarborTown_Text_MyPreciousAzurill:
	.string "Olha! Olha só!\n"
	.string "Este é o meu precioso AZURILL!\p"
	.string "É lisinho, macio e fofinho!$"

FallarborTown_Text_Azurill:
	.string "AZURILL: Ruriluuu.$"

FallarborTown_Text_HaveYouChallengedFlannery:
	.string "Já desafiou a FLANNERY, a LÍDER do\n"
	.string "GINÁSIO de LAVARIDGE?\p"
	.string "O avô dela era famoso.\n"
	.string "Já foi da ELITE DOS QUATRO da\l"
	.string "LIGA POKéMON.\p"
	.string "Não me surpreenderia ver a FLANNERY\n"
	.string "virar uma grande TREINADORA\l"
	.string "também.$"

FallarborTown_Text_BattleTentSign:
	.string "TENDA DE BATALHA DE FALLARBOR\n"
	.string "“Que venham as melhores equipes!”$"

FallarborTown_Text_TownSign:
	.string "CIDADE DE FALLARBOR\n"
	.string "“Comunidade rural de hortas.”$"

FallarborTown_Text_MoveTutorSign:
	.string "CASA DO TUTOR DE GOLPES\n"
	.string "“Golpes novos para POKéMON.”$"

