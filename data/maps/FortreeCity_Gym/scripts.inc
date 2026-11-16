FortreeCity_Gym_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, FortreeCity_Gym_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, FortreeCity_Gym_OnWarp
	.byte 0

FortreeCity_Gym_OnTransition:
	special RotatingGate_InitPuzzle
	end

FortreeCity_Gym_OnWarp:
	map_script_2 VAR_TEMP_0, VAR_TEMP_0, FortreeCity_Gym_EventScript_InitRotatingGates
	.short 0

@ NOTE: This rotating gate puzzle makes use of VAR_TEMP_0 - VAR_TEMP_3
FortreeCity_Gym_EventScript_InitRotatingGates::
	special RotatingGate_InitPuzzleAndGraphics
	end

FortreeCity_Gym_EventScript_Winona::
	trainerbattle_single TRAINER_WINONA_1, FortreeCity_Gym_Text_WinonaIntro, FortreeCity_Gym_Text_WinonaDefeat, FortreeCity_Gym_EventScript_WinonaDefeated, NO_MUSIC
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, FortreeCity_Gym_EventScript_WinonaRematch
	goto_if_unset FLAG_RECEIVED_TM_AERIAL_ACE, FortreeCity_Gym_EventScript_GiveAerialAce2
	msgbox FortreeCity_Gym_Text_WinonaPostBattle, MSGBOX_DEFAULT
	release
	end

FortreeCity_Gym_EventScript_WinonaDefeated::
	message FortreeCity_Gym_Text_ReceivedFeatherBadge
	waitmessage
	call Common_EventScript_PlayGymBadgeFanfare
	msgbox FortreeCity_Gym_Text_ExplainFeatherBadgeTakeThis, MSGBOX_DEFAULT
	setflag FLAG_DEFEATED_FORTREE_GYM
	setflag FLAG_BADGE06_GET
	setvar VAR_0x8008, 6
	call Common_EventScript_SetGymTrainers
	call FortreeCity_Gym_EventScript_GiveAerialAce
	closemessage
	delay 30
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox FortreeCity_Gym_Text_RegisteredWinona, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	delay 30
	setflag FLAG_ENABLE_WINONA_MATCH_CALL
	setvar VAR_SCOTT_FORTREE_CALL_STEP_COUNTER, 0
	setflag FLAG_SCOTT_CALL_FORTREE_GYM
	release
	end

FortreeCity_Gym_EventScript_GiveAerialAce2::
	giveitem ITEM_TM_AERIAL_ACE
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	msgbox FortreeCity_Gym_Text_ExplainAerialAce, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_AERIAL_ACE
	release
	end

FortreeCity_Gym_EventScript_GiveAerialAce::
	giveitem ITEM_TM_AERIAL_ACE
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_BagIsFull
	msgbox FortreeCity_Gym_Text_ExplainAerialAce, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_AERIAL_ACE
	return

FortreeCity_Gym_EventScript_WinonaRematch::
	trainerbattle_rematch_double TRAINER_WINONA_1, FortreeCity_Gym_Text_WinonaPreRematch, FortreeCity_Gym_Text_WinonaRematchDefeat, FortreeCity_Gym_Text_WinonaRematchNeedTwoMons
	msgbox FortreeCity_Gym_Text_WinonaPostRematch, MSGBOX_AUTOCLOSE
	end

FortreeCity_Gym_EventScript_Jared::
	trainerbattle_single TRAINER_JARED, FortreeCity_Gym_Text_JaredIntro, FortreeCity_Gym_Text_JaredDefeat
	msgbox FortreeCity_Gym_Text_JaredPostBattle, MSGBOX_AUTOCLOSE
	end

FortreeCity_Gym_EventScript_Edwardo::
	trainerbattle_single TRAINER_EDWARDO, FortreeCity_Gym_Text_EdwardoIntro, FortreeCity_Gym_Text_EdwardoDefeat
	msgbox FortreeCity_Gym_Text_EdwardoPostBattle, MSGBOX_AUTOCLOSE
	end

FortreeCity_Gym_EventScript_Flint::
	trainerbattle_single TRAINER_FLINT, FortreeCity_Gym_Text_FlintIntro, FortreeCity_Gym_Text_FlintDefeat
	msgbox FortreeCity_Gym_Text_FlintPostBattle, MSGBOX_AUTOCLOSE
	end

FortreeCity_Gym_EventScript_Ashley::
	trainerbattle_single TRAINER_ASHLEY, FortreeCity_Gym_Text_AshleyIntro, FortreeCity_Gym_Text_AshleyDefeat
	msgbox FortreeCity_Gym_Text_AshleyPostBattle, MSGBOX_AUTOCLOSE
	end

FortreeCity_Gym_EventScript_Humberto::
	trainerbattle_single TRAINER_HUMBERTO, FortreeCity_Gym_Text_HumbertoIntro, FortreeCity_Gym_Text_HumbertoDefeat
	msgbox FortreeCity_Gym_Text_HumbertoPostBattle, MSGBOX_AUTOCLOSE
	end

