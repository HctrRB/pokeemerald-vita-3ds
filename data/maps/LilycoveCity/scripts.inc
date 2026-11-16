LilycoveCity_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, LilycoveCity_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, LilycoveCity_OnLoad
	.byte 0

LilycoveCity_OnTransition:
	setflag FLAG_VISITED_LILYCOVE_CITY
	setvar VAR_CONTEST_HALL_STATE, 0
	setflag FLAG_HIDE_LILYCOVE_CONTEST_HALL_REPORTER
	call_if_set FLAG_SYS_WEATHER_CTRL, Common_EventScript_SetAbnormalWeather
	call Common_EventScript_SetupRivalGfxId
	end

LilycoveCity_OnLoad:
	call_if_unset FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE, LilycoveCity_EventScript_SetWailmerMetatiles
	end

LilycoveCity_EventScript_SetWailmerMetatiles::
	setmetatile 76, 12, METATILE_Lilycove_Wailmer0, TRUE
	setmetatile 77, 12, METATILE_Lilycove_Wailmer1, TRUE
	setmetatile 76, 13, METATILE_Lilycove_Wailmer2, TRUE
	setmetatile 77, 13, METATILE_Lilycove_Wailmer3, TRUE
	setmetatile 76, 14, METATILE_Lilycove_Wailmer0_Alt, TRUE
	setmetatile 77, 14, METATILE_Lilycove_Wailmer1_Alt, TRUE
	setmetatile 76, 15, METATILE_Lilycove_Wailmer2, TRUE
	setmetatile 77, 15, METATILE_Lilycove_Wailmer3, TRUE
	setmetatile 77, 16, METATILE_Lilycove_Wailmer0_Alt, TRUE
	setmetatile 78, 16, METATILE_Lilycove_Wailmer1_Alt, TRUE
	setmetatile 77, 17, METATILE_Lilycove_Wailmer2, TRUE
	setmetatile 78, 17, METATILE_Lilycove_Wailmer3, TRUE
	return

LilycoveCity_EventScript_BerryGentleman::
	lock
	faceplayer
	dotimebasedevents
	goto_if_set FLAG_DAILY_LILYCOVE_RECEIVED_BERRY, LilycoveCity_EventScript_ReceivedBerry
	msgbox LilycoveCity_Text_BerrySuitsYou, MSGBOX_DEFAULT
	random 10
	addvar VAR_RESULT, FIRST_BERRY_INDEX
	giveitem VAR_RESULT
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_DAILY_LILYCOVE_RECEIVED_BERRY
	msgbox LilycoveCity_Text_BecauseYoureTrainer, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_ReceivedBerry::
	msgbox LilycoveCity_Text_PokeblocksSuitPokemon, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_Man3::
	msgbox LilycoveCity_Text_ContestHallInTown, MSGBOX_NPC
	end

LilycoveCity_EventScript_Girl::
	lock
	faceplayer
	goto_if_set FLAG_BADGE07_GET, LilycoveCity_EventScript_GirlAquaGone
	msgbox LilycoveCity_Text_StrangeCaveInCove, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_GirlAquaGone::
	msgbox LilycoveCity_Text_GoingToMoveDeleterForHMs, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_RichBoy::
	msgbox LilycoveCity_Text_ImFromKanto, MSGBOX_NPC
	end

LilycoveCity_EventScript_Sailor2::
	lock
	faceplayer
	goto_if_set FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE, LilycoveCity_EventScript_Sailor2AquaGone
	msgbox LilycoveCity_Text_TeamAquaBeenTrainingWailmer, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_Sailor2AquaGone::
	msgbox LilycoveCity_Text_SomeonePuntedTeamAquaOut, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_Woman2::
	lock
	faceplayer
	goto_if_set FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE, LilycoveCity_EventScript_Woman2AquaGone
	msgbox LilycoveCity_Text_SomeoneStoleMyPokemon, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_Woman2AquaGone::
	msgbox LilycoveCity_Text_MissingPokemonCameBack, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_Man2::
	msgbox LilycoveCity_Text_ImArtDealer, MSGBOX_NPC
	end

LilycoveCity_EventScript_ExpertM1::
	msgbox LilycoveCity_Text_SeaRemainsForeverYoung, MSGBOX_SIGN
	end

LilycoveCity_EventScript_ExpertF::
	msgbox LilycoveCity_Text_SixtyYearsAgoHusbandProposed, MSGBOX_SIGN
	end

