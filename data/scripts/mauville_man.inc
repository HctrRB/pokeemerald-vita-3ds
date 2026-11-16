MauvilleCity_PokemonCenter_1F_EventScript_MauvilleOldMan::
	special Script_GetCurrentMauvilleMan
	switch VAR_RESULT
	case MAUVILLE_MAN_BARD, MauvilleCity_PokemonCenter_1F_EventScript_Bard
	case MAUVILLE_MAN_HIPSTER, MauvilleCity_PokemonCenter_1F_EventScript_Hipster
	case MAUVILLE_MAN_TRADER, MauvilleCity_PokemonCenter_1F_EventScript_Trader
	case MAUVILLE_MAN_STORYTELLER, MauvilleCity_PokemonCenter_1F_EventScript_Storyteller
	case MAUVILLE_MAN_GIDDY, MauvilleCity_PokemonCenter_1F_EventScript_Giddy
	end

/* Bard*/
MauvilleCity_PokemonCenter_1F_EventScript_Bard::
	lock
	faceplayer
	msgbox MauvilleCity_PokemonCenter_1F_Text_WouldYouLikeToHearMySong, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, MauvilleCity_PokemonCenter_1F_EventScript_PlaySong
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_DeclineSong
	end

MauvilleCity_PokemonCenter_1F_EventScript_PlaySong::
	setvar VAR_0x8004, FALSE  @ Play his current song
	special PlayBardSong
	delay 60
	special HasBardSongBeenChanged
	goto_if_eq VAR_RESULT, FALSE, MauvilleCity_PokemonCenter_1F_EventScript_AskToWriteLyrics
	msgbox MauvilleCity_PokemonCenter_1F_Text_WishICouldPlaySongForOthers, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_DeclineSong::
	msgbox MauvilleCity_PokemonCenter_1F_Text_BardFeelingTheBlues1, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_AskToWriteLyrics::
	msgbox MauvilleCity_PokemonCenter_1F_Text_WouldYouLikeToWriteSomeLyrics, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, MauvilleCity_PokemonCenter_1F_EventScript_WriteLyrics
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_DeclineWritingLyrics
	end

MauvilleCity_PokemonCenter_1F_EventScript_WriteLyrics::
	setvar VAR_0x8004, EASY_CHAT_TYPE_BARD_SONG
	call Common_ShowEasyChatScreen
	lock
	faceplayer
	goto_if_eq VAR_RESULT, 0, MauvilleCity_PokemonCenter_1F_EventScript_DeclineWritingLyrics
	msgbox MauvilleCity_PokemonCenter_1F_Text_LetMeSingItForYou, MSGBOX_DEFAULT
	setvar VAR_0x8004, TRUE  @ Play the new song
	special PlayBardSong
	delay 60
	msgbox MauvilleCity_PokemonCenter_1F_Text_ThatHowYouWantedSongToGo, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_WriteLyrics
	special SaveBardSongLyrics
	msgbox MauvilleCity_PokemonCenter_1F_Text_IllSingThisSongForAWhile, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_DeclineWritingLyrics::
	msgbox MauvilleCity_PokemonCenter_1F_Text_BardFeelingTheBlues2, MSGBOX_DEFAULT
	release
	end

/* Hipster*/
MauvilleCity_PokemonCenter_1F_EventScript_Hipster::
	lock
	faceplayer
	setflag FLAG_UNLOCKED_TRENDY_SAYINGS
	msgbox MauvilleCity_PokemonCenter_1F_Text_TeachWhatsHipAndHappening, MSGBOX_DEFAULT
	special HasHipsterTaughtWord
	goto_if_eq VAR_RESULT, FALSE, MauvilleCity_PokemonCenter_1F_EventScript_TryTeachWord
	msgbox MauvilleCity_PokemonCenter_1F_Text_IAlreadyTaughtYou, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_TryTeachWord::
	special HipsterTryTeachWord
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_PokemonCenter_1F_EventScript_TeachWord
	msgbox MauvilleCity_PokemonCenter_1F_Text_IveGotNothingNewToTeach, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_TeachWord::
	msgbox MauvilleCity_PokemonCenter_1F_Text_HaveYouHeardOfWord, MSGBOX_DEFAULT
	special SetHipsterTaughtWord
	release
	end

/* Trader*/
MauvilleCity_PokemonCenter_1F_Text_WantToTradeDecor:
	.string "Oi, eu sou o NEGOCIANTE.\n"
	.string "Quer trocar decorações comigo?$"

MauvilleCity_PokemonCenter_1F_Text_TraderFeelingTheBlues:
	.string "Ah…\n"
	.string "Você me deixou na fossa…$"

MauvilleCity_PokemonCenter_1F_Text_WeveAlreadyTraded:
	.string "Mas a gente já trocou decorações,\n"
	.string "você e eu.$"

MauvilleCity_PokemonCenter_1F_Text_PickADecorItem:
	.string "Se gostar de alguma decoração\n"
	.string "minha, é só falar.$"