FortreeCity_Gym_EventScript_Darius::
	trainerbattle_single TRAINER_DARIUS, FortreeCity_Gym_Text_DariusIntro, FortreeCity_Gym_Text_DariusDefeat
	msgbox FortreeCity_Gym_Text_DariusPostBattle, MSGBOX_AUTOCLOSE
	end

FortreeCity_Gym_EventScript_GymGuide::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_FORTREE_GYM, FortreeCity_Gym_EventScript_GymGuidePostVictory
	msgbox FortreeCity_Gym_Text_GymGuideAdvice, MSGBOX_DEFAULT
	release
	end

FortreeCity_Gym_EventScript_GymGuidePostVictory::
	msgbox FortreeCity_Gym_Text_GymGuidePostVictory, MSGBOX_DEFAULT
	release
	end

FortreeCity_Gym_EventScript_LeftGymStatue::
	lockall
	goto_if_set FLAG_BADGE06_GET, FortreeCity_Gym_EventScript_GymStatueCertified
	goto FortreeCity_Gym_EventScript_GymStatue
	end

FortreeCity_Gym_EventScript_RightGymStatue::
	lockall
	goto_if_set FLAG_BADGE06_GET, FortreeCity_Gym_EventScript_GymStatueCertified
	goto FortreeCity_Gym_EventScript_GymStatue
	end

FortreeCity_Gym_EventScript_GymStatueCertified::
	msgbox FortreeCity_Gym_Text_GymStatueCertified, MSGBOX_DEFAULT
	releaseall
	end

FortreeCity_Gym_EventScript_GymStatue::
	msgbox FortreeCity_Gym_Text_GymStatue, MSGBOX_DEFAULT
	releaseall
	end

FortreeCity_Gym_Text_GymGuideAdvice:
	.string "E aí, como vai, {PLAYER},\n"
	.string "futura estrela da LIGA?\p"
	.string "A WINONA, LÍDER do GINÁSIO de\n"
	.string "FORTREE, é mestre do tipo VOADOR.\p"
	.string "Ela espera no fundo do GINÁSIO,\n"
	.string "atrás das portas giratórias.\p"
	.string "Ela aguarda novos desafiantes\n"
	.string "que queiram alçar voo!\p"
	.string "Certo, manda ver!$"

FortreeCity_Gym_Text_GymGuidePostVictory:
	.string "Conseguiu!\n"
	.string "Você decolou!$"

FortreeCity_Gym_Text_JaredIntro:
	.string "Contemple o estilo elegante de\n"
	.string "batalha dos POKéMON PÁSSAROS!$"

FortreeCity_Gym_Text_JaredDefeat:
	.string "Você…\n"
	.string "Você é forte…$"

FortreeCity_Gym_Text_JaredPostBattle:
	.string "Um TREINADOR precisa ser esperto pra\n"
	.string "acompanhar as reviravoltas.\p"
	.string "Você tem esperteza pra chegar até\n"
	.string "a nossa LÍDER?$"

FortreeCity_Gym_Text_EdwardoIntro:
	.string "Quem ama POKéMON PÁSSAROS sonha\n"
	.string "em entrar neste GINÁSIO.\p"
	.string "Como membro do GINÁSIO de FORTREE,\n"
	.string "não posso perder!$"

FortreeCity_Gym_Text_EdwardoDefeat:
	.string "Foi peso demais pra eu aguentar…$"

FortreeCity_Gym_Text_EdwardoPostBattle:
	.string "O mundo é enorme e está cheio de\n"
	.string "TREINADORES fortes.\p"
	.string "Vou continuar treinando pra ficar\n"
	.string "ainda mais forte.$"

FortreeCity_Gym_Text_FlintIntro:
	.string "A WINONA, a nossa LÍDER, nem precisa\n"
	.string "perder tempo com você!\p"
	.string "Eu já dou conta!$"

FortreeCity_Gym_Text_FlintDefeat:
	.string "WINONA, eu…\n"
	.string "Eu perdi!$"

FortreeCity_Gym_Text_FlintPostBattle:
	.string "A WINONA é linda e forte.\n"
	.string "É a LÍDER perfeita!\p"
	.string "Que vergonha…$"

FortreeCity_Gym_Text_AshleyIntro:
	.string "A própria WINONA me ensinou!\n"
	.string "Você não me vence fácil!$"

FortreeCity_Gym_Text_AshleyDefeat:
	.string "Fui derrotada…$"

FortreeCity_Gym_Text_AshleyPostBattle:
	.string "Graças à WINONA, o povo de FORTREE\n"
	.string "vive sem medo.$"

FortreeCity_Gym_Text_HumbertoIntro:
	.string "Quando a WINONA batalha, o rosto dela\n"
	.string "brilha com uma bela determinação…\p"
	.string "Não vou deixar você ver essa cena\n"
	.string "linda!$"

