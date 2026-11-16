DewfordTown_Hall_MapScripts::
	.byte 0

DewfordTown_Hall_EventScript_Girl::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	special IsTrendyPhraseBoring
	goto_if_eq VAR_RESULT, TRUE, DewfordTown_Hall_EventScript_GirlBoredOfTrend
	msgbox DewfordTown_Hall_Text_CantImagineLifeWithoutTrend, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_GirlBoredOfTrend::
	msgbox DewfordTown_Hall_Text_GettingBoredOfTrend, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_Woman::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_TeachingMonAboutTrend, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_Man::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	special BufferDeepLinkPhrase
	msgbox DewfordTown_Hall_Text_DeepLinkBetweenXAndY, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, DewfordTown_Hall_EventScript_ConfirmTrendLink
	goto_if_eq VAR_RESULT, NO, DewfordTown_Hall_EventScript_RejectTrendLink
	end

DewfordTown_Hall_EventScript_ConfirmTrendLink::
	msgbox DewfordTown_Hall_Text_MyHunchWasRight, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_RejectTrendLink::
	msgbox DewfordTown_Hall_Text_NotEasyToKeepUp, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_ExpertM::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_TVShowAboutTrend, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_DEWFORD_HALL_EXPERT_M, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	release
	end

DewfordTown_Hall_EventScript_Twin::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_IsTrendMorePopularAcrossSea, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_DEWFORD_HALL_TWIN, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	release
	end

DewfordTown_Hall_EventScript_LittleBoy::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_CollectTrendMerchandise, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_Bookshelf::
	lockall
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_BooksAboutTrend, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_Painting::
	lockall
	call Common_EventScript_BufferTrendyPhrase
	special GetDewfordHallPaintingNameIndex
	switch VAR_RESULT
	case 0, DewfordTown_Hall_EventScript_ScreamTitle
	case 4, DewfordTown_Hall_EventScript_ScreamTitle
	case 1, DewfordTown_Hall_EventScript_SmileTitle
	case 5, DewfordTown_Hall_EventScript_ScreamTitle
	case 2, DewfordTown_Hall_EventScript_LastTitle
	case 6, DewfordTown_Hall_EventScript_LastTitle
	case 3, DewfordTown_Hall_EventScript_BirthTitle
	case 7, DewfordTown_Hall_EventScript_LastTitle
	end

DewfordTown_Hall_EventScript_ScreamTitle::
	msgbox DewfordTown_Hall_Text_TrendsScream, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_SmileTitle::
	msgbox DewfordTown_Hall_Text_TrendsSmile, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_LastTitle::
	msgbox DewfordTown_Hall_Text_LastTrend, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_BirthTitle::
	msgbox DewfordTown_Hall_Text_BirthOfTrend, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_SchoolKidM::
	lockall
	setvar VAR_0x8008, 0
	goto DewfordTown_Hall_EventScript_DoTrendDebate
	end

DewfordTown_Hall_EventScript_PsychicM::
	lockall
	setvar VAR_0x8008, 1
	goto DewfordTown_Hall_EventScript_DoTrendDebate
	end

DewfordTown_Hall_EventScript_DoTrendDebate::
	call Common_EventScript_BufferTrendyPhrase
	special GetDewfordHallPaintingNameIndex
	switch VAR_RESULT
	case 0, DewfordTown_Hall_EventScript_TrendDebate1
	case 1, DewfordTown_Hall_EventScript_TrendDebate1
	case 2, DewfordTown_Hall_EventScript_TrendDebate2
	case 3, DewfordTown_Hall_EventScript_TrendDebate2
	case 4, DewfordTown_Hall_EventScript_TrendDebate3
	case 5, DewfordTown_Hall_EventScript_TrendDebate3
	case 6, DewfordTown_Hall_EventScript_TrendDebate4
	case 7, DewfordTown_Hall_EventScript_TrendDebate5
	end

