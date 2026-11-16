BattleFrontier_Lounge5_MapScripts::
	.byte 0

BattleFrontier_Lounge5_EventScript_NatureGirl::
	lock
	faceplayer
	msgbox BattleFrontier_Lounge5_Text_NatureGirlGreeting, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, BattleFrontier_Lounge5_EventScript_NatureGirlNoneShown
	special ChoosePartyMon
	waitstate
	lock
	faceplayer
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, BattleFrontier_Lounge5_EventScript_NatureGirlNoneShown
	specialvar VAR_RESULT, ScriptGetPartyMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, BattleFrontier_Lounge5_EventScript_NatureGirlEgg
	special ShowNatureGirlMessage
	waitmessage
	waitbuttonpress
	release
	end

BattleFrontier_Lounge5_EventScript_NatureGirlEgg::
	msgbox BattleFrontier_Lounge5_Text_NatureGirlEgg, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Lounge5_EventScript_NatureGirlNoneShown::
	msgbox BattleFrontier_Lounge5_Text_NatureGirlNoneShown, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Lounge5_EventScript_Gentleman::
	msgbox BattleFrontier_Lounge5_Text_LadyClaimsSheUnderstandsPokemon, MSGBOX_NPC
	end

BattleFrontier_Lounge5_EventScript_BlackBelt::
	msgbox BattleFrontier_Lounge5_Text_GirlSayingSomethingProfound, MSGBOX_NPC
	end

BattleFrontier_Lounge5_EventScript_LittleBoy::
	msgbox BattleFrontier_Lounge5_Text_GirlPlaysAtRedHouseALot, MSGBOX_NPC
	end

BattleFrontier_Lounge5_Text_NatureGirlGreeting::
	.string "Ehehe!\n"
	.string "Eu sei o que os POKéMON pensam!\p"
	.string "Por favor!\n"
	.string "Posso ver os seus POKéMON?$"

BattleFrontier_Lounge5_Text_NatureGirlNoneShown::
	.string "Buu!\n"
	.string "Mão de vaca!$"

BattleFrontier_Lounge5_Text_NatureGirlHardy::
	.string "Humhum…\p"
	.string "Este diz que gosta de batalhar!\n"
	.string "Batalha mesmo com muito dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlLonely::
	.string "Humhum…\p"
	.string "Este diz que gosta de ser sapeca!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele revida!$"

BattleFrontier_Lounge5_Text_NatureGirlBrave::
	.string "Humhum…\p"
	.string "Este diz que gosta de batalhar!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele se preocupa consigo!$"

BattleFrontier_Lounge5_Text_NatureGirlAdamant::
	.string "Humhum…\p"
	.string "Este diz que gosta de batalhar!\n"
	.string "Batalha mesmo com muito dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlNaughty::
	.string "Humhum…\p"
	.string "Este diz que se cuida!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele revida!$"

BattleFrontier_Lounge5_Text_NatureGirlBold::
	.string "Humhum…\p"
	.string "Este diz que gosta de ser sapeca!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele se preocupa consigo!$"

BattleFrontier_Lounge5_Text_NatureGirlDocileNaiveQuietQuirky::
	.string "Humhum…\p"
	.string "Este diz que gosta de batalhar!\n"
	.string "Batalha mesmo com muito dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlRelaxed::
	.string "Humhum…\p"
	.string "Este diz que gosta de ser sapeca!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele revida!$"

BattleFrontier_Lounge5_Text_NatureGirlImpish::
	.string "Humhum…\p"
	.string "Este diz que gosta de batalhar!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele se preocupa consigo!$"

BattleFrontier_Lounge5_Text_NatureGirlLax::
	.string "Humhum…\p"
	.string "Este diz que gosta de ser sapeca!\n"
	.string "É sapeca mesmo com muito dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlTimid::
	.string "Humhum…\p"
	.string "Este diz que gosta de batalhar!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele fica sapeca!$"

BattleFrontier_Lounge5_Text_NatureGirlHasty::
	.string "Humhum…\p"
	.string "Este diz que gosta de batalhar!\n"
	.string "Batalha mesmo com muito dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlSerious::
	.string "Humhum…\p"
	.string "Este diz que gosta de ser sapeca!\n"
	.string "É sapeca mesmo com muito dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlJolly::
	.string "Humhum…\p"
	.string "Este diz que gosta de ser sapeca!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele se preocupa consigo!$"

BattleFrontier_Lounge5_Text_NatureGirlModest::
	.string "Humhum…\p"
	.string "Este diz que se cuida!\n"
	.string "Se preocupa consigo, tendo muito\l"
	.string "ou pouco dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlMild::
	.string "Humhum…\p"
	.string "Este diz que se cuida!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele fica sapeca!$"

BattleFrontier_Lounge5_Text_NatureGirlBashful::
	.string "Humhum…\p"
	.string "Este diz que se cuida!\n"
	.string "Se preocupa consigo mesmo com\l"
	.string "muito dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlRash::
	.string "Humhum…\p"
	.string "Este diz que gosta de ser sapeca!\n"
	.string "É sapeca mesmo com muito dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlCalm::
	.string "Humhum…\p"
	.string "Este diz que se cuida!\n"
	.string "Se preocupa consigo mesmo com\l"
	.string "muito dodói!$"

BattleFrontier_Lounge5_Text_NatureGirlGentle::
	.string "Humhum…\p"
	.string "Este diz que se cuida!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele revida!$"

BattleFrontier_Lounge5_Text_NatureGirlSassy::
	.string "Humhum…\p"
	.string "Este diz que gosta de batalhar!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele fica sapeca!$"

BattleFrontier_Lounge5_Text_NatureGirlCareful::
	.string "Humhum…\p"
	.string "Este diz que se cuida!\n"
	.string "Mas, se tiver dodói demais,\l"
	.string "ele fica sapeca!$"

BattleFrontier_Lounge5_Text_NatureGirlEgg::
	.string "Que bobagem! O OVO está dormindo!\n"
	.string "Não dá pra falar com ele!$"

BattleFrontier_Lounge5_Text_LadyClaimsSheUnderstandsPokemon::
	.string "Que graça!\n"
	.string "Aquela mocinha diz que entende\l"
	.string "os POKéMON!$"

BattleFrontier_Lounge5_Text_GirlSayingSomethingProfound::
	.string "Tenho a impressão de que a menininha\n"
	.string "está dizendo algo profundo.$"

BattleFrontier_Lounge5_Text_GirlPlaysAtRedHouseALot::
	.string "Eu sei uma coisa!\p"
	.string "Aquela menininha brinca muito na\n"
	.string "casa vermelha!$"