LilycoveCity_EventScript_ExpertM2::
	lock
	faceplayer
	goto_if_set FLAG_BADGE07_GET, LilycoveCity_EventScript_ExpertM2AquaGone
	msgbox LilycoveCity_Text_TeamAquaRenovatedCavern, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_ExpertM2AquaGone::
	msgbox LilycoveCity_Text_TeamAquaLotGoneForGood, MSGBOX_DEFAULT
	release
	end

LilycoveCity_EventScript_Sailor1::
	msgbox LilycoveCity_Text_HeardTowerCalledSkyPillar, MSGBOX_NPC
	end

LilycoveCity_EventScript_FatMan::
	msgbox LilycoveCity_Text_SawTallTowerOnRoute131, MSGBOX_NPC
	end

LilycoveCity_EventScript_Man1::
	lockall
	msgbox LilycoveCity_Text_JustArrivedAndSawRarePokemon, MSGBOX_NPC
	applymovement LOCALID_LILYCOVE_MAN_1, Common_Movement_FaceOriginalDirection
	end

LilycoveCity_EventScript_Woman1::
	lockall
	msgbox LilycoveCity_Text_HoneymoonVowToSeeRarePokemon, MSGBOX_NPC
	applymovement LOCALID_LILYCOVE_WOMAN_1, Common_Movement_FaceOriginalDirection
	end

LilycoveCity_EventScript_CitySign::
	msgbox LilycoveCity_Text_CitySign, MSGBOX_SIGN
	end

LilycoveCity_EventScript_ContestHallSign::
	msgbox LilycoveCity_Text_ContestHallSign, MSGBOX_SIGN
	end

LilycoveCity_EventScript_MotelSign::
	msgbox LilycoveCity_Text_MotelSign, MSGBOX_SIGN
	end

LilycoveCity_EventScript_MuseumSign::
	lockall
	specialvar VAR_0x8004, CountPlayerMuseumPaintings
	switch VAR_0x8004
	case 0, LilycoveCity_EventScript_MuseumSignNoPaintings
	msgbox LilycoveCity_Text_MuseumSignPlayersExhibit, MSGBOX_DEFAULT
	releaseall
	end

LilycoveCity_EventScript_MuseumSignNoPaintings::
	msgbox LilycoveCity_Text_MuseumSign, MSGBOX_DEFAULT
	releaseall
	end

LilycoveCity_EventScript_HarborSign::
	lockall
	goto_if_set FLAG_SYS_GAME_CLEAR, LilycoveCity_EventScript_HarborSignFerryReady
	msgbox LilycoveCity_Text_HarborSignUnderConstruction, MSGBOX_DEFAULT
	releaseall
	end

LilycoveCity_EventScript_HarborSignFerryReady::
	msgbox LilycoveCity_Text_HarborSign, MSGBOX_DEFAULT
	releaseall
	end

LilycoveCity_EventScript_TrainerFanClubSign::
	msgbox LilycoveCity_Text_TrainerFanClubSign, MSGBOX_SIGN
	end

LilycoveCity_EventScript_DepartmentStoreSign::
	msgbox LilycoveCity_Text_DepartmentStoreSign, MSGBOX_SIGN
	end

LilycoveCity_EventScript_MoveDeletersHouseSign::
	msgbox LilycoveCity_Text_MoveDeletersHouseSign, MSGBOX_SIGN
	end

LilycoveCity_EventScript_WailmerTrainerGrunt::
	lockall
	goto_if_set FLAG_MET_WAILMER_TRAINER, LilycoveCity_EventScript_MetWailmerTrainer
	msgbox LilycoveCity_Text_WailmerLeapOutOfWater, MSGBOX_DEFAULT
	applymovement VAR_LAST_TALKED, Common_Movement_FacePlayer
	waitmovement 0
	msgbox LilycoveCity_Text_GetLostMessingUpTraining, MSGBOX_DEFAULT
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	setflag FLAG_MET_WAILMER_TRAINER
	releaseall
	end

LilycoveCity_EventScript_MetWailmerTrainer::
	msgbox LilycoveCity_Text_WailmerLeapOutOfWater, MSGBOX_DEFAULT
	releaseall
	end

LilycoveCity_EventScript_AquaGrunt1::
	msgbox LilycoveCity_Text_MovedLootIntoHideoutToday, MSGBOX_NPC
	end

