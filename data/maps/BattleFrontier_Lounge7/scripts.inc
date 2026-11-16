BattleFrontier_Lounge7_MapScripts::
	.byte 0

BattleFrontier_Lounge7_EventScript_LeftMoveTutor::
	lock
	faceplayer
	setvar VAR_TEMP_C, SCROLL_MULTI_BF_MOVE_TUTOR_1
	goto_if_set FLAG_MET_FRONTIER_BEAUTY_MOVE_TUTOR, BattleFrontier_Lounge7_EventScript_AlreadyMetLeftTutor
	msgbox BattleFrontier_Lounge7_Text_LeftTutorIntro, MSGBOX_DEFAULT
	setflag FLAG_MET_FRONTIER_BEAUTY_MOVE_TUTOR
	goto BattleFrontier_Lounge7_EventScript_ChooseLeftTutorMove
	end

BattleFrontier_Lounge7_EventScript_AlreadyMetLeftTutor::
	msgbox BattleFrontier_Lounge7_Text_LeftTutorWelcomeBack, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge7_EventScript_ChooseLeftTutorMove
	end

BattleFrontier_Lounge7_EventScript_ChooseLeftTutorMove::
	message BattleFrontier_Lounge7_Text_TeachWhichMove
	waitmessage
	special ShowBattlePointsWindow
	setvar VAR_TEMP_FRONTIER_TUTOR_ID, 0
	setvar VAR_0x8004, SCROLL_MULTI_BF_MOVE_TUTOR_1
	setvar VAR_0x8006, 0
	special ShowScrollableMultichoice
	waitstate
	copyvar VAR_TEMP_FRONTIER_TUTOR_SELECTION, VAR_RESULT
	switch VAR_RESULT
	case 0, BattleFrontier_Lounge7_EventScript_Softboiled
	case 1, BattleFrontier_Lounge7_EventScript_SeismicToss
	case 2, BattleFrontier_Lounge7_EventScript_DreamEater
	case 3, BattleFrontier_Lounge7_EventScript_MegaPunch
	case 4, BattleFrontier_Lounge7_EventScript_MegaKick
	case 5, BattleFrontier_Lounge7_EventScript_BodySlam
	case 6, BattleFrontier_Lounge7_EventScript_RockSlide
	case 7, BattleFrontier_Lounge7_EventScript_Counter
	case 8, BattleFrontier_Lounge7_EventScript_ThunderWave
	case 9, BattleFrontier_Lounge7_EventScript_SwordsDance
	case 10, BattleFrontier_Lounge7_EventScript_ExitTutorMoveSelect
	case MULTI_B_PRESSED, BattleFrontier_Lounge7_EventScript_ExitTutorMoveSelect
	end

BattleFrontier_Lounge7_EventScript_ChooseNewLeftTutorMove::
	message BattleFrontier_Lounge7_Text_TeachWhichMove
	waitmessage
	setvar VAR_TEMP_FRONTIER_TUTOR_ID, 0
	setvar VAR_0x8004, SCROLL_MULTI_BF_MOVE_TUTOR_1
	setvar VAR_0x8006, 1
	special ShowScrollableMultichoice
	waitstate
	copyvar VAR_TEMP_FRONTIER_TUTOR_SELECTION, VAR_RESULT
	switch VAR_RESULT
	case 0, BattleFrontier_Lounge7_EventScript_Softboiled
	case 1, BattleFrontier_Lounge7_EventScript_SeismicToss
	case 2, BattleFrontier_Lounge7_EventScript_DreamEater
	case 3, BattleFrontier_Lounge7_EventScript_MegaPunch
	case 4, BattleFrontier_Lounge7_EventScript_MegaKick
	case 5, BattleFrontier_Lounge7_EventScript_BodySlam
	case 6, BattleFrontier_Lounge7_EventScript_RockSlide
	case 7, BattleFrontier_Lounge7_EventScript_Counter
	case 8, BattleFrontier_Lounge7_EventScript_ThunderWave
	case 9, BattleFrontier_Lounge7_EventScript_SwordsDance
	case 10, BattleFrontier_Lounge7_EventScript_ExitTutorMoveSelect
	case MULTI_B_PRESSED, BattleFrontier_Lounge7_EventScript_ExitTutorMoveSelect
	end

