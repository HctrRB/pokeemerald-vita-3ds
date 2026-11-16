MeteorFalls_1F_2R_MapScripts::
	.byte 0

MeteorFalls_1F_2R_EventScript_Nicolas::
	trainerbattle_single TRAINER_NICOLAS_1, MeteorFalls_1F_2R_Text_NicolasIntro, MeteorFalls_1F_2R_Text_NicolasDefeat, MeteorFalls_1F_2R_EventScript_RegisterNicolas
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, MeteorFalls_1F_2R_EventScript_RematchNicolas
	msgbox MeteorFalls_1F_2R_Text_NicolasPostBattle, MSGBOX_DEFAULT
	release
	end

MeteorFalls_1F_2R_EventScript_RegisterNicolas::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	msgbox MeteorFalls_1F_2R_Text_NicolasRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_NICOLAS_1
	release
	end

MeteorFalls_1F_2R_EventScript_RematchNicolas::
	trainerbattle_rematch TRAINER_NICOLAS_1, MeteorFalls_1F_2R_Text_NicolasRematchIntro, MeteorFalls_1F_2R_Text_NicolasRematchDefeat
	msgbox MeteorFalls_1F_2R_Text_NicolasPostRematch, MSGBOX_AUTOCLOSE
	end

MeteorFalls_1F_2R_EventScript_John::
	trainerbattle_double TRAINER_JOHN_AND_JAY_1, MeteorFalls_1F_2R_Text_JohnIntro, MeteorFalls_1F_2R_Text_JohnDefeat, MeteorFalls_1F_2R_Text_JohnNotEnoughMons, MeteorFalls_1F_2R_EventScript_RegisterJohn
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, MeteorFalls_1F_2R_EventScript_RematchJohn
	msgbox MeteorFalls_1F_2R_Text_JohnPostBattle, MSGBOX_DEFAULT
	release
	end

MeteorFalls_1F_2R_EventScript_RegisterJohn::
	msgbox MeteorFalls_1F_2R_Text_JohnRegister, MSGBOX_DEFAULT
	register_matchcall TRAINER_JOHN_AND_JAY_1
	release
	end

MeteorFalls_1F_2R_EventScript_RematchJohn::
	trainerbattle_rematch_double TRAINER_JOHN_AND_JAY_1, MeteorFalls_1F_2R_Text_JohnRematchIntro, MeteorFalls_1F_2R_Text_JohnRematchDefeat, MeteorFalls_1F_2R_Text_JohnRematchNotEnoughMons
	msgbox MeteorFalls_1F_2R_Text_JohnPostRematch, MSGBOX_AUTOCLOSE
	end

MeteorFalls_1F_2R_EventScript_Jay::
	trainerbattle_double TRAINER_JOHN_AND_JAY_1, MeteorFalls_1F_2R_Text_JayIntro, MeteorFalls_1F_2R_Text_JayDefeat, MeteorFalls_1F_2R_Text_JayNotEnoughMons, MeteorFalls_1F_2R_EventScript_RegisterJay
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, MeteorFalls_1F_2R_EventScript_RematchJay
	msgbox MeteorFalls_1F_2R_Text_JayPostBattle, MSGBOX_DEFAULT
	release
	end

MeteorFalls_1F_2R_EventScript_RegisterJay::
	msgbox MeteorFalls_1F_2R_Text_JohnRegister, MSGBOX_DEFAULT  /* John speaks for both during register*/
	register_matchcall TRAINER_JOHN_AND_JAY_1
	release
	end

MeteorFalls_1F_2R_EventScript_RematchJay::
	trainerbattle_rematch_double TRAINER_JOHN_AND_JAY_1, MeteorFalls_1F_2R_Text_JayRematchIntro, MeteorFalls_1F_2R_Text_JayRematchDefeat, MeteorFalls_1F_2R_Text_JayRematchNotEnoughMons
	msgbox MeteorFalls_1F_2R_Text_JayPostRematch, MSGBOX_AUTOCLOSE
	end

MeteorFalls_1F_2R_Text_NicolasIntro:
	.string "É aqui que nós, usuários de DRAGÃO,\n"
	.string "treinamos.\p"
	.string "Até o CAMPEÃO vem aqui.\n"
	.string "Viu como este lugar é especial?$"

MeteorFalls_1F_2R_Text_NicolasDefeat:
	.string "Argh!\n"
	.string "Não esperava tanta força!$"

MeteorFalls_1F_2R_Text_NicolasPostBattle:
	.string "O caminho ainda é longo e duro.\p"
	.string "Quando é que eu e os meus POKéMON\n"
	.string "vamos ser os melhores?$"