LilycoveCity_EventScript_AquaGrunt2::
	msgbox LilycoveCity_Text_ChanceToDoBigThings, MSGBOX_NPC
	end

LilycoveCity_EventScript_AquaGrunt3::
	msgbox LilycoveCity_Text_DontGoNearCaveInCove, MSGBOX_NPC
	end

LilycoveCity_EventScript_AquaGrunt4::
	msgbox LilycoveCity_Text_IfWorldBecomesOurs, MSGBOX_NPC
	end

LilycoveCity_EventScript_Rival::
	lock
	faceplayer
	checkplayergender
	goto_if_eq VAR_RESULT, MALE, LilycoveCity_EventScript_May
	goto_if_eq VAR_RESULT, FEMALE, LilycoveCity_EventScript_Brendan
	end

LilycoveCity_EventScript_May::
	playbgm MUS_ENCOUNTER_MAY, TRUE
	call_if_set FLAG_DECLINED_RIVAL_BATTLE_LILYCOVE, LilycoveCity_EventScript_MayAskToBattleAgain
	call_if_unset FLAG_DECLINED_RIVAL_BATTLE_LILYCOVE, LilycoveCity_EventScript_MayAskToBattle
	goto_if_eq VAR_RESULT, NO, LilycoveCity_EventScript_DeclineMayBattle
	msgbox LilycoveCity_Text_MayWontBeBeaten, MSGBOX_DEFAULT
	switch VAR_STARTER_MON
	case 0, LilycoveCity_EventScript_BattleMayTreecko
	case 1, LilycoveCity_EventScript_BattleMayTorchic
	case 2, LilycoveCity_EventScript_BattleMayMudkip
	end

LilycoveCity_EventScript_MayAskToBattleAgain::
	msgbox LilycoveCity_Text_MayBattleMe, MSGBOX_YESNO
	return

LilycoveCity_EventScript_MayAskToBattle::
	msgbox LilycoveCity_Text_MayShoppingLetsBattle, MSGBOX_YESNO
	return

LilycoveCity_EventScript_DeclineMayBattle::
	setflag FLAG_DECLINED_RIVAL_BATTLE_LILYCOVE
	msgbox LilycoveCity_Text_MayNotRaisingPokemon, MSGBOX_DEFAULT
	savebgm MUS_DUMMY
	fadedefaultbgm
	release
	end

LilycoveCity_EventScript_Brendan::
	playbgm MUS_ENCOUNTER_BRENDAN, TRUE
	call_if_set FLAG_DECLINED_RIVAL_BATTLE_LILYCOVE, LilycoveCity_EventScript_BrendanAskToBattleAgain
	call_if_unset FLAG_DECLINED_RIVAL_BATTLE_LILYCOVE, LilycoveCity_EventScript_BrendanAskToBattle
	goto_if_eq VAR_RESULT, NO, LilycoveCity_EventScript_DeclineBrendanBattle
	msgbox LilycoveCity_Text_BrendanWontBeBeaten, MSGBOX_DEFAULT
	switch VAR_STARTER_MON
	case 0, LilycoveCity_EventScript_BattleBrendanTreecko
	case 1, LilycoveCity_EventScript_BattleBrendanTorchic
	case 2, LilycoveCity_EventScript_BattleBrendanMudkip
	end

LilycoveCity_EventScript_BrendanAskToBattleAgain::
	msgbox LilycoveCity_Text_BrendanBattleMe, MSGBOX_YESNO
	return

LilycoveCity_EventScript_BrendanAskToBattle::
	msgbox LilycoveCity_Text_BrendanShoppingLetsBattle, MSGBOX_YESNO
	return

LilycoveCity_EventScript_DeclineBrendanBattle::
	setflag FLAG_DECLINED_RIVAL_BATTLE_LILYCOVE
	msgbox LilycoveCity_Text_BrendanNoConfidence, MSGBOX_DEFAULT
	savebgm MUS_DUMMY
	fadedefaultbgm
	release
	end

LilycoveCity_EventScript_BattleMayTreecko::
	trainerbattle_no_intro TRAINER_MAY_LILYCOVE_TREECKO, LilycoveCity_Text_MayDefeat
	goto LilycoveCity_EventScript_DefeatedMay
	end

LilycoveCity_EventScript_BattleMayTorchic::
	trainerbattle_no_intro TRAINER_MAY_LILYCOVE_TORCHIC, LilycoveCity_Text_MayDefeat
	goto LilycoveCity_EventScript_DefeatedMay
	end

