BattleFrontier_BattlePyramidLobby_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, BattleFrontier_BattlePyramidLobby_OnFrame
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, BattleFrontier_BattleDomeLobby_OnWarp
	.byte 0

/* Unused. Pyramid uses Dome's OnWarp (presumably by mistake). Their effects are identical*/
BattleFrontier_BattlePyramidLobby_OnWarp:
	map_script_2 VAR_TEMP_1, 0, BattleFrontier_BattlePyramidLobby_EventScript_TurnPlayerNorth
	.short 0

BattleFrontier_BattlePyramidLobby_EventScript_TurnPlayerNorth:
	setvar VAR_TEMP_1, 1
	turnobject LOCALID_PLAYER, DIR_NORTH
	end

BattleFrontier_BattlePyramidLobby_OnFrame:
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, 0, BattleFrontier_BattlePyramidLobby_EventScript_GetChallengeStatus
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_SAVING, BattleFrontier_BattlePyramidLobby_EventScript_QuitWithoutSaving
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_PAUSED, BattleFrontier_BattlePyramidLobby_EventScript_ResumeChallenge
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_WON, BattleFrontier_BattlePyramidLobby_EventScript_WonChallenge
	map_script_2 VAR_TEMP_CHALLENGE_STATUS, CHALLENGE_STATUS_LOST, BattleFrontier_BattlePyramidLobby_EventScript_LostChallenge
	.short 0

BattleFrontier_BattlePyramidLobby_EventScript_GetChallengeStatus::
	frontier_getstatus
	end

BattleFrontier_BattlePyramidLobby_EventScript_QuitWithoutSaving::
	lockall
	message BattleFrontier_BattlePyramidLobby_Text_DidntSaveBeforeQuittingTakeBag
	waitmessage
	playse SE_EXP_MAX
	waitse
	closemessage
	pyramid_set PYRAMID_DATA_WIN_STREAK, 0
	pyramid_set PYRAMID_DATA_WIN_STREAK_ACTIVE, FALSE
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	releaseall
	end

BattleFrontier_BattlePyramidLobby_EventScript_WonChallenge::
	lockall
	frontier_isbrain
	goto_if_eq VAR_RESULT, TRUE, BattleFrontier_BattlePyramidLobby_EventScript_DefeatedKing
	msgbox BattleFrontier_BattlePyramidLobby_Text_YouveConqueredPyramid, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_GiveBattlePoints

BattleFrontier_BattlePyramidLobby_EventScript_DefeatedKing::
	msgbox BattleFrontier_BattlePyramidLobby_Text_YouveDefeatedPyramidKing, MSGBOX_DEFAULT
BattleFrontier_BattlePyramidLobby_EventScript_GiveBattlePoints::
	special DoBattlePyramidMonsHaveHeldItem
	call_if_eq VAR_RESULT, TRUE, BattleFrontier_BattlePyramidLobby_EventScript_StoreHeldItemsInPyramidBag
	clearflag FLAG_STORING_ITEMS_IN_PYRAMID_BAG
	frontier_checkairshow
	special LoadPlayerParty
	pyramid_clearhelditems
	special HealPlayerParty
	message BattleFrontier_BattlePyramidLobby_Text_UsedBattleBagWillBeKept
	waitmessage
	playse SE_EXP_MAX
	waitse
	msgbox BattleFrontier_BattlePyramidLobby_Text_GiveYouTheseBattlePoints, MSGBOX_DEFAULT
	frontier_givepoints
	msgbox BattleFrontier_Text_ObtainedXBattlePoints, MSGBOX_GETPOINTS
	message BattleFrontier_BattlePyramidLobby_Text_RecordResultsWait
	waitmessage
	pyramid_setprize
	pyramid_save 0
	playse SE_SAVE
	waitse
	msgbox BattleFrontier_BattlePyramidLobby_Text_LookForwardToNextChallenge, MSGBOX_DEFAULT
	closemessage
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	releaseall
	end

BattleFrontier_BattlePyramidLobby_EventScript_LostChallenge::
	frontier_checkairshow
	special LoadPlayerParty
	pyramid_clearhelditems
	special HealPlayerParty
	lockall
	message BattleFrontier_BattlePyramidLobby_Text_DisappointingHereIsBag
	waitmessage
	playse SE_EXP_MAX
	waitse
	message BattleFrontier_BattlePyramidLobby_Text_RecordResultsWait
	waitmessage
	pyramid_set PYRAMID_DATA_WIN_STREAK_ACTIVE, FALSE
	pyramid_save 0
	playse SE_SAVE
	waitse
	msgbox BattleFrontier_BattlePyramidLobby_Text_LookForwardToNextChallenge, MSGBOX_DEFAULT
	closemessage
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	releaseall
	end

BattleFrontier_BattlePyramidLobby_EventScript_ResumeChallenge::
	goto BattleFrontier_BattlePyramidLobby_EventScript_EnterChallenge

BattleFrontier_BattlePyramidLobby_EventScript_Attendant::
	lock
	faceplayer
	setvar VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_PYRAMID
	setvar VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES
	special SavePlayerParty
	msgbox BattleFrontier_BattlePyramidLobby_Text_WelcomeToBattlePyramid, MSGBOX_DEFAULT