MauvilleCity_PokemonCenter_1F_Text_YouDontWantAnything:
	.string "Não quer nada?\n"
	.string "Me sinto rejeitado…$"

MauvilleCity_PokemonCenter_1F_Text_OnceBelongedToPlayerDoYouWantIt:
	.string "Essa decoração já foi de\n"
	.string "{STR_VAR_1}.\p"
	.string "Você quer?$"

MauvilleCity_PokemonCenter_1F_Text_YouDontHaveAnyDecor:
	.string "Hã… Peraí. Você não tem nem\n"
	.string "uma decoração!$"

MauvilleCity_PokemonCenter_1F_Text_PickTheDecorToTrade:
	.string "Certo, escolha a decoração que\n"
	.string "vai trocar comigo.$"

MauvilleCity_PokemonCenter_1F_Text_YouDontWantToTrade:
	.string "Não vai trocar comigo?\n"
	.string "Me sinto rejeitado…$"

MauvilleCity_PokemonCenter_1F_Text_YouveNoRoomForThis:
	.string "Você já tem o máximo de {STR_VAR_2}S\n"
	.string "que cabe. Não tem espaço pra isto.$"

MauvilleCity_PokemonCenter_1F_Text_SoWellTradeTheseDecor:
	.string "Certo, então eu troco {STR_VAR_3}\n"
	.string "por {STR_VAR_2}?$"

MauvilleCity_PokemonCenter_1F_Text_ThatDecorIsInUse:
	.string "Essa decoração tá em uso.\n"
	.string "Não dá pra trocar.$"

MauvilleCity_PokemonCenter_1F_Text_SendDecorToYourPC:
	.string "Então vamos trocar!\n"
	.string "Vou mandar a decoração pro seu PC.$"

MauvilleCity_PokemonCenter_1F_Text_CantTradeThatOne:
	.string "Opa! Foi mal! Essa decoração\n"
	.string "é raríssima.\l"
	.string "Essa eu não posso trocar!\p"
	.string "Quer alguma outra coisa?$"

MauvilleCity_PokemonCenter_1F_EventScript_Trader::
	lock
	faceplayer
	msgbox MauvilleCity_PokemonCenter_1F_Text_WantToTradeDecor, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_DeclineTrade
	special GetTraderTradedFlag
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_PokemonCenter_1F_EventScript_AlreadyTraded
	message MauvilleCity_PokemonCenter_1F_Text_PickADecorItem
	waitmessage
	goto MauvilleCity_PokemonCenter_1F_EventScript_PickDecorToReceive
	end

MauvilleCity_PokemonCenter_1F_EventScript_DeclineTrade::
	msgbox MauvilleCity_PokemonCenter_1F_Text_TraderFeelingTheBlues, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_AlreadyTraded::
	msgbox MauvilleCity_PokemonCenter_1F_Text_WeveAlreadyTraded, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_PickDecorToReceive::
	special TraderMenuGetDecoration
	waitstate
	goto_if_eq VAR_0x8004, 0, MauvilleCity_PokemonCenter_1F_EventScript_CancelPickDecor
	goto_if_eq VAR_0x8004, 0xFFFF, MauvilleCity_PokemonCenter_1F_EventScript_InvalidDecor
	msgbox MauvilleCity_PokemonCenter_1F_Text_OnceBelongedToPlayerDoYouWantIt, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_PickDifferentDecor
	special DoesPlayerHaveNoDecorations
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_PokemonCenter_1F_EventScript_DontHaveAnyDecor
	goto MauvilleCity_PokemonCenter_1F_EventScript_PickDecorToGive
	end

MauvilleCity_PokemonCenter_1F_EventScript_CancelPickDecor::
	msgbox MauvilleCity_PokemonCenter_1F_Text_YouDontWantAnything, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_InvalidDecor::
	message MauvilleCity_PokemonCenter_1F_Text_CantTradeThatOne
	waitmessage
	goto MauvilleCity_PokemonCenter_1F_EventScript_PickDecorToReceive
	end

MauvilleCity_PokemonCenter_1F_EventScript_PickDifferentDecor::
	message MauvilleCity_PokemonCenter_1F_Text_PickADecorItem
	waitmessage
	goto MauvilleCity_PokemonCenter_1F_EventScript_PickDecorToReceive
	end

