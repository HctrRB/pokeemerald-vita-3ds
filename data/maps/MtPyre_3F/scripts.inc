MtPyre_3F_MapScripts::
	.byte 0

MtPyre_3F_EventScript_William::
	trainerbattle_single TRAINER_WILLIAM, MtPyre_3F_Text_WilliamIntro, MtPyre_3F_Text_WilliamDefeat
	msgbox MtPyre_3F_Text_WilliamPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_3F_EventScript_Kayla::
	trainerbattle_single TRAINER_KAYLA, MtPyre_3F_Text_KaylaIntro, MtPyre_3F_Text_KaylaDefeat
	msgbox MtPyre_3F_Text_KaylaPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_3F_EventScript_Gabrielle::
	trainerbattle_single TRAINER_GABRIELLE_1, MtPyre_3F_Text_GabrielleIntro, MtPyre_3F_Text_GabrielleDefeat, MtPyre_3F_EventScript_RegisterGabrielle
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, MtPyre_3F_EventScript_RematchGabrielle
	msgbox MtPyre_3F_Text_GabriellePostBattle, MSGBOX_DEFAULT
	release
	end

MtPyre_3F_EventScript_RegisterGabrielle::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox MtPyre_3F_Text_GabrielleRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_GABRIELLE_1
	release
	end

MtPyre_3F_EventScript_RematchGabrielle::
	trainerbattle_rematch TRAINER_GABRIELLE_1, MtPyre_3F_Text_GabrielleRematchIntro, MtPyre_3F_Text_GabrielleRematchDefeat
	msgbox MtPyre_3F_Text_GabriellePostRematch, MSGBOX_AUTOCLOSE
	end

MtPyre_3F_Text_WilliamIntro:
	.string "A atmosfera da montanha elevou o\n"
	.string "meu poder psíquico!\p"
	.string "Uma simples criança como você…\n"
	.string "Sonha em vencer?$"

MtPyre_3F_Text_WilliamDefeat:
	.string "Afundo na autopiedade…$"

MtPyre_3F_Text_WilliamPostBattle:
	.string "O meu poder psíquico com certeza\n"
	.string "cresceu várias vezes, mas…$"

MtPyre_3F_Text_KaylaIntro:
	.string "Ahahahaha!\p"
	.string "Isto não é lugar pra criança, muito\n"
	.string "menos pra você!$"

MtPyre_3F_Text_KaylaDefeat:
	.string "Perdi de lavada…$"

MtPyre_3F_Text_KaylaPostBattle:
	.string "Quer dizer que o meu treino ainda\n"
	.string "não basta…\p"
	.string "Tenho que continuar subindo até\n"
	.string "o topo…\p"
	.string "Vamos lá, eu!$"

MtPyre_3F_Text_GabrielleIntro:
	.string "Por que você veio aqui?$"

MtPyre_3F_Text_GabrielleDefeat:
	.string "Que incrível!\n"
	.string "Você é um TREINADOR muito especial.$"

MtPyre_3F_Text_GabriellePostBattle:
	.string "Os POKéMON que já não são deste mundo.\n"
	.string "Os POKéMON que estão com você agora.\p"
	.string "E os POKéMON que você vai conhecer\n"
	.string "no futuro…\p"
	.string "Todos merecem o mesmo carinho.\n"
	.string "Por favor, lembre-se disso.$"

MtPyre_3F_Text_GabrielleRegister:
	.string "Quero ver os seus POKéMON quando\n"
	.string "crescerem mais um pouco…\p"
	.string "Por favor, preciso ver o seu POKéNAV.$"

MtPyre_3F_Text_GabrielleRematchIntro:
	.string "Ah, é você…\p"
	.string "Veio me mostrar os seus POKéMON\n"
	.string "mais crescidos?$"

MtPyre_3F_Text_GabrielleRematchDefeat:
	.string "Que incrível!\n"
	.string "Você é uma pessoa especial.$"

MtPyre_3F_Text_GabriellePostRematch:
	.string "Os POKéMON que já não são deste mundo.\n"
	.string "Os POKéMON que estão com você agora.\p"
	.string "E os POKéMON que você vai conhecer\n"
	.string "no futuro…\p"
	.string "Todos merecem o mesmo carinho.\n"
	.string "Vejo que você se lembrou disso.$"

