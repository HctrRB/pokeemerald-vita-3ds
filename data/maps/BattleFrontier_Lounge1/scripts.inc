BattleFrontier_Lounge1_MapScripts::
	.byte 0

/* NPC that rates pokemon based on their IVs*/
BattleFrontier_Lounge1_EventScript_Breeder::
	lock
	faceplayer
	call_if_unset FLAG_MET_BATTLE_FRONTIER_BREEDER, BattleFrontier_Lounge1_EventScript_BreederIntro
	call_if_set FLAG_MET_BATTLE_FRONTIER_BREEDER, BattleFrontier_Lounge1_EventScript_AlreadyMetBreeder
	setflag FLAG_MET_BATTLE_FRONTIER_BREEDER
	goto BattleFrontier_Lounge1_EventScript_ChooseMonToShowBreeder
	end

BattleFrontier_Lounge1_EventScript_ChooseMonToShowBreeder::
	special ChoosePartyMon
	waitstate
	goto_if_ne VAR_0x8004, PARTY_NOTHING_CHOSEN, BattleFrontier_Lounge1_EventScript_ShowMonToBreeder
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, BattleFrontier_Lounge1_EventScript_CancelMonSelect
	end

BattleFrontier_Lounge1_EventScript_BreederIntro::
	msgbox BattleFrontier_Lounge1_Text_PokemonBreederIntro, MSGBOX_DEFAULT
	return

BattleFrontier_Lounge1_EventScript_AlreadyMetBreeder::
	msgbox BattleFrontier_Lounge1_Text_LetsLookAtYourPokemon, MSGBOX_DEFAULT
	return

/* BufferVarsForIVRater buffers the following values*/
/* VAR_0x8005: Sum of the mons IVs*/
/* VAR_0x8006: Stat id of highest IV stat*/
/* VAR_0x8007: IV of the highest IV stat*/
BattleFrontier_Lounge1_EventScript_ShowMonToBreeder::
	specialvar VAR_RESULT, ScriptGetPartyMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, BattleFrontier_Lounge1_EventScript_ShowEggToBreeder
	special BufferVarsForIVRater
	goto_if_le VAR_0x8005, 90, BattleFrontier_Lounge1_EventScript_AverageTotalIVs       /* Average of 15*/
	goto_if_le VAR_0x8005, 120, BattleFrontier_Lounge1_EventScript_AboveAverageTotalIVs /* Average of 20*/
	goto_if_le VAR_0x8005, 150, BattleFrontier_Lounge1_EventScript_HighTotalIVs         /* Average of 25*/
	goto_if_ge VAR_0x8005, 151, BattleFrontier_Lounge1_EventScript_VeryHighTotalIVs     /* Average of > 25*/
	end

BattleFrontier_Lounge1_EventScript_ShowEggToBreeder::
	msgbox BattleFrontier_Lounge1_Text_EvenICantTell, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_ChooseMonToShowBreeder
	end

/* Comment on the highest IV stat*/
BattleFrontier_Lounge1_EventScript_HighestIVStat::
	goto_if_eq VAR_0x8006, STAT_HP, BattleFrontier_Lounge1_EventScript_HighestIVHP
	goto_if_eq VAR_0x8006, STAT_ATK, BattleFrontier_Lounge1_EventScript_HighestIVAtk
	goto_if_eq VAR_0x8006, STAT_DEF, BattleFrontier_Lounge1_EventScript_HighestIVDef
	goto_if_eq VAR_0x8006, STAT_SPEED, BattleFrontier_Lounge1_EventScript_HighestIVSpeed
	goto_if_eq VAR_0x8006, STAT_SPATK, BattleFrontier_Lounge1_EventScript_HighestIVSpAtk
	goto_if_eq VAR_0x8006, STAT_SPDEF, BattleFrontier_Lounge1_EventScript_HighestIVSpDef
	end