BattleFrontier_Lounge7_EventScript_Softboiled::
	setvar VAR_0x8008, 16
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_SeismicToss::
	setvar VAR_0x8008, 24
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_DreamEater::
	setvar VAR_0x8008, 24
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_MegaPunch::
	setvar VAR_0x8008, 24
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_MegaKick::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_BodySlam::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_RockSlide::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_Counter::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_ThunderWave::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_SwordsDance::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_RightMoveTutor::
	lock
	faceplayer
	setvar VAR_TEMP_C, SCROLL_MULTI_BF_MOVE_TUTOR_2
	goto_if_set FLAG_MET_FRONTIER_SWIMMER_MOVE_TUTOR, BattleFrontier_Lounge7_EventScript_AlreadyMetRightTutor
	msgbox BattleFrontier_Lounge7_Text_RightTutorIntro, MSGBOX_DEFAULT
	setflag FLAG_MET_FRONTIER_SWIMMER_MOVE_TUTOR
	goto BattleFrontier_Lounge7_EventScript_ChooseRightTutorMove
	end

BattleFrontier_Lounge7_EventScript_AlreadyMetRightTutor::
	msgbox BattleFrontier_Lounge7_Text_RightTutorWelcomeBack, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge7_EventScript_ChooseRightTutorMove
	end

BattleFrontier_Lounge7_EventScript_ChooseRightTutorMove::
	message BattleFrontier_Lounge7_Text_TeachWhichMove
	waitmessage
	special ShowBattlePointsWindow
	setvar VAR_TEMP_FRONTIER_TUTOR_ID, 1
	setvar VAR_0x8004, SCROLL_MULTI_BF_MOVE_TUTOR_2
	setvar VAR_0x8006, 0
	special ShowScrollableMultichoice
	waitstate
	copyvar VAR_TEMP_FRONTIER_TUTOR_SELECTION, VAR_RESULT
	switch VAR_RESULT
	case 0, BattleFrontier_Lounge7_EventScript_DefenseCurl
	case 1, BattleFrontier_Lounge7_EventScript_Snore
	case 2, BattleFrontier_Lounge7_EventScript_MudSlap
	case 3, BattleFrontier_Lounge7_EventScript_Swift
	case 4, BattleFrontier_Lounge7_EventScript_IcyWind
	case 5, BattleFrontier_Lounge7_EventScript_Endure
	case 6, BattleFrontier_Lounge7_EventScript_PsychUp
	case 7, BattleFrontier_Lounge7_EventScript_IcePunch
	case 8, BattleFrontier_Lounge7_EventScript_ThunderPunch
	case 9, BattleFrontier_Lounge7_EventScript_FirePunch
	case 10, BattleFrontier_Lounge7_EventScript_ExitTutorMoveSelect
	case MULTI_B_PRESSED, BattleFrontier_Lounge7_EventScript_ExitTutorMoveSelect
	end

BattleFrontier_Lounge7_EventScript_ChooseNewRightTutorMove::
	message BattleFrontier_Lounge7_Text_TeachWhichMove
	waitmessage
	setvar VAR_TEMP_FRONTIER_TUTOR_ID, 1
	setvar VAR_0x8004, SCROLL_MULTI_BF_MOVE_TUTOR_2
	setvar VAR_0x8006, 1
	special ShowScrollableMultichoice
	waitstate
	copyvar VAR_TEMP_FRONTIER_TUTOR_SELECTION, VAR_RESULT
	switch VAR_RESULT
	case 0, BattleFrontier_Lounge7_EventScript_DefenseCurl
	case 1, BattleFrontier_Lounge7_EventScript_Snore
	case 2, BattleFrontier_Lounge7_EventScript_MudSlap
	case 3, BattleFrontier_Lounge7_EventScript_Swift
	case 4, BattleFrontier_Lounge7_EventScript_IcyWind
	case 5, BattleFrontier_Lounge7_EventScript_Endure
	case 6, BattleFrontier_Lounge7_EventScript_PsychUp
	case 7, BattleFrontier_Lounge7_EventScript_IcePunch
	case 8, BattleFrontier_Lounge7_EventScript_ThunderPunch
	case 9, BattleFrontier_Lounge7_EventScript_FirePunch
	case 10, BattleFrontier_Lounge7_EventScript_ExitTutorMoveSelect
	case MULTI_B_PRESSED, BattleFrontier_Lounge7_EventScript_ExitTutorMoveSelect
	end