LilycoveCity_EventScript_BattleMayMudkip::
	trainerbattle_no_intro TRAINER_MAY_LILYCOVE_MUDKIP, LilycoveCity_Text_MayDefeat
	goto LilycoveCity_EventScript_DefeatedMay
	end

LilycoveCity_EventScript_BattleBrendanTreecko::
	trainerbattle_no_intro TRAINER_BRENDAN_LILYCOVE_TREECKO, LilycoveCity_Text_BrendanDefeat
	goto LilycoveCity_EventScript_DefeatedBrendan
	end

LilycoveCity_EventScript_BattleBrendanTorchic::
	trainerbattle_no_intro TRAINER_BRENDAN_LILYCOVE_TORCHIC, LilycoveCity_Text_BrendanDefeat
	goto LilycoveCity_EventScript_DefeatedBrendan
	end

LilycoveCity_EventScript_BattleBrendanMudkip::
	trainerbattle_no_intro TRAINER_BRENDAN_LILYCOVE_MUDKIP, LilycoveCity_Text_BrendanDefeat
	goto LilycoveCity_EventScript_DefeatedBrendan
	end

LilycoveCity_EventScript_DefeatedMay::
	msgbox LilycoveCity_Text_MayGoingBackToLittleroot, MSGBOX_DEFAULT
	setvar VAR_RESULT, FALSE
	call_if_set FLAG_BADGE06_GET, LilycoveCity_EventScript_CheckFinalBadge
	call_if_eq VAR_RESULT, FALSE, LilycoveCity_EventScript_MayCollectBadges
	call_if_eq VAR_RESULT, TRUE, LilycoveCity_EventScript_MayPokemonLeague
	clearflag FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_RIVAL_BEDROOM
	clearflag FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_2F_PICHU_DOLL
	goto LilycoveCity_EventScript_RivalFlyAway
	end

LilycoveCity_EventScript_CheckFinalBadge::
	goto_if_set FLAG_BADGE08_GET, LilycoveCity_EventScript_HasFinalBadge
	return

LilycoveCity_EventScript_HasFinalBadge::
	setvar VAR_RESULT, TRUE
	return

LilycoveCity_EventScript_MayCollectBadges::
	msgbox LilycoveCity_Text_MayYouGoingToCollectBadges, MSGBOX_DEFAULT
	return

LilycoveCity_EventScript_MayPokemonLeague::
	goto_if_set FLAG_SYS_GAME_CLEAR, LilycoveCity_EventScript_MayBattleFrontier
	msgbox LilycoveCity_Text_MayYouGoingToPokemonLeague, MSGBOX_DEFAULT
	return

LilycoveCity_EventScript_MayBattleFrontier::
	msgbox LilycoveCity_Text_MayYouGoingToBattleFrontier, MSGBOX_DEFAULT
	return

LilycoveCity_EventScript_DefeatedBrendan::
	msgbox LilycoveCity_Text_BrendanGoingBackToLittleroot, MSGBOX_DEFAULT
	setvar VAR_RESULT, FALSE
	call_if_set FLAG_BADGE06_GET, LilycoveCity_EventScript_CheckFinalBadge
	call_if_eq VAR_RESULT, FALSE, LilycoveCity_EventScript_BrendanCollectBadges
	call_if_eq VAR_RESULT, TRUE, LilycoveCity_EventScript_BrendanPokemonLeague
	clearflag FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_RIVAL_BEDROOM
	clearflag FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F_SWABLU_DOLL
	goto LilycoveCity_EventScript_RivalFlyAway
	end

LilycoveCity_EventScript_BrendanCollectBadges::
	msgbox LilycoveCity_Text_BrendanYouGoingToCollectBadges, MSGBOX_DEFAULT
	return

LilycoveCity_EventScript_BrendanPokemonLeague::
	goto_if_set FLAG_SYS_GAME_CLEAR, LilycoveCity_EventScript_BrendanBattleFrontier
	msgbox LilycoveCity_Text_BrendanYouGoingToPokemonLeague, MSGBOX_DEFAULT
	return

LilycoveCity_EventScript_BrendanBattleFrontier::
	msgbox LilycoveCity_Text_BrendanYouGoingToBattleFrontier, MSGBOX_DEFAULT
	return