MauvilleCity_PokemonCenter_1F_EventScript_DontHaveAnyDecor::
	msgbox MauvilleCity_PokemonCenter_1F_Text_YouDontHaveAnyDecor, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_PickDecorToGive::
	msgbox MauvilleCity_PokemonCenter_1F_Text_PickTheDecorToTrade, MSGBOX_DEFAULT
	special TraderShowDecorationMenu
	waitstate
	goto_if_eq VAR_0x8006, 0, MauvilleCity_PokemonCenter_1F_EventScript_CancelGiveDecor
	goto_if_eq VAR_0x8006, 0xFFFF, MauvilleCity_PokemonCenter_1F_EventScript_DecorInUse
	special IsDecorationCategoryFull
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_PokemonCenter_1F_EventScript_NoRoomForDecor
	msgbox MauvilleCity_PokemonCenter_1F_Text_SoWellTradeTheseDecor, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_PickDecorToGive
	special TraderDoDecorationTrade
	msgbox MauvilleCity_PokemonCenter_1F_Text_SendDecorToYourPC, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_CancelGiveDecor::
	msgbox MauvilleCity_PokemonCenter_1F_Text_YouDontWantToTrade, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_DecorInUse::
	msgbox MauvilleCity_PokemonCenter_1F_Text_ThatDecorIsInUse, MSGBOX_DEFAULT
	goto MauvilleCity_PokemonCenter_1F_EventScript_PickDecorToGive
	end

MauvilleCity_PokemonCenter_1F_EventScript_NoRoomForDecor::
	msgbox MauvilleCity_PokemonCenter_1F_Text_YouveNoRoomForThis, MSGBOX_DEFAULT
	release
	end

/* Storyteller*/
MauvilleCity_PokemonCenter_1F_Text_WillYouHearMyTale:
	.string "Eu sou o CONTADOR DE HISTÓRIAS.\n"
	.string "Conto lendas de TREINADORES\l"
	.string "lendários.\p"
	.string "Quer ouvir uma história?$"

MauvilleCity_PokemonCenter_1F_Text_StorytellerFeelingTheBlues:
	.string "Ah…\n"
	.string "Você me deixou na fossa…$"

MauvilleCity_PokemonCenter_1F_Text_WhichTaleToTell:
	.string "Eu conheço estas lendas.\n"
	.string "Qual história você quer ouvir?$"

MauvilleCity_PokemonCenter_1F_Text_IKnowNoTales:
	.string "Mas não conheço nenhum TREINADOR\n"
	.string "lendário. Então não tenho histórias.\p"
	.string "Onde será que se encontra alguém\n"
	.string "digno de uma história lendária?$"

MauvilleCity_PokemonCenter_1F_Text_YouDidStatXTimes:
	.string "O quê?!\n"
	.string "Você… Você…\p"
	.string "{STR_VAR_2}\n"
	.string "{STR_VAR_1} vez(es)?!\p"
	.string "Isso é mesmo magnífico!\n"
	.string "É o nascimento de uma nova lenda!$"

MauvilleCity_PokemonCenter_1F_Text_CouldThereBeOtherLegends:
	.string "Fico pensando… Será que existem\n"
	.string "outros TREINADORES com lendas\l"
	.string "ainda mais incríveis por aí?$"

MauvilleCity_PokemonCenter_1F_Text_HaveYouAnyLegendaryTales:
	.string "Você treina POKéMON?\p"
	.string "Então me diga: tem alguma história\n"
	.string "que seja um pouquinho lendária?$"

/* Unused*/
MauvilleCity_PokemonCenter_1F_Text_HearAnotherLegendaryTale:
	.string "Aliás… Quer ouvir outra\n"
	.string "história lendária?$"

MauvilleCity_PokemonCenter_1F_Text_NotWorthyOfLegend:
	.string "Hmm…\n"
	.string "Não me convenceu…\p"
	.string "Queria que você me trouxesse algo\n"
	.string "digno de ser chamado de lenda.$"

MauvilleCity_PokemonCenter_1F_Text_IWishMorePeopleWereInterested:
	.string "Queria que mais gente se interessasse\n"
	.string "pelas minhas histórias épicas de\l"
	.string "TREINADORES lendários.$"

MauvilleCity_PokemonCenter_1F_Text_SavedGameTitle::
	.string "Lenda do Jogo Salvo$"

MauvilleCity_PokemonCenter_1F_Text_SavedGameAction::
	.string "salvou o jogo$"

MauvilleCity_PokemonCenter_1F_Text_SavedGameStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} salvou o jogo\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "Ninguém no mundo é mais\n"
	.string "precavido que {STR_VAR_3}!$"

MauvilleCity_PokemonCenter_1F_Text_TrendsStartedTitle::
	.string "Lenda das Modas$"

MauvilleCity_PokemonCenter_1F_Text_TrendsStartedAction::
	.string "lançou modas$"

MauvilleCity_PokemonCenter_1F_Text_TrendsStartedStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} lançou novas modas\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} dita as modas em\n"
	.string "toda a região de HOENN!$"

MauvilleCity_PokemonCenter_1F_Text_BerriesPlantedTitle::
	.string "Lenda das FRUTAS$"

MauvilleCity_PokemonCenter_1F_Text_BerriesPlantedAction::
	.string "plantou FRUTAS$"

MauvilleCity_PokemonCenter_1F_Text_BerriesPlantedStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} plantou FRUTAS\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "O amor de {STR_VAR_3} pelas\n"
	.string "FRUTAS é lendário!$"

MauvilleCity_PokemonCenter_1F_Text_BikeTradesTitle::
	.string "Lenda das BICICLETAS$"

