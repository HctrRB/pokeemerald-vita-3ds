PetalburgWoods_MapScripts::
	.byte 0

PetalburgWoods_EventScript_DevonResearcherLeft::
	lockall
	call PetalburgWoods_EventScript_DevonResearcherIntro
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherApproachPlayerLeft
	waitmovement 0
	msgbox PetalburgWoods_Text_HaveYouSeenShroomish, MSGBOX_DEFAULT
	closemessage
	playbgm MUS_ENCOUNTER_AQUA, FALSE
	applymovement LOCALID_PETALBURG_WOODS_GRUNT, PetalburgWoods_Movement_AquaEntrance
	waitmovement 0
	msgbox PetalburgWoods_Text_IWasGoingToAmbushYou, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PETALBURG_WOODS_GRUNT, PetalburgWoods_Movement_AquaApproachResearcherLeft
	waitmovement 0
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PetalburgWoods_Text_HandOverThosePapers, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherFleeToPlayerLeft
	waitmovement 0
	msgbox PetalburgWoods_Text_YouHaveToHelpMe, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PETALBURG_WOODS_GRUNT, PetalburgWoods_Movement_AquaApproachPlayer
	waitmovement 0
	msgbox PetalburgWoods_Text_NoOneCrossesTeamAqua, MSGBOX_DEFAULT
	trainerbattle_no_intro TRAINER_GRUNT_PETALBURG_WOODS, PetalburgWoods_Text_YoureKiddingMe
	applymovement LOCALID_PETALBURG_WOODS_GRUNT, PetalburgWoods_Movement_AquaBackOff
	waitmovement 0
	call PetalburgWoods_EventScript_DevonResearcherPostBattle
	applymovement LOCALID_PLAYER, PetalburgWoods_Movement_WatchResearcherLeave
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherExitLeft
	waitmovement 0
	goto PetalburgWoods_EventScript_RemoveDevonResearcher
	end

PetalburgWoods_EventScript_DevonResearcherRight::
	lockall
	call PetalburgWoods_EventScript_DevonResearcherIntro
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherApproachPlayerRight
	waitmovement 0
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	msgbox PetalburgWoods_Text_HaveYouSeenShroomish, MSGBOX_DEFAULT
	closemessage
	playbgm MUS_ENCOUNTER_AQUA, FALSE
	applymovement LOCALID_PETALBURG_WOODS_GRUNT, PetalburgWoods_Movement_AquaEntrance
	waitmovement 0
	msgbox PetalburgWoods_Text_IWasGoingToAmbushYou, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PETALBURG_WOODS_GRUNT, PetalburgWoods_Movement_AquaApproachResearcherRight
	waitmovement 0
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PetalburgWoods_Text_HandOverThosePapers, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherFleeToPlayerRight
	waitmovement 0
	msgbox PetalburgWoods_Text_YouHaveToHelpMe, MSGBOX_DEFAULT
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PetalburgWoods_Text_NoOneCrossesTeamAqua, MSGBOX_DEFAULT
	trainerbattle_no_intro TRAINER_GRUNT_PETALBURG_WOODS, PetalburgWoods_Text_YoureKiddingMe
	applymovement LOCALID_PETALBURG_WOODS_GRUNT, PetalburgWoods_Movement_AquaBackOff
	waitmovement 0
	call PetalburgWoods_EventScript_DevonResearcherPostBattle
	applymovement LOCALID_PLAYER, PetalburgWoods_Movement_WatchResearcherLeave
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherExitRight
	waitmovement 0
	goto PetalburgWoods_EventScript_RemoveDevonResearcher
	end

PetalburgWoods_EventScript_DevonResearcherIntro::
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherLookAround
	waitmovement 0
	msgbox PetalburgWoods_Text_NotAOneToBeFound, MSGBOX_DEFAULT
	closemessage
	return

PetalburgWoods_EventScript_DevonResearcherPostBattle::
	msgbox PetalburgWoods_Text_YouveGotSomeNerve, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PETALBURG_WOODS_GRUNT, PetalburgWoods_Movement_AquaRunAway
	waitmovement 0
	removeobject LOCALID_PETALBURG_WOODS_GRUNT
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	msgbox PetalburgWoods_Text_ThatWasAwfullyClose, MSGBOX_DEFAULT
	giveitem ITEM_GREAT_BALL
	goto_if_eq VAR_RESULT, FALSE, PetalburgWoods_EventScript_BagFull
	goto PetalburgWoods_EventScript_DevonResearcherFinish
	end

