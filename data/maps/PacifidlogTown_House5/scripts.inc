PacifidlogTown_House5_MapScripts::
	.byte 0

PacifidlogTown_House5_EventScript_MirageIslandWatcher::
	lock
	faceplayer
	specialvar VAR_RESULT, IsMirageIslandPresent
	goto_if_eq VAR_RESULT, TRUE, PacifidlogTown_House5_EventScript_MirageIslandPresent
	msgbox PacifidlogTown_House5_Text_CantSeeMirageIslandToday, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House5_EventScript_MirageIslandPresent::
	msgbox PacifidlogTown_House5_Text_CanSeeMirageIslandToday, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House5_EventScript_Gentleman::
	msgbox PacifidlogTown_House5_Text_MirageIslandAppearDependingOnWeather, MSGBOX_NPC
	end

PacifidlogTown_House5_Text_CantSeeMirageIslandToday:
	.string "Não dá pra ver a ILHA MIRAGEM hoje…$"

PacifidlogTown_House5_Text_CanSeeMirageIslandToday:
	.string "Oh! Minha nossa!\n"
	.string "Dá pra ver a ILHA MIRAGEM hoje!$"

PacifidlogTown_House5_Text_MirageIslandAppearDependingOnWeather:
	.string "A ILHA MIRAGEM…\p"
	.string "Ela deve aparecer e sumir conforme\n"
	.string "o tempo, que cria as miragens.\p"
	.string "Ou será que ela aparece e some de\n"
	.string "verdade?$"