BattleFrontier_Lounge7_EventScript_DefenseCurl::
	setvar VAR_0x8008, 16
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_Snore::
	setvar VAR_0x8008, 24
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_MudSlap::
	setvar VAR_0x8008, 24
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_Swift::
	setvar VAR_0x8008, 24
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_IcyWind::
	setvar VAR_0x8008, 24
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_Endure::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_PsychUp::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_IcePunch::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_ThunderPunch::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_FirePunch::
	setvar VAR_0x8008, 48
	goto BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection
	end

BattleFrontier_Lounge7_EventScript_ExitTutorMoveSelect::
	special CloseBattleFrontierTutorWindow
	special CloseBattlePointsWindow
	msgbox BattleFrontier_Lounge7_Text_YouDontWantTo, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Lounge7_EventScript_CancelChooseMon::
	msgbox BattleFrontier_Lounge7_Text_YouDontWantTo, MSGBOX_DEFAULT
	release
	end

/* VAR_0x8008 is the price*/
/* VAR_TEMP_C is the scroll multichoice ID*/
/* VAR_TEMP_FRONTIER_TUTOR_SELECTION is the move selection*/
/* VAR_TEMP_FRONTIER_TUTOR_ID is which move tutor was spoken to*/
BattleFrontier_Lounge7_EventScript_ConfirmMoveSelection::
	copyvar VAR_0x8004, VAR_TEMP_FRONTIER_TUTOR_SELECTION
	copyvar VAR_0x8005, VAR_TEMP_FRONTIER_TUTOR_ID
	special BufferBattleFrontierTutorMoveName
	buffernumberstring STR_VAR_2, VAR_0x8008
	copyvar VAR_0x8004, VAR_TEMP_C
	msgbox BattleFrontier_Lounge7_Text_MoveWillBeXBattlePoints, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, BattleFrontier_Lounge7_EventScript_ChooseNewMove
	specialvar VAR_TEMP_1, GetFrontierBattlePoints
	goto_if_ge VAR_TEMP_1, VAR_0x8008, BattleFrontier_Lounge7_EventScript_TeachTutorMove
	msgbox BattleFrontier_Lounge7_Text_HaventGotEnoughPoints, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge7_EventScript_ChooseNewMove
	end

BattleFrontier_Lounge7_EventScript_TeachTutorMove::
	msgbox BattleFrontier_Lounge7_Text_TeachMoveToWhichMon, MSGBOX_DEFAULT
	special GetBattleFrontierTutorMoveIndex
	fadescreen FADE_TO_BLACK
	special CloseBattlePointsWindow
	special CloseBattleFrontierTutorWindow
	special ChooseMonForMoveTutor
	waitstate
	goto_if_eq VAR_RESULT, FALSE, BattleFrontier_Lounge7_EventScript_CancelChooseMon
	msgbox BattleFrontier_Lounge7_Text_IllTakeBattlePoints, MSGBOX_DEFAULT
	copyvar VAR_0x8004, VAR_0x8008
	special TakeFrontierBattlePoints
	release
	end

BattleFrontier_Lounge7_EventScript_ChooseNewMove::
	goto_if_eq VAR_TEMP_FRONTIER_TUTOR_ID, 0, BattleFrontier_Lounge7_EventScript_ChooseNewLeftTutorMove
	goto BattleFrontier_Lounge7_EventScript_ChooseNewRightTutorMove
	end

BattleFrontier_Lounge7_EventScript_Sailor::
	msgbox BattleFrontier_Lounge7_Text_ThinkLadiesDontGetAlong, MSGBOX_NPC
	end