BattleFrontier_BattlePyramidLobby_EventScript_AskTakeChallenge::
	message BattleFrontier_BattlePyramidLobby_Text_EmbarkOnChallenge
	waitmessage
	multichoice 17, 6, MULTI_CHALLENGEINFO, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_BattlePyramidLobby_EventScript_TryEnterChallenge
	case 1, BattleFrontier_BattlePyramidLobby_EventScript_ExplainChallenge
	case 2, BattleFrontier_BattlePyramidLobby_EventScript_CancelChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattlePyramidLobby_EventScript_CancelChallenge

BattleFrontier_BattlePyramidLobby_EventScript_TryEnterChallenge::
	message BattleFrontier_BattlePyramidLobby_Text_WhichLevelMode
	waitmessage
	multichoice 17, 6, MULTI_LEVEL_MODE, FALSE
	switch VAR_RESULT
	case FRONTIER_LVL_TENT, BattleFrontier_BattlePyramidLobby_EventScript_CancelChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattlePyramidLobby_EventScript_CancelChallenge
	frontier_checkineligible
	goto_if_eq VAR_0x8004, TRUE, BattleFrontier_BattlePyramidLobby_EventScript_NotEnoughValidMons
	frontier_set FRONTIER_DATA_LVL_MODE, VAR_RESULT
	msgbox BattleFrontier_BattlePyramidLobby_Text_SelectThreeMons, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	call BattleFrontier_EventScript_GetLvlMode
	copyvar VAR_0x8004, VAR_RESULT
	setvar VAR_0x8005, FRONTIER_PARTY_SIZE
	special ChoosePartyForBattleFrontier
	waitstate
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattlePyramidLobby_EventScript_LoadPartyAndCancelChallenge
	msgbox BattleFrontier_BattlePyramidLobby_Text_OkayToSaveBeforeChallenge, MSGBOX_YESNO
	switch VAR_RESULT
	case NO, BattleFrontier_BattlePyramidLobby_EventScript_LoadPartyAndCancelChallenge
	case YES, BattleFrontier_BattlePyramidLobby_EventScript_SaveBeforeChallenge
	case MULTI_B_PRESSED, BattleFrontier_BattlePyramidLobby_EventScript_LoadPartyAndCancelChallenge

BattleFrontier_BattlePyramidLobby_EventScript_SaveBeforeChallenge::
	setvar VAR_TEMP_CHALLENGE_STATUS, 0
	frontier_set FRONTIER_DATA_SELECTED_MON_ORDER
	pyramid_init
	pyramid_set PYRAMID_DATA_WIN_STREAK_ACTIVE, TRUE
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, CHALLENGE_STATUS_SAVING
	frontier_set FRONTIER_DATA_PAUSED, FALSE
	pyramid_seedfloor
	pyramid_settrainers
	special LoadPlayerParty
	closemessage
	delay 2
	call Common_EventScript_SaveGame
	setvar VAR_TEMP_CHALLENGE_STATUS, 255
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattlePyramidLobby_EventScript_CancelChallengeSaveFailed
BattleFrontier_BattlePyramidLobby_EventScript_EnterChallenge::
	special SavePlayerParty
	frontier_setpartyorder FRONTIER_PARTY_SIZE
	msgbox BattleFrontier_BattlePyramidLobby_Text_ShowYouIntoPyramid, MSGBOX_DEFAULT
	closemessage
	call BattleFrontier_BattlePyramidLobby_EventScript_WalkToPanelAndReceiveBag
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	setvar VAR_RESULT, 0
	special HealPlayerParty
	warp MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR, 1, 1
	setvar VAR_TEMP_CHALLENGE_STATUS, 0
	waitstate
	end

BattleFrontier_BattlePyramidLobby_EventScript_ExplainChallenge::
	msgbox BattleFrontier_BattlePyramidLobby_Text_ExplainBattlePyramid, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_AskTakeChallenge

BattleFrontier_BattlePyramidLobby_EventScript_NotEnoughValidMons::
	switch VAR_RESULT
	case FRONTIER_LVL_50, BattleFrontier_BattlePyramidLobby_EventScript_NotEnoughValidMonsLv50
	case FRONTIER_LVL_OPEN, BattleFrontier_BattlePyramidLobby_EventScript_NotEnoughValidMonsLvOpen

BattleFrontier_BattlePyramidLobby_EventScript_NotEnoughValidMonsLv50::
	msgbox BattleFrontier_BattlePyramidLobby_Text_NotEnoughValidMonsLv50, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_EndCancelChallenge

BattleFrontier_BattlePyramidLobby_EventScript_NotEnoughValidMonsLvOpen::
	msgbox BattleFrontier_BattlePyramidLobby_Text_NotEnoughValidMonsLvOpen, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_EndCancelChallenge

BattleFrontier_BattlePyramidLobby_EventScript_CancelChallengeSaveFailed::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	goto BattleFrontier_BattlePyramidLobby_EventScript_CancelChallenge

BattleFrontier_BattlePyramidLobby_EventScript_LoadPartyAndCancelChallenge::
	special LoadPlayerParty
BattleFrontier_BattlePyramidLobby_EventScript_CancelChallenge::
	msgbox BattleFrontier_BattlePyramidLobby_Text_AwaitFutureChallenge, MSGBOX_DEFAULT
