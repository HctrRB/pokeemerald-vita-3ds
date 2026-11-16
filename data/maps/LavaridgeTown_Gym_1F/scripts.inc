LavaridgeTown_Gym_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, LavaridgeTown_Gym_1F_OnTransition
	.byte 0

LavaridgeTown_Gym_1F_OnTransition:
	call LavaridgeTown_Gym_1F_EventScript_SetTrainerTempVars
	call LavaridgeTown_Gym_1F_EventScript_CheckBuryTrainers
	end

/* Unclear where/if these temp vars are getting checked*/
LavaridgeTown_Gym_1F_EventScript_SetTrainerTempVars::
	setvar VAR_TEMP_B, 0
	setvar VAR_TEMP_C, 0
	setvar VAR_TEMP_D, 0
	setvar VAR_TEMP_E, 0
	setvar VAR_TEMP_F, 0
	goto_if_defeated TRAINER_COLE, LavaridgeTown_Gym_1F_EventScript_SetGeraldTempVar
	setvar VAR_TEMP_B, 1
LavaridgeTown_Gym_1F_EventScript_SetGeraldTempVar::
	goto_if_defeated TRAINER_GERALD, LavaridgeTown_Gym_1F_EventScript_SetAxleTempVar
	setvar VAR_TEMP_C, 1
LavaridgeTown_Gym_1F_EventScript_SetAxleTempVar::
	goto_if_defeated TRAINER_AXLE, LavaridgeTown_Gym_1F_EventScript_SetDanielleTempVar
	setvar VAR_TEMP_D, 1
LavaridgeTown_Gym_1F_EventScript_SetDanielleTempVar::
	goto_if_defeated TRAINER_DANIELLE, LavaridgeTown_Gym_1F_EventScript_EndSetTrainerTempVars
	setvar VAR_TEMP_E, 1
LavaridgeTown_Gym_1F_EventScript_EndSetTrainerTempVars::
	return

LavaridgeTown_Gym_1F_EventScript_CheckBuryTrainers::
	goto_if_defeated TRAINER_COLE, LavaridgeTown_Gym_1F_EventScript_CheckBuryGerald
	setobjectmovementtype LOCALID_COLE, MOVEMENT_TYPE_BURIED
LavaridgeTown_Gym_1F_EventScript_CheckBuryGerald::
	goto_if_defeated TRAINER_GERALD, LavaridgeTown_Gym_1F_EventScript_CheckBuryAxle
	setobjectmovementtype LOCALID_GERALD, MOVEMENT_TYPE_BURIED
LavaridgeTown_Gym_1F_EventScript_CheckBuryAxle::
	goto_if_defeated TRAINER_AXLE, LavaridgeTown_Gym_1F_EventScript_CheckBuryDanielle
	setobjectmovementtype LOCALID_AXLE, MOVEMENT_TYPE_BURIED
LavaridgeTown_Gym_1F_EventScript_CheckBuryDanielle::
	goto_if_defeated TRAINER_DANIELLE, LavaridgeTown_Gym_1F_EventScript_EndCheckBuryTrainers
	setobjectmovementtype LOCALID_DANIELLE, MOVEMENT_TYPE_BURIED
LavaridgeTown_Gym_1F_EventScript_EndCheckBuryTrainers::
	return

LavaridgeTown_Gym_1F_EventScript_Flannery::
	trainerbattle_single TRAINER_FLANNERY_1, LavaridgeTown_Gym_1F_Text_FlanneryIntro, LavaridgeTown_Gym_1F_Text_FlanneryDefeat, LavaridgeTown_Gym_1F_EventScript_FlanneryDefeated, NO_MUSIC
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, LavaridgeTown_Gym_1F_EventScript_FlanneryRematch
	goto_if_unset FLAG_RECEIVED_TM_OVERHEAT, LavaridgeTown_Gym_1F_EventScript_GiveOverheat2
	msgbox LavaridgeTown_Gym_1F_Text_FlanneryPostBattle, MSGBOX_DEFAULT
	release
	end

