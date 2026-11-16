RustboroCity_PokemonSchool_MapScripts::
	.byte 0

RustboroCity_PokemonSchool_EventScript_Blackboard::
	lockall
	msgbox RustboroCity_PokemonSchool_Text_BlackboardListsStatusChanges, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic::
	message RustboroCity_PokemonSchool_Text_ReadWhichTopic
	waitmessage
	multichoicegrid 8, 1, MULTI_STATUS_INFO, 3, FALSE
	switch VAR_RESULT
	case 0, RustboroCity_PokemonSchool_EventScript_Poison
	case 1, RustboroCity_PokemonSchool_EventScript_Paralysis
	case 2, RustboroCity_PokemonSchool_EventScript_Sleep
	case 3, RustboroCity_PokemonSchool_EventScript_Burn
	case 4, RustboroCity_PokemonSchool_EventScript_Freeze
	case 5, RustboroCity_PokemonSchool_EventScript_ExitTopicSelect
	case MULTI_B_PRESSED, RustboroCity_PokemonSchool_EventScript_ExitTopicSelect
	end

RustboroCity_PokemonSchool_EventScript_Poison::
	msgbox RustboroCity_PokemonSchool_Text_ExplainPoison, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_Paralysis::
	msgbox RustboroCity_PokemonSchool_Text_ExplainParalysis, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_Sleep::
	msgbox RustboroCity_PokemonSchool_Text_ExplainSleep, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_Burn::
	msgbox RustboroCity_PokemonSchool_Text_ExplainBurn, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_Freeze::
	msgbox RustboroCity_PokemonSchool_Text_ExplainFreeze, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_ExitTopicSelect::
	releaseall
	end

RustboroCity_PokemonSchool_EventScript_GameboyKid1::
	msgbox RustboroCity_PokemonSchool_Text_TradingRightNow, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_GameboyKid2::
	msgbox RustboroCity_PokemonSchool_Text_AlwaysWantedSeedot, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_RichBoy::
	msgbox RustboroCity_PokemonSchool_Text_PokemontCantUseManMadeItems, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_Lass::
	msgbox RustboroCity_PokemonSchool_Text_ConfusedPokemonAttacksItself, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_SchoolKidM::
	msgbox RustboroCity_PokemonSchool_Text_PokemonHealItselfWithBerry, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_StudentNotebook::
	msgbox RustboroCity_PokemonSchool_Text_StudentsNotes, MSGBOX_SIGN
	end

RustboroCity_PokemonSchool_EventScript_Teacher::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_QUICK_CLAW, RustboroCity_PokemonSchool_EventScript_GaveQuickClaw
	call_if_eq VAR_FACING, DIR_EAST, RustboroCity_PokemonSchool_EventScript_TeacherCheckOnStudentsEast
	call_if_eq VAR_FACING, DIR_WEST, RustboroCity_PokemonSchool_EventScript_TeacherCheckOnStudentsWest
	msgbox RustboroCity_PokemonSchool_Text_StudentsWhoDontStudyGetQuickClaw, MSGBOX_DEFAULT
	giveitem ITEM_QUICK_CLAW
	goto_if_eq VAR_RESULT, 0, Common_EventScript_ShowBagIsFull
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	setflag FLAG_RECEIVED_QUICK_CLAW
	release
	end

RustboroCity_PokemonSchool_EventScript_TeacherCheckOnStudentsEast::
	applymovement VAR_LAST_TALKED, RustboroCity_PokemonSchool_Movement_TeacherCheckOnStudentsEast
	waitmovement 0
	return

RustboroCity_PokemonSchool_EventScript_TeacherCheckOnStudentsWest::
	applymovement VAR_LAST_TALKED, RustboroCity_PokemonSchool_Movement_TeacherCheckOnStudentsWest
	waitmovement 0
	return

RustboroCity_PokemonSchool_EventScript_GaveQuickClaw::
	msgbox RustboroCity_PokemonSchool_Text_ExplainQuickClaw, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	release
	end

RustboroCity_PokemonSchool_Movement_TeacherCheckOnStudentsWest:
	walk_left
	walk_down
	walk_down
	walk_right
	walk_in_place_faster_down
	delay_16
	delay_16
	delay_16
	walk_down
	walk_in_place_faster_left
	delay_16
	delay_16
	walk_right
	delay_16
	delay_16
	delay_8
	walk_up
	walk_left
	walk_left
	walk_up
	walk_up
	walk_right
	step_end

RustboroCity_PokemonSchool_Movement_TeacherCheckOnStudentsEast:
	walk_right
	walk_right
	walk_down
	walk_down
	walk_left
	walk_left
	walk_in_place_faster_down
	delay_16
	delay_16
	delay_16
	walk_down
	walk_in_place_faster_left
	delay_16
	delay_16
	walk_right
	delay_16
	delay_16
	delay_8
	walk_up
	walk_right
	walk_up
	walk_up
	walk_left
	walk_left
	step_end

