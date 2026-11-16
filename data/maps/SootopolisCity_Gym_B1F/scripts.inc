SootopolisCity_Gym_B1F_MapScripts::
	.byte 0

SootopolisCity_Gym_B1F_EventScript_Andrea::
	trainerbattle_single TRAINER_ANDREA, SootopolisCity_Gym_B1F_Text_AndreaIntro, SootopolisCity_Gym_B1F_Text_AndreaDefeat
	msgbox SootopolisCity_Gym_B1F_Text_AndreaPostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_EventScript_Crissy::
	trainerbattle_single TRAINER_CRISSY, SootopolisCity_Gym_B1F_Text_CrissyIntro, SootopolisCity_Gym_B1F_Text_CrissyDefeat
	msgbox SootopolisCity_Gym_B1F_Text_CrissyPostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_EventScript_Brianna::
	trainerbattle_single TRAINER_BRIANNA, SootopolisCity_Gym_B1F_Text_BriannaIntro, SootopolisCity_Gym_B1F_Text_BriannaDefeat
	msgbox SootopolisCity_Gym_B1F_Text_BriannaPostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_EventScript_Connie::
	trainerbattle_single TRAINER_CONNIE, SootopolisCity_Gym_B1F_Text_ConnieIntro, SootopolisCity_Gym_B1F_Text_ConnieDefeat
	msgbox SootopolisCity_Gym_B1F_Text_ConniePostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_EventScript_Bridget::
	trainerbattle_single TRAINER_BRIDGET, SootopolisCity_Gym_B1F_Text_BridgetIntro, SootopolisCity_Gym_B1F_Text_BridgetDefeat
	msgbox SootopolisCity_Gym_B1F_Text_BridgetPostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_EventScript_Olivia::
	trainerbattle_single TRAINER_OLIVIA, SootopolisCity_Gym_B1F_Text_OliviaIntro, SootopolisCity_Gym_B1F_Text_OliviaDefeat
	msgbox SootopolisCity_Gym_B1F_Text_OliviaPostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_EventScript_Tiffany::
	trainerbattle_single TRAINER_TIFFANY, SootopolisCity_Gym_B1F_Text_TiffanyIntro, SootopolisCity_Gym_B1F_Text_TiffanyDefeat
	msgbox SootopolisCity_Gym_B1F_Text_TiffanyPostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_EventScript_Bethany::
	trainerbattle_single TRAINER_BETHANY, SootopolisCity_Gym_B1F_Text_BethanyIntro, SootopolisCity_Gym_B1F_Text_BethanyDefeat
	msgbox SootopolisCity_Gym_B1F_Text_BethanyPostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_EventScript_Annika::
	trainerbattle_single TRAINER_ANNIKA, SootopolisCity_Gym_B1F_Text_AnnikaIntro, SootopolisCity_Gym_B1F_Text_AnnikaDefeat
	msgbox SootopolisCity_Gym_B1F_Text_AnnikaPostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_EventScript_Daphne::
	trainerbattle_single TRAINER_DAPHNE, SootopolisCity_Gym_B1F_Text_DaphneIntro, SootopolisCity_Gym_B1F_Text_DaphneDefeat
	msgbox SootopolisCity_Gym_B1F_Text_DaphnePostBattle, MSGBOX_AUTOCLOSE
	end

SootopolisCity_Gym_B1F_Text_AndreaIntro:
	.string "Vou te mostrar as técnicas sublimes\n"
	.string "que aprendi com o nosso LÍDER JUAN!$"

SootopolisCity_Gym_B1F_Text_AndreaDefeat:
	.string "Por favor, me perdoe, JUAN…$"

SootopolisCity_Gym_B1F_Text_AndreaPostBattle:
	.string "Veja o que acontece se quebrar\n"
	.string "todos os ladrilhos do chão.$"

SootopolisCity_Gym_B1F_Text_CrissyIntro:
	.string "Você veio até aqui, mas não vai\n"
	.string "ver o JUAN.\p"
	.string "Não se perder pra mim, não vai!$"

SootopolisCity_Gym_B1F_Text_CrissyDefeat:
	.string "Que força!\n"
	.string "Essa carinha fofa me enganou!$"

SootopolisCity_Gym_B1F_Text_CrissyPostBattle:
	.string "Talvez você aguente mais que um\n"
	.string "golpe só do JUAN.$"

SootopolisCity_Gym_B1F_Text_DaphneIntro:
	.string "Ver o JUAN conduzindo uma batalha…\p"
	.string "Foi essa beleza que me fez virar\n"
	.string "TREINADORA.$"

