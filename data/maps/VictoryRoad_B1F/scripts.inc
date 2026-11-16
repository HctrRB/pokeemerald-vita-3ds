VictoryRoad_B1F_MapScripts::
	.byte 0

VictoryRoad_B1F_EventScript_Samuel::
	trainerbattle_single TRAINER_SAMUEL, VictoryRoad_B1F_Text_SamuelIntro, VictoryRoad_B1F_Text_SamuelDefeat
	msgbox VictoryRoad_B1F_Text_SamuelPostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B1F_EventScript_Shannon::
	trainerbattle_single TRAINER_SHANNON, VictoryRoad_B1F_Text_ShannonIntro, VictoryRoad_B1F_Text_ShannonDefeat
	msgbox VictoryRoad_B1F_Text_ShannonPostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B1F_EventScript_Michelle::
	trainerbattle_single TRAINER_MICHELLE, VictoryRoad_B1F_Text_MichelleIntro, VictoryRoad_B1F_Text_MichelleDefeat
	msgbox VictoryRoad_B1F_Text_MichellePostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B1F_EventScript_Mitchell::
	trainerbattle_single TRAINER_MITCHELL, VictoryRoad_B1F_Text_MitchellIntro, VictoryRoad_B1F_Text_MitchellDefeat
	msgbox VictoryRoad_B1F_Text_MitchellPostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B1F_EventScript_Halle::
	trainerbattle_single TRAINER_HALLE, VictoryRoad_B1F_Text_HalleIntro, VictoryRoad_B1F_Text_HalleDefeat
	msgbox VictoryRoad_B1F_Text_HallePostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B1F_Text_SamuelIntro:
	.string "Só de pensar que estou chegando\n"
	.string "perto da LIGA POKéMON…\p"
	.string "Me dá um frio na barriga…$"

VictoryRoad_B1F_Text_SamuelDefeat:
	.string "Não consegui fazer nada…$"

VictoryRoad_B1F_Text_SamuelPostBattle:
	.string "A LIGA POKéMON ficou longe de\n"
	.string "novo… Que decepção…$"

VictoryRoad_B1F_Text_ShannonIntro:
	.string "Pra vencer a LIGA POKéMON, você\n"
	.string "precisa da confiança dos seus\l"
	.string "POKéMON.$"

VictoryRoad_B1F_Text_ShannonDefeat:
	.string "A relação de vocês se baseia numa\n"
	.string "confiança sólida.$"

VictoryRoad_B1F_Text_ShannonPostBattle:
	.string "Ficando juntos o tempo todo, a\n"
	.string "confiança cresce entre POKéMON e\l"
	.string "TREINADORES.$"

VictoryRoad_B1F_Text_MichelleIntro:
	.string "Aqui não é a chegada. É só um ponto\n"
	.string "no caminho pra LIGA POKéMON.$"

VictoryRoad_B1F_Text_MichelleDefeat:
	.string "É assim que se faz!$"

VictoryRoad_B1F_Text_MichellePostBattle:
	.string "Você vai se sair bem, com certeza!\n"
	.string "Os seus POKéMON estão loucos pra ir!$"

VictoryRoad_B1F_Text_MitchellIntro:
	.string "Os meus POKéMON são cosmicamente\n"
	.string "impressionantes!$"

VictoryRoad_B1F_Text_MitchellDefeat:
	.string "Nunca conheci ninguém como você.$"

VictoryRoad_B1F_Text_MitchellPostBattle:
	.string "Mesmo fora da batalha, sinto um poder\n"
	.string "incrível vindo de você e dos seus\l"
	.string "POKéMON.$"

VictoryRoad_B1F_Text_HalleIntro:
	.string "Calma, não precisa se irritar!\n"
	.string "Relaxa, vamos com calma!$"

VictoryRoad_B1F_Text_HalleDefeat:
	.string "Uau!\n"
	.string "Maravilha!$"

VictoryRoad_B1F_Text_HallePostBattle:
	.string "Claro, esta é a ESTRADA DA VITÓRIA.\p"
	.string "Mas não é tão diferente do caminho\n"
	.string "que você fez até agora.\p"
	.string "Tente curtir o resto do caminho!$"

