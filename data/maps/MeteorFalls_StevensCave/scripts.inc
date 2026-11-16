MeteorFalls_StevensCave_MapScripts::
	.byte 0

MeteorFalls_StevensCave_EventScript_Steven::
	lock
	goto_if_set FLAG_DEFEATED_METEOR_FALLS_STEVEN, MeteorFalls_StevensCave_EventScript_Defeated
	waitse
	playse SE_PIN
	applymovement LOCALID_METEOR_FALLS_STEVEN, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_METEOR_FALLS_STEVEN, Common_Movement_Delay48
	waitmovement 0
	applymovement LOCALID_METEOR_FALLS_STEVEN, Common_Movement_FacePlayer
	waitmovement 0
	msgbox MeteorFalls_StevensCave_Text_ShouldKnowHowGoodIAmExpectWorst, MSGBOX_DEFAULT
	trainerbattle_no_intro TRAINER_STEVEN, MeteorFalls_StevensCave_Text_StevenDefeat
	msgbox MeteorFalls_StevensCave_Text_MyPredictionCameTrue, MSGBOX_DEFAULT
	setflag FLAG_DEFEATED_METEOR_FALLS_STEVEN
	release
	end

MeteorFalls_StevensCave_EventScript_Defeated::
	applymovement LOCALID_METEOR_FALLS_STEVEN, Common_Movement_FacePlayer
	waitmovement 0
	msgbox MeteorFalls_StevensCave_Text_MyPredictionCameTrue, MSGBOX_DEFAULT
	release
	end

MeteorFalls_StevensCave_Text_ShouldKnowHowGoodIAmExpectWorst:
	.string "STEVEN: Nossa, {PLAYER}{KUN}.\n"
	.string "Estou impressionado que me achou.\p"
	.string "Você, hã… me acha só um maníaco\n"
	.string "por pedras?\p"
	.string "Não, não pode ser.\p"
	.string "Lutamos lado a lado no CENTRO\n"
	.string "ESPACIAL de SOOTOPOLIS.\p"
	.string "Você deve saber muito bem o quanto\n"
	.string "eu sou bom.\p"
	.string "Certo, {PLAYER}{KUN}, se vai me desafiar\n"
	.string "pra valer, espere o pior!$"

MeteorFalls_StevensCave_Text_StevenDefeat:
	.string "Você…\n"
	.string "Não imaginava que tinha tanta força…$"

MeteorFalls_StevensCave_Text_MyPredictionCameTrue:
	.string "STEVEN: Pensando bem, desde que nos\n"
	.string "vimos na CAVERNA GRANITO, em DEWFORD,\l"
	.string "tive esse pressentimento.\p"
	.string "Achei que um dia você ia chegar\n"
	.string "ao topo da LIGA.\p"
	.string "Costumo acertar as minhas previsões.\p"
	.string "E pra onde você vai agora?\p"
	.string "… … … … … …\n"
	.string "… … … … … …\p"
	.string "Fufufu, nem eu sei te dizer.$"

