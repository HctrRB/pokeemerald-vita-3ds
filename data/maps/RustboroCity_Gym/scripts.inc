RustboroCity_Gym_MapScripts::
	.byte 0

RustboroCity_Gym_EventScript_Roxanne::
	trainerbattle_single TRAINER_ROXANNE_1, RustboroCity_Gym_Text_RoxanneIntro, RustboroCity_Gym_Text_RoxanneDefeat, RustboroCity_Gym_EventScript_RoxanneDefeated, NO_MUSIC
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, RustboroCity_Gym_EventScript_RoxanneRematch
	goto_if_unset FLAG_RECEIVED_TM_ROCK_TOMB, RustboroCity_Gym_EventScript_GiveRockTomb
	msgbox RustboroCity_Gym_Text_RoxannePostBattle, MSGBOX_DEFAULT
	release
	end

RustboroCity_Gym_EventScript_RoxanneDefeated::
	message RustboroCity_Gym_Text_ReceivedStoneBadge
	waitmessage
	call Common_EventScript_PlayGymBadgeFanfare
	msgbox RustboroCity_Gym_Text_StoneBadgeInfoTakeThis, MSGBOX_DEFAULT
	setflag FLAG_DEFEATED_RUSTBORO_GYM
	setflag FLAG_BADGE01_GET
	setvar VAR_RUSTBORO_CITY_STATE, 1
	addvar VAR_PETALBURG_GYM_STATE, 1
	setvar VAR_0x8008, 1
	call Common_EventScript_SetGymTrainers
	call_if_eq VAR_PETALBURG_GYM_STATE, 6, Common_EventScript_ReadyPetalburgGymForBattle
	goto RustboroCity_Gym_EventScript_GiveRockTomb
	end

RustboroCity_Gym_EventScript_GiveRockTomb::
	giveitem ITEM_TM_ROCK_TOMB
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM_ROCK_TOMB
	msgbox RustboroCity_Gym_Text_ExplainRockTomb, MSGBOX_DEFAULT
	release
	end

RustboroCity_Gym_EventScript_RoxanneRematch::
	trainerbattle_rematch_double TRAINER_ROXANNE_1, RustboroCity_Gym_Text_RoxannePreRematch, RustboroCity_Gym_Text_RoxanneRematchDefeat, RustboroCity_Gym_Text_RoxanneRematchNeedTwoMons
	msgbox RustboroCity_Gym_Text_RoxannePostRematch, MSGBOX_AUTOCLOSE
	end

RustboroCity_Gym_EventScript_Josh::
	trainerbattle_single TRAINER_JOSH, RustboroCity_Gym_Text_JoshIntro, RustboroCity_Gym_Text_JoshDefeat
	msgbox RustboroCity_Gym_Text_JoshPostBattle, MSGBOX_AUTOCLOSE
	end

RustboroCity_Gym_EventScript_Tommy::
	trainerbattle_single TRAINER_TOMMY, RustboroCity_Gym_Text_TommyIntro, RustboroCity_Gym_Text_TommyDefeat
	msgbox RustboroCity_Gym_Text_TommyPostBattle, MSGBOX_AUTOCLOSE
	end

RustboroCity_Gym_EventScript_Marc::
	trainerbattle_single TRAINER_MARC, RustboroCity_Gym_Text_MarcIntro, RustboroCity_Gym_Text_MarcDefeat
	msgbox RustboroCity_Gym_Text_MarcPostBattle, MSGBOX_AUTOCLOSE
	end

RustboroCity_Gym_EventScript_GymGuide::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_RUSTBORO_GYM, RustboroCity_Gym_EventScript_GymGuidePostVictory
	msgbox RustboroCity_Gym_Text_GymGuideAdvice, MSGBOX_DEFAULT
	release
	end

RustboroCity_Gym_EventScript_GymGuidePostVictory::
	msgbox RustboroCity_Gym_Text_GymGuidePostVictory, MSGBOX_DEFAULT
	release
	end

RustboroCity_Gym_EventScript_LeftGymStatue::
	lockall
	goto_if_set FLAG_BADGE01_GET, RustboroCity_Gym_EventScript_GymStatueCertified
	goto RustboroCity_Gym_EventScript_GymStatue
	end

RustboroCity_Gym_EventScript_RightGymStatue::
	lockall
	goto_if_set FLAG_BADGE01_GET, RustboroCity_Gym_EventScript_GymStatueCertified
	goto RustboroCity_Gym_EventScript_GymStatue
	end

RustboroCity_Gym_EventScript_GymStatueCertified::
	msgbox RustboroCity_Gym_Text_GymStatueCertified, MSGBOX_DEFAULT
	releaseall
	end