PetalburgWoods_EventScript_BagFull::
	msgbox PetalburgWoods_Text_YoureLoadedWithItems, MSGBOX_DEFAULT
	goto PetalburgWoods_EventScript_DevonResearcherFinish
	end

PetalburgWoods_EventScript_DevonResearcherFinish::
	msgbox PetalburgWoods_Text_TeamAquaAfterSomethingInRustboro, MSGBOX_DEFAULT
	applymovement LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherStartExit
	waitmovement 0
	msgbox PetalburgWoods_Text_ICantBeWastingTime, MSGBOX_DEFAULT
	closemessage
	return

PetalburgWoods_EventScript_RemoveDevonResearcher::
	removeobject LOCALID_PETALBURG_WOODS_DEVON_EMPLOYEE
	setvar VAR_PETALBURG_WOODS_STATE, 1
	releaseall
	end

PetalburgWoods_Movement_DevonResearcherLookAround:
	face_up
	delay_16
	delay_4
	face_right
	delay_16
	delay_8
	face_left
	delay_16
	delay_8
	face_down
	delay_16
	face_right
	delay_16
	delay_8
	face_up
	delay_16
	step_end

PetalburgWoods_Movement_DevonResearcherExitLeft:
	walk_fast_right
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	step_end

PetalburgWoods_Movement_DevonResearcherApproachPlayerLeft:
	delay_16
	face_player
	walk_down
	walk_down
	step_end

PetalburgWoods_Movement_DevonResearcherApproachPlayerRight:
	delay_16
	face_player
	walk_down
	walk_down
	walk_down
	walk_in_place_faster_right
	step_end

PetalburgWoods_Movement_DevonResearcherExitRight:
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	step_end

PetalburgWoods_Movement_WatchResearcherLeave:
	delay_16
	delay_16
	walk_in_place_faster_up
	step_end

PetalburgWoods_Movement_DevonResearcherFleeToPlayerLeft:
	walk_fast_right
	walk_fast_down
	walk_fast_down
	walk_fast_left
	walk_in_place_faster_up
	step_end

PetalburgWoods_Movement_DevonResearcherFleeToPlayerRight:
	walk_fast_down
	walk_fast_right
	walk_in_place_faster_up
	step_end

PetalburgWoods_Movement_DevonResearcherStartExit:
	walk_in_place_faster_down
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	face_up
	step_end

PetalburgWoods_Movement_AquaApproachResearcherLeft:
	walk_fast_down
	walk_fast_down
	step_end

PetalburgWoods_Movement_AquaBackOff:
	lock_facing_direction
	walk_up
	unlock_facing_direction
	step_end

PetalburgWoods_Movement_AquaRunAway:
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	delay_16
	delay_16
	step_end

PetalburgWoods_Movement_AquaApproachResearcherRight:
	walk_fast_down
	walk_fast_down
	walk_fast_down
	step_end

PetalburgWoods_Movement_AquaEntrance:
	walk_down
	walk_down
	delay_16
	delay_16
	step_end

PetalburgWoods_Movement_AquaApproachPlayer:
	walk_down
	step_end

PetalburgWoods_EventScript_Boy1::
	msgbox PetalburgWoods_Text_StayOutOfTallGrass, MSGBOX_NPC
	end

PetalburgWoods_EventScript_Boy2::
	msgbox PetalburgWoods_Text_HiddenItemsExplanation, MSGBOX_NPC
	end

PetalburgWoods_EventScript_Girl::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_MIRACLE_SEED, PetalburgWoods_EventScript_ExplainMiracleSeed
	msgbox PetalburgWoods_Text_TryUsingThisItem, MSGBOX_DEFAULT
	giveitem ITEM_MIRACLE_SEED
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_MIRACLE_SEED
	release
	end

PetalburgWoods_EventScript_ExplainMiracleSeed::
	msgbox PetalburgWoods_Text_MiracleSeedExplanation, MSGBOX_DEFAULT
	release
	end

PetalburgWoods_EventScript_Sign1::
	msgbox PetalburgWoods_Text_TrainerTipsExperience, MSGBOX_SIGN
	end

PetalburgWoods_EventScript_Sign2::
	msgbox PetalburgWoods_Text_TrainerTipsPP, MSGBOX_SIGN
	end

PetalburgWoods_EventScript_Lyle::
	trainerbattle_single TRAINER_LYLE, PetalburgWoods_Text_GoBugPokemonTeam, PetalburgWoods_Text_ICouldntWin
	msgbox PetalburgWoods_Text_ImOutOfPokeBalls, MSGBOX_AUTOCLOSE
	end