BattleFrontier_BattlePyramidLobby_EventScript_EndCancelChallenge::
	release
	end

BattleFrontier_BattlePyramidLobby_EventScript_HintGiver::
	lockall
	applymovement LOCALID_PYRAMID_LOBBY_HINT_GIVER, Common_Movement_FacePlayer
	waitmovement 0
	msgbox BattleFrontier_BattlePyramidLobby_Text_TellYouWhatMisfortunesAwait, MSGBOX_DEFAULT
	call BattleFrontier_BattlePyramidLobby_EventScript_GiveHint
	msgbox BattleFrontier_BattlePyramidLobby_Text_BelieveMyFortunesOrNot, MSGBOX_DEFAULT
	releaseall
	end

BattleFrontier_BattlePyramidLobby_EventScript_GiveHint::
	multichoice 17, 6, MULTI_LEVEL_MODE, FALSE
	switch VAR_RESULT
	case FRONTIER_LVL_50, BattleFrontier_BattlePyramidLobby_EventScript_GiveHintLv50
	case FRONTIER_LVL_OPEN, BattleFrontier_BattlePyramidLobby_EventScript_GiveHintLvOpen
	case FRONTIER_LVL_TENT, BattleFrontier_BattlePyramidLobby_EventScript_NoHint
	case MULTI_B_PRESSED, BattleFrontier_BattlePyramidLobby_EventScript_NoHint
	return

/* Shouldnt occur*/
BattleFrontier_BattlePyramidLobby_EventScript_NoHint::
	return

BattleFrontier_BattlePyramidLobby_EventScript_GiveHintLv50::
	msgbox BattleFrontier_BattlePyramidLobby_Text_Aah, MSGBOX_DEFAULT
	pyramid_get PYRAMID_DATA_WIN_STREAK_ACTIVE_50
	goto_if_ne VAR_RESULT, FALSE, BattleFrontier_BattlePyramidLobby_EventScript_GiveHintGetLv50Streak
	setvar VAR_RESULT, 0
	goto BattleFrontier_BattlePyramidLobby_EventScript_DoHintComment
	return

BattleFrontier_BattlePyramidLobby_EventScript_GiveHintGetLv50Streak::
	pyramid_get PYRAMID_DATA_WIN_STREAK_50
	goto BattleFrontier_BattlePyramidLobby_EventScript_DoHintComment
	return

BattleFrontier_BattlePyramidLobby_EventScript_GiveHintLvOpen::
	msgbox BattleFrontier_BattlePyramidLobby_Text_Aah, MSGBOX_DEFAULT
	pyramid_get PYRAMID_DATA_WIN_STREAK_ACTIVE_OPEN
	goto_if_ne VAR_RESULT, FALSE, BattleFrontier_BattlePyramidLobby_EventScript_GiveHintGetLvOpenStreak
	setvar VAR_RESULT, 0
	goto BattleFrontier_BattlePyramidLobby_EventScript_DoHintComment
	return

BattleFrontier_BattlePyramidLobby_EventScript_GiveHintGetLvOpenStreak::
	pyramid_get PYRAMID_DATA_WIN_STREAK_OPEN
	goto BattleFrontier_BattlePyramidLobby_EventScript_DoHintComment
	return

BattleFrontier_BattlePyramidLobby_EventScript_DoHintComment::
	copyvar VAR_0x8004, VAR_RESULT
	special GetBattlePyramidHint
	switch VAR_RESULT
	case 0, BattleFrontier_BattlePyramidLobby_EventScript_HintParalysis
	case 1, BattleFrontier_BattlePyramidLobby_EventScript_HintPoison
	case 2, BattleFrontier_BattlePyramidLobby_EventScript_HintBurn
	case 3, BattleFrontier_BattlePyramidLobby_EventScript_HintPPWaste
	case 4, BattleFrontier_BattlePyramidLobby_EventScript_HintLevitate
	case 5, BattleFrontier_BattlePyramidLobby_EventScript_HintTrapAbility
	case 6, BattleFrontier_BattlePyramidLobby_EventScript_HintIce
	case 7, BattleFrontier_BattlePyramidLobby_EventScript_HintExplosion
	case 8, BattleFrontier_BattlePyramidLobby_EventScript_HintPsychic
	case 9, BattleFrontier_BattlePyramidLobby_EventScript_HintRock
	case 10, BattleFrontier_BattlePyramidLobby_EventScript_HintFighting
	case 11, BattleFrontier_BattlePyramidLobby_EventScript_HintWeather
	case 12, BattleFrontier_BattlePyramidLobby_EventScript_HintBug
	case 13, BattleFrontier_BattlePyramidLobby_EventScript_HintDark
	case 14, BattleFrontier_BattlePyramidLobby_EventScript_HintWater
	case 15, BattleFrontier_BattlePyramidLobby_EventScript_HintGhost
	case 16, BattleFrontier_BattlePyramidLobby_EventScript_HintSteel
	case 17, BattleFrontier_BattlePyramidLobby_EventScript_HintFlyingDragon
	case 18, BattleFrontier_BattlePyramidLobby_EventScript_HintStoneEvolve
	case 19, BattleFrontier_BattlePyramidLobby_EventScript_HintNormal
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintParalysis::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintParalysis, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintPoison::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintPoison, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintBurn::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintBurn, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintPPWaste::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintPPWaste, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintLevitate::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintLevitate, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintTrapAbility::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintTrapAbility, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintIce::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintIce, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintExplosion::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintExplosion, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintPsychic::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintPsychic, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintRock::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintRock, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintFighting::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintFighting, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintWeather::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintWeather, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintBug::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintBug, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintDark::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintDark, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintWater::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintWater, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintGhost::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintGhost, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintSteel::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintSteel, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintFlyingDragon::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintFlyingDragon, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintStoneEvolve::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintStoneEvolve, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_HintNormal::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HintNormal, MSGBOX_DEFAULT
	return