MauvilleCity_PokemonCenter_1F_Text_BikeTradesAction::
	.string "trocou de BICICLETA$"

MauvilleCity_PokemonCenter_1F_Text_BikeTradesStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} trocou de BICICLETA\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve amar BICICLETAS\n"
	.string "de paixão!$"

MauvilleCity_PokemonCenter_1F_Text_InterviewsTitle::
	.string "Lenda das Entrevistas$"

MauvilleCity_PokemonCenter_1F_Text_InterviewsAction::
	.string "deu entrevistas$"

MauvilleCity_PokemonCenter_1F_Text_InterviewsStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} deu entrevistas\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve chamar muita\n"
	.string "atenção por aí!$"

MauvilleCity_PokemonCenter_1F_Text_TrainerBattlesTitle::
	.string "Lenda das Batalhas$"

MauvilleCity_PokemonCenter_1F_Text_TrainerBattlesAction::
	.string "batalhou$"

MauvilleCity_PokemonCenter_1F_Text_TrainerBattlesStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} batalhou\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} nunca recusa uma\n"
	.string "chance de batalhar!$"

MauvilleCity_PokemonCenter_1F_Text_PokemonCaughtTitle::
	.string "Lenda das Capturas$"

MauvilleCity_PokemonCenter_1F_Text_PokemonCaughtAction::
	.string "capturou POKéMON$"

MauvilleCity_PokemonCenter_1F_Text_PokemonCaughtStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} capturou\n"
	.string "{STR_VAR_1} POKéMON!\p"
	.string "{STR_VAR_3} é uma lenda na captura\n"
	.string "de POKéMON selvagens!$"

MauvilleCity_PokemonCenter_1F_Text_FishingPokemonCaughtTitle::
	.string "Lenda da Pescaria$"

MauvilleCity_PokemonCenter_1F_Text_FishingPokemonCaughtAction::
	.string "pescou POKéMON com VARA$"

MauvilleCity_PokemonCenter_1F_Text_FishingPokemonCaughtStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} pescou\n"
	.string "{STR_VAR_1} POKéMON!\p"
	.string "{STR_VAR_3} é uma lenda\n"
	.string "da pescaria!$"

MauvilleCity_PokemonCenter_1F_Text_EggsHatchedTitle::
	.string "Lenda dos OVOS$"

MauvilleCity_PokemonCenter_1F_Text_EggsHatchedAction::
	.string "chocou OVOS$"

MauvilleCity_PokemonCenter_1F_Text_EggsHatchedStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} chocou {STR_VAR_1} POKéMON\n"
	.string "de OVOS!\p"
	.string "{STR_VAR_3} é uma lenda\n"
	.string "em chocar OVOS!$"

MauvilleCity_PokemonCenter_1F_Text_PokemonEvolvedTitle::
	.string "Lenda da Evolução$"

MauvilleCity_PokemonCenter_1F_Text_PokemonEvolvedAction::
	.string "evoluiu POKéMON$"

MauvilleCity_PokemonCenter_1F_Text_PokemonEvolvedStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} fez {STR_VAR_1} POKéMON\n"
	.string "evoluírem!\p"
	.string "Ninguém faz POKéMON evoluírem\n"
	.string "como {STR_VAR_3}!$"

MauvilleCity_PokemonCenter_1F_Text_UsedPokemonCenterTitle::
	.string "Lenda do CENTRO POKéMON$"

MauvilleCity_PokemonCenter_1F_Text_UsedPokemonCenterAction::
	.string "usou o CENTRO POKéMON$"

MauvilleCity_PokemonCenter_1F_Text_UsedPokemonCenterStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} curou POKéMON\n"
	.string "{STR_VAR_1} vezes no CENTRO POKéMON!\p"
	.string "Ninguém ama mais os CENTROS\n"
	.string "POKéMON que {STR_VAR_3}!$"

MauvilleCity_PokemonCenter_1F_Text_RestedAtHomeTitle::
	.string "Lenda do Sofá$"

MauvilleCity_PokemonCenter_1F_Text_RestedAtHomeAction::
	.string "descansou POKéMON em casa$"

MauvilleCity_PokemonCenter_1F_Text_RestedAtHomeStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} curou POKéMON\n"
	.string "{STR_VAR_1} vezes em casa!\p"
	.string "Ninguém é mais caseiro\n"
	.string "que {STR_VAR_3}!$"

MauvilleCity_PokemonCenter_1F_Text_SafariGamesTitle::
	.string "Lenda do SAFÁRI$"

MauvilleCity_PokemonCenter_1F_Text_SafariGamesAction::
	.string "entrou na ZONA DE SAFÁRI$"

MauvilleCity_PokemonCenter_1F_Text_SafariGamesStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} entrou na ZONA DE SAFÁRI\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "O lado selvagem de {STR_VAR_3}\n"
	.string "aparece na ZONA DE SAFÁRI!$"