PetalburgWoods_EventScript_James::
	trainerbattle_single TRAINER_JAMES_1, PetalburgWoods_Text_InstantlyPopularWithBugPokemon, PetalburgWoods_Text_CantBePopularIfILose, PetalburgWoods_EventScript_TryRegisterJames
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, PetalburgWoods_EventScript_JamesRematch
	setvar VAR_0x8004, TRAINER_JAMES_1
	specialvar VAR_RESULT, IsTrainerRegistered
	goto_if_eq VAR_RESULT, FALSE, PetalburgWoods_EventScript_TryRegisterJames2
	msgbox PetalburgWoods_Text_PeopleRespectYou, MSGBOX_DEFAULT
	release
	end

PetalburgWoods_EventScript_TryRegisterJames::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	goto_if_set FLAG_HAS_MATCH_CALL, PetalburgWoods_EventScript_RegisterJames
	release
	end

PetalburgWoods_EventScript_RegisterJames::
	msgbox PetalburgWoods_Text_IWantRematch1, MSGBOX_DEFAULT
	register_matchcall TRAINER_JAMES_1
	release
	end

PetalburgWoods_EventScript_TryRegisterJames2::
	goto_if_set FLAG_HAS_MATCH_CALL, PetalburgWoods_EventScript_RegisterJames2
	msgbox PetalburgWoods_Text_PeopleRespectYou, MSGBOX_DEFAULT
	release
	end

PetalburgWoods_EventScript_RegisterJames2::
	msgbox PetalburgWoods_Text_IWantRematch2, MSGBOX_DEFAULT
	register_matchcall TRAINER_JAMES_1
	release
	end

PetalburgWoods_EventScript_JamesRematch::
	trainerbattle_rematch TRAINER_JAMES_1, PetalburgWoods_Text_MyPokemonHaveGrown, PetalburgWoods_Text_CantBePopularIfLose
	msgbox PetalburgWoods_Text_IveBeenTrainingHard, MSGBOX_AUTOCLOSE
	end

PetalburgWoods_Text_NotAOneToBeFound:
	.string "Hummm…\n"
	.string "Não tem nenhum…$"

PetalburgWoods_Text_HaveYouSeenShroomish:
	.string "Olá, você viu algum POKéMON\n"
	.string "chamado SHROOMISH por aqui?\p"
	.string "Eu adoro esse POKéMON.$"

PetalburgWoods_Text_IWasGoingToAmbushYou:
	.string "Eu ia te emboscar, mas você tinha\n"
	.string "que ficar enrolando no BOSQUE DE\l"
	.string "PETALBURG pra sempre, né?\p"
	.string "Cansei de esperar, então aqui estou!$"

PetalburgWoods_Text_HandOverThosePapers:
	.string "Você! PESQUISADOR DA DEVON!\p"
	.string "Passa esses documentos pra cá!$"

PetalburgWoods_Text_YouHaveToHelpMe:
	.string "Aaaaai!\p"
	.string "Você treina POKéMON, não treina?\n"
	.string "Você tem que me ajudar, por favor!$"

PetalburgWoods_Text_NoOneCrossesTeamAqua:
	.string "Hã? O que pensa que está fazendo?\n"
	.string "Vai proteger ele, é?\p"
	.string "Quem se mete com a EQUIPE AQUA\n"
	.string "não tem perdão, nem criança!\p"
	.string "Vem, batalha comigo!$"

PetalburgWoods_Text_YoureKiddingMe:
	.string "Tá de brincadeira! Você é fera!$"

PetalburgWoods_Text_YouveGotSomeNerve:
	.string "Grrr… Que coragem a sua, se\n"
	.string "metendo com a EQUIPE AQUA!\l"
	.string "Vem, batalha comigo de novo!\p"
	.string "Bem que eu queria, mas acabaram\n"
	.string "meus POKéMON…\p"
	.string "E, olha, a EQUIPE AQUA também está\n"
	.string "atrás de uma coisa em RUSTBORO.\p"
	.string "Hoje eu te deixo ir!$"

PetalburgWoods_Text_ThatWasAwfullyClose:
	.string "Ufa…\n"
	.string "Essa foi por pouco!\p"
	.string "Graças a você, ele não roubou\n"
	.string "esses documentos importantes.\p"
	.string "Já sei, vou te dar uma GRANDE BOLA\n"
	.string "como agradecimento!$"

PetalburgWoods_Text_TeamAquaAfterSomethingInRustboro:
	.string "Aquele bandido da EQUIPE AQUA não\n"
	.string "disse que eles também estão atrás\l"
	.string "de alguma coisa em RUSTBORO?$"