/* Comment on the highest IV value*/
BattleFrontier_Lounge1_EventScript_HighestIVValue::
	goto_if_le VAR_0x8007, 15, BattleFrontier_Lounge1_EventScript_HighestIVLow
	goto_if_le VAR_0x8007, 25, BattleFrontier_Lounge1_EventScript_HighestIVMid
	goto_if_le VAR_0x8007, 30, BattleFrontier_Lounge1_EventScript_HighestIVHigh
	goto_if_ge VAR_0x8007, 31, BattleFrontier_Lounge1_EventScript_HighestIVMax
	end

BattleFrontier_Lounge1_EventScript_EndBreederComments::
	release
	end

BattleFrontier_Lounge1_EventScript_AverageTotalIVs::
	msgbox BattleFrontier_Lounge1_Text_AverageAbility, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVStat
	end

BattleFrontier_Lounge1_EventScript_AboveAverageTotalIVs::
	msgbox BattleFrontier_Lounge1_Text_BetterThanAverageAbility, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVStat
	end

BattleFrontier_Lounge1_EventScript_HighTotalIVs::
	msgbox BattleFrontier_Lounge1_Text_ImpressiveAbility, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVStat
	end

BattleFrontier_Lounge1_EventScript_VeryHighTotalIVs::
	msgbox BattleFrontier_Lounge1_Text_OutstandingAbility, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVStat
	end

BattleFrontier_Lounge1_EventScript_HighestIVHP::
	msgbox BattleFrontier_Lounge1_Text_BestAspectHP, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVValue
	end

BattleFrontier_Lounge1_EventScript_HighestIVAtk::
	msgbox BattleFrontier_Lounge1_Text_BestAspectAtk, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVValue
	end

BattleFrontier_Lounge1_EventScript_HighestIVDef::
	msgbox BattleFrontier_Lounge1_Text_BestAspectDef, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVValue
	end

BattleFrontier_Lounge1_EventScript_HighestIVSpeed::
	msgbox BattleFrontier_Lounge1_Text_BestAspectSpeed, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVValue
	end

BattleFrontier_Lounge1_EventScript_HighestIVSpAtk::
	msgbox BattleFrontier_Lounge1_Text_BestAspectSpAtk, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVValue
	end

BattleFrontier_Lounge1_EventScript_HighestIVSpDef::
	msgbox BattleFrontier_Lounge1_Text_BestAspectSpDef, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_HighestIVValue
	end

BattleFrontier_Lounge1_EventScript_HighestIVLow::
	msgbox BattleFrontier_Lounge1_Text_StatRelativelyGood, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_EndBreederComments
	end

BattleFrontier_Lounge1_EventScript_HighestIVMid::
	msgbox BattleFrontier_Lounge1_Text_StatImpressive, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_EndBreederComments
	end

BattleFrontier_Lounge1_EventScript_HighestIVHigh::
	msgbox BattleFrontier_Lounge1_Text_StatOutstanding, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_EndBreederComments
	end

BattleFrontier_Lounge1_EventScript_HighestIVMax::
	msgbox BattleFrontier_Lounge1_Text_StatFlawless, MSGBOX_DEFAULT
	goto BattleFrontier_Lounge1_EventScript_EndBreederComments
	end

BattleFrontier_Lounge1_EventScript_CancelMonSelect::
	msgbox BattleFrontier_Lounge1_Text_NoTimeForMyAdvice, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Lounge1_EventScript_Boy1::
	msgbox BattleFrontier_Lounge1_Text_SaidMyMonIsOutstanding, MSGBOX_NPC
	end

BattleFrontier_Lounge1_EventScript_Boy2::
	msgbox BattleFrontier_Lounge1_Text_DidntDoAnythingSpecialRaisingIt, MSGBOX_NPC
	end

BattleFrontier_Lounge1_Text_PokemonBreederIntro:
	.string "Crio POKéMON há 70 anos!\n"
	.string "Sou o homem que veneram como o\l"
	.string "lendário maior CRIADOR POKéMON!\p"
	.string "Quando tiver a minha experiência,\n"
	.string "vai ver as qualidades dos POKéMON\l"
	.string "num piscar de olhos.\p"
	.string "Você é TREINADOR. Não quer saber\n"
	.string "das qualidades dos seus POKéMON?\p"
	.string "Aqui!\n"
	.string "Vamos ver os seus POKéMON!$"

