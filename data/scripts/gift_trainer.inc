MysteryGiftScript_VisitingTrainer::
	setvaddress MysteryGiftScript_VisitingTrainer
	special ValidateEReaderTrainer
	vgoto_if_eq VAR_RESULT, 0, MysteryGiftScript_VisitingTrainerArrived
	lock
	faceplayer
	vmessage sText_MysteryGiftVisitingTrainerInstructions
	waitmessage
	waitbuttonpress
	release
	end

MysteryGiftScript_VisitingTrainerArrived:
	lock
	faceplayer
	vmessage sText_MysteryGiftVisitingTrainerArrived
	waitmessage
	waitbuttonpress
	release
	end

sText_MysteryGiftVisitingTrainerInstructions:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.\p"
	.string "Com este CARTÃO MARAVILHA, você\n"
	.string "pode responder a uma pesquisa\l"
	.string "num POKéMON MART.\p"
	.string "Use as pesquisas pra convidar\n"
	.string "TREINADORES pra SOOTOPOLIS.\p"
	.string "“Vou te passar uma senha\n"
	.string "secreta pra pesquisa:\p"
	.string "“GIVE ME\n"
	.string "AWESOME TRAINER”\p"
	.string "Escreva isso numa pesquisa e\n"
	.string "mande pro SISTEMA DE\l"
	.string "COMUNICAÇÃO SEM FIO.$"

sText_MysteryGiftVisitingTrainerArrived:
	.string "Obrigado por usar o sistema de\n"
	.string "PRESENTE MISTERIOSO.\p"
	.string "Um TREINADOR chegou em\n"
	.string "SOOTOPOLIS procurando você.\p"
	.string "Tomara que você curta\n"
	.string "batalhar com o visitante.\p"
	.string "Dá pra convidar outros\n"
	.string "TREINADORES com outras senhas.\p"
	.string "Tente descobrir outras senhas\n"
	.string "que funcionem.$"