LavaridgeTown_Gym_1F_EventScript_FlanneryDefeated::
	message LavaridgeTown_Gym_1F_Text_ReceivedHeatBadge
	waitmessage
	call Common_EventScript_PlayGymBadgeFanfare
	msgbox LavaridgeTown_Gym_1F_Text_ExplainHeatBadgeTakeThis, MSGBOX_DEFAULT
	setflag FLAG_WHITEOUT_TO_LAVARIDGE
	setflag FLAG_DEFEATED_LAVARIDGE_GYM
	setflag FLAG_BADGE04_GET
	addvar VAR_PETALBURG_GYM_STATE, 1
	call_if_eq VAR_PETALBURG_GYM_STATE, 6, Common_EventScript_ReadyPetalburgGymForBattle
	setvar VAR_0x8008, 4
	call Common_EventScript_SetGymTrainers
	setflag FLAG_HIDE_VERDANTURF_TOWN_WANDAS_HOUSE_WALLY
	setvar VAR_LAVARIDGE_TOWN_STATE, 1
	call LavaridgeTown_Gym_1F_EventScript_GiveOverheat
	closemessage
	delay 30
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox LavaridgeTown_Gym_1F_Text_RegisteredFlannery, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	delay 30
	setflag FLAG_ENABLE_FLANNERY_MATCH_CALL
	release
	end

LavaridgeTown_Gym_1F_EventScript_GiveOverheat2::
	giveitem ITEM_TM_OVERHEAT
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	msgbox LavaridgeTown_Gym_1F_Text_ExplainOverheat, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_OVERHEAT
	release
	end

LavaridgeTown_Gym_1F_EventScript_GiveOverheat::
	giveitem ITEM_TM_OVERHEAT
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_BagIsFull
	msgbox LavaridgeTown_Gym_1F_Text_ExplainOverheat, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM_OVERHEAT
	return

LavaridgeTown_Gym_1F_EventScript_FlanneryRematch::
	trainerbattle_rematch_double TRAINER_FLANNERY_1, LavaridgeTown_Gym_1F_Text_FlanneryPreRematch, LavaridgeTown_Gym_1F_Text_FlanneryRematchDefeat, LavaridgeTown_Gym_1F_Text_FlanneryRematchNeedTwoMons
	msgbox LavaridgeTown_Gym_1F_Text_FlanneryPostRematch, MSGBOX_AUTOCLOSE
	end

LavaridgeTown_Gym_1F_EventScript_Cole::
	trainerbattle TRAINER_BATTLE_CONTINUE_SCRIPT, TRAINER_COLE, LOCALID_COLE, LavaridgeTown_Gym_1F_Text_ColeIntro, LavaridgeTown_Gym_1F_Text_ColeDefeat, LavaridgeTown_Gym_EventScript_CheckTrainerScript
	msgbox LavaridgeTown_Gym_1F_Text_ColePostBattle, MSGBOX_AUTOCLOSE
	end

LavaridgeTown_Gym_EventScript_CheckTrainerScript::
	call LavaridgeTown_Gym_1F_EventScript_SetTrainerTempVars
	release
	special ShouldTryGetTrainerScript
	goto_if_eq VAR_RESULT, 1, EventScript_GotoTrainerScript
	end

LavaridgeTown_Gym_1F_EventScript_Axle::
	trainerbattle TRAINER_BATTLE_CONTINUE_SCRIPT, TRAINER_AXLE, LOCALID_AXLE, LavaridgeTown_Gym_1F_Text_AxleIntro, LavaridgeTown_Gym_1F_Text_AxleDefeat, LavaridgeTown_Gym_EventScript_CheckTrainerScript
	msgbox LavaridgeTown_Gym_1F_Text_AxlePostBattle, MSGBOX_AUTOCLOSE
	end

LavaridgeTown_Gym_B1F_EventScript_Keegan::
	trainerbattle TRAINER_BATTLE_CONTINUE_SCRIPT, TRAINER_KEEGAN, LOCALID_KEEGAN, LavaridgeTown_Gym_B1F_Text_KeeganIntro, LavaridgeTown_Gym_B1F_Text_KeeganDefeat, LavaridgeTown_Gym_EventScript_CheckTrainerScript
	msgbox LavaridgeTown_Gym_B1F_Text_KeeganPostBattle, MSGBOX_AUTOCLOSE
	end

LavaridgeTown_Gym_1F_EventScript_Danielle::
	trainerbattle TRAINER_BATTLE_CONTINUE_SCRIPT, TRAINER_DANIELLE, LOCALID_DANIELLE, LavaridgeTown_Gym_1F_Text_DanielleIntro, LavaridgeTown_Gym_1F_Text_DanielleDefeat, LavaridgeTown_Gym_EventScript_CheckTrainerScript
	msgbox LavaridgeTown_Gym_1F_Text_DaniellePostBattle, MSGBOX_AUTOCLOSE
	end