MauvilleCity_PokemonCenter_1F_Text_UsedCutTitle::
	.string "Lenda do CORTE$"

MauvilleCity_PokemonCenter_1F_Text_UsedCutAction::
	.string "usou CORTE$"

MauvilleCity_PokemonCenter_1F_Text_UsedCutStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} usou CORTE\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve adorar\n"
	.string "sair cortando tudo!$"

MauvilleCity_PokemonCenter_1F_Text_UsedRockSmashTitle::
	.string "Lenda das Pedras$"

MauvilleCity_PokemonCenter_1F_Text_UsedRockSmashAction::
	.string "quebrou pedras$"

MauvilleCity_PokemonCenter_1F_Text_UsedRockSmashStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} usou ESMAGA PEDRA\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} não deixa pedra\n"
	.string "sobre pedra!$"

MauvilleCity_PokemonCenter_1F_Text_MovedBasesTitle::
	.string "Lenda das Mudanças$"

MauvilleCity_PokemonCenter_1F_Text_MovedBasesAction::
	.string "mudou a BASE SECRETA$"

MauvilleCity_PokemonCenter_1F_Text_MovedBasesStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} mudou a BASE SECRETA\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} adora viver\n"
	.string "de mudança!$"

MauvilleCity_PokemonCenter_1F_Text_UsedSplashTitle::
	.string "Lenda da BORRIFADA$"

MauvilleCity_PokemonCenter_1F_Text_UsedSplashAction::
	.string "usou BORRIFADA$"

MauvilleCity_PokemonCenter_1F_Text_UsedSplashStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} usou BORRIFADA\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve adorar\n"
	.string "sair borrifando por aí!$"

MauvilleCity_PokemonCenter_1F_Text_UsedStruggleTitle::
	.string "Lenda da Insistência$"

MauvilleCity_PokemonCenter_1F_Text_UsedStruggleAction::
	.string "apelou pra INSISTÊNCIA$"

MauvilleCity_PokemonCenter_1F_Text_UsedStruggleStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} precisou de INSISTÊNCIA\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} nunca se rende,\n"
	.string "nem diante das adversidades!$"

MauvilleCity_PokemonCenter_1F_Text_SlotJackpotsTitle::
	.string "Lenda do CAÇA-NÍQUEIS$"

MauvilleCity_PokemonCenter_1F_Text_SlotJackpotsAction::
	.string "tirou a bolada no CAÇA-NÍQUEIS$"

MauvilleCity_PokemonCenter_1F_Text_SlotJackpotsStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} tirou a bolada no\n"
	.string "CAÇA-NÍQUEIS {STR_VAR_1} vezes.\p"
	.string "{STR_VAR_3} teve muita sorte\n"
	.string "no CAÇA-NÍQUEIS!$"

MauvilleCity_PokemonCenter_1F_Text_RouletteWinsTitle::
	.string "Lenda da ROLETA$"

MauvilleCity_PokemonCenter_1F_Text_RouletteWinsAction::
	.string "ganhou seguidas na ROLETA$"

MauvilleCity_PokemonCenter_1F_Text_RouletteWinsStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} ganhou na ROLETA\n"
	.string "{STR_VAR_1} vezes seguidas.\p"
	.string "{STR_VAR_3} teve sorte quando\n"
	.string "a bola quicou na ROLETA!$"

MauvilleCity_PokemonCenter_1F_Text_BattleTowerChallengesTitle::
	.string "Lenda da TORRE$"

MauvilleCity_PokemonCenter_1F_Text_BattleTowerChallengesAction::
	.string "desafiou a TORRE DE BATALHA$"

MauvilleCity_PokemonCenter_1F_Text_BattleTowerChallengesStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} desafiou a TORRE DE\n"
	.string "BATALHA {STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} busca a excelência\n"
	.string "na TORRE DE BATALHA!$"

MauvilleCity_PokemonCenter_1F_Text_MadePokeblocksTitle::
	.string "Lenda do LIQUIDIFICADOR$"

MauvilleCity_PokemonCenter_1F_Text_MadePokeblocksAction::
	.string "fez {POKEBLOCK}S$"

MauvilleCity_PokemonCenter_1F_Text_MadePokeblocksStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} fez {POKEBLOCK}S\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "Ninguém usa o LIQUIDIFICADOR\n"
	.string "melhor que {STR_VAR_3}!$"

MauvilleCity_PokemonCenter_1F_Text_EnteredContestsTitle::
	.string "Lenda dos CONCURSOS$"

MauvilleCity_PokemonCenter_1F_Text_EnteredContestsAction::
	.string "entrou em CONCURSOS$"

MauvilleCity_PokemonCenter_1F_Text_EnteredContestsStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} entrou em CONCURSOS\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve adorar exibir\n"
	.string "POKéMON por aí!$"

MauvilleCity_PokemonCenter_1F_Text_WonContestsTitle::
	.string "Mestre dos CONCURSOS$"

