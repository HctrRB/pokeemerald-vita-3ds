RustboroCity_House3_MapScripts::
	.byte 0

RustboroCity_House3_EventScript_OldMan::
	msgbox RustboroCity_House3_Text_IGivePerfectlySuitedNicknames, MSGBOX_NPC
	end

RustboroCity_House3_EventScript_OldWoman::
	msgbox RustboroCity_House3_Text_NamingPikachuPekachu, MSGBOX_NPC
	end

/* Misspelling on purpose, see nickname*/
RustboroCity_House3_EventScript_Pekachu::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_PIKACHU, CRY_MODE_NORMAL
	msgbox RustboroCity_House3_Text_Pekachu, MSGBOX_DEFAULT
	waitmoncry
	release
	end

RustboroCity_House3_Text_IGivePerfectlySuitedNicknames:
	.string "Eu dou apelidos perfeitos pros\n"
	.string "meus POKéMON!\p"
	.string "É a minha forma de mostrar, hã…\n"
	.string "originalidade, isso mesmo!$"

RustboroCity_House3_Text_NamingPikachuPekachu:
	.string "Mas dar o nome de PEKACHU a um\n"
	.string "PIKACHU? Parece meio sem sentido.\p"
	.string "Acho bom usar um nome fácil de\n"
	.string "entender, mas…$"

RustboroCity_House3_Text_Pekachu:
	.string "PEKACHU: Peka!$"

