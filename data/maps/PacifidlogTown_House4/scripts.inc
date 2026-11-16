PacifidlogTown_House4_MapScripts::
	.byte 0

PacifidlogTown_House4_EventScript_LittleGirl::
	msgbox PacifidlogTown_House4_Text_SkyPokemon, MSGBOX_NPC
	end

PacifidlogTown_House4_EventScript_Woman::
	msgbox PacifidlogTown_House4_Text_PeopleSawHighFlyingPokemon, MSGBOX_NPC
	end

PacifidlogTown_House4_EventScript_Boy::
	lock
	faceplayer
	msgbox PacifidlogTown_House4_Text_WhereDidYouComeFrom, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, PacifidlogTown_House4_EventScript_Yes
	goto_if_eq VAR_RESULT, NO, PacifidlogTown_House4_EventScript_No
	end

PacifidlogTown_House4_EventScript_Yes::
	msgbox PacifidlogTown_House4_Text_YesTown, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House4_EventScript_No::
	msgbox PacifidlogTown_House4_Text_YouHaveToComeFromSomewhere, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House4_Text_PeopleSawHighFlyingPokemon:
	.string "O pessoal disse que viu um POKéMON\n"
	.string "voando bem alto sobre HOENN.\p"
	.string "Ele voa o tempo todo?\n"
	.string "Não precisa descansar em algum lugar?$"

PacifidlogTown_House4_Text_SkyPokemon:
	.string "Um POKéMON do céu!\n"
	.string "Um POKéMON do céu!$"

PacifidlogTown_House4_Text_WhereDidYouComeFrom:
	.string "De onde você veio?$"

PacifidlogTown_House4_Text_YesTown:
	.string "Sim?\n"
	.string "CIDADE DO SIM?\p"
	.string "Nunca ouvi falar de um lugar assim.$"

PacifidlogTown_House4_Text_YouHaveToComeFromSomewhere:
	.string "Não? Isso não faz sentido.\n"
	.string "Você tem que ter vindo de algum lugar.\p"
	.string "Ah! Espera! Não vai me dizer que\n"
	.string "nasceu no fundo do mar?$"