MauvilleCity_PokemonCenter_1F_Text_WonContestsAction::
	.string "venceu CONCURSOS$"

MauvilleCity_PokemonCenter_1F_Text_WonContestsStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} venceu CONCURSOS\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve ser fera\n"
	.string "nos CONCURSOS!$"

MauvilleCity_PokemonCenter_1F_Text_TimesShoppedTitle::
	.string "Lenda das Compras$"

MauvilleCity_PokemonCenter_1F_Text_TimesShoppedAction::
	.string "fez compras$"

MauvilleCity_PokemonCenter_1F_Text_TimesShoppedStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} comprou nas lojas\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve ter nascido\n"
	.string "pra fazer compras.$"

MauvilleCity_PokemonCenter_1F_Text_UsedItemFinderTitle::
	.string "Lenda dos Achados$"

MauvilleCity_PokemonCenter_1F_Text_UsedItemFinderAction::
	.string "usou o ACHADOR DE ITENS$"

MauvilleCity_PokemonCenter_1F_Text_UsedItemFinderStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} usou o ACHADOR DE\n"
	.string "ITENS {STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve adorar vasculhar\n"
	.string "o chão atrás de itens!$"

MauvilleCity_PokemonCenter_1F_Text_TimesRainedTitle::
	.string "Lenda da Chuva$"

MauvilleCity_PokemonCenter_1F_Text_TimesRainedAction::
	.string "tomou chuva$"

MauvilleCity_PokemonCenter_1F_Text_TimesRainedStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} tomou chuva\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "O carisma de {STR_VAR_3} deve\n"
	.string "atrair até a chuva!$"

MauvilleCity_PokemonCenter_1F_Text_CheckedPokedexTitle::
	.string "Lenda da POKéDEX$"

MauvilleCity_PokemonCenter_1F_Text_CheckedPokedexAction::
	.string "consultou a POKéDEX$"

MauvilleCity_PokemonCenter_1F_Text_CheckedPokedexStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} consultou a POKéDEX\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve adorar estudar\n"
	.string "POKéMON na POKéDEX!$"

MauvilleCity_PokemonCenter_1F_Text_ReceivedRibbonsTitle::
	.string "Lenda das FITAS$"

MauvilleCity_PokemonCenter_1F_Text_ReceivedRibbonsAction::
	.string "recebeu FITAS$"

MauvilleCity_PokemonCenter_1F_Text_ReceivedRibbonsStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} recebeu FITAS\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve adorar\n"
	.string "colecionar FITAS!$"

MauvilleCity_PokemonCenter_1F_Text_LedgesJumpedTitle::
	.string "Lenda dos Saltos$"

MauvilleCity_PokemonCenter_1F_Text_LedgesJumpedAction::
	.string "pulou de barrancos$"

MauvilleCity_PokemonCenter_1F_Text_LedgesJumpedStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} pulou de barrancos\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "Se tem um barranco pra pular,\n"
	.string "{STR_VAR_3} não resiste!$"

MauvilleCity_PokemonCenter_1F_Text_TVWatchedTitle::
	.string "Lenda da TV$"

MauvilleCity_PokemonCenter_1F_Text_TVWatchedAction::
	.string "assistiu TV$"

MauvilleCity_PokemonCenter_1F_Text_TVWatchedStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} assistiu TV\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve adorar ver TV!$"

MauvilleCity_PokemonCenter_1F_Text_CheckedClockTitle::
	.string "Lenda do Relógio$"

MauvilleCity_PokemonCenter_1F_Text_CheckedClockAction::
	.string "olhou as horas$"

MauvilleCity_PokemonCenter_1F_Text_CheckedClockStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} olhou as horas\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve ser super\n"
	.string "pontual!$"

MauvilleCity_PokemonCenter_1F_Text_WonLotteryTitle::
	.string "Lenda da LOTERIA$"

MauvilleCity_PokemonCenter_1F_Text_WonLotteryAction::
	.string "ganhou na LOTERIA POKéMON$"

MauvilleCity_PokemonCenter_1F_Text_WonLotteryStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} ganhou na LOTERIA\n"
	.string "POKéMON {STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve ter muitos amigos\n"
	.string "pra trocar POKéMON!$"

MauvilleCity_PokemonCenter_1F_Text_UsedDaycareTitle::
	.string "Lenda do CENTRO DE CUIDADOS$"

MauvilleCity_PokemonCenter_1F_Text_UsedDaycareAction::
	.string "deixou POKéMON no CENTRO DE CUIDADOS$"

MauvilleCity_PokemonCenter_1F_Text_UsedDaycareStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} deixou POKéMON no CENTRO\n"
	.string "DE CUIDADOS {STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve levar muito a\n"
	.string "sério a criação de POKéMON!$"

MauvilleCity_PokemonCenter_1F_Text_RodeCableCarTitle::
	.string "Lenda do TELEFÉRICO$"

