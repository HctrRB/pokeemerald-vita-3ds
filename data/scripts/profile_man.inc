ProfileMan_EventScript_Man::
	lock
	faceplayer
	goto_if_set FLAG_SYS_CHAT_USED, ProfileMan_EventScript_GivenProfileBefore
	msgbox ProfileMan_Text_CollectTrainerProfiles, MSGBOX_DEFAULT
	goto ProfileMan_EventScript_AskToSeeProfile
	end

ProfileMan_EventScript_AskToSeeProfile::
	msgbox ProfileMan_Text_MayISeeYourProfile, MSGBOX_DEFAULT
	multichoice 17, 6, MULTI_YESNOINFO_2, FALSE
	switch VAR_RESULT
	case 0, ProfileMan_EventScript_CreateProfile
	case 1, ProfileMan_EventScript_DeclineShowProfile
	case 2, ProfileMan_EventScript_Info
	case MULTI_B_PRESSED, ProfileMan_EventScript_DeclineShowProfile
	end

ProfileMan_EventScript_Info::
	msgbox ProfileMan_Text_EasyChatExplanation, MSGBOX_DEFAULT
	goto ProfileMan_EventScript_AskToSeeProfile
	end

ProfileMan_EventScript_CreateProfile::
	msgbox ProfileMan_Text_LetsSeeItThen, MSGBOX_DEFAULT
	closemessage
	setvar VAR_0x8004, EASY_CHAT_TYPE_PROFILE
	call Common_ShowEasyChatScreen
	lock
	faceplayer
	goto_if_eq VAR_RESULT, 0, ProfileMan_EventScript_CancelShowProfile
	goto_if_eq VAR_RESULT, 1, ProfileMan_EventScript_ShowProfile
	end

ProfileMan_EventScript_CancelShowProfile::
	msgbox ProfileMan_Text_NotIntoItRightNow, MSGBOX_DEFAULT
	release
	end

ProfileMan_EventScript_ShowProfile::
	setvar VAR_0x8004, 0
	special ShowEasyChatProfile
	waitmessage
	delay 80
	msgbox ProfileMan_Text_FantasticProfile, MSGBOX_DEFAULT
	release
	end

ProfileMan_EventScript_DeclineShowProfile::
	msgbox ProfileMan_Text_ImagineYouWouldHaveWonderfulProfile, MSGBOX_DEFAULT
	release
	end

ProfileMan_EventScript_GivenProfileBefore::
	msgbox ProfileMan_Text_YouHaveWonderfulSmile, MSGBOX_DEFAULT
	goto ProfileMan_EventScript_AskToSeeNewProfile
	end

ProfileMan_EventScript_AskToSeeNewProfile::
	msgbox ProfileMan_Text_MayISeeYourNewProfile, MSGBOX_DEFAULT
	multichoice 17, 6, MULTI_YESNOINFO_2, FALSE
	switch VAR_RESULT
	case 0, ProfileMan_EventScript_CreateNewProfile
	case 1, ProfileMan_EventScript_DeclineNewProfile
	case 2, ProfileMan_EventScript_InfoNewProfile
	case MULTI_B_PRESSED, ProfileMan_EventScript_DeclineNewProfile
	end

ProfileMan_EventScript_InfoNewProfile::
	msgbox ProfileMan_Text_EasyChatExplanation, MSGBOX_DEFAULT
	goto ProfileMan_EventScript_AskToSeeNewProfile
	end

ProfileMan_EventScript_CreateNewProfile::
	msgbox ProfileMan_Text_EvenBetterThanLastProfile, MSGBOX_DEFAULT
	closemessage
	setvar VAR_0x8004, EASY_CHAT_TYPE_PROFILE
	call Common_ShowEasyChatScreen
	lock
	faceplayer
	goto_if_eq VAR_RESULT, 0, ProfileMan_EventScript_CancelShowProfile
	goto_if_eq VAR_RESULT, 1, ProfileMan_EventScript_ShowProfile
	end

ProfileMan_EventScript_DeclineNewProfile::
	msgbox ProfileMan_Text_LikeProfileWayItIs, MSGBOX_DEFAULT
	release
	end

/* Unused*/
ProfileMan_EventScript_Ret::
	return

ProfileMan_Text_CollectTrainerProfiles:
	.string "Olá, TREINADOR!\n"
	.string "Que sorriso lindo, hein?\p"
	.string "Eu tenho um hobby: colecionar os\n"
	.string "perfis de TREINADORES POKéMON.$"

ProfileMan_Text_MayISeeYourProfile:
	.string "E aí, topa?\n"
	.string "Posso ver o seu perfil?$"

ProfileMan_Text_EasyChatExplanation:
	.string "Você monta o seu perfil juntando\n"
	.string "quatro palavras ou frases.\p"
	.string "Olha, vou te mostrar um exemplo\n"
	.string "de perfil com quatro partes.\p"
	.string "Dá pra trocar essas quatro partes\n"
	.string "por outras do jeito que quiser\l"
	.string "pra montar o seu perfil.\p"
	.string "Tem um monte de partes que\n"
	.string "você pode usar.\p"
	.string "Elas ficam em grupos, como\n"
	.string "POKéMON, estilo de vida e hobbies,\l"
	.string "pra ficar fácil de achar.\p"
	.string "Primeiro, escolha um grupo pra\n"
	.string "ver a lista de opções.\p"
	.string "Depois, escolha a que quiser.\p"
	.string "Repita com as outras partes e\n"
	.string "pronto: o perfil é todo seu.$"

ProfileMan_Text_LetsSeeItThen:
	.string "Isso! Valeu!\n"
	.string "Então vamos ver.$"

ProfileMan_Text_ImagineYouWouldHaveWonderfulProfile:
	.string "Ah, não, sério?\p"
	.string "Aposto que alguém como você\n"
	.string "teria um perfil incrível…$"

ProfileMan_Text_NotIntoItRightNow:
	.string "Ué? Não tá no clima agora?\p"
	.string "Tudo bem, pode ser quando quiser!$"

ProfileMan_Text_YouHaveWonderfulSmile:
	.string "Olá, TREINADOR!\n"
	.string "Que sorriso lindo.$"

ProfileMan_Text_MayISeeYourNewProfile:
	.string "Posso ver o seu perfil novo?$"

ProfileMan_Text_EvenBetterThanLastProfile:
	.string "Isso! Valeu!\p"
	.string "Tomara que seja ainda melhor que\n"
	.string "o perfil que você me mostrou.$"

ProfileMan_Text_LikeProfileWayItIs:
	.string "Ah, você gosta do perfil do jeito\n"
	.string "que tá.\p"
	.string "Não te culpo: ele já é\n"
	.string "incrível assim.$"

ProfileMan_Text_FantasticProfile:
	.string "F-fantástico!\p"
	.string "Seu perfil é incrível!\n"
	.string "Diz tudo sobre você.\p"
	.string "Quem ouvir esse perfil vai ficar\n"
	.string "encantado com você!\p"
	.string "Valeu!$"

/* Unused*/
ProfileMan_Text_YouKnowSecretSaying:
	.string "Opa!\n"
	.string "Você sabe a frase secreta!\p"
	.string "Isso quer dizer que agora você é\n"
	.string "parte do CLUBE DO EVENTO MISTERIOSO!$"
