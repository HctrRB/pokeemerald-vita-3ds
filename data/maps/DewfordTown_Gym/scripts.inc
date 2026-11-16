DewfordTown_Gym_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, DewfordTown_Gym_OnTransition
	.byte 0

DewfordTown_Gym_OnTransition:
	call DewfordTown_Gym_EventScript_SetFlashLevel
	end

DewfordTown_Gym_EventScript_SetFlashLevel::
	goto_if_defeated TRAINER_BRAWLY_1, DewfordTown_Gym_EventScript_SetLightsOn
	call DewfordTown_Gym_EventScript_CountTrainersDefeated
	copyvar VAR_0x8001, VAR_0x8000
	goto_if_eq VAR_0x8000, 0, DewfordTown_Gym_EventScript_SetFlashLevel7
	goto_if_eq VAR_0x8000, 1, DewfordTown_Gym_EventScript_SetFlashLevel6
	goto_if_eq VAR_0x8000, 2, DewfordTown_Gym_EventScript_SetFlashLevel5
	goto_if_eq VAR_0x8000, 3, DewfordTown_Gym_EventScript_SetFlashLevel4
	goto_if_eq VAR_0x8000, 4, DewfordTown_Gym_EventScript_SetFlashLevel3
	goto_if_eq VAR_0x8000, 5, DewfordTown_Gym_EventScript_SetFlashLevel2
	goto DewfordTown_Gym_EventScript_SetFlashLevel1

DewfordTown_Gym_EventScript_SetLightsOn::
	setflashlevel 0
	return

@ Brightest
DewfordTown_Gym_EventScript_SetFlashLevel1::
	setflashlevel 1
	return

DewfordTown_Gym_EventScript_SetFlashLevel2::
	setflashlevel 2
	return

DewfordTown_Gym_EventScript_SetFlashLevel3::
	setflashlevel 3
	return

DewfordTown_Gym_EventScript_SetFlashLevel4::
	setflashlevel 4
	return

DewfordTown_Gym_EventScript_SetFlashLevel5::
	setflashlevel 5
	return

DewfordTown_Gym_EventScript_SetFlashLevel6::
	setflashlevel 6
	return

@ Darkest
DewfordTown_Gym_EventScript_SetFlashLevel7::
	setflashlevel 7
	return

DewfordTown_Gym_EventScript_BrightenRoom::
	call DewfordTown_Gym_EventScript_CountTrainersDefeated
	nop1
	goto_if_eq VAR_0x8000, VAR_0x8001, DewfordTown_Gym_EventScript_NoLightChange
	copyvar VAR_0x8001, VAR_0x8000
	goto_if_eq VAR_0x8000, 1, DewfordTown_Gym_EventScript_AnimateFlash1Trainer
	goto_if_eq VAR_0x8000, 2, DewfordTown_Gym_EventScript_AnimateFlash2Trainers
	goto_if_eq VAR_0x8000, 3, DewfordTown_Gym_EventScript_AnimateFlash3Trainers
	goto_if_eq VAR_0x8000, 4, DewfordTown_Gym_EventScript_AnimateFlash4Trainers
	goto_if_eq VAR_0x8000, 5, DewfordTown_Gym_EventScript_AnimateFlash5Trainers
	goto_if_eq VAR_0x8000, 6, DewfordTown_Gym_EventScript_AnimateFlash6Trainers

DewfordTown_Gym_EventScript_NoLightChange::
	return

/* NOTE: A little confusingly, a larger animateflash param value is a smaller flash radius*/
DewfordTown_Gym_EventScript_AnimateFlash1Trainer::
	playse SE_SWITCH
	animateflash 6
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash2Trainers::
	playse SE_SWITCH
	animateflash 5
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash3Trainers::
	playse SE_SWITCH
	animateflash 4
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash4Trainers::
	playse SE_SWITCH
	animateflash 3
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash5Trainers::
	playse SE_SWITCH
	animateflash 2
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash6Trainers::
	playse SE_SWITCH
	animateflash 1
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlashFullBrightness::
	playse SE_SWITCH
	animateflash 0
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_CountTrainersDefeated::
	setvar VAR_0x8000, 0
	goto_if_not_defeated TRAINER_TAKAO, DewfordTown_Gym_EventScript_CheckJocelyn
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckJocelyn::
	goto_if_not_defeated TRAINER_JOCELYN, DewfordTown_Gym_EventScript_CheckLaura
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckLaura::
	goto_if_not_defeated TRAINER_LAURA, DewfordTown_Gym_EventScript_CheckBrenden
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckBrenden::
	goto_if_not_defeated TRAINER_BRENDEN, DewfordTown_Gym_EventScript_CheckCristian
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckCristian::
	goto_if_not_defeated TRAINER_CRISTIAN, DewfordTown_Gym_EventScript_CheckLilith
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckLilith::
	goto_if_not_defeated TRAINER_LILITH, DewfordTown_Gym_EventScript_StopCountingTrainers
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_StopCountingTrainers::
	return

