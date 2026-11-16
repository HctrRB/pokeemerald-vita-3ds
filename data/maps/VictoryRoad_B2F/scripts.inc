VictoryRoad_B2F_MapScripts::
	.byte 0

VictoryRoad_B2F_EventScript_Vito::
	trainerbattle_single TRAINER_VITO, VictoryRoad_B2F_Text_VitoIntro, VictoryRoad_B2F_Text_VitoDefeat
	msgbox VictoryRoad_B2F_Text_VitoPostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B2F_EventScript_Owen::
	trainerbattle_single TRAINER_OWEN, VictoryRoad_B2F_Text_OwenIntro, VictoryRoad_B2F_Text_OwenDefeat
	msgbox VictoryRoad_B2F_Text_OwenPostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B2F_EventScript_Caroline::
	trainerbattle_single TRAINER_CAROLINE, VictoryRoad_B2F_Text_CarolineIntro, VictoryRoad_B2F_Text_CarolineDefeat
	msgbox VictoryRoad_B2F_Text_CarolinePostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B2F_EventScript_Julie::
	trainerbattle_single TRAINER_JULIE, VictoryRoad_B2F_Text_JulieIntro, VictoryRoad_B2F_Text_JulieDefeat
	msgbox VictoryRoad_B2F_Text_JuliePostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B2F_EventScript_Felix::
	trainerbattle_single TRAINER_FELIX, VictoryRoad_B2F_Text_FelixIntro, VictoryRoad_B2F_Text_FelixDefeat
	msgbox VictoryRoad_B2F_Text_FelixPostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B2F_EventScript_Dianne::
	trainerbattle_single TRAINER_DIANNE, VictoryRoad_B2F_Text_DianneIntro, VictoryRoad_B2F_Text_DianneDefeat
	msgbox VictoryRoad_B2F_Text_DiannePostBattle, MSGBOX_AUTOCLOSE
	end

VictoryRoad_B2F_Text_VitoIntro:
	.string "Treinei com a minha família inteira,\n"
	.string "cada um de nós!\l"
	.string "Não perco pra ninguém!$"

VictoryRoad_B2F_Text_VitoDefeat:
	.string "Melhor que a minha família?!\n"
	.string "Como é possível?!$"

VictoryRoad_B2F_Text_VitoPostBattle:
	.string "Eu era o melhor da minha família.\n"
	.string "Nunca tinha perdido…\p"
	.string "Perdi a confiança…\n"
	.string "Acho que vou pra casa…$"

VictoryRoad_B2F_Text_OwenIntro:
	.string "Ouvi dizer que tinha uma criança forte\n"
	.string "por aí. Estão falando de você?$"

VictoryRoad_B2F_Text_OwenDefeat:
	.string "Essa tampinha é forte!$"

VictoryRoad_B2F_Text_OwenPostBattle:
	.string "Pelos boatos, essa criança forte é\n"
	.string "da CIDADE DE PETALBURG.$"

VictoryRoad_B2F_Text_CarolineIntro:
	.string "Você já deve estar sem fôlego.$"

VictoryRoad_B2F_Text_CarolineDefeat:
	.string "Nenhum sinal de cansaço!$"

VictoryRoad_B2F_Text_CarolinePostBattle:
	.string "A ESTRADA DA VITÓRIA e a LIGA POKéMON\n"
	.string "são desafios longos e puxados.\l"
	.string "Cuidado com o cansaço!$"

VictoryRoad_B2F_Text_JulieIntro:
	.string "Não se acomode só porque tem muitas\n"
	.string "INSÍGNIAS.\p"
	.string "Sempre vai existir alguém melhor\n"
	.string "que você!$"

VictoryRoad_B2F_Text_JulieDefeat:
	.string "Você é melhor que eu!$"

VictoryRoad_B2F_Text_JuliePostBattle:
	.string "Olhe as INSÍGNIAS que juntou e\n"
	.string "lembre dos TREINADORES que enfrentou.$"

VictoryRoad_B2F_Text_FelixIntro:
	.string "Cheguei até aqui, mas o nervoso está\n"
	.string "me dando uma dor de barriga horrível…$"

VictoryRoad_B2F_Text_FelixDefeat:
	.string "Ui…\n"
	.string "Dói…$"

VictoryRoad_B2F_Text_FelixPostBattle:
	.string "Não tem como não ficar nervoso\n"
	.string "sabendo que estou perto da LIGA.\p"
	.string "O máximo que consigo é fingir calma.$"

VictoryRoad_B2F_Text_DianneIntro:
	.string "A elite da elite se reúne nesta\n"
	.string "caverna.\p"
	.string "O que está achando?$"

VictoryRoad_B2F_Text_DianneDefeat:
	.string "Nem se abalou!$"

VictoryRoad_B2F_Text_DiannePostBattle:
	.string "Você tem garra! Gosto disso!\n"
	.string "Continue assim!$"