BattleFrontier_BattlePyramidLobby_EventScript_ShowResults::
	lockall
	frontier_results FRONTIER_FACILITY_PYRAMID
	waitbuttonpress
	special RemoveRecordsWindow
	releaseall
	end

BattleFrontier_BattlePyramidLobby_EventScript_WalkToPanelAndReceiveBag::
	applymovement LOCALID_PYRAMID_LOBBY_ATTENDANT, BattleFrontier_BattlePyramidLobby_Movement_AttendantWalkToPanel
	applymovement LOCALID_PLAYER, BattleFrontier_BattlePyramidLobby_Movement_PlayerWalkToPanel
	waitmovement 0
	msgbox BattleFrontier_BattlePyramidLobby_Text_WeWillHoldBagForSafekeeping, MSGBOX_DEFAULT
	pyramid_get PYRAMID_DATA_WIN_STREAK
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattlePyramidLobby_EventScript_ReceiveNewBattleBag
	msgbox BattleFrontier_BattlePyramidLobby_Text_PleaseTakePreviousBattleBag, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_ReceiveBattleBag

BattleFrontier_BattlePyramidLobby_EventScript_ReceiveNewBattleBag::
	msgbox BattleFrontier_BattlePyramidLobby_Text_PleaseTakeThisBattleBag, MSGBOX_DEFAULT
BattleFrontier_BattlePyramidLobby_EventScript_ReceiveBattleBag::
	message BattleFrontier_BattlePyramidLobby_Text_ExchangedBagForBattleBag
	waitmessage
	playse SE_EXP_MAX
	waitse
	msgbox BattleFrontier_BattlePyramidLobby_Text_StepOnFloorPanel, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_PYRAMID_LOBBY_ATTENDANT, BattleFrontier_BattlePyramidLobby_Movement_AttendantMoveAside
	waitmovement 0
	applymovement LOCALID_PLAYER, BattleFrontier_BattlePyramidLobby_Movement_PlayerStepOnPanel
	waitmovement 0
	return

BattleFrontier_BattlePyramidLobby_Movement_AttendantWalkToPanel:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	face_down
	step_end

BattleFrontier_BattlePyramidLobby_Movement_PlayerWalkToPanel:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

BattleFrontier_BattlePyramidLobby_Movement_AttendantMoveAside:
	walk_right
	face_left
	step_end

BattleFrontier_BattlePyramidLobby_Movement_PlayerStepOnPanel:
	walk_up
	walk_up
	step_end

BattleFrontier_BattlePyramidLobby_EventScript_StoreHeldItemsInPyramidBag::
	msgbox BattleFrontier_BattlePyramidLobby_Text_MonHoldingItemCannotTake, MSGBOX_DEFAULT
	setflag FLAG_STORING_ITEMS_IN_PYRAMID_BAG
	special TryStoreHeldItemsInPyramidBag
	goto_if_eq VAR_RESULT, 0, BattleFrontier_BattlePyramidLobby_EventScript_HeldItemsStoredInPyramidBag
	message BattleFrontier_BattlePyramidLobby_Text_BagCannotHoldPickItemsToKeep
	waitmessage
	goto BattleFrontier_BattlePyramidLobby_EventScript_PickItemsToKeep
	end

BattleFrontier_BattlePyramidLobby_EventScript_HeldItemsStoredInPyramidBag::
	msgbox BattleFrontier_BattlePyramidLobby_Text_HeldItemsMovedToBag, MSGBOX_DEFAULT
	return

/* When exiting Battle Pyramid with a full pyramid bag and held items the player must select to keep/toss party held items and make room for any kept items by tossing from the pyramid bag*/
BattleFrontier_BattlePyramidLobby_EventScript_PickItemsToKeep::
	multichoice 17, 6, MULTI_FRONTIER_ITEM_CHOOSE, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_BattlePyramidLobby_EventScript_PickItemsFromBag
	case 1, BattleFrontier_BattlePyramidLobby_EventScript_PickItemsFromParty
	case 2, BattleFrontier_BattlePyramidLobby_EventScript_ExitPickItems
	case MULTI_B_PRESSED, BattleFrontier_BattlePyramidLobby_EventScript_ExitPickItems
	end

BattleFrontier_BattlePyramidLobby_EventScript_PickItemsFromBag::
	special ChooseItemsToTossFromPyramidBag
	waitstate
	message BattleFrontier_BattlePyramidLobby_Text_PickItemsToKeep
	waitmessage
	goto BattleFrontier_BattlePyramidLobby_EventScript_PickItemsToKeep
	end