RustboroCity_Gym_EventScript_GymStatue::
	msgbox RustboroCity_Gym_Text_GymStatue, MSGBOX_DEFAULT
	releaseall
	end

RustboroCity_Gym_EventScript_RegisterRoxanne::
	lockall
	pokenavcall RustboroCity_Gym_Text_RoxanneRegisterCall
	waitmessage
	delay 30
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox RustboroCity_Gym_Text_RegisteredRoxanne, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	delay 30
	setflag FLAG_ENABLE_ROXANNE_MATCH_CALL
	clearflag FLAG_ENABLE_ROXANNE_FIRST_CALL
	releaseall
	end

RustboroCity_Gym_Text_GymGuideAdvice:
	.string "E aí, tudo certo?\p"
	.string "Escuta aqui, meu amigo!\n"
	.string "Quer ganhar o título de CAMPEÃO?\p"
	.string "Eu não treino POKéMON, nada disso, mas\n"
	.string "dou conselhos que fazem vencer.\p"
	.string "Então está combinado! Vamos juntos\n"
	.string "rumo ao título de CAMPEÃO POKéMON!\p"
	.string "O seu trabalho é juntar INSÍGNIAS\n"
	.string "dos GINÁSIOS, né?\p"
	.string "Mas os LÍDERES DE GINÁSIO não são\n"
	.string "moleza!\p"
	.string "E é aí que eu entro!\n"
	.string "Estou aqui pra dar conselhos de expert!\p"
	.string "A LÍDER DE GINÁSIO, ROXANNE, usa\n"
	.string "POKéMON do tipo PEDRA.\p"
	.string "O tipo PEDRA é muito resistente, mas\n"
	.string "não aguenta golpes dos tipos ÁGUA\l"
	.string "e PLANTA.\p"
	.string "Volte pra falar comigo depois, se\n"
	.string "vencer a LÍDER.\p"
	.string "Bom, vai fundo!$"

RustboroCity_Gym_Text_GymGuidePostVictory:
	.string "Uau! Que vitória de tirar o fôlego!\n"
	.string "Minha torcida deve ter ajudado!\p"
	.string "Ótimo!\n"
	.string "Olha o seu CARTÃO DE TREINADOR.\p"
	.string "A INSÍGNIA DA PEDRA que você ganhou\n"
	.string "já deve estar registrada nele.\p"
	.string "Ou seja…\p"
	.string "Você subiu o primeiro degrau da\n"
	.string "escada rumo ao título!\p"
	.string "Isso deve ser demais!$"

RustboroCity_Gym_Text_JoshIntro:
	.string "Não subestime a gente do GINÁSIO!\n"
	.string "Vou mostrar por que somos melhores!$"

RustboroCity_Gym_Text_JoshDefeat:
	.string "Você é demais pra mim…$"

RustboroCity_Gym_Text_JoshPostBattle:
	.string "Você ainda não viu nada do poder\n"
	.string "terrível do tipo PEDRA!$"

RustboroCity_Gym_Text_TommyIntro:
	.string "Se não conseguir me vencer, não vai\n"
	.string "ter chance contra a ROXANNE!$"

RustboroCity_Gym_Text_TommyDefeat:
	.string "Uau! Você tem potencial!$"

RustboroCity_Gym_Text_TommyPostBattle:
	.string "A ROXANNE é várias vezes mais forte\n"
	.string "que eu.\p"
	.string "É melhor ficar de olho aberto!$"

RustboroCity_Gym_Text_MarcIntro:
	.string "A gente pode estar no meio da cidade,\n"
	.string "mas e daí?\p"
	.string "Meus POKéMON PEDRA precisam de\n"
	.string "espaço pra fazer estrago!$"

RustboroCity_Gym_Text_MarcDefeat:
	.string "Puxa vida!\n"
	.string "Quem desafiou é osso duro de roer!$"

RustboroCity_Gym_Text_MarcPostBattle:
	.string "Tiro o chapéu pra nossa LÍDER.\p"
	.string "Foi preciso inteligência e bom senso\n"
	.string "pra ela escolher o tipo PEDRA tão nova.$"

RustboroCity_Gym_Text_RoxanneIntro:
	.string "Olá, eu sou ROXANNE, a LÍDER DO\n"
	.string "GINÁSIO POKéMON de RUSTBORO.\p"
	.string "Virei LÍDER DE GINÁSIO para pôr em\n"
	.string "prática nas batalhas o que aprendi na\l"
	.string "ESCOLA DE TREINADORES POKéMON.\p"
	.string "Poderia me mostrar como você batalha,\n"
	.string "e com quais POKéMON?$"