RustboroCity_PokemonSchool_EventScript_Scott::
	lock
	faceplayer
	goto_if_set FLAG_MET_SCOTT_AFTER_OBTAINING_STONE_BADGE, RustboroCity_PokemonSchool_EventScript_ScottWatchStudents
	goto_if_set FLAG_MET_SCOTT_RUSTBORO, RustboroCity_PokemonSchool_EventScript_ScottSpokeAlready
	goto_if_set FLAG_BADGE01_GET, RustboroCity_PokemonSchool_EventScript_ScottGreetHasBadge
	msgbox RustboroCity_PokemonSchool_Text_ScottMetAlreadyCut, MSGBOX_DEFAULT
	addvar VAR_SCOTT_STATE, 1
	setflag FLAG_MET_SCOTT_RUSTBORO
	release
	end

RustboroCity_PokemonSchool_EventScript_ScottSpokeAlready::
	goto_if_set FLAG_BADGE01_GET, RustboroCity_PokemonSchool_EventScript_ScottNoticeBadge
	msgbox RustboroCity_PokemonSchool_Text_StudentTalentLevelUnknown, MSGBOX_DEFAULT
	release
	end

RustboroCity_PokemonSchool_EventScript_ScottGreetHasBadge::
	msgbox RustboroCity_PokemonSchool_Text_ScottMetAlreadyStoneBadge, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_MetScottAfterBadge
	end

RustboroCity_PokemonSchool_EventScript_ScottNoticeBadge::
	msgbox RustboroCity_PokemonSchool_Text_ScottStoneBadge, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_MetScottAfterBadge
	end

RustboroCity_PokemonSchool_EventScript_MetScottAfterBadge::
	addvar VAR_SCOTT_STATE, 1
	setflag FLAG_MET_SCOTT_AFTER_OBTAINING_STONE_BADGE
	release
	end

RustboroCity_PokemonSchool_EventScript_ScottWatchStudents::
	msgbox RustboroCity_PokemonSchool_Text_StudentTalentLevelUnknown, MSGBOX_DEFAULT
	release
	end

RustboroCity_PokemonSchool_Text_BlackboardListsStatusChanges:
	.string "A lousa lista as condições de status\n"
	.string "que afetam os POKéMON na batalha.$"

RustboroCity_PokemonSchool_Text_ReadWhichTopic:
	.string "Qual assunto você quer ler?$"

RustboroCity_PokemonSchool_Text_ExplainPoison:
	.string "Se um POKéMON é envenenado, ele vai\n"
	.string "perdendo PS aos poucos.\p"
	.string "O veneno continua depois\n"
	.string "da batalha.\p"
	.string "Os PS de um POKéMON envenenado caem\n"
	.string "enquanto ele viaja.\p"
	.string "Cure o veneno com um ANTÍDOTO.$"

RustboroCity_PokemonSchool_Text_ExplainParalysis:
	.string "Se um POKéMON fica paralisado,\n"
	.string "a VELOCIDADE dele cai.\p"
	.string "E ele pode não conseguir se mexer\n"
	.string "na batalha.\p"
	.string "A paralisia continua depois da batalha.\n"
	.string "Cure com um CURA PARALISIA.$"

RustboroCity_PokemonSchool_Text_ExplainSleep:
	.string "Se um POKéMON pega no sono, ele não\n"
	.string "consegue atacar.\p"
	.string "Ele pode acordar sozinho, mas, se a\n"
	.string "batalha acabar enquanto ele dorme,\l"
	.string "ele continua dormindo.\p"
	.string "Acorde-o com um DESPERTAR.$"

RustboroCity_PokemonSchool_Text_ExplainBurn:
	.string "A queimadura diminui o ATAQUE e vai\n"
	.string "tirando PS da vítima aos poucos.\p"
	.string "A queimadura continua após a batalha.\n"
	.string "Cure com um ANTIQUEIMADURA.$"

RustboroCity_PokemonSchool_Text_ExplainFreeze:
	.string "Se um POKéMON é congelado, ele fica\n"
	.string "completamente indefeso.\p"
	.string "Continua congelado depois da batalha.\n"
	.string "Descongele com um DESCONGELANTE.$"

RustboroCity_PokemonSchool_Text_StudentsWhoDontStudyGetQuickClaw:
	.string "Aluno que não estuda ganha um\n"
	.string "gostinho da minha GARRA RÁPIDA.\p"
	.string "Se você é bom aluno ou não, vai\n"
	.string "ficar claro pelo jeito que você usa\l"
	.string "este item.$"