BattleFrontier_BattlePyramidLobby_EventScript_PickItemsFromParty::
	special BattlePyramidChooseMonHeldItems
	waitstate
	message BattleFrontier_BattlePyramidLobby_Text_PickItemsToKeep
	waitmessage
	goto BattleFrontier_BattlePyramidLobby_EventScript_PickItemsToKeep
	end

BattleFrontier_BattlePyramidLobby_EventScript_ExitPickItems::
	special DoBattlePyramidMonsHaveHeldItem
	goto_if_eq VAR_RESULT, TRUE, BattleFrontier_BattlePyramidLobby_EventScript_PartyStillHasHeldItems
	return

BattleFrontier_BattlePyramidLobby_EventScript_PartyStillHasHeldItems::
	msgbox BattleFrontier_BattlePyramidLobby_Text_LeastOneMonHoldingItem, MSGBOX_DEFAULT
	message BattleFrontier_BattlePyramidLobby_Text_PickItemsToKeep
	waitmessage
	goto BattleFrontier_BattlePyramidLobby_EventScript_PickItemsToKeep
	end

BattleFrontier_BattlePyramidLobby_EventScript_Woman::
	msgbox BattleFrontier_BattlePyramidLobby_Text_TrainersNoticeRunning, MSGBOX_NPC
	end

BattleFrontier_BattlePyramidLobby_EventScript_FatMan::
	msgbox BattleFrontier_BattlePyramidLobby_Text_LostLotOfItems, MSGBOX_NPC
	end

BattleFrontier_BattlePyramidLobby_EventScript_RulesBoard::
	lockall
	msgbox BattleFrontier_BattlePyramidLobby_Text_RulesAreListed, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattlePyramidLobby_EventScript_ReadRulesBoard::
	message BattleFrontier_BattlePyramidLobby_Text_ReadWhichHeading
	waitmessage
	multichoice 15, 2, MULTI_BATTLE_PYRAMID_RULES, FALSE
	switch VAR_RESULT
	case 0, BattleFrontier_BattlePyramidLobby_EventScript_RulesPokemon
	case 1, BattleFrontier_BattlePyramidLobby_EventScript_RulesTrainers
	case 2, BattleFrontier_BattlePyramidLobby_EventScript_RulesMaze
	case 3, BattleFrontier_BattlePyramidLobby_EventScript_RulesBag
	case 4, BattleFrontier_BattlePyramidLobby_EventScript_ExitRules
	case MULTI_B_PRESSED, BattleFrontier_BattlePyramidLobby_EventScript_ExitRules
	end

BattleFrontier_BattlePyramidLobby_EventScript_RulesPokemon::
	msgbox BattleFrontier_BattlePyramidLobby_Text_ExplainMonRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattlePyramidLobby_EventScript_RulesTrainers::
	msgbox BattleFrontier_BattlePyramidLobby_Text_ExplainTrainerRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattlePyramidLobby_EventScript_RulesMaze::
	msgbox BattleFrontier_BattlePyramidLobby_Text_ExplainMazeRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattlePyramidLobby_EventScript_RulesBag::
	msgbox BattleFrontier_BattlePyramidLobby_Text_ExplainBagRules, MSGBOX_DEFAULT
	goto BattleFrontier_BattlePyramidLobby_EventScript_ReadRulesBoard
	end

BattleFrontier_BattlePyramidLobby_EventScript_ExitRules::
	releaseall
	end

BattleFrontier_BattlePyramidLobby_Text_WelcomeToBattlePyramid:
	.string "Onde a coragem dos TREINADORES\n"
	.string "é posta à prova!\p"
	.string "Boas-vindas à PIRÂMIDE DE BATALHA!\p"
	.string "Sou o guia da\n"
	.string "Missão de Batalha.$"

BattleFrontier_BattlePyramidLobby_Text_EmbarkOnChallenge:
	.string "Tem coragem de embarcar no desafio\n"
	.string "da Missão de Batalha?$"

BattleFrontier_BattlePyramidLobby_Text_AwaitFutureChallenge:
	.string "Aguardamos o seu desafio no futuro!$"

BattleFrontier_BattlePyramidLobby_Text_ExplainBattlePyramid:
	.string "A Missão de Batalha é um evento em\n"
	.string "que você explora a PIRÂMIDE e tenta\l"
	.string "chegar ao topo.\p"
	.string "Na PIRÂMIDE há sete andares de\n"
	.string "labirintos, cheios de TREINADORES\l"
	.string "hábeis e também POKéMON selvagens.\p"
	.string "Você entra com três POKéMON, sem\n"
	.string "nenhum deles segurar item.\p"
	.string "Na subida, você recebe uma BOLSA\n"
	.string "DE BATALHA pros itens.\p"
	.string "Se falhar no desafio, todo item da\n"
	.string "BOLSA DE BATALHA desaparece.\p"
	.string "Se quiser interromper o desafio,\n"
	.string "escolha “PAUSAR” e salve o jogo.\p"
	.string "Se não salvar antes de interromper,\n"
	.string "o desafio será anulado.$"

BattleFrontier_BattlePyramidLobby_Text_WhichLevelMode:
	.string "A PIRÂMIDE tem duas modalidades,\n"
	.string "Nível 50 e Nível Livre.\l"
	.string "Em qual vai entrar?$"