MeteorFalls_1F_2R_Text_NicolasRegister:
	.string "Quero conhecer melhor a sua força.\n"
	.string "Vou te registrar no meu POKéNAV.$"

MeteorFalls_1F_2R_Text_NicolasRematchIntro:
	.string "Desde que nos vimos, treinamos\n"
	.string "duro de olho no primeiro lugar.\p"
	.string "Ajude a gente a ver o quanto\n"
	.string "ficamos mais fortes!$"

MeteorFalls_1F_2R_Text_NicolasRematchDefeat:
	.string "Argh!\n"
	.string "Não esperava tanta força!$"

MeteorFalls_1F_2R_Text_NicolasPostRematch:
	.string "Dá pra ver que você continua\n"
	.string "treinando os seus POKéMON.\p"
	.string "Enquanto você for forte, eu também\n"
	.string "posso ficar mais forte!$"

MeteorFalls_1F_2R_Text_JohnIntro:
	.string "JOHN: Sempre batalhamos em dupla.\n"
	.string "Confiamos em nós mesmos.$"

MeteorFalls_1F_2R_Text_JohnDefeat:
	.string "JOHN: Puxa vida.\n"
	.string "Perdemos, querida.$"

MeteorFalls_1F_2R_Text_JohnPostBattle:
	.string "JOHN: Estamos casados há cinquenta\n"
	.string "anos.\p"
	.string "Pensando bem, eu ainda não venci\n"
	.string "a minha querida numa batalha.$"

MeteorFalls_1F_2R_Text_JohnNotEnoughMons:
	.string "JOHN: Ora, ora, que TREINADOR jovem!\p"
	.string "Quer batalhar com a gente? Então\n"
	.string "volte com mais POKéMON.$"

MeteorFalls_1F_2R_Text_JohnRegister:
	.string "JOHN: Jovem TREINADOR, se tiver a\n"
	.string "chance, batalha com a gente de novo?$"

MeteorFalls_1F_2R_Text_JayIntro:
	.string "JAY: Estamos casados há cinquenta\n"
	.string "anos.\p"
	.string "O laço que temos como casal\n"
	.string "nunca vai se romper.$"

MeteorFalls_1F_2R_Text_JayDefeat:
	.string "JAY: Ai, ai.\n"
	.string "Perdemos, meu querido.$"

MeteorFalls_1F_2R_Text_JayPostBattle:
	.string "JAY: Cinquenta anos de casamento…\p"
	.string "Se a gente brigava, sempre resolvia\n"
	.string "com uma batalha POKéMON…$"

MeteorFalls_1F_2R_Text_JayNotEnoughMons:
	.string "JAY: Ora, ora, que TREINADOR jovem!\p"
	.string "Se quiser batalhar com a gente,\n"
	.string "volte com mais POKéMON.$"

MeteorFalls_1F_2R_Text_JohnRematchIntro:
	.string "JOHN: Sempre batalhamos em dupla.\n"
	.string "Confiamos em nós mesmos.$"

MeteorFalls_1F_2R_Text_JohnRematchDefeat:
	.string "JOHN: Puxa vida.\n"
	.string "Perdemos, querida.$"

MeteorFalls_1F_2R_Text_JohnPostRematch:
	.string "JOHN: Casados há cinquenta anos…\p"
	.string "Pensando bem, eu e a minha querida\n"
	.string "batalhamos todo santo dia…$"

MeteorFalls_1F_2R_Text_JohnRematchNotEnoughMons:
	.string "JOHN: Ora, ora, que TREINADOR jovem!\p"
	.string "Quer batalhar com a gente? Então\n"
	.string "volte com mais POKéMON.$"

MeteorFalls_1F_2R_Text_JayRematchIntro:
	.string "JAY: Estamos casados há cinquenta\n"
	.string "anos.\p"
	.string "Sempre apoiamos um ao outro.\n"
	.string "Foi assim que ficamos fortes.$"

MeteorFalls_1F_2R_Text_JayRematchDefeat:
	.string "JAY: Ai, ai.\n"
	.string "Perdemos, meu querido.$"

MeteorFalls_1F_2R_Text_JayPostRematch:
	.string "JAY: Cinquenta anos de casamento…\n"
	.string "Muita coisa aconteceu.\p"
	.string "Espero que a gente continue criando\n"
	.string "lembranças felizes juntos.$"

MeteorFalls_1F_2R_Text_JayRematchNotEnoughMons:
	.string "JAY: Ora, ora, que TREINADOR jovem!\p"
	.string "Se quiser batalhar com a gente,\n"
	.string "volte com mais POKéMON.$"