LavaridgeTown_Gym_1F_EventScript_Gerald::
	trainerbattle TRAINER_BATTLE_CONTINUE_SCRIPT, TRAINER_GERALD, LOCALID_GERALD, LavaridgeTown_Gym_1F_Text_GeraldIntro, LavaridgeTown_Gym_1F_Text_GeraldDefeat, LavaridgeTown_Gym_EventScript_CheckTrainerScript
	msgbox LavaridgeTown_Gym_1F_Text_GeraldPostBattle, MSGBOX_AUTOCLOSE
	end

LavaridgeTown_Gym_B1F_EventScript_Jace::
	trainerbattle TRAINER_BATTLE_CONTINUE_SCRIPT, TRAINER_JACE, LOCALID_JACE, LavaridgeTown_Gym_B1F_Text_JaceIntro, LavaridgeTown_Gym_B1F_Text_JaceDefeat, LavaridgeTown_Gym_EventScript_CheckTrainerScript
	msgbox LavaridgeTown_Gym_B1F_Text_JacePostBattle, MSGBOX_AUTOCLOSE
	end

LavaridgeTown_Gym_B1F_EventScript_Jeff::
	trainerbattle TRAINER_BATTLE_CONTINUE_SCRIPT, TRAINER_JEFF, LOCALID_JEFF, LavaridgeTown_Gym_B1F_Text_JeffIntro, LavaridgeTown_Gym_B1F_Text_JeffDefeat, LavaridgeTown_Gym_EventScript_CheckTrainerScript
	msgbox LavaridgeTown_Gym_B1F_Text_JeffPostBattle, MSGBOX_AUTOCLOSE
	end

LavaridgeTown_Gym_B1F_EventScript_Eli::
	trainerbattle TRAINER_BATTLE_CONTINUE_SCRIPT, TRAINER_ELI, LOCALID_ELI, LavaridgeTown_Gym_B1F_Text_EliIntro, LavaridgeTown_Gym_B1F_Text_EliDefeat, LavaridgeTown_Gym_EventScript_CheckTrainerScript
	msgbox LavaridgeTown_Gym_B1F_Text_EliPostBattle, MSGBOX_AUTOCLOSE
	end

LavaridgeTown_Gym_1F_EventScript_GymGuide::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_LAVARIDGE_GYM, LavaridgeTown_Gym_1F_EventScript_GymGuidePostVictory
	msgbox LavaridgeTown_Gym_1F_Text_GymGuideAdvice, MSGBOX_DEFAULT
	release
	end

LavaridgeTown_Gym_1F_EventScript_GymGuidePostVictory::
	msgbox LavaridgeTown_Gym_1F_Text_GymGuidePostVictory, MSGBOX_DEFAULT
	release
	end

LavaridgeTown_Gym_1F_EventScript_LeftGymStatue::
	lockall
	goto_if_set FLAG_BADGE04_GET, LavaridgeTown_Gym_1F_EventScript_GymStatueCertified
	goto LavaridgeTown_Gym_1F_EventScript_GymStatue
	end

LavaridgeTown_Gym_1F_EventScript_RightGymStatue::
	lockall
	goto_if_set FLAG_BADGE04_GET, LavaridgeTown_Gym_1F_EventScript_GymStatueCertified
	goto LavaridgeTown_Gym_1F_EventScript_GymStatue
	end

LavaridgeTown_Gym_1F_EventScript_GymStatueCertified::
	msgbox LavaridgeTown_Gym_1F_Text_GymStatueCertified, MSGBOX_DEFAULT
	releaseall
	end

LavaridgeTown_Gym_1F_EventScript_GymStatue::
	msgbox LavaridgeTown_Gym_1F_Text_GymStatue, MSGBOX_DEFAULT
	releaseall
	end

LavaridgeTown_Gym_1F_Text_GymGuideAdvice:
	.string "E aí, como vai, {PLAYER},\n"
	.string "futura estrela da LIGA?\p"
	.string "FLANNERY, a LÍDER do GINÁSIO de\n"
	.string "LAVARIDGE, usa POKéMON de FOGO.\p"
	.string "A paixão dela pelos POKéMON queima\n"
	.string "mais forte que um vulcão.\p"
	.string "Não chegue perto demais: você vai\n"
	.string "se queimar! Jogue água nela e\l"
	.string "manda ver!$"

LavaridgeTown_Gym_1F_Text_GymGuidePostVictory:
	.string "Uau! Que batalha escaldante!$"

LavaridgeTown_Gym_1F_Text_ColeIntro:
	.string "Aiaiaiaiai!\n"
	.string "Nossa, como é quente!$"

