MysteryGiftScript_AlteringCave::
	setvaddress MysteryGiftScript_AlteringCave
	addvar VAR_ALTERING_CAVE_WILD_SET, 1
	vgoto_if_ne VAR_ALTERING_CAVE_WILD_SET, (NUM_ALTERING_CAVE_TABLES + 1), MysteryGiftScript_AlteringCave_
	setvar VAR_ALTERING_CAVE_WILD_SET, 0
MysteryGiftScript_AlteringCave_:
	lock
	faceplayer
	vmessage sText_MysteryGiftAlteringCave
	waitmessage
	waitbuttonpress
	release
	end

sText_MysteryGiftAlteringCave:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.\p"
	.string "Anda rolando um boato de que\n"
	.string "POKéMON raros foram vistos.\p"
	.string "Dizem que eles apareceram na\n"
	.string "CAVERNA MUTANTE, na ROTA 103.\p"
	.string "Talvez valha a pena ir lá\n"
	.string "investigar esse boato.$"