BattleFrontier_Lounge1_Text_AverageAbility:
	.string "…Hum…\p"
	.string "Este, no geral, eu diria que tem\n"
	.string "qualidades medianas.$"

BattleFrontier_Lounge1_Text_BetterThanAverageAbility:
	.string "…Hum…\p"
	.string "Este, no geral, eu diria que tem\n"
	.string "qualidades acima da média.$"

BattleFrontier_Lounge1_Text_ImpressiveAbility:
	.string "…Hum…\p"
	.string "Este, no geral, eu diria que tem\n"
	.string "qualidades impressionantes!$"

BattleFrontier_Lounge1_Text_OutstandingAbility:
	.string "…Hum…\p"
	.string "Este, no geral, eu diria que tem\n"
	.string "qualidades excepcionais!$"

BattleFrontier_Lounge1_Text_BestAspectHP:
	.string "Aliás, o melhor dele, eu diria,\n"
	.string "são os PS…$"

BattleFrontier_Lounge1_Text_BestAspectAtk:
	.string "Aliás, o melhor dele, eu diria,\n"
	.string "é o ATAQUE…$"

BattleFrontier_Lounge1_Text_BestAspectDef:
	.string "Aliás, o melhor dele, eu diria,\n"
	.string "é a DEFESA…$"

BattleFrontier_Lounge1_Text_BestAspectSpAtk:
	.string "Aliás, o melhor dele, eu diria,\n"
	.string "é o ATAQUE ESPECIAL…$"

BattleFrontier_Lounge1_Text_BestAspectSpDef:
	.string "Aliás, o melhor dele, eu diria,\n"
	.string "é a DEFESA ESPECIAL…$"

BattleFrontier_Lounge1_Text_BestAspectSpeed:
	.string "Aliás, o melhor dele, eu diria,\n"
	.string "é a VELOCIDADE…$"

BattleFrontier_Lounge1_Text_StatRelativelyGood:
	.string "Esse atributo é relativamente bom.\n"
	.string "…Hum… É o que eu acho.$"

BattleFrontier_Lounge1_Text_StatImpressive:
	.string "Esse atributo é impressionante.\n"
	.string "…Hum… É o que eu acho.$"

BattleFrontier_Lounge1_Text_StatOutstanding:
	.string "Esse atributo é excepcional!\n"
	.string "…Hum… É o que eu acho.$"

BattleFrontier_Lounge1_Text_StatFlawless:
	.string "Impecável! Uma perfeição!\n"
	.string "…Hum… É o que eu acho.$"

BattleFrontier_Lounge1_Text_NoTimeForMyAdvice:
	.string "Quê?\n"
	.string "Não tem tempo pros meus conselhos?\p"
	.string "Devia ter vontade de aprender com\n"
	.string "a experiência dos mais velhos!$"

/* Unused*/
BattleFrontier_Lounge1_Text_HaveBusinessNeedsTending:
	.string "Sim, o que foi agora?\p"
	.string "Tenho assuntos pra resolver!\n"
	.string "Deixe pra próxima!$"

BattleFrontier_Lounge1_Text_LetsLookAtYourPokemon:
	.string "Ah, jovem! Tem curiosidade sobre as\n"
	.string "qualidades dos seus POKéMON?\p"
	.string "Aqui, aqui!\n"
	.string "Vamos ver os seus POKéMON!$"

BattleFrontier_Lounge1_Text_EvenICantTell:
	.string "Sou especialista, mas nem eu sei\n"
	.string "dizer nada de um POKéMON no OVO!\p"
	.string "Mostre um POKéMON!\n"
	.string "É um POKéMON que eu preciso ver!$"

BattleFrontier_Lounge1_Text_SaidMyMonIsOutstanding:
	.string "Ele disse que o meu POKéMON é\n"
	.string "excepcional! Que bom que criei ele\l"
	.string "com cuidado!$"

BattleFrontier_Lounge1_Text_DidntDoAnythingSpecialRaisingIt:
	.string "Ele disse que o meu POKéMON é\n"
	.string "excepcional! Mas eu não fiz nada\l"
	.string "de especial pra criar ele…$"