DewfordTown_Gym_EventScript_Brawly::
	trainerbattle_single TRAINER_BRAWLY_1, DewfordTown_Gym_Text_BrawlyIntro, DewfordTown_Gym_Text_BrawlyDefeat, DewfordTown_Gym_EventScript_BrawlyDefeated, NO_MUSIC
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, DewfordTown_Gym_EventScript_BrawlyRematch
	goto_if_unset FLAG_RECEIVED_TM_BULK_UP, DewfordTown_Gym_EventScript_GiveBulkUp2
	msgbox DewfordTown_Gym_Text_BrawlyPostBattle, MSGBOX_DEFAULT
	release
	end

DewfordTown_Gym_EventScript_BrawlyDefeated::
	call DewfordTown_Gym_EventScript_AnimateFlashFullBrightness
	message DewfordTown_Gym_Text_ReceivedKnuckleBadge
	waitmessage
	call Common_EventScript_PlayGymBadgeFanfare
	msgbox DewfordTown_Gym_Text_KnuckleBadgeInfoTakeThis, MSGBOX_DEFAULT
	setflag FLAG_DEFEATED_DEWFORD_GYM
	setflag FLAG_BADGE02_GET
	addvar VAR_PETALBURG_GYM_STATE, 1
	call_if_eq VAR_PETALBURG_GYM_STATE, 6, Common_EventScript_ReadyPetalburgGymForBattle
	setvar VAR_0x8008, 2
	call Common_EventScript_SetGymTrainers
	call DewfordTown_Gym_EventScript_GiveBulkUp
	closemessage
	delay 30
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox DewfordTown_Gym_Text_RegisteredBrawly, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	delay 30
	setflag FLAG_ENABLE_BRAWLY_MATCH_CALL
	setvar VAR_ROXANNE_CALL_STEP_COUNTER, 0
	setflag FLAG_ENABLE_ROXANNE_FIRST_CALL
	release
	end

DewfordTown_Gym_EventScript_GiveBulkUp::
	giveitem ITEM_TM_BULK_UP
	goto_if_eq VAR_RESULT, 0, Common_EventScript_BagIsFull
	msgbox DewfordTown_Gym_Text_ExplainBulkUp, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_BULK_UP
	return

DewfordTown_Gym_EventScript_GiveBulkUp2:
	giveitem ITEM_TM_BULK_UP
	goto_if_eq VAR_RESULT, 0, Common_EventScript_ShowBagIsFull
	msgbox DewfordTown_Gym_Text_ExplainBulkUp, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_BULK_UP
	release
	end

DewfordTown_Gym_EventScript_BrawlyRematch::
	trainerbattle_rematch_double TRAINER_BRAWLY_1, DewfordTown_Gym_Text_BrawlyPreRematch, DewfordTown_Gym_Text_BrawlyRematchDefeat, DewfordTown_Gym_Text_BrawlyRematchNeedTwoMons
	msgbox DewfordTown_Gym_Text_BrawlyPostRematch, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_Takao::
	trainerbattle_single TRAINER_TAKAO, DewfordTown_Gym_Text_TakaoIntro, DewfordTown_Gym_Text_TakaoDefeat, DewfordTown_Gym_EventScript_TakaoBrightenRoom
	msgbox DewfordTown_Gym_Text_TakaoPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_TakaoBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Jocelyn::
	trainerbattle_single TRAINER_JOCELYN, DewfordTown_Gym_Text_JocelynIntro, DewfordTown_Gym_Text_JocelynDefeat, DewfordTown_Gym_EventScript_JocelynBrightenRoom
	msgbox DewfordTown_Gym_Text_JocelynPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_JocelynBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Laura::
	trainerbattle_single TRAINER_LAURA, DewfordTown_Gym_Text_LauraIntro, DewfordTown_Gym_Text_LauraDefeat, DewfordTown_Gym_EventScript_LauraBrightenRoom
	msgbox DewfordTown_Gym_Text_LauraPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_LauraBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Brenden::
	trainerbattle_single TRAINER_BRENDEN, DewfordTown_Gym_Text_BrendenIntro, DewfordTown_Gym_Text_BrendenDefeat, DewfordTown_Gym_EventScript_BrendenBrightenRoom
	msgbox DewfordTown_Gym_Text_BrendenPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_BrendenBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Cristian::
	trainerbattle_single TRAINER_CRISTIAN, DewfordTown_Gym_Text_CristianIntro, DewfordTown_Gym_Text_CristianDefeat, DewfordTown_Gym_EventScript_CristianBrightenRoom
	msgbox DewfordTown_Gym_Text_CristianPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_CristianBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Lilith::
	trainerbattle_single TRAINER_LILITH, DewfordTown_Gym_Text_LilithIntro, DewfordTown_Gym_Text_LilithDefeat, DewfordTown_Gym_EventScript_LilithBrightenRoom
	msgbox DewfordTown_Gym_Text_LilithPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_LilithBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_GymGuide::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_DEWFORD_GYM, DewfordTown_Gym_EventScript_GymGuidePostVictory
	msgbox DewfordTown_Gym_Text_GymGuideAdvice, MSGBOX_DEFAULT
	release
	end