RustboroCity_PokemonSchool_Text_ExplainQuickClaw:
	.string "Um POKéMON com a GARRA RÁPIDA às\n"
	.string "vezes acelera e age antes do\l"
	.string "oponente.\p"
	.string "Tem muitos outros itens feitos para\n"
	.string "os POKéMON segurarem.\p"
	.string "Só eles já dão muito assunto\n"
	.string "pra estudar!$"

RustboroCity_PokemonSchool_Text_TradingRightNow:
	.string "Estou trocando POKéMON com o meu\n"
	.string "amigo agora mesmo.$"

RustboroCity_PokemonSchool_Text_AlwaysWantedSeedot:
	.string "Eu sempre quis um SEEDOT, e\n"
	.string "finalmente vou ter um!$"

RustboroCity_PokemonSchool_Text_PokemontCantUseManMadeItems:
	.string "Os POKéMON seguram itens, mas não\n"
	.string "sabem usar coisas feitas por gente,\l"
	.string "como POÇÃO e ANTÍDOTO.$"

RustboroCity_PokemonSchool_Text_ConfusedPokemonAttacksItself:
	.string "Sabe aqueles golpes que deixam o\n"
	.string "POKéMON confuso?\p"
	.string "Um POKéMON confuso às vezes ataca\n"
	.string "a si mesmo sem querer.\p"
	.string "Mas, quando sai da batalha, ele\n"
	.string "volta ao normal.$"

RustboroCity_PokemonSchool_Text_PokemonHealItselfWithBerry:
	.string "Um POKéMON segurando uma FRUTA se\n"
	.string "cura sozinho…\p"
	.string "Tem muitos tipos de itens que os\n"
	.string "POKéMON podem segurar…\p"
	.string "Puxa, como é difícil tomar nota\n"
	.string "de tudo…$"

RustboroCity_PokemonSchool_Text_StudentsNotes:
	.string "É o caderno deste aluno…\p"
	.string "Os POKéMON são capturados com\n"
	.string "POKé BOLAS.\p"
	.string "Até seis POKéMON podem acompanhar\n"
	.string "quem treina.\p"
	.string "TREINADOR é quem pega POKéMON,\n"
	.string "cria e batalha com eles.\p"
	.string "A missão de um TREINADOR é derrotar\n"
	.string "os TREINADORES fortes que esperam\l"
	.string "desafiantes nos GINÁSIOS POKéMON.$"

RustboroCity_PokemonSchool_Text_ScottMetAlreadyCut:
	.string "Olá? A gente já não se conhece?\n"
	.string "Acho que foi em PETALBURG.\p"
	.string "Deixa eu me apresentar.\n"
	.string "Meu nome é SCOTT.\p"
	.string "Estou viajando por toda parte em\n"
	.string "busca de gente talentosa que treina.\p"
	.string "Mais exatamente, procuro especialistas\n"
	.string "em batalhas POKéMON.\p"
	.string "E você, o que faz nesta ESCOLA?\n"
	.string "Também treina POKéMON?\p"
	.string "A primeira coisa a fazer é ensinar\n"
	.string "o golpe CORTAR a um POKéMON.\p"
	.string "Se não me engano, alguém nesta\n"
	.string "cidade tem CORTAR.$"

RustboroCity_PokemonSchool_Text_StudentTalentLevelUnknown:
	.string "SCOTT: Hmm…\p"
	.string "Não dá pra saber o talento dos alunos\n"
	.string "daqui. Mas o potencial existe.$"

RustboroCity_PokemonSchool_Text_ScottStoneBadge:
	.string "SCOTT: Ah, o que é isso?\p"
	.string "É uma INSÍGNIA DA PEDRA, né?\n"
	.string "Isso é bem impressionante, eu diria.\p"
	.string "Mas, sabe, eu queria muito ter visto\n"
	.string "você batalhar.\p"
	.string "Só pela INSÍGNIA não dá pra saber\n"
	.string "como você é treinando POKéMON.$"

RustboroCity_PokemonSchool_Text_ScottMetAlreadyStoneBadge:
	.string "Olá? A gente já não se conhece?\n"
	.string "Acho que foi em PETALBURG.\p"
	.string "Deixa eu me apresentar.\n"
	.string "Meu nome é SCOTT.\p"
	.string "Estou viajando por toda parte em\n"
	.string "busca de gente talentosa que treina.\p"
	.string "Mais exatamente, procuro especialistas\n"
	.string "em batalhas POKéMON.\p"
	.string "Ah, o que é isso?\n"
	.string "É uma INSÍGNIA DA PEDRA, né?\p"
	.string "Bem impressionante, eu diria, pra\n"
	.string "quem está começando.\p"
	.string "Mas, sabe, eu queria muito ter visto\n"
	.string "você batalhar.\p"
	.string "Só pela INSÍGNIA não dá pra saber\n"
	.string "como você é treinando POKéMON.$"