DewfordTown_Hall_EventScript_TrendDebate1::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_SawTrendCoolestThing, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_AllegedTrendNotAuthentic, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_TrendDebate2::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_ComposedTrendThemeSong, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_WorkOnYourSinging, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_TrendDebate3::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_OrganizeTrendParty, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_BrilliantIndeed, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_TrendDebate4::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_TrendHasBecomePartOfLife, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_TrendWeighsHeavilyOnMind, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_TrendDebate5::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_IfWeTeamUpWellBeInvincible, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_WellBeTrendDuo, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_DontMovePlayer1::
	return

DewfordTown_Hall_EventScript_DebateReact1::
	applymovement LOCALID_DEWFORD_HALL_PSYCHIC_M, DewfordTown_Hall_Movement_PsychicWalkInPlaceLeft
	waitmovement 0
	goto_if_eq VAR_0x8008, 0, DewfordTown_Hall_EventScript_PlayerReactWest
	goto_if_eq VAR_0x8008, 1, DewfordTown_Hall_EventScript_DontMovePlayer2
	end

DewfordTown_Hall_EventScript_PlayerReactWest::
	goto_if_eq VAR_FACING, DIR_EAST, DewfordTown_Hall_EventScript_DontMovePlayer1
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	return

DewfordTown_Hall_EventScript_DontMovePlayer2::
	return

DewfordTown_Hall_EventScript_DebateReact2::
	applymovement LOCALID_DEWFORD_HALL_SCHOOL_KID_M, DewfordTown_Hall_Movement_SchoolKidWalkInPlaceRight
	waitmovement 0
	goto_if_eq VAR_0x8008, 0, DewfordTown_Hall_EventScript_PlayerReactNorthSouth
	goto_if_eq VAR_0x8008, 1, DewfordTown_Hall_EventScript_PlayerReactEast
	end

DewfordTown_Hall_EventScript_PlayerReactNorthSouth::
	call_if_eq VAR_FACING, DIR_NORTH, DewfordTown_Hall_EventScript_PlayerWalkInPlaceUp
	call_if_eq VAR_FACING, DIR_SOUTH, DewfordTown_Hall_EventScript_PlayerWalkInPlaceDown
	return

DewfordTown_Hall_EventScript_PlayerWalkInPlaceUp::
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	return

DewfordTown_Hall_EventScript_PlayerWalkInPlaceDown::
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	return

DewfordTown_Hall_EventScript_PlayerReactEast::
	goto_if_eq VAR_FACING, DIR_WEST, DewfordTown_Hall_EventScript_DontMovePlayer1
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	return

DewfordTown_Hall_Movement_PsychicWalkInPlaceLeft:
	walk_in_place_left
	step_end

DewfordTown_Hall_Movement_SchoolKidWalkInPlaceRight:
	walk_in_place_right
	step_end

DewfordTown_Hall_EventScript_SludgeBombMan::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	goto_if_set FLAG_RECEIVED_TM_SLUDGE_BOMB, DewfordTown_Hall_EventScript_ReceivedSludgeBomb
	msgbox DewfordTown_Hall_Text_GiveYouSludgeBomb, MSGBOX_DEFAULT
	giveitem ITEM_TM_SLUDGE_BOMB
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM_SLUDGE_BOMB
	release
	end

DewfordTown_Hall_EventScript_ReceivedSludgeBomb::
	msgbox DewfordTown_Hall_Text_LoveSludgeBombButTrendInToo, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_Text_CantImagineLifeWithoutTrend:
	.string "O que está na moda? Ora, só pode ser\n"
	.string "“{STR_VAR_1}”!\p"
	.string "Nem imagino como seria a vida\n"
	.string "sem “{STR_VAR_1}”!$"

DewfordTown_Hall_Text_GettingBoredOfTrend:
	.string "O que está na moda? Ora, só pode ser\n"
	.string "“{STR_VAR_1}”!\p"
	.string "Mas já estou meio enjoado.\p"
	.string "Preciso procurar a próxima moda.$"

