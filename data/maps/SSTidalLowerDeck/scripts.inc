SSTidalLowerDeck_MapScripts::
	.byte 0

SSTidalLowerDeck_EventScript_Phillip::
	trainerbattle_single TRAINER_PHILLIP, SSTidalLowerDeck_Text_PhillipIntro, SSTidalLowerDeck_Text_PhillipDefeat
	msgbox SSTidalLowerDeck_Text_PhillipPostBattle, MSGBOX_AUTOCLOSE
	end

SSTidalLowerDeck_EventScript_Leonard::
	trainerbattle_single TRAINER_LEONARD, SSTidalLowerDeck_Text_LeonardIntro, SSTidalLowerDeck_Text_LeonardDefeat
	msgbox SSTidalLowerDeck_Text_LeonardPostBattle, MSGBOX_AUTOCLOSE
	end

SSTidalLowerDeck_Text_PhillipIntro:
	.string "Aaargh! Cansei de limpar este\n"
	.string "lugarzão!\p"
	.string "Vamos a uma batalha rápida!$"

SSTidalLowerDeck_Text_PhillipDefeat:
	.string "Maninho, perdi!$"

SSTidalLowerDeck_Text_PhillipPostBattle:
	.string "Somos os IRMÃOS DA FAXINA!\p"
	.string "O velho joga o detergente,\n"
	.string "e o novo esfrega!$"

SSTidalLowerDeck_Text_LeonardIntro:
	.string "Este é o fundo do casco do navio.\n"
	.string "Tem espaço de sobra.\l"
	.string "Dá pra uma batalha POKéMON.$"

SSTidalLowerDeck_Text_LeonardDefeat:
	.string "Manão, perdi!$"

SSTidalLowerDeck_Text_LeonardPostBattle:
	.string "Somos os IRMÃOS DA FAXINA!\p"
	.string "O velho joga o detergente,\n"
	.string "e o novo esfrega!$"