DewfordTown_Gym_EventScript_GymGuidePostVictory::
	msgbox DewfordTown_Gym_Text_GymGuidePostVictory, MSGBOX_DEFAULT
	release
	end

DewfordTown_Gym_EventScript_LeftGymStatue::
	lockall
	goto_if_set FLAG_BADGE02_GET, DewfordTown_Gym_EventScript_GymStatueCertified
	goto DewfordTown_Gym_EventScript_GymStatue
	end

DewfordTown_Gym_EventScript_RightGymStatue::
	lockall
	goto_if_set FLAG_BADGE02_GET, DewfordTown_Gym_EventScript_GymStatueCertified
	goto DewfordTown_Gym_EventScript_GymStatue
	end

DewfordTown_Gym_EventScript_GymStatueCertified::
	msgbox DewfordTown_Gym_Text_GymStatueCertified, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Gym_EventScript_GymStatue::
	msgbox DewfordTown_Gym_Text_GymStatue, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Gym_Text_GymGuideAdvice:
	.string "E aí, tudo certo, {PLAYER}, rumo\n"
	.string "ao título de CAMPEÃO?\p"
	.string "O LÍDER DE GINÁSIO de DEWFORD, BRAWLY,\n"
	.string "comanda POKéMON do tipo LUTADOR.\p"
	.string "Enfrentá-lo com POKéMON do tipo\n"
	.string "NORMAL é pedir pra ter problema.\p"
	.string "POKéMON LUTADORES dão uma surra\n"
	.string "em POKéMON NORMAIS,\l"
	.string "então cuidado!\p"
	.string "E pior: o GINÁSIO é escuro como\n"
	.string "o fundo do mar.\p"
	.string "Mas ele vai clareando quando você\n"
	.string "vence os TREINADORES do caminho.\p"
	.string "Bom, vai fundo!$"

DewfordTown_Gym_Text_GymGuidePostVictory:
	.string "Uau! Ficou claro demais aqui!\p"
	.string "O seu futuro também é brilhante!$"

DewfordTown_Gym_Text_TakaoIntro:
	.string "Quem pode mais, chora menos!\n"
	.string "Vem!$"

DewfordTown_Gym_Text_TakaoDefeat:
	.string "Faltou força…$"

DewfordTown_Gym_Text_TakaoPostBattle:
	.string "A sua técnica venceu a minha força!\n"
	.string "Merece elogios!$"

DewfordTown_Gym_Text_JocelynIntro:
	.string "Não precisa nem chamar o BRAWLY!\p"
	.string "Eu acabo com você!$"

DewfordTown_Gym_Text_JocelynDefeat:
	.string "Mas… Como?\n"
	.string "Como eu perdi tão fácil?$"

DewfordTown_Gym_Text_JocelynPostBattle:
	.string "Não vou desperdiçar esta derrota.\n"
	.string "Vou usar como motivação pra treinar!\p"
	.string "Um dia vou virar LÍDER DE GINÁSIO…\p"
	.string "Não, vou ser CAMPEÃ da LIGA\n"
	.string "POKéMON!$"

DewfordTown_Gym_Text_LauraIntro:
	.string "Nem pense em me ignorar!\n"
	.string "Não é culpa minha se você chorar!$"

DewfordTown_Gym_Text_LauraDefeat:
	.string "Buáááááá!\n"
	.string "Perdi!$"

DewfordTown_Gym_Text_LauraPostBattle:
	.string "Eu batalho no escuro pra deixar meu\n"
	.string "coração mais forte.\p"
	.string "Mas, como é tão escuro, nunca\n"
	.string "consigo ver o rosto do BRAWLY…$"

DewfordTown_Gym_Text_LilithIntro:
	.string "Hihihi…\n"
	.string "Tropeçando no escuro, é?$"

DewfordTown_Gym_Text_LilithDefeat:
	.string "Ah, você…\n"
	.string "Não quero nem te conhecer!$"

DewfordTown_Gym_Text_LilithPostBattle:
	.string "Bem que você merecia se perder e\n"
	.string "voltar pra entrada de novo!$"