BattleFrontier_BattlePyramidLobby_Text_SelectThreeMons:
	.string "Muito bem. Agora escolha os três\n"
	.string "POKéMON que vão te acompanhar.$"

BattleFrontier_BattlePyramidLobby_Text_NotEnoughValidMonsLvOpen:
	.string "Um probleminha!\p"
	.string "Parece que você não tem os três\n"
	.string "POKéMON aptos pro desafio.\p"
	.string "Lembre também de tirar todos os\n"
	.string "itens dos seus POKéMON.\p"
	.string "OVOS{STR_VAR_1} proibidos.\p"
	.string "Quando estiver tudo pronto, venha\n"
	.string "falar comigo.$"

BattleFrontier_BattlePyramidLobby_Text_NotEnoughValidMonsLv50:
	.string "Um probleminha!\p"
	.string "Parece que você não tem os três\n"
	.string "POKéMON aptos pro desafio.\p"
	.string "Os três POKéMON precisam ser\n"
	.string "diferentes e de nível 50 ou menos.\p"
	.string "Lembre também de tirar todos os\n"
	.string "itens dos seus POKéMON.\p"
	.string "OVOS{STR_VAR_1} proibidos.\p"
	.string "Quando estiver tudo pronto, venha\n"
	.string "falar comigo.$"

BattleFrontier_BattlePyramidLobby_Text_OkayToSaveBeforeChallenge:
	.string "Antes de entrar na PIRÂMIDE DE\n"
	.string "BATALHA, o jogo precisa ser salvo.\l"
	.string "Tudo bem?$"

BattleFrontier_BattlePyramidLobby_Text_ShowYouIntoPyramid:
	.string "Muito bem. Agora vou te levar pra\n"
	.string "PIRÂMIDE DE BATALHA.$"

BattleFrontier_BattlePyramidLobby_Text_WeWillHoldBagForSafekeeping:
	.string "Vamos guardar a sua BOLSA, {PLAYER},\n"
	.string "enquanto você explora.$"

BattleFrontier_BattlePyramidLobby_Text_PleaseTakePreviousBattleBag:
	.string "Em troca, leve esta BOLSA DE\n"
	.string "BATALHA, a que você usou antes.$"

BattleFrontier_BattlePyramidLobby_Text_PleaseTakeThisBattleBag:
	.string "Em troca, leve esta\n"
	.string "BOLSA DE BATALHA.$"

BattleFrontier_BattlePyramidLobby_Text_ExchangedBagForBattleBag:
	.string "{PLAYER} trocou a BOLSA pela\n"
	.string "BOLSA DE BATALHA.$"

BattleFrontier_BattlePyramidLobby_Text_StepOnFloorPanel:
	.string "Ao pisar neste painel, você vai\n"
	.string "pra um andar mais alto da PIRÂMIDE.\p"
	.string "Espero que a sua missão corra\n"
	.string "em segurança!$"

BattleFrontier_BattlePyramidLobby_Text_DidntSaveBeforeQuittingTakeBag:
	.string "Um grande problema!\p"
	.string "Você não salvou antes de encerrar\n"
	.string "o desafio da última vez.\p"
	.string "Por isso, sinto dizer, o seu desafio\n"
	.string "foi anulado.\p"
	.string "Aqui está a BOLSA que guardamos\n"
	.string "pra você.\p"
	.string "{PLAYER} pegou a BOLSA de volta.$"

BattleFrontier_BattlePyramidLobby_Text_YouveConqueredPyramid:
	.string "Que ótimo ver você de volta!\p"
	.string "Você conquistou a PIRÂMIDE!\n"
	.string "Esplêndido!$"

BattleFrontier_BattlePyramidLobby_Text_MonHoldingItemCannotTake:
	.string "Ah, um probleminha.\p"
	.string "Pelo menos um POKéMON está\n"
	.string "segurando um item.\p"
	.string "Sinto dizer, mas itens obtidos na\n"
	.string "PIRÂMIDE não podem sair dela.$"

BattleFrontier_BattlePyramidLobby_Text_HeldItemsMovedToBag:
	.string "Todos os itens dos seus POKéMON vão\n"
	.string "pra BOLSA DE BATALHA, {PLAYER}.$"

BattleFrontier_BattlePyramidLobby_Text_BagCannotHoldPickItemsToKeep:
	.string "Sinto dizer, mas a BOLSA DE BATALHA\n"
	.string "não comporta todos os seus itens.\p"
	.string "Escolha os itens que vão ficar na\n"
	.string "BOLSA DE BATALHA e com os POKéMON.$"

BattleFrontier_BattlePyramidLobby_Text_LeastOneMonHoldingItem:
	.string "Pelo menos um POKéMON ainda está\n"
	.string "segurando um item.$"

BattleFrontier_BattlePyramidLobby_Text_PickItemsToKeep:
	.string "Escolha os itens que vão ficar na\n"
	.string "BOLSA DE BATALHA e com os POKéMON.$"

/* Unused*/
BattleFrontier_BattlePyramidLobby_Text_ReturnedEverythingMonsHeld:
	.string "{PLAYER} devolveu tudo o que os\n"
	.string "POKéMON seguravam.$"