MauvilleCity_PokemonCenter_1F_Text_RodeCableCarAction::
	.string "andou de TELEFÉRICO$"

MauvilleCity_PokemonCenter_1F_Text_RodeCableCarStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} andou de TELEFÉRICO\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} vive num sobe\n"
	.string "e desce sem fim!$"

MauvilleCity_PokemonCenter_1F_Text_HotSpringsTitle::
	.string "Lenda das Termas$"

MauvilleCity_PokemonCenter_1F_Text_HotSpringsAction::
	.string "se banhou nas termas$"

MauvilleCity_PokemonCenter_1F_Text_HotSpringsStory::
	.string "Lá vai uma história lendária…\p"
	.string "{STR_VAR_3} se banhou nas termas\n"
	.string "{STR_VAR_1} vezes!\p"
	.string "{STR_VAR_3} deve ter a pele\n"
	.string "lisinha feito bebê!$"


MauvilleCity_PokemonCenter_1F_EventScript_Storyteller::
	lock
	faceplayer
	setvar VAR_0x8008, 0
	setvar VAR_0x8009, 0
	setvar VAR_0x800A, 0
	setvar VAR_0x800B, 0
	msgbox MauvilleCity_PokemonCenter_1F_Text_WillYouHearMyTale, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_DeclineStoryteller
	specialvar VAR_RESULT, StorytellerGetFreeStorySlot
	goto_if_eq VAR_RESULT, 0, MauvilleCity_PokemonCenter_1F_EventScript_KnowNoTales
	message MauvilleCity_PokemonCenter_1F_Text_WhichTaleToTell
	waitmessage
	special StorytellerStoryListMenu
	waitstate
	goto_if_eq VAR_RESULT, 0, MauvilleCity_PokemonCenter_1F_EventScript_CancelStorySelection
	setvar VAR_0x8008, 1
	special Script_StorytellerDisplayStory
	waitmessage
	waitbuttonpress
	specialvar VAR_RESULT, StorytellerUpdateStat
	goto_if_eq VAR_RESULT, 0, MauvilleCity_PokemonCenter_1F_EventScript_AreThereOtherTales
	goto MauvilleCity_PokemonCenter_1F_EventScript_TellPlayersTale

MauvilleCity_PokemonCenter_1F_EventScript_CancelStorySelection::
	goto MauvilleCity_PokemonCenter_1F_EventScript_DeclineStoryteller
	end

MauvilleCity_PokemonCenter_1F_EventScript_AreThereOtherTales::
	msgbox MauvilleCity_PokemonCenter_1F_Text_CouldThereBeOtherLegends, MSGBOX_DEFAULT
	specialvar VAR_RESULT, HasStorytellerAlreadyRecorded
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_PokemonCenter_1F_EventScript_StorytellerEnd
	goto MauvilleCity_PokemonCenter_1F_EventScript_DoYouHaveAnyTales

MauvilleCity_PokemonCenter_1F_EventScript_KnowNoTales::
	msgbox MauvilleCity_PokemonCenter_1F_Text_IKnowNoTales, MSGBOX_DEFAULT
MauvilleCity_PokemonCenter_1F_EventScript_DoYouHaveAnyTales::
	msgbox MauvilleCity_PokemonCenter_1F_Text_HaveYouAnyLegendaryTales, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_DeclineStoryteller
	specialvar VAR_RESULT, Script_StorytellerInitializeRandomStat
	goto_if_eq VAR_RESULT, 1, MauvilleCity_PokemonCenter_1F_EventScript_TellPlayersTale
	msgbox MauvilleCity_PokemonCenter_1F_Text_NotWorthyOfLegend, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_TellPlayersTale::
	msgbox MauvilleCity_PokemonCenter_1F_Text_YouDidStatXTimes, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_DeclineStoryteller::
	msgbox MauvilleCity_PokemonCenter_1F_Text_StorytellerFeelingTheBlues, MSGBOX_DEFAULT
	release
	end

/* Unused*/
MauvilleCity_PokemonCenter_1F_EventScript_WaitingForRecordMix::
	msgbox MauvilleCity_PokemonCenter_1F_Text_IWishMorePeopleWereInterested, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_StorytellerEnd::
	release
	end

/* Giddy*/
MauvilleCity_PokemonCenter_1F_Text_HearMyStory:
	.string "Eu sou GIDDY!\n"
	.string "Tenho uma história incrível pra você!\p"
	.string "Quer ouvir?$"

MauvilleCity_PokemonCenter_1F_Text_GiddyFeelingTheBlues:
	.string "Ah…\n"
	.string "Você me deixou na fossa…$"

MauvilleCity_PokemonCenter_1F_Text_AlsoIWasThinking:
	.string "E também, eu tava pensando…$"

MauvilleCity_PokemonCenter_1F_Text_WeShouldChatAgain:
	.string "Acho que é isso…\p"
	.string "Vamos conversar de novo!\n"
	.string "Tchauzinho!$"