DewfordTown_Gym_Text_BrendenIntro:
	.string "Vou te mostrar a garra de um\n"
	.string "homem do mar!$"

DewfordTown_Gym_Text_BrendenDefeat:
	.string "Como isso aconteceu?\n"
	.string "Não é do meu feitio perder assim!$"

DewfordTown_Gym_Text_BrendenPostBattle:
	.string "Ah, é! Você merece ver o\n"
	.string "nosso LÍDER DE GINÁSIO.$"

DewfordTown_Gym_Text_CristianIntro:
	.string "Se quer passar, vai ter que\n"
	.string "passar por mim!$"

DewfordTown_Gym_Text_CristianDefeat:
	.string "Grrrrr!\n"
	.string "Fui completamente superado!$"

DewfordTown_Gym_Text_CristianPostBattle:
	.string "Você venceu!\n"
	.string "Vai atrás dessa INSÍGNIA!$"

DewfordTown_Gym_Text_BrawlyIntro:
	.string "Eu sou o BRAWLY!\n"
	.string "O LÍDER DE GINÁSIO de DEWFORD!\p"
	.string "Fui sacudido pelas ondas bravas desta\n"
	.string "região e fiquei forte na caverna\l"
	.string "escura como breu!\p"
	.string "Então você quer me desafiar?\n"
	.string "Vamos ver do que você é capaz!$"

DewfordTown_Gym_Text_BrawlyDefeat:
	.string "Uau, nossa! Você fez uma onda bem\n"
	.string "maior do que eu esperava!\p"
	.string "Você me afogou!\p"
	.string "Tá bom, você me pegou.\n"
	.string "Pegue esta INSÍGNIA!$"

DewfordTown_Gym_Text_ReceivedKnuckleBadge:
	.string "{PLAYER} recebeu a INSÍGNIA DO PUNHO\n"
	.string "do BRAWLY.$"

DewfordTown_Gym_Text_KnuckleBadgeInfoTakeThis:
	.string "A INSÍGNIA DO PUNHO faz todos os\n"
	.string "POKéMON até o Nível 30, até os de\l"
	.string "troca, obedecerem sem discutir.\p"
	.string "E você vai poder usar a MO CLARÃO\n"
	.string "fora das batalhas.\p"
	.string "Se quiser, use esta MÁQUINA TÉCNICA\n"
	.string "também.$"

DewfordTown_Gym_Text_ExplainBulkUp:
	.string "Essa MT08 tem FORÇA BRUTA.\p"
	.string "É um golpe que aumenta o ATAQUE\n"
	.string "e a DEFESA.\p"
	.string "… … … … … …$"

DewfordTown_Gym_Text_RegisteredBrawly:
	.string "O LÍDER DE GINÁSIO BRAWLY foi\n"
	.string "registrado no POKéNAV.$"

DewfordTown_Gym_Text_BrawlyPostBattle:
	.string "Já vejo o seu talento virando uma onda\n"
	.string "gigante que vai causar uma tempestade\l"
	.string "de espanto entre os TREINADORES!$"

DewfordTown_Gym_Text_GymStatue:
	.string "GINÁSIO POKéMON DE DEWFORD$"

DewfordTown_Gym_Text_GymStatueCertified:
	.string "GINÁSIO POKéMON DE DEWFORD\p"
	.string "TREINADORES APROVADOS POR BRAWLY:\n"
	.string "{PLAYER}$"

DewfordTown_Gym_Text_BrawlyPreRematch:
	.string "BRAWLY: A onda pode recuar, mas\n"
	.string "sempre volta pra praia.\p"
	.string "Uma onda gigante de talento como\n"
	.string "você… Eu sabia que ia voltar!\p"
	.string "Mostra o quanto você\n"
	.string "subiu!$"

DewfordTown_Gym_Text_BrawlyRematchDefeat:
	.string "Uau!\n"
	.string "Me afoguei de novo!$"

DewfordTown_Gym_Text_BrawlyPostRematch:
	.string "BRAWLY: Batalhar é vasto e profundo.\n"
	.string "Não dá pra ver o fundo.\l"
	.string "É igualzinho ao mar de HOENN!$"

DewfordTown_Gym_Text_BrawlyRematchNeedTwoMons:
	.string "BRAWLY: A onda pode recuar, mas\n"
	.string "sempre volta pra praia.\p"
	.string "Uma onda gigante de talento como\n"
	.string "você… Eu sabia que ia voltar!\p"
	.string "Hã? Você só tem um POKéMON\n"
	.string "que pode batalhar.\p"
	.string "Desculpa, mas queria que você voltasse\n"
	.string "com pelo menos dois POKéMON.$"