PetalburgWoods_Text_ICantBeWastingTime:
	.string "Ai, ai! É uma crise!\n"
	.string "Não posso perder tempo!$"

PetalburgWoods_Text_YoureLoadedWithItems:
	.string "A sua BOLSA está cheia de itens.\n"
	.string "Não dá pra te dar esta GRANDE BOLA.$"

PetalburgWoods_Text_GoBugPokemonTeam:
	.string "Peguei um montão de POKéMON!\p"
	.string "Vai, vai, vai!\n"
	.string "Minha equipe de POKéMON INSETO!$"

PetalburgWoods_Text_ICouldntWin:
	.string "Eu tenho todos esses POKéMON,\n"
	.string "mas não consegui ganhar…$"

PetalburgWoods_Text_ImOutOfPokeBalls:
	.string "Peguei um monte de POKéMON.\n"
	.string "Agora acabaram minhas POKé BOLAS.$"

PetalburgWoods_Text_InstantlyPopularWithBugPokemon:
	.string "Se você leva POKéMON INSETO pra escola,\n"
	.string "fica popular na hora!$"

PetalburgWoods_Text_CantBePopularIfILose:
	.string "Não dá pra ser popular se eu perder.$"

PetalburgWoods_Text_PeopleRespectYou:
	.string "Se você tem um POKéMON INSETO grande,\n"
	.string "o pessoal te respeita.$"

PetalburgWoods_Text_IWantRematch1:
	.string "Quero revanche quando meus POKéMON\n"
	.string "INSETO crescerem!\p"
	.string "Vou te registrar no meu POKéNAV!$"

PetalburgWoods_Text_IWantRematch2:
	.string "Quero revanche quando meus POKéMON\n"
	.string "INSETO crescerem!\p"
	.string "Vou te registrar no meu POKéNAV!$"

PetalburgWoods_Text_MyPokemonHaveGrown:
	.string "Meus POKéMON INSETO cresceram.\n"
	.string "Vou ficar popular de novo.$"

PetalburgWoods_Text_CantBePopularIfLose:
	.string "Não dá pra ser popular se eu perder.$"

PetalburgWoods_Text_IveBeenTrainingHard:
	.string "Quem tem POKéMON forte fica popular,\n"
	.string "né? Então eu treinei bastante.$"

PetalburgWoods_Text_StayOutOfTallGrass:
	.string "E aí!\n"
	.string "Seus POKéMON estão bem?\p"
	.string "Se eles estão fracos e você quer\n"
	.string "evitar batalhas, fique longe do\l"
	.string "mato alto.$"

PetalburgWoods_Text_HiddenItemsExplanation:
	.string "Às vezes tem coisas no chão, mesmo\n"
	.string "que você não consiga ver.\p"
	.string "Por isso eu sempre olho onde\n"
	.string "estou pisando.$"

PetalburgWoods_Text_TryUsingThisItem:
	.string "Que legal!\n"
	.string "É a INSÍGNIA do GINÁSIO de RUSTBORO!\p"
	.string "Você deve treinar POKéMON.\n"
	.string "Experimenta usar este item.$"

PetalburgWoods_Text_MiracleSeedExplanation:
	.string "É uma SEMENTE MILAGROSA.\n"
	.string "Se um POKéMON segura esse item, os\l"
	.string "golpes PLANTA ficam mais fortes.\p"
	.string "Acho que tem outros itens úteis\n"
	.string "para os POKéMON segurarem, além da\l"
	.string "SEMENTE MILAGROSA.$"

PetalburgWoods_Text_TrainerTipsExperience:
	.string "DICAS DE TREINADOR\p"
	.string "Todo POKéMON que entra numa batalha,\n"
	.string "nem que seja uma vez, ganha EXP.\p"
	.string "Para treinar um POKéMON fraco,\n"
	.string "coloque-o primeiro na lista da equipe.\p"
	.string "Assim que a batalha começar, troque-o.\n"
	.string "Ele ganha EXP sem correr\l"
	.string "nenhum perigo.$"

PetalburgWoods_Text_TrainerTipsPP:
	.string "DICAS DE TREINADOR\p"
	.string "Além dos Pontos de Saúde (PS), os\n"
	.string "POKéMON têm Pontos de Poder (PP),\l"
	.string "gastos para usar golpes na batalha.\p"
	.string "Se um POKéMON ficar sem PP, ele precisa\n"
	.string "ir a um CENTRO POKéMON.$"