MauvilleCity_PokemonCenter_1F_EventScript_Giddy::
	lock
	faceplayer
	msgbox MauvilleCity_PokemonCenter_1F_Text_HearMyStory, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, MauvilleCity_PokemonCenter_1F_EventScript_TryTellTale
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_DeclineGiddy
	end

MauvilleCity_PokemonCenter_1F_EventScript_TryTellTale::
	special GiddyShouldTellAnotherTale
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_PokemonCenter_1F_EventScript_GiddyTellTale
	goto_if_eq VAR_RESULT, FALSE, MauvilleCity_PokemonCenter_1F_EventScript_ToldEnoughTales
	end

MauvilleCity_PokemonCenter_1F_EventScript_TryTellNewTale::
	special GiddyShouldTellAnotherTale
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_PokemonCenter_1F_EventScript_GiddyStartNewTale
	goto_if_eq VAR_RESULT, FALSE, MauvilleCity_PokemonCenter_1F_EventScript_ToldEnoughTales
	end

MauvilleCity_PokemonCenter_1F_EventScript_GiddyStartNewTale::
	msgbox MauvilleCity_PokemonCenter_1F_Text_AlsoIWasThinking, MSGBOX_DEFAULT
	goto MauvilleCity_PokemonCenter_1F_EventScript_GiddyTellTale
	end

/* Giddy will continue to tell stories regardless of whether yes or no is selected below.*/
/* Each story there is a 10% chance it will be his last. Otherwise he will stop at 10 stories.*/
MauvilleCity_PokemonCenter_1F_EventScript_GiddyTellTale::
	special GenerateGiddyLine
	special ShowFieldMessageStringVar4
	waitmessage
	yesnobox 20, 8
	goto_if_eq VAR_RESULT, YES, MauvilleCity_PokemonCenter_1F_EventScript_TryTellNewTale
	goto_if_eq VAR_RESULT, NO, MauvilleCity_PokemonCenter_1F_EventScript_TryTellNewTale
	end

MauvilleCity_PokemonCenter_1F_EventScript_DeclineGiddy::
	msgbox MauvilleCity_PokemonCenter_1F_Text_GiddyFeelingTheBlues, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_EventScript_ToldEnoughTales::
	msgbox MauvilleCity_PokemonCenter_1F_Text_WeShouldChatAgain, MSGBOX_DEFAULT
	release
	end

MauvilleCity_PokemonCenter_1F_Text_WouldYouLikeToHearMySong:
	.string "Oi, eu sou o BARDO.\n"
	.string "Quer ouvir a minha canção?$"

MauvilleCity_PokemonCenter_1F_Text_BardFeelingTheBlues1:
	.string "Ah…\n"
	.string "Você me deixou na fossa…$"

MauvilleCity_PokemonCenter_1F_Text_WishICouldPlaySongForOthers:
	.string "Ah, que canção comovente…\n"
	.string "Queria tocar ela pra mais gente…$"

MauvilleCity_PokemonCenter_1F_Text_WouldYouLikeToWriteSomeLyrics:
	.string "E aí?\n"
	.string "Gostou da minha canção?\p"
	.string "Mas não tô muito feliz com\n"
	.string "a letra.\p"
	.string "Que tal escrever uma letra\n"
	.string "nova pra mim?$"

MauvilleCity_PokemonCenter_1F_Text_BardFeelingTheBlues2:
	.string "Ah…\n"
	.string "Você me deixou na fossa…$"

MauvilleCity_PokemonCenter_1F_Text_LetMeSingItForYou:
	.string "Muito obrigado!\n"
	.string "Vou cantar pra você.$"

MauvilleCity_PokemonCenter_1F_Text_ThatHowYouWantedSongToGo:
	.string "Era assim que você queria\n"
	.string "a canção?$"

MauvilleCity_PokemonCenter_1F_Text_IllSingThisSongForAWhile:
	.string "Beleza! Então é isso.\n"
	.string "Vou cantar esta por um tempo.$"

MauvilleCity_PokemonCenter_1F_Text_TeachWhatsHipAndHappening:
	.string "E aí! Me chamam de DESCOLADO.\n"
	.string "Vou te ensinar o que tá na moda.$"

MauvilleCity_PokemonCenter_1F_Text_IAlreadyTaughtYou:
	.string "Mas, ó, eu já te ensinei o que\n"
	.string "tá na moda.\p"
	.string "Quero espalhar a novidade\n"
	.string "pra outras pessoas.$"

MauvilleCity_PokemonCenter_1F_Text_IveGotNothingNewToTeach:
	.string "Mas, ó, você já sabe muito sobre\n"
	.string "o que tá na moda.\p"
	.string "Não tenho nada novo pra te ensinar!$"

MauvilleCity_PokemonCenter_1F_Text_HaveYouHeardOfWord:
	.string "Ei, já ouviu falar de\n"
	.string "“{STR_VAR_1}”?\p"
	.string "O que quer dizer? Bom…\n"
	.string "Pergunta pro seu pai ou sua mãe!$"