FortreeCity_Gym_Text_HumbertoDefeat:
	.string "Ugh!\n"
	.string "Não consegui te parar.$"

FortreeCity_Gym_Text_HumbertoPostBattle:
	.string "Tome cuidado!\n"
	.string "Se ficar olhando pra WINONA, você\l"
	.string "vai se esborrachar no chão!$"

FortreeCity_Gym_Text_DariusIntro:
	.string "Saiba que existem POKéMON do tipo\n"
	.string "VOADOR de todo tipo.\p"
	.string "Você sabe disso, né?$"

FortreeCity_Gym_Text_DariusDefeat:
	.string "Parece que você entende do assunto!$"

FortreeCity_Gym_Text_DariusPostBattle:
	.string "Tá, você me venceu.\n"
	.string "Mas tome cuidado! Os POKéMON da\l"
	.string "nossa LÍDER WINONA não brincam.$"

FortreeCity_Gym_Text_WinonaIntro:
	.string "Sou a WINONA. Sou a LÍDER do\n"
	.string "GINÁSIO POKéMON de FORTREE.\p"
	.string "Tornei-me uma só com os POKéMON\n"
	.string "PÁSSAROS e cruzei os céus…\p"
	.string "Por mais dura que fosse a batalha,\n"
	.string "triunfamos com elegância…\p"
	.string "Contemple a coreografia elegante\n"
	.string "minha e dos POKéMON PÁSSAROS!$"

FortreeCity_Gym_Text_WinonaDefeat:
	.string "Nunca vi um TREINADOR comandar\n"
	.string "POKéMON com mais elegância que eu…\p"
	.string "Em reconhecimento à sua habilidade,\n"
	.string "entrego esta INSÍGNIA.$"

FortreeCity_Gym_Text_ReceivedFeatherBadge:
	.string "{PLAYER} recebeu a INSÍGNIA DA PENA\n"
	.string "da WINONA.$"

FortreeCity_Gym_Text_ExplainFeatherBadgeTakeThis:
	.string "Com a INSÍGNIA DA PENA, todos os\n"
	.string "POKéMON até o NV. 70, mesmo os\l"
	.string "recebidos em trocas, vão obedecer\l"
	.string "a todos os seus comandos.\p"
	.string "Você também poderá usar o golpe\n"
	.string "de MO VOAR fora da batalha.\p"
	.string "E isto…\n"
	.string "Isto é um presente meu.$"

FortreeCity_Gym_Text_ExplainAerialAce:
	.string "A MT40 contém ÁS DOS ARES.\p"
	.string "A velocidade dele…\n"
	.string "Nenhum POKéMON consegue se esquivar.\p"
	.string "… … … … … …$"

FortreeCity_Gym_Text_RegisteredWinona:
	.string "A LÍDER WINONA foi registrada\n"
	.string "no POKéNAV.$"

FortreeCity_Gym_Text_WinonaPostBattle:
	.string "Mesmo derrotada por você, continuo\n"
	.string "dedicada aos POKéMON PÁSSAROS.$"

FortreeCity_Gym_Text_GymStatue:
	.string "GINÁSIO POKéMON DE FORTREE$"

FortreeCity_Gym_Text_GymStatueCertified:
	.string "GINÁSIO POKéMON DE FORTREE\p"
	.string "TREINADORES APROVADOS POR WINONA:\n"
	.string "{PLAYER}$"

FortreeCity_Gym_Text_WinonaPreRematch:
	.string "WINONA: Nós, humanos, nunca\n"
	.string "escapamos da gravidade.\p"
	.string "Mas, buscando a excelência, temos\n"
	.string "o poder de voar!\p"
	.string "Você alcançou o topo como atual\n"
	.string "CAMPEÃO da LIGA.\p"
	.string "Por favor, me mostre o seu poder\n"
	.string "em pleno voo!$"

FortreeCity_Gym_Text_WinonaRematchDefeat:
	.string "De novo não alcancei a sua altura…$"

FortreeCity_Gym_Text_WinonaPostRematch:
	.string "WINONA: Mesmo tendo perdido, as asas\n"
	.string "do meu coração seguem inteiras.\p"
	.string "Posso subir e voar outra vez, e\n"
	.string "mais outra.\p"
	.string "Tenho certeza disso!$"

FortreeCity_Gym_Text_WinonaRematchNeedTwoMons:
	.string "WINONA: Nós, humanos, nunca\n"
	.string "escapamos da gravidade.\p"
	.string "Mas, buscando a excelência, temos\n"
	.string "o poder de voar!\p"
	.string "Você alcançou o topo como atual\n"
	.string "CAMPEÃO da LIGA.\p"
	.string "Por favor, me mostre o seu poder\n"
	.string "em pleno voo!\p"
	.string "Ah, não… Você só tem um POKéMON?\p"
	.string "Por favor, volte com pelo menos\n"
	.string "dois POKéMON.$"