LilycoveCity_EventScript_RivalFlyAway::
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	delay 50
	setfieldeffectargument 0, 1
	dofieldeffect FLDEFF_NPCFLY_OUT
	delay 15
	removeobject VAR_LAST_TALKED
	waitfieldeffect FLDEFF_NPCFLY_OUT
	savebgm MUS_DUMMY
	fadedefaultbgm
	setflag FLAG_MET_RIVAL_LILYCOVE
	release
	end

LilycoveCity_EventScript_SchoolKidM::
	lock
	faceplayer
	msgbox LilycoveCity_Text_DoYouKnowAboutBerryBlender, MSGBOX_YESNO
	call_if_eq VAR_RESULT, YES, LilycoveCity_EventScript_KnowAboutBerryBlender
	call_if_eq VAR_RESULT, NO, LilycoveCity_EventScript_DontKnowAboutBerryBlender
	release
	end

LilycoveCity_EventScript_KnowAboutBerryBlender::
	msgbox LilycoveCity_Text_FasterSpinBetterPokeblocks, MSGBOX_DEFAULT
	return

LilycoveCity_EventScript_DontKnowAboutBerryBlender::
	msgbox LilycoveCity_Text_ExplainBerryBlender, MSGBOX_DEFAULT
	return

LilycoveCity_Text_MayShoppingLetsBattle:
	.string "MAY: Ué, oi?\n"
	.string "{PLAYER}{KUN}, também veio às compras?\p"
	.string "Comprei um monte de BONECOS e\n"
	.string "PÔSTERES.\p"
	.string "Claro que também peguei muitos\n"
	.string "POKéMON, e estou treinando eles\l"
	.string "direitinho.\p"
	.string "Na verdade, estou mais preocupada se\n"
	.string "você treina direito, {PLAYER}{KUN}.\p"
	.string "Vamos batalhar, pra você me mostrar\n"
	.string "os seus POKéMON, {PLAYER}{KUN}.$"

LilycoveCity_Text_MayNotRaisingPokemon:
	.string "MAY: Ué, por quê? {PLAYER}{KUN}, você não\n"
	.string "anda treinando os seus POKéMON?\p"
	.string "Isso não fica bem pra um\n"
	.string "TREINADOR…$"

LilycoveCity_Text_MayBattleMe:
	.string "MAY: Vamos, batalhe comigo, pra eu\n"
	.string "ver os seus POKéMON!$"

LilycoveCity_Text_MayWontBeBeaten:
	.string "MAY: Os POKéMON que eu treinei não vão\n"
	.string "perder pros seus, {PLAYER}{KUN}!$"

LilycoveCity_Text_MayDefeat:
	.string "… … … … … … … …\p"
	.string "Lembro da batalha que tive com você,\n"
	.string "{PLAYER}{KUN}, na ROTA 103.\p"
	.string "Aquela batalha te ajudou a ficar\n"
	.string "tão forte, {PLAYER}{KUN}, não foi?$"

LilycoveCity_Text_MayGoingBackToLittleroot:
	.string "MAY: Estou pensando em voltar logo\n"
	.string "pra LITTLEROOT.\p"
	.string "Peguei um bom grupo de POKéMON, e a\n"
	.string "minha POKéDEX está andando, então\l"
	.string "vou pra casa mostrar pro meu pai.$"

LilycoveCity_Text_MayYouGoingToCollectBadges:
	.string "{PLAYER}{KUN}, o que você vai fazer?\p"
	.string "Juntar todas as INSÍGNIAS e desafiar\n"
	.string "a LIGA POKéMON?\p"
	.string "Bom, enquanto você junta INSÍGNIAS,\n"
	.string "{PLAYER}{KUN}, eu cuido da minha\l"
	.string "POKéDEX. Vou completar antes de você!\p"
	.string "Até mais!$"

LilycoveCity_Text_MayYouGoingToPokemonLeague:
	.string "{PLAYER}{KUN}, o que você vai fazer?\p"
	.string "Vai desafiar a LIGA POKéMON?\p"
	.string "Ou vai completar a sua POKéDEX?\p"
	.string "Posso ter perdido na batalha, mas\n"
	.string "vou terminar a POKéDEX antes de você!\p"
	.string "Até mais!$"

LilycoveCity_Text_MayYouGoingToBattleFrontier:
	.string "{PLAYER}{KUN}, o que você vai fazer?\p"
	.string "Vai encarar a FRONTEIRA DE BATALHA?\p"
	.string "Ou vai completar a sua POKéDEX?\p"
	.string "Posso ter perdido na batalha, mas\n"
	.string "vou terminar a POKéDEX antes de você!\p"
	.string "Até mais!$"