BattleFrontier_Lounge7_EventScript_Gentleman::
	msgbox BattleFrontier_Lounge7_Text_LadiesWereStrongAndBeautiful, MSGBOX_NPC
	end

BattleFrontier_Lounge7_Text_LeftTutorIntro:
	.string "Buhahaha!\p"
	.string "Olhando agora você não diria, mas\n"
	.string "eu já fui uma TREINADORA e tanto.\p"
	.string "Eu era famosa como a BELDADE mais\n"
	.string "forte da região, viu?\p"
	.string "… … … … … …\p"
	.string "O que foi?\n"
	.string "Não acredita em mim.\p"
	.string "Não sou como aquela fanfarrona ali.\n"
	.string "Eu tenho talento de verdade!\p"
	.string "Vou te provar.\n"
	.string "Posso ensinar golpes especiais, e\l"
	.string "ainda fofos, aos seus POKéMON.\p"
	.string "Mas as aulas não são de graça.\n"
	.string "Que tal pagar os golpes com uns\l"
	.string "Pontos de Batalha?$"

BattleFrontier_Lounge7_Text_LeftTutorWelcomeBack:
	.string "Buhahaha!\p"
	.string "Voltou pra aprender golpes POKéMON\n"
	.string "especiais, e ainda fofos?$"

BattleFrontier_Lounge7_Text_TeachWhichMove:
	.string "Pois bem, olhe aqui!\n"
	.string "Que golpe eu devo ensinar?$"

BattleFrontier_Lounge7_Text_MoveWillBeXBattlePoints:
	.string "O golpe {STR_VAR_1}, é?\n"
	.string "Fica {STR_VAR_2} Pontos de Batalha, tá?$"

BattleFrontier_Lounge7_Text_TeachMoveToWhichMon:
	.string "Pois bem, agora escolha o POKéMON\n"
	.string "que vai aprender o golpe.$"

BattleFrontier_Lounge7_Text_HaventGotEnoughPoints:
	.string "Mas o quê…\n"
	.string "Você não tem Pontos de Batalha\l"
	.string "suficientes!$"

BattleFrontier_Lounge7_Text_IllTakeBattlePoints:
	.string "Viu como eu sou habilidosa?\n"
	.string "Vou ficar com os Pontos, obrigada!$"

BattleFrontier_Lounge7_Text_YouDontWantTo:
	.string "O que é isso?\n"
	.string "Não quer…\p"
	.string "Se quiser ver a minha habilidade,\n"
	.string "venha me ver quando quiser!$"

BattleFrontier_Lounge7_Text_RightTutorIntro:
	.string "Ihihihi!\p"
	.string "Sei que é difícil de ver agora, mas\n"
	.string "eu já fui uma TREINADORA fantástica.\p"
	.string "Eu era famosa como a NADADORA mais\n"
	.string "invencível da região, viu?\p"
	.string "… … … … … …\p"
	.string "O que houve?\n"
	.string "Não acredita em mim.\p"
	.string "Não sou como aquela palhaça ali.\n"
	.string "Eu tenho experiência de verdade!\p"
	.string "Posso provar.\n"
	.string "Posso ensinar golpes fortes, e\l"
	.string "ainda bonitos, aos seus POKéMON.\p"
	.string "Mas as aulas não são de graça.\n"
	.string "Que tal pagar os golpes com uns\l"
	.string "Pontos de Batalha?$"

BattleFrontier_Lounge7_Text_RightTutorWelcomeBack:
	.string "Ihihihi!\p"
	.string "Veio aprender golpes POKéMON fortes,\n"
	.string "e ainda bonitos?$"

BattleFrontier_Lounge7_Text_ThinkLadiesDontGetAlong:
	.string "Pelo jeito que aquelas senhoras falam\n"
	.string "mal uma da outra, você deve achar\l"
	.string "que elas não se dão bem.\p"
	.string "Mas, se fosse verdade, elas não\n"
	.string "ficariam juntas aqui, não é?$"