BattleFrontier_BattlePyramidLobby_Text_UsedBattleBagWillBeKept:
	.string "A BOLSA DE BATALHA que você usou\n"
	.string "fica guardada pro próximo desafio.\p"
	.string "{PLAYER} entregou a BOLSA DE BATALHA\n"
	.string "e recebeu a BOLSA de volta.$"

BattleFrontier_BattlePyramidLobby_Text_RecordResultsWait:
	.string "Preciso registrar os seus resultados.\n"
	.string "Aguarde, por favor.$"

/* Unused*/
BattleFrontier_BattlePyramidLobby_Text_ForConqueringPyramidTakeThis:
	.string "De lembrança por conquistar a\n"
	.string "PIRÂMIDE DE BATALHA, leve isto.$"

/* Unused*/
BattleFrontier_BattlePyramidLobby_Text_ReceivedPrizeItem:
	.string "{PLAYER} recebeu o prêmio:\n"
	.string "{STR_VAR_1}.$"

/* Unused*/
BattleFrontier_BattlePyramidLobby_Text_BagIsFull:
	.string "…Ah…\n"
	.string "A sua BOLSA parece estar cheia.\p"
	.string "Volte depois de arrumar o que tem\n"
	.string "na BOLSA.$"

BattleFrontier_BattlePyramidLobby_Text_DisappointingHereIsBag:
	.string "Que decepção pra você…\p"
	.string "Aqui está a BOLSA que guardamos\n"
	.string "pra você.\p"
	.string "{PLAYER} pegou a BOLSA de volta.$"

BattleFrontier_BattlePyramidLobby_Text_LookForwardToNextChallenge:
	.string "Aguardamos o seu próximo\n"
	.string "desafio!$"

/* Unused*/
BattleFrontier_BattlePyramidLobby_Text_HereIsPrize:
	.string "Estávamos esperando a sua chegada!\p"
	.string "Aqui está o seu prêmio por\n"
	.string "conquistar a PIRÂMIDE.$"

BattleFrontier_BattlePyramidLobby_Text_TellYouWhatMisfortunesAwait:
	.string "Boas-vindas…\p"
	.string "Vou te contar com prazer que\n"
	.string "desgraças esperam na PIRÂMIDE…$"

BattleFrontier_BattlePyramidLobby_Text_Aah:
	.string "… … … … … …\n"
	.string "… … … … … …\p"
	.string "… … … … … …\n"
	.string "Aah!$"

BattleFrontier_BattlePyramidLobby_Text_HintParalysis:
	.string "Vejo uma chuva de faíscas…\p"
	.string "…E nela vejo os seus POKéMON\n"
	.string "lutando contra a paralisia…$"

BattleFrontier_BattlePyramidLobby_Text_HintPoison:
	.string "Vejo veneno…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com os efeitos do veneno…$"

BattleFrontier_BattlePyramidLobby_Text_HintBurn:
	.string "Vejo chamas vermelhas e vivas…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com queimaduras…$"

BattleFrontier_BattlePyramidLobby_Text_HintPPWaste:
	.string "Sinto a pressão terrível de uma\n"
	.string "raiva não correspondida…\p"
	.string "É uma maldição…\p"
	.string "…E vejo os seus POKéMON sem Pontos\n"
	.string "de Poder, sem outra saída a não ser\l"
	.string "usar INSISTÊNCIA…$"

BattleFrontier_BattlePyramidLobby_Text_HintLevitate:
	.string "Vejo POKéMON pairando no alto…\p"
	.string "…E vejo os seus POKéMON frustrados\n"
	.string "com golpes TERRESTRES sem efeito…$"

BattleFrontier_BattlePyramidLobby_Text_HintTrapAbility:
	.string "Sinto uma energia tremenda subindo\n"
	.string "do chão lá embaixo…\p"
	.string "…E vejo os seus POKéMON sem conseguir\n"
	.string "escapar das garras desse poder…$"

BattleFrontier_BattlePyramidLobby_Text_HintIce:
	.string "Vejo POKéMON de GELO…\p"
	.string "…E vejo os seus POKéMON enfrentando\n"
	.string "o congelamento do gelo…$"

BattleFrontier_BattlePyramidLobby_Text_HintExplosion:
	.string "Vejo uma rajada de golpes que põem\n"
	.string "em perigo quem os usa…\p"
	.string "…E vejo os seus POKéMON caindo\n"
	.string "diante deles…$"

BattleFrontier_BattlePyramidLobby_Text_HintPsychic:
	.string "Vejo POKéMON PSÍQUICOS…\p"
	.string "…E vejo os seus POKéMON atormentados\n"
	.string "por golpes PSÍQUICOS…$"

BattleFrontier_BattlePyramidLobby_Text_HintRock:
	.string "Vejo POKéMON de PEDRA…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com golpes de PEDRA…$"

BattleFrontier_BattlePyramidLobby_Text_HintFighting:
	.string "Vejo POKéMON LUTADORES…\p"
	.string "…E vejo os seus POKéMON surrados\n"
	.string "por golpes de LUTADOR…$"

BattleFrontier_BattlePyramidLobby_Text_HintWeather:
	.string "DANÇA CHUVA… DIA DE SOL…\n"
	.string "TEMPESTADE DE AREIA… GRANIZO…\p"
	.string "Vejo POKéMON que ficam mais fortes\n"
	.string "com o clima…\p"
	.string "…E vejo os seus POKéMON confusos\n"
	.string "com golpes de vários tipos…$"