LilycoveCity_Text_BrendanShoppingLetsBattle:
	.string "BRENDAN: Opa, é a {PLAYER}.\p"
	.string "Estou fazendo um serviço pro meu pai.\n"
	.string "Não, não estou comprando BONECOS.\p"
	.string "Enfim, quanto tempo.\p"
	.string "Quer batalhar pra ver quem anda\n"
	.string "treinando melhor os POKéMON?$"

LilycoveCity_Text_BrendanNoConfidence:
	.string "BRENDAN: Ué, o que foi?\n"
	.string "Falta confiança?$"

LilycoveCity_Text_BrendanBattleMe:
	.string "BRENDAN: Quer batalhar pra ver quem\n"
	.string "anda treinando melhor os POKéMON?$"

LilycoveCity_Text_BrendanWontBeBeaten:
	.string "BRENDAN: Ah, mas você sabe que eu não\n"
	.string "vou perder pra {PLAYER} nenhuma.$"

LilycoveCity_Text_BrendanDefeat:
	.string "Hunf…\n"
	.string "Você treinou bastante.\p"
	.string "Isso dói um pouco… Eu comecei como\n"
	.string "TREINADOR antes de você…$"

LilycoveCity_Text_BrendanGoingBackToLittleroot:
	.string "BRENDAN: Eu…\n"
	.string "Vou voltar pra casa, em LITTLEROOT.\p"
	.string "Você sabe que ajudo o meu pai na\n"
	.string "POKéDEX dele. Está ficando bem\l"
	.string "decente, então vou mostrar pra ele.$"

LilycoveCity_Text_BrendanYouGoingToCollectBadges:
	.string "{PLAYER}, o que você vai fazer?\p"
	.string "Juntar todas as INSÍGNIAS e desafiar\n"
	.string "a LIGA POKéMON?\p"
	.string "Talvez eu faça isso também…\p"
	.string "Bom, de qualquer jeito, força aí!$"

LilycoveCity_Text_BrendanYouGoingToPokemonLeague:
	.string "{PLAYER}, o que você vai fazer?\p"
	.string "Forte desse jeito, vai desafiar a\n"
	.string "LIGA POKéMON?\p"
	.string "Heh, até eu… se treinasse mais um\n"
	.string "pouco os meus POKéMON, ficaria mais\l"
	.string "forte que você, {PLAYER}.\p"
	.string "Bom, de qualquer jeito, força aí!$"

LilycoveCity_Text_BrendanYouGoingToBattleFrontier:
	.string "{PLAYER}, o que você vai fazer?\p"
	.string "Vai encarar a FRONTEIRA DE BATALHA?\p"
	.string "Ou vai completar a sua POKéDEX?\p"
	.string "Forte desse jeito, devia pensar na\n"
	.string "FRONTEIRA DE BATALHA.\p"
	.string "Heh, se eu treinar mais um pouco os\n"
	.string "meus POKéMON, fico mais forte que\l"
	.string "você, {PLAYER}.\p"
	.string "Talvez eu devesse ir pra FRONTEIRA\n"
	.string "DE BATALHA.\p"
	.string "Bom, de qualquer jeito, força aí!$"

LilycoveCity_Text_MovedLootIntoHideoutToday:
	.string "Fufufu…\p"
	.string "Hoje levamos mais muamba pro nosso\n"
	.string "ESCONDERIJO secreto…\p"
	.string "Q-quem é você?!\n"
	.string "Eu só estava falando sozinho!$"

LilycoveCity_Text_ChanceToDoBigThings:
	.string "Sou só um CAPANGA, não sei o que o\n"
	.string "CHEFE está pensando…\p"
	.string "Mas, na EQUIPE AQUA, sei que vou ter\n"
	.string "chance de fazer grandes coisas!\p"
	.string "Q-quem é você?!\n"
	.string "Eu só estava falando sozinho!$"

LilycoveCity_Text_DontGoNearCaveInCove:
	.string "Ei, você!\p"
	.string "Não chegue perto da caverna da\n"
	.string "enseada!\p"
	.string "Por quê? Não precisa de motivo!\n"
	.string "Eu sou adulto, então me obedeça!$"

LilycoveCity_Text_IfWorldBecomesOurs:
	.string "Se este mundo todo for nosso, da\n"
	.string "EQUIPE AQUA, vai ser um lugar mais\l"
	.string "feliz pros POKéMON também.$"