LavaridgeTown_Gym_1F_Text_ColeDefeat:
	.string "O suor está caindo nos meus olhos…$"

LavaridgeTown_Gym_1F_Text_ColePostBattle:
	.string "Ficar enterrado na areia quente\n"
	.string "melhora a circulação.\p"
	.string "Faz bem pra dor nas juntas.$"

LavaridgeTown_Gym_1F_Text_AxleIntro:
	.string "Estou tentando relaxar.\n"
	.string "Não venha me estressar!$"

LavaridgeTown_Gym_1F_Text_AxleDefeat:
	.string "Tomara que a FLANNERY te torre!$"

LavaridgeTown_Gym_1F_Text_AxlePostBattle:
	.string "Aaah… Ufa…\p"
	.string "Ficar muito tempo enterrado na areia\n"
	.string "quente cansa a gente…$"

LavaridgeTown_Gym_B1F_Text_KeeganIntro:
	.string "Você já deve estar sem fôlego.\n"
	.string "Quer descansar na areia quente,\l"
	.string "não quer?\p"
	.string "Mas saiba que manter a força de\n"
	.string "vontade é uma habilidade importante\l"
	.string "pra todo TREINADOR.$"

LavaridgeTown_Gym_B1F_Text_KeeganDefeat:
	.string "Quem brinca com fogo se queima…$"

LavaridgeTown_Gym_B1F_Text_KeeganPostBattle:
	.string "A sua habilidade é real…\n"
	.string "Mas a nossa LÍDER FLANNERY é forte.\p"
	.string "Se não tomar cuidado, você vai\n"
	.string "se queimar feio.$"

LavaridgeTown_Gym_1F_Text_GeraldIntro:
	.string "Os seus POKéMON aguentam\n"
	.string "200 graus de calor?$"

LavaridgeTown_Gym_1F_Text_GeraldDefeat:
	.string "Não queimou o bastante…$"

LavaridgeTown_Gym_1F_Text_GeraldPostBattle:
	.string "A temperatura do magma é de\n"
	.string "200 graus.\p"
	.string "Os seus POKéMON me venceram, então\n"
	.string "sobrevivem fácil no magma.$"

LavaridgeTown_Gym_1F_Text_DanielleIntro:
	.string "Hã…\n"
	.string "Tá bom, vou batalhar com você.$"

LavaridgeTown_Gym_1F_Text_DanielleDefeat:
	.string "Ah, mas você é forte demais.$"

LavaridgeTown_Gym_1F_Text_DaniellePostBattle:
	.string "Vou ser uma TREINADORA linda e forte\n"
	.string "que nem a FLANNERY.$"

LavaridgeTown_Gym_B1F_Text_JaceIntro:
	.string "Vamos, anda logo!\n"
	.string "Antes que o meu ânimo esfrie!$"

LavaridgeTown_Gym_B1F_Text_JaceDefeat:
	.string "Está tão quente, mas o meu coração\n"
	.string "ficou gelado…$"

LavaridgeTown_Gym_B1F_Text_JacePostBattle:
	.string "Com esse espírito de luta queimando\n"
	.string "aí dentro, você pode ter chance\l"
	.string "contra a nossa LÍDER.$"

LavaridgeTown_Gym_B1F_Text_JeffIntro:
	.string "Está vendo como as chamas ardem?\n"
	.string "É que elas já sentem a minha vitória!$"

LavaridgeTown_Gym_B1F_Text_JeffDefeat:
	.string "Alguma coisa deu errado.$"

LavaridgeTown_Gym_B1F_Text_JeffPostBattle:
	.string "E daí? E daí, hein?\n"
	.string "Eu ando descalço em brasa!\p"
	.string "…Nem pense em tentar isso!$"

LavaridgeTown_Gym_B1F_Text_EliIntro:
	.string "Adoro montanhas, mas adoro\n"
	.string "principalmente vulcões.$"

LavaridgeTown_Gym_B1F_Text_EliDefeat:
	.string "Bom, parece que perdi sem nunca\n"
	.string "ter o controle.$"

LavaridgeTown_Gym_B1F_Text_EliPostBattle:
	.string "Fico aqui porque virei fã do poder\n"
	.string "da FLANNERY.\p"
	.string "Hehehehe.$"