DewfordTown_Hall_Text_TeachingMonAboutTrend:
	.string "Estou ensinando o meu POKéMON sobre\n"
	.string "“{STR_VAR_1}”,\l"
	.string "mas não está indo bem.\p"
	.string "Acho que é demais pra ele.$"

DewfordTown_Hall_Text_DeepLinkBetweenXAndY:
	.string "Estou estudando o modernoso\n"
	.string "“{STR_VAR_1}” agora.\p"
	.string "É verdade que tem uma ligação forte\n"
	.string "entre “{STR_VAR_1}”\l"
	.string "e “{STR_VAR_2}”?$"

DewfordTown_Hall_Text_MyHunchWasRight:
	.string "Ah!\n"
	.string "Então eu estava certo!\p"
	.string "Estou um passo mais perto de ficar\n"
	.string "na moda, oba!$"

DewfordTown_Hall_Text_NotEasyToKeepUp:
	.string "O quê?!\n"
	.string "É mesmo?!\p"
	.string "Não é fácil pra um velho como eu\n"
	.string "acompanhar as modas!$"

DewfordTown_Hall_Text_TVShowAboutTrend:
	.string "Essa história toda de\n"
	.string "“{STR_VAR_1}”…\l"
	.string "Não tem um programa de TV sobre isso?$"

DewfordTown_Hall_Text_IsTrendMorePopularAcrossSea:
	.string "Do outro lado do mar…\p"
	.string "“{STR_VAR_1}”\n"
	.string "faz ainda mais sucesso?$"

DewfordTown_Hall_Text_CollectTrendMerchandise:
	.string "Eu coleciono produtos oficiais\n"
	.string "licenciados de\l"
	.string "“{STR_VAR_1}”.\p"
	.string "Tenho BONECOS oficiais de\n"
	.string "“{STR_VAR_1}”…\p"
	.string "Roupas oficiais da marca\n"
	.string "“{STR_VAR_1}”…\p"
	.string "E livros ilustrados licenciados\n"
	.string "de “{STR_VAR_1}”.\p"
	.string "Hehe, são meus!\n"
	.string "Não vou dividir nada com você!$"

DewfordTown_Hall_Text_BooksAboutTrend:
	.string "COMO DISTINGUIR O BOM\n"
	.string "“{STR_VAR_1}” DO\l"
	.string "MAU “{STR_VAR_1}”…\p"
	.string "A LIGAÇÃO ENTRE\n"
	.string "“{STR_VAR_1}” E OS\l"
	.string "POKéMON…\p"
	.string "O ÚTIL\n"
	.string "“{STR_VAR_1}”…\p"
	.string "A estante está lotada de livros sobre\n"
	.string "“{STR_VAR_1}”.$"

DewfordTown_Hall_Text_TrendsScream:
	.string "O título é “O GRITO DE\n"
	.string "{STR_VAR_1}”.$"

DewfordTown_Hall_Text_TrendsSmile:
	.string "O título é “O SORRISO DE\n"
	.string "{STR_VAR_1}”.$"

DewfordTown_Hall_Text_LastTrend:
	.string "O título é “O ÚLTIMO\n"
	.string "{STR_VAR_1}”.$"

DewfordTown_Hall_Text_BirthOfTrend:
	.string "O título é “O NASCIMENTO DE\n"
	.string "{STR_VAR_1}”.$"

DewfordTown_Hall_Text_SawTrendCoolestThing:
	.string "Eu vi “{STR_VAR_1}”!\n"
	.string "Legal, né?\p"
	.string "É, tipo, a coisa mais legal que tem!\p"
	.string "Foi incrível!\n"
	.string "Era de verdade, ah, se era!$"