LilycoveCity_Text_WailmerLeapOutOfWater:
	.string "Vai! WAILMER!\n"
	.string "Pule pra fora da água agora!$"

LilycoveCity_Text_GetLostMessingUpTraining:
	.string "Hã? O que você quer?\p"
	.string "Você está atrapalhando o nosso treino,\n"
	.string "então, tipo, dá o fora?$"

LilycoveCity_Text_ContestHallInTown:
	.string "Tem um SALÃO DE CONCURSOS POKéMON\n"
	.string "bem aqui nesta cidade.\p"
	.string "Quer dizer que POKéMON bem cuidados\n"
	.string "vêm do país inteiro.\p"
	.string "O meu coração até dispara!$"

LilycoveCity_Text_StrangeCaveInCove:
	.string "Já viu aquela caverna estranha na\n"
	.string "enseada, no fim da cidade?$"

LilycoveCity_Text_GoingToMoveDeleterForHMs:
	.string "Vou ensinar golpes novos aos meus\n"
	.string "POKéMON pra entrar num CONCURSO.\p"
	.string "Então vou ao APAGADOR DE GOLPES\n"
	.string "pra eles esquecerem os golpes de MO.$"

LilycoveCity_Text_ImFromKanto:
	.string "Eu vim de KANTO.\p"
	.string "A região de HOENN é linda, com\n"
	.string "tanta água e natureza verdinha.\p"
	.string "Hã? Pera aí…\p"
	.string "Será que tem POKéMON raros que\n"
	.string "só vivem por aqui?$"

LilycoveCity_Text_TeamAquaBeenTrainingWailmer:
	.string "A EQUIPE AQUA anda treinando os\n"
	.string "WAILMER na enseada.\p"
	.string "Nós, MARINHEIROS, não conseguimos\n"
	.string "levar os barcos pro mar com eles ali!$"

LilycoveCity_Text_SomeonePuntedTeamAquaOut:
	.string "Ahoy, sabia?\p"
	.string "Alguém botou a EQUIPE AQUA pra\n"
	.string "correr pra gente!\p"
	.string "Os WAILMER saíram da enseada, e\n"
	.string "já dá pra levar os barcos de novo.$"

LilycoveCity_Text_SomeoneStoleMyPokemon:
	.string "Eu tirava um cochilo gostoso na\n"
	.string "pousada da praia, ouvindo as ondas…\p"
	.string "Quando alguém roubou o meu POKéMON!\p"
	.string "Quem foi o ladrão?\n"
	.string "A EQUIPE AQUA? Foi a EQUIPE AQUA?$"

LilycoveCity_Text_MissingPokemonCameBack:
	.string "O meu POKéMON sumido…\n"
	.string "Voltou sem eu nem perceber!$"

LilycoveCity_Text_ImArtDealer:
	.string "Sou NEGOCIANTE DE ARTE.\p"
	.string "Compro e vendo belas-artes,\n"
	.string "principalmente quadros.\p"
	.string "Ouvi dizer que o MUSEU daqui tem\n"
	.string "obras magníficas em exposição…$"

LilycoveCity_Text_SeaRemainsForeverYoung:
	.string "Mesmo que a gente envelheça e fique\n"
	.string "cheio de rugas, o mar continua jovem,\l"
	.string "com o brilho vivo da vida…\p"
	.string "Fuhoho fuhohohoho…$"

LilycoveCity_Text_SixtyYearsAgoHusbandProposed:
	.string "Foi aqui que o meu marido me pediu\n"
	.string "em casamento, há sessenta anos.\l"
	.string "O mar continua lindo como sempre.\p"
	.string "Mufufufu mufufufufufu…$"

LilycoveCity_Text_TeamAquaRenovatedCavern:
	.string "Hum, conhece a caverna esquisita\n"
	.string "desta enseada?\p"
	.string "Era uma formação natural, mas aí\n"
	.string "aquela turma da EQUIPE AQUA veio\l"
	.string "e fez a reforma deles.\p"
	.string "Eles se chamam de “EQUIPE AQUA,\n"
	.string "amante da natureza”!\p"
	.string "Mas o que fazem e o que dizem\n"
	.string "não combinam nem um pouco!$"