RustboroCity_Gym_Text_RoxanneDefeat:
	.string "Então…\n"
	.string "Eu perdi…\p"
	.string "Parece que ainda tenho muito\n"
	.string "a aprender…\p"
	.string "Entendi.\p"
	.string "As regras da LIGA POKéMON dizem que\n"
	.string "os TREINADORES recebem isto quando\l"
	.string "vencem um LÍDER DE GINÁSIO.\p"
	.string "Por favor, aceite a INSÍGNIA DA PEDRA\n"
	.string "oficial da LIGA POKéMON.$"

RustboroCity_Gym_Text_ReceivedStoneBadge:
	.string "{PLAYER} recebeu a INSÍGNIA DA PEDRA\n"
	.string "da ROXANNE.$"

RustboroCity_Gym_Text_StoneBadgeInfoTakeThis:
	.string "A INSÍGNIA DA PEDRA aumenta o ATAQUE\n"
	.string "dos seus POKéMON.\p"
	.string "Também permite usar a MO CORTAR\n"
	.string "fora das batalhas.\p"
	.string "Por favor, leve isto também.$"

RustboroCity_Gym_Text_ExplainRockTomb:
	.string "Essa MÁQUINA TÉCNICA, a MT39,\n"
	.string "tem TUMBA ROCHAS.\p"
	.string "Além de causar dano jogando pedras,\n"
	.string "ela baixa a VELOCIDADE.\p"
	.string "Ao usar uma MT, o POKéMON aprende\n"
	.string "o golpe na hora.\p"
	.string "Lembre: uma MT só pode ser usada\n"
	.string "uma vez, então pense antes de usar.$"

RustboroCity_Gym_Text_RoxannePostBattle:
	.string "Já que você é tão forte, devia\n"
	.string "desafiar outros LÍDERES DE GINÁSIO.\p"
	.string "Batalhando com muitos TREINADORES,\n"
	.string "você vai aprender muita coisa.$"

RustboroCity_Gym_Text_GymStatue:
	.string "GINÁSIO POKéMON DE RUSTBORO$"

RustboroCity_Gym_Text_GymStatueCertified:
	.string "GINÁSIO POKéMON DE RUSTBORO\p"
	.string "TREINADORES APROVADOS POR ROXANNE:\n"
	.string "{PLAYER}$"

RustboroCity_Gym_Text_RoxanneRegisterCall:
	.string "… … … … … …\n"
	.string "… … … … … Bipe!\p"
	.string "ROXANNE: Ah, alô, {PLAYER}?\n"
	.string "Aqui é a ROXANNE, de RUSTBORO.\p"
	.string "O BRAWLY me contou que você está\n"
	.string "subindo no ranking dos TREINADORES\l"
	.string "numa velocidade incrível.\p"
	.string "Bom, prefiro não pensar que\n"
	.string "não consigo te vencer.\p"
	.string "Prometo deixar meu GINÁSIO em forma\n"
	.string "pra você nos desafiar de novo.\p"
	.string "Espero que esse dia chegue logo,\n"
	.string "pra você e pra mim!\p"
	.string "… … … … … …\n"
	.string "… … … … … Clique!$"

RustboroCity_Gym_Text_RegisteredRoxanne:
	.string "A LÍDER DE GINÁSIO ROXANNE foi\n"
	.string "registrada no POKéNAV.$"

RustboroCity_Gym_Text_RoxannePreRematch:
	.string "ROXANNE: Que bom te ver de novo.\n"
	.string "Eu sou ROXANNE, a LÍDER daqui.\p"
	.string "Tenho certeza de que tivemos muitas\n"
	.string "batalhas desde a última vez.\p"
	.string "Quero ver o quanto melhoramos.\n"
	.string "Vamos batalhar!$"

RustboroCity_Gym_Text_RoxanneRematchDefeat:
	.string "Grr…\n"
	.string "Perdi de novo…$"

RustboroCity_Gym_Text_RoxannePostRematch:
	.string "ROXANNE: Ainda tenho muito a aprender\n"
	.string "sobre batalhas POKéMON.\p"
	.string "E saber disso me faz amar as\n"
	.string "batalhas ainda mais!$"

RustboroCity_Gym_Text_RoxanneRematchNeedTwoMons:
	.string "ROXANNE: Que bom te ver de novo.\n"
	.string "Eu sou ROXANNE, a LÍDER daqui.\p"
	.string "Tenho certeza de que tivemos muitas\n"
	.string "batalhas desde a última vez.\p"
	.string "Quero ver o quanto melhoramos…\p"
	.string "Ah, não…\n"
	.string "Você só tem um POKéMON com você?\p"
	.string "Volte com pelo menos mais um\n"
	.string "POKéMON, por favor.$"