DewfordTown_Hall_Text_AllegedTrendNotAuthentic:
	.string "Ah, não, não, não.\p"
	.string "Esse tal de\n"
	.string "“{STR_VAR_1}”\l"
	.string "que você diz que viu, não é.\p"
	.string "O original é muito…\n"
	.string "Como posso dizer?\l"
	.string "Mais afiado, e mais suave!\p"
	.string "Ah, tanto faz. É impressionante!$"

DewfordTown_Hall_Text_ComposedTrendThemeSong:
	.string "Ei, escuta, compus uma música-tema\n"
	.string "para “{STR_VAR_1}”.\p"
	.string "“{STR_VAR_1}”\n"
	.string "faz o maior sucesso agora.\p"
	.string "Beleza, escuta!\n"
	.string "… …\p"
	.string "Ah!\n"
	.string "{STR_VAR_1}!\p"
	.string "Maravilhoso\n"
	.string "{STR_VAR_1}!$"

DewfordTown_Hall_Text_WorkOnYourSinging:
	.string "… …\p"
	.string "Ouso dizer, meu caro, que valeria a\n"
	.string "pena treinar o seu canto antes de\l"
	.string "se meter com\l"
	.string "“{STR_VAR_1}”.$"

DewfordTown_Hall_Text_OrganizeTrendParty:
	.string "Enfim, como eu dizia, a gente devia\n"
	.string "se juntar e organizar uma festa de\l"
	.string "“{STR_VAR_1}”\l"
	.string "na ilha.$"

DewfordTown_Hall_Text_BrilliantIndeed:
	.string "Ah, que ideia esplêndida!\p"
	.string "Vai decidir de uma vez por todas\n"
	.string "quem é o melhor em\l"
	.string "“{STR_VAR_1}”.\l"
	.string "Brilhante, de fato!\p"
	.string "A partir de hoje, nossa vida vai girar\n"
	.string "em torno de “{STR_VAR_1}”!$"

DewfordTown_Hall_Text_TrendHasBecomePartOfLife:
	.string "Mas eu estava pensando…\p"
	.string "Você não concorda que\n"
	.string "“{STR_VAR_1}”\l"
	.string "deixou de ser só moda e virou\l"
	.string "parte do nosso dia a dia?$"

DewfordTown_Hall_Text_TrendWeighsHeavilyOnMind:
	.string "Como disse?\n"
	.string "Tanto assim, é?\p"
	.string "Mas é verdade que\n"
	.string "“{STR_VAR_1}”\l"
	.string "pesa na cabeça da gente, acordado\l"
	.string "ou dormindo.\p"
	.string "Sem dúvida, você é o PROFESSOR de\n"
	.string "“{STR_VAR_1}”,\l"
	.string "meu caro!$"

DewfordTown_Hall_Text_IfWeTeamUpWellBeInvincible:
	.string "Se você e eu formarmos uma dupla,\n"
	.string "ninguém vai nos vencer em tudo que\l"
	.string "for “{STR_VAR_1}”!$"

DewfordTown_Hall_Text_WellBeTrendDuo:
	.string "Na mosca, meu amigo!\p"
	.string "Seremos a DUPLA\n"
	.string "“{STR_VAR_1}”!\p"
	.string "Não é uma bela imagem?\n"
	.string "Hahahah!$"

DewfordTown_Hall_Text_GiveYouSludgeBomb:
	.string "Pra mim, BOMBA LODO está no auge\n"
	.string "da popularidade. É a tal.\p"
	.string "Hã? Você está me dizendo que não\n"
	.string "conhece BOMBA LODO?\p"
	.string "Isso é de dar dó.\n"
	.string "Vou te dar uma.$"

DewfordTown_Hall_Text_LoveSludgeBombButTrendInToo:
	.string "Eu adoro BOMBA LODO.\p"
	.string "Mas POKéMON com o golpe\n"
	.string "“{STR_VAR_1}”\l"
	.string "também estão na moda.$"