BattleFrontier_BattlePyramidLobby_Text_HintBug:
	.string "Vejo POKéMON INSETO…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com vários tipos de ataque…$"

BattleFrontier_BattlePyramidLobby_Text_HintDark:
	.string "Vejo POKéMON SOMBRIOS…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com golpes SOMBRIOS…$"

BattleFrontier_BattlePyramidLobby_Text_HintWater:
	.string "Vejo POKéMON de ÁGUA…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com golpes de ÁGUA…$"

BattleFrontier_BattlePyramidLobby_Text_HintGhost:
	.string "Vejo POKéMON FANTASMA…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com golpes FANTASMA…$"

BattleFrontier_BattlePyramidLobby_Text_HintSteel:
	.string "Vejo POKéMON de AÇO…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com golpes poderosíssimos…$"

BattleFrontier_BattlePyramidLobby_Text_HintFlyingDragon:
	.string "Vejo POKéMON voadores…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com golpes poderosíssimos…$"

BattleFrontier_BattlePyramidLobby_Text_HintStoneEvolve:
	.string "Vejo os que evoluíram com o poder\n"
	.string "das pedras…\p"
	.string "Também sinto fogo, água e\n"
	.string "eletricidade…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com esses três poderes…$"

BattleFrontier_BattlePyramidLobby_Text_HintNormal:
	.string "Vejo POKéMON NORMAIS…\p"
	.string "…E vejo os seus POKéMON sofrendo\n"
	.string "com golpes poderosíssimos…$"

BattleFrontier_BattlePyramidLobby_Text_BelieveMyFortunesOrNot:
	.string "Acreditar ou não nas minhas\n"
	.string "previsões é escolha sua…\p"
	.string "O futuro pode mudar a qualquer hora…\n"
	.string "Desejo uma travessia segura…$"

BattleFrontier_BattlePyramidLobby_Text_TrainersNoticeRunning:
	.string "Sabia?\p"
	.string "Se você correr, os TREINADORES podem\n"
	.string "notar e vir atrás de uma batalha.\p"
	.string "Então, se quiser evitar TREINADORES,\n"
	.string "não chame atenção: passe por eles\l"
	.string "com cuidado e em silêncio.$"

BattleFrontier_BattlePyramidLobby_Text_LostLotOfItems:
	.string "Aaaaaaaaargh!\p"
	.string "Eu tinha um monte de itens, mas\n"
	.string "perdi todos quando perdi!\p"
	.string "Aaaaaaaaargh!$"

BattleFrontier_BattlePyramidLobby_Text_YouveDefeatedPyramidKing:
	.string "Boas-vindas de volta!\n"
	.string "Você fez o impensável!\p"
	.string "Venceu o REI DA PIRÂMIDE e\n"
	.string "conquistou a PIRÂMIDE DE BATALHA!$"

BattleFrontier_BattlePyramidLobby_Text_GiveYouTheseBattlePoints:
	.string "Jovem!\n"
	.string "Pela sua coragem, aqui estão\l"
	.string "os seus Pontos de Batalha!$"

BattleFrontier_BattlePyramidLobby_Text_RulesAreListed:
	.string "Aqui estão as regras da Missão\n"
	.string "de Batalha.$"

BattleFrontier_BattlePyramidLobby_Text_ReadWhichHeading:
	.string "Qual tópico você quer ler?$"

BattleFrontier_BattlePyramidLobby_Text_ExplainMonRules:
	.string "Quando a PIRÂMIDE é conquistada, os\n"
	.string "POKéMON selvagens dela trocam por\l"
	.string "outras espécies.\p"
	.string "Explore, observe e descubra que\n"
	.string "POKéMON selvagens pode encontrar.$"

BattleFrontier_BattlePyramidLobby_Text_ExplainTrainerRules:
	.string "TREINADORES esperam você dentro\n"
	.string "da PIRÂMIDE.\p"
	.string "Em cada andar, há até oito\n"
	.string "TREINADORES.\p"
	.string "Ao vencer um TREINADOR, você ganha\n"
	.string "uma dica útil pra aventura.$"

BattleFrontier_BattlePyramidLobby_Text_ExplainMazeRules:
	.string "Os labirintos da PIRÂMIDE mudam toda\n"
	.string "vez que você entra.\p"
	.string "Os labirintos são mal iluminados.\n"
	.string "Avance com cuidado.\p"
	.string "A luz fica mais forte sempre que você\n"
	.string "vence um POKéMON selvagem ou um\l"
	.string "TREINADOR.$"

BattleFrontier_BattlePyramidLobby_Text_ExplainBagRules:
	.string "A BOLSA DE BATALHA serve de BOLSA\n"
	.string "dentro da PIRÂMIDE.\p"
	.string "Há duas BOLSAS DE BATALHA: uma pro\n"
	.string "Nível 50 e outra pro Nível Livre.\p"
	.string "Cada uma guarda até 99 unidades de\n"
	.string "dez tipos de itens.\p"
	.string "O conteúdo da BOLSA DE BATALHA se\n"
	.string "perde se você falhar na missão.$"