SootopolisCity_Gym_B1F_Text_DaphneDefeat:
	.string "Você batalhou com mais beleza do\n"
	.string "que eu consegui…$"

SootopolisCity_Gym_B1F_Text_DaphnePostBattle:
	.string "A graça que você leva à batalha é\n"
	.string "fabulosa.\p"
	.string "Ah… Que sorte a minha ter\n"
	.string "conhecido os POKéMON.$"

SootopolisCity_Gym_B1F_Text_ConnieIntro:
	.string "Vou te ensinar como as batalhas\n"
	.string "podem ser duras.$"

SootopolisCity_Gym_B1F_Text_ConnieDefeat:
	.string "Ah.\n"
	.string "Você é forte.$"

SootopolisCity_Gym_B1F_Text_ConniePostBattle:
	.string "Vou te contar uma coisa boa.\p"
	.string "Pra chegar ao JUAN, você tem que\n"
	.string "pisar em cada ladrilho uma vez.$"

SootopolisCity_Gym_B1F_Text_BridgetIntro:
	.string "O GINÁSIO POKéMON de nível mais alto\n"
	.string "da região de HOENN…\p"
	.string "É o GINÁSIO de SOOTOPOLIS.$"

SootopolisCity_Gym_B1F_Text_BridgetDefeat:
	.string "Que nível alto o seu!$"

SootopolisCity_Gym_B1F_Text_BridgetPostBattle:
	.string "Em vez de se contentar com um\n"
	.string "GINÁSIO forte, acho que treinar em\l"
	.string "outros lugares te deixa mais forte.\l"
	.string "E, acima de tudo, parece mais legal.$"

SootopolisCity_Gym_B1F_Text_OliviaIntro:
	.string "Treino os meus POKéMON junto com\n"
	.string "o JUAN.\p"
	.string "Não pense que sou fácil.$"

SootopolisCity_Gym_B1F_Text_OliviaDefeat:
	.string "Fui derrotada…$"

SootopolisCity_Gym_B1F_Text_OliviaPostBattle:
	.string "Acho que você tem potencial.\n"
	.string "Por que não fica e treina com a gente?$"

SootopolisCity_Gym_B1F_Text_TiffanyIntro:
	.string "Deslizar com graça pelo gelo sem\n"
	.string "cruzar nenhuma linha…\p"
	.string "Um TREINADOR fazendo isso seria\n"
	.string "de uma beleza elegante!$"

SootopolisCity_Gym_B1F_Text_TiffanyDefeat:
	.string "Ora, como é que é?!$"

SootopolisCity_Gym_B1F_Text_TiffanyPostBattle:
	.string "É óbvio, mas a força de um TREINADOR\n"
	.string "não tem nada a ver com ser novo ou\l"
	.string "velho.$"

SootopolisCity_Gym_B1F_Text_BethanyIntro:
	.string "Quando estou com os meus POKéMON, o\n"
	.string "tempo voa antes de dar pra dizer “Opa!”$"

SootopolisCity_Gym_B1F_Text_BethanyDefeat:
	.string "Opa!$"

SootopolisCity_Gym_B1F_Text_BethanyPostBattle:
	.string "Queria esquecer as causas perdidas\n"
	.string "antes de conseguir dizer “Opa!”$"

SootopolisCity_Gym_B1F_Text_AnnikaIntro:
	.string "Posso batalhar com POKéMON bem raros,\n"
	.string "se quiser.$"

SootopolisCity_Gym_B1F_Text_AnnikaDefeat:
	.string "Pronto! Deu uma boa olhada nos\n"
	.string "meus POKéMON?$"

SootopolisCity_Gym_B1F_Text_AnnikaPostBattle:
	.string "Vim a este GINÁSIO porque o JUAN\n"
	.string "elogiou o meu POKéMON querido.\p"
	.string "Ah, se eu tivesse conhecido o JUAN\n"
	.string "anos atrás, quando era mais nova…$"

SootopolisCity_Gym_B1F_Text_BriannaIntro:
	.string "Hihi…\n"
	.string "Essa sua cara séria é um charme.$"

SootopolisCity_Gym_B1F_Text_BriannaDefeat:
	.string "Ai, ai.\n"
	.string "Peguei leve demais com você.$"

SootopolisCity_Gym_B1F_Text_BriannaPostBattle:
	.string "Você nem encostaria no JUAN,\n"
	.string "tenho certeza. Hihi…$"