LavaridgeTown_Gym_1F_Text_FlanneryIntro:
	.string "Boas-vindas… Não, espera.\p"
	.string "TREINADOR insignificante, que bom\n"
	.string "que chegou até aqui!\p"
	.string "Fui encarregada de… Não, espera.\p"
	.string "Eu sou a FLANNERY, a LÍDER deste\n"
	.string "GINÁSIO!\p"
	.string "Hã…\n"
	.string "Não me subestime, mesmo que eu\l"
	.string "seja LÍDER há pouco tempo!\p"
	.string "Com as técnicas herdadas do meu avô,\n"
	.string "vou, hã… mostrar os golpes quentes\l"
	.string "que aperfeiçoamos nesta terra!$"

LavaridgeTown_Gym_1F_Text_FlanneryDefeat:
	.string "Ah…\n"
	.string "Acho que me esforcei demais…\p"
	.string "Eu… Virei LÍDER de GINÁSIO há\n"
	.string "pouco tempo.\p"
	.string "Tentei ser alguém que eu não sou.\p"
	.string "Tenho que agir do meu jeito. Senão,\n"
	.string "os meus POKéMON ficam confusos.\p"
	.string "Obrigada por me ensinar isso.\n"
	.string "Por isso, você merece isto.$"

LavaridgeTown_Gym_1F_Text_ReceivedHeatBadge:
	.string "{PLAYER} recebeu a INSÍGNIA DO CALOR\n"
	.string "da FLANNERY.$"

LavaridgeTown_Gym_1F_Text_ExplainHeatBadgeTakeThis:
	.string "Com a INSÍGNIA DO CALOR, todos os\n"
	.string "POKéMON até o NV. 50, mesmo os\l"
	.string "recebidos em trocas, vão te\l"
	.string "obedecer direitinho.\p"
	.string "E os POKéMON podem usar o golpe\n"
	.string "de MO FORÇA fora da batalha.\p"
	.string "Isto é um sinal da minha gratidão.\n"
	.string "Pode pegar sem vergonha!$"

LavaridgeTown_Gym_1F_Text_ExplainOverheat:
	.string "Essa MT50 contém SUPERAQUECER.\p"
	.string "Esse golpe causa um dano enorme\n"
	.string "no oponente.\p"
	.string "Mas também reduz muito o AT. ESP.\n"
	.string "de quem usa. Talvez não seja bom\l"
	.string "pra batalhas longas.$"

LavaridgeTown_Gym_1F_Text_RegisteredFlannery:
	.string "A LÍDER FLANNERY foi registrada\n"
	.string "no POKéNAV.$"

LavaridgeTown_Gym_1F_Text_FlanneryPostBattle:
	.string "A sua força me lembra alguém…\p"
	.string "Ah! Já sei! Você luta igual ao NORMAN,\n"
	.string "o LÍDER do GINÁSIO de PETALBURG.$"

LavaridgeTown_Gym_1F_Text_GymStatue:
	.string "GINÁSIO POKéMON DE LAVARIDGE$"

LavaridgeTown_Gym_1F_Text_GymStatueCertified:
	.string "GINÁSIO POKéMON DE LAVARIDGE\p"
	.string "TREINADORES APROVADOS POR FLANNERY:\n"
	.string "{PLAYER}$"

LavaridgeTown_Gym_1F_Text_FlanneryPreRematch:
	.string "FLANNERY: Perder uma batalha não\n"
	.string "vai me desanimar.\p"
	.string "Eu amo os POKéMON.\n"
	.string "Amo batalhar.\l"
	.string "E… amo este GINÁSIO!\p"
	.string "Vamos trocar golpes superquentes\n"
	.string "em outra batalha!$"

LavaridgeTown_Gym_1F_Text_FlanneryRematchDefeat:
	.string "Ufa!\n"
	.string "Quase entrei em erupção!$"

LavaridgeTown_Gym_1F_Text_FlanneryPostRematch:
	.string "FLANNERY: Perdi, mas fiquei\n"
	.string "completamente satisfeita.\p"
	.string "Não é sempre que tenho uma\n"
	.string "batalha tão quente assim.\p"
	.string "Vamos ter outra dessas um dia!$"

LavaridgeTown_Gym_1F_Text_FlanneryRematchNeedTwoMons:
	.string "FLANNERY: Perder uma batalha não\n"
	.string "vai me desanimar.\p"
	.string "Eu amo os POKéMON.\n"
	.string "Amo batalhar.\l"
	.string "E… amo este GINÁSIO!\p"
	.string "Vamos trocar golpes superquentes\n"
	.string "em outra batalha!\p"
	.string "Opa, espera. Você só tem um POKéMON\n"
	.string "que pode batalhar?\p"
	.string "Desculpa, mas, quando vier me ver,\n"
	.string "traga pelo menos dois POKéMON.$"