BattleFrontier_Lounge7_Text_LadiesWereStrongAndBeautiful:
	.string "Quando eu era só um JOVEM\n"
	.string "TREINADOR, aquelas senhoras eram\l"
	.string "fortes e lindas.\p"
	.string "Eram ídolos entre nós, TREINADORES.\p"
	.string "Mesmo agora, a idade não diminuiu\n"
	.string "o talento delas.\p"
	.string "Na verdade, os golpes dos POKéMON\n"
	.string "delas estão ainda mais refinados.\p"
	.string "Mas… Por algum motivo, não consigo\n"
	.string "evitar sentir isto…\p"
	.string "O tempo é tão cruel…$"

BattleFrontier_Lounge7_Text_SoftboiledDesc::
	.string "Recupera até\n"
	.string "metade dos PS\n"
	.string "máximos.$"

BattleFrontier_Lounge7_Text_SeismicTossDesc::
	.string "Tira tantos PS\n"
	.string "quanto o nível\n"
	.string "do usuário.$"

BattleFrontier_Lounge7_Text_DreamEaterDesc::
	.string "Rouba metade do\n"
	.string "dano de um alvo\n"
	.string "adormecido.$"

BattleFrontier_Lounge7_Text_MegaPunchDesc::
	.string "Um soco dado\n"
	.string "com uma força\n"
	.string "incrível.$"

BattleFrontier_Lounge7_Text_MegaKickDesc::
	.string "Um chute\n"
	.string "fortíssimo, com\n"
	.string "muita força.$"

BattleFrontier_Lounge7_Text_BodySlamDesc::
	.string "Uma pancada com\n"
	.string "o corpo que pode\n"
	.string "paralisar.$"

BattleFrontier_Lounge7_Text_RockSlideDesc::
	.string "Arremessa rochas\n"
	.string "enormes. Pode\n"
	.string "fazer hesitar.$"

BattleFrontier_Lounge7_Text_CounterDesc::
	.string "Devolve um golpe\n"
	.string "físico com o\n"
	.string "dobro da força.$"

BattleFrontier_Lounge7_Text_ThunderWaveDesc::
	.string "Um choque fraco\n"
	.string "que paralisa\n"
	.string "o alvo.$"

BattleFrontier_Lounge7_Text_SwordsDanceDesc::
	.string "Uma dança de\n"
	.string "luta que sobe\n"
	.string "muito o ATAQUE.$"

BattleFrontier_Lounge7_Text_DefenseCurlDesc::
	.string "Se enrola pra\n"
	.string "esconder pontos\n"
	.string "fracos. Sobe DEF.$"

BattleFrontier_Lounge7_Text_SnoreDesc::
	.string "Ataque barulhento\n"
	.string "que só funciona\n"
	.string "dormindo.$"

BattleFrontier_Lounge7_Text_MudSlapDesc::
	.string "Joga lama na\n"
	.string "cara do alvo e\n"
	.string "baixa a PRECISÃO.$"

BattleFrontier_Lounge7_Text_SwiftDesc::
	.string "Raios em forma\n"
	.string "de estrela que\n"
	.string "nunca erram.$"

BattleFrontier_Lounge7_Text_IcyWindDesc::
	.string "Um ataque gelado\n"
	.string "que baixa a\n"
	.string "VELOC. do alvo.$"

BattleFrontier_Lounge7_Text_EndureDesc::
	.string "Aguenta qualquer\n"
	.string "ataque por 1\n"
	.string "turno com 1 PS.$"

BattleFrontier_Lounge7_Text_PsychUpDesc::
	.string "Copia os efeitos\n"
	.string "do alvo para\n"
	.string "o usuário.$"

BattleFrontier_Lounge7_Text_IcePunchDesc::
	.string "Um soco gelado\n"
	.string "que pode\n"
	.string "congelar o alvo.$"

BattleFrontier_Lounge7_Text_ThunderPunchDesc::
	.string "Um soco elétrico\n"
	.string "que pode\n"
	.string "paralisar o alvo.$"

BattleFrontier_Lounge7_Text_FirePunchDesc::
	.string "Um soco de fogo\n"
	.string "que pode\n"
	.string "queimar o alvo.$"