LilycoveCity_Text_TeamAquaLotGoneForGood:
	.string "Aquela caverna da enseada…\p"
	.string "Não percebi de cara, mas agora\n"
	.string "está completamente vazia.\p"
	.string "Aquela turma da EQUIPE AQUA deve\n"
	.string "ter ido embora de vez.$"

LilycoveCity_Text_CitySign:
	.string "CIDADE DE LILYCOVE\p"
	.string "“Onde a terra acaba e o mar\n"
	.string "começa.”$"

LilycoveCity_Text_ContestHallSign:
	.string "SALÃO DE CONCURSOS POKéMON\n"
	.string "“O encontro dos TREINADORES!”$"

LilycoveCity_Text_MotelSign:
	.string "MOTEL COVE LILY\p"
	.string "“Lembre-se de nós: COVE LILY,\n"
	.string "de LILYCOVE.”$"

LilycoveCity_Text_MuseumSign:
	.string "MUSEU DE LILYCOVE\p"
	.string "“Exposição de obras-primas\n"
	.string "com POKéMON!”$"

LilycoveCity_Text_MuseumSignPlayersExhibit:
	.string "MUSEU DE LILYCOVE\p"
	.string "“Exposição da coleção POKéMON\n"
	.string "de {PLAYER}!”$"

LilycoveCity_Text_HarborSignUnderConstruction:
	.string "PORTO DA CIDADE DE LILYCOVE\p"
	.string "“A balsa S.S. TIDAL está sendo\n"
	.string "construída em SLATEPORT.\p"
	.string "“O serviço começa em breve.”$"

LilycoveCity_Text_HarborSign:
	.string "PORTO DA CIDADE DE LILYCOVE\p"
	.string "“Faça um cruzeiro delicioso na\n"
	.string "balsa S.S. TIDAL.”$"

LilycoveCity_Text_TrainerFanClubSign:
	.string "FÃ-CLUBE DOS TREINADORES POKéMON\p"
	.string "A placa está toda rabiscada com\n"
	.string "nomes de TREINADORES…$"

LilycoveCity_Text_DepartmentStoreSign:
	.string "LOJA DE DEPARTAMENTOS DE LILYCOVE\p"
	.string "“Transbordando de ótimas mercadorias\n"
	.string "e emoção!\p"
	.string "“O lugar certo pra achar aquilo\n"
	.string "de que você precisa!”$"

LilycoveCity_Text_MoveDeletersHouseSign:
	.string "CASA DO APAGADOR DE GOLPES\p"
	.string "“Golpes indesejados apagados.”$"

LilycoveCity_Text_DoYouKnowAboutBerryBlender:
	.string "Conhece a máquina chamada\n"
	.string "LIQUIDIFICADOR DE FRUTAS?$"

LilycoveCity_Text_FasterSpinBetterPokeblocks:
	.string "As pessoas têm que trabalhar juntas\n"
	.string "pra ela girar mais rápido.\p"
	.string "Quanto mais rápido girar, melhores\n"
	.string "os {POKEBLOCK}S, pelo que ouvi.$"

LilycoveCity_Text_ExplainBerryBlender:
	.string "Ela é usada pra fazer um docinho\n"
	.string "chamado {POKEBLOCK}.\p"
	.string "Se der um {POKEBLOCK} bom pra um POKéMON,\n"
	.string "a condição dele melhora bastante.$"

LilycoveCity_Text_HeardTowerCalledSkyPillar:
	.string "Ouvi dizer que tem uma torre em algum\n"
	.string "lugar das rotas marítimas.\p"
	.string "Dizem que se chama PILAR CELESTE.$"

LilycoveCity_Text_SawTallTowerOnRoute131:
	.string "Vi uma torre alta em algum lugar\n"
	.string "perto da ROTA 131.\p"
	.string "Será que aquilo era…?$"

LilycoveCity_Text_JustArrivedAndSawRarePokemon:
	.string "Acabamos de chegar aqui, em lua\n"
	.string "de mel.\p"
	.string "Por acaso, vimos um POKéMON do tipo\n"
	.string "DRAGÃO voando lá no alto do céu.\p"
	.string "POKéMON legais assim vivem na\n"
	.string "região de HOENN?$"

LilycoveCity_Text_HoneymoonVowToSeeRarePokemon:
	.string "Na lua de mel, juramos ver o máximo\n"
	.string "de POKéMON raros que der.\p"
	.string "Então adoramos ver um POKéMON raro\n"
	.string "assim que chegamos.\p"
	.string "Quer dizer, estamos felizes demais!$"

