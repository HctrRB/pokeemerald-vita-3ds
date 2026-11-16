LilycoveCity_House1_MapScripts::
	.byte 0

LilycoveCity_House1_EventScript_ExpertM::
	msgbox LilycoveCity_House1_Text_PokemonPartnersNotTools, MSGBOX_NPC
	end

LilycoveCity_House1_EventScript_Kecleon::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_KECLEON, CRY_MODE_NORMAL
	msgbox LilycoveCity_House1_Text_Kecleon, MSGBOX_DEFAULT
	waitmoncry
	release
	end

LilycoveCity_House1_Text_PokemonPartnersNotTools:
	.string "Os POKéMON são parceiros das pessoas.\n"
	.string "Não são ferramentas.\p"
	.string "Infelizmente, tem gente que não\n"
	.string "entende isso…$"

LilycoveCity_House1_Text_Kecleon:
	.string "KECLEON: Ruroro?$"

