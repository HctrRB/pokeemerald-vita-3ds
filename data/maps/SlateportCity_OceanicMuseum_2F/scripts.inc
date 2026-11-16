SlateportCity_OceanicMuseum_2F_MapScripts::
	.byte 0

SlateportCity_OceanicMuseum_2F_EventScript_CaptStern::
	lock
	faceplayer
	msgbox SlateportCity_OceanicMuseum_2F_Text_ThankYouForTheParts, MSGBOX_DEFAULT
	closemessage
	playbgm MUS_ENCOUNTER_AQUA, TRUE
	addobject LOCALID_OCEANIC_MUSEUM_2F_GRUNT_1
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_1, SlateportCity_OceanicMuseum_2F_Movement_FirstGruntEnter
	waitmovement 0
	addobject LOCALID_OCEANIC_MUSEUM_2F_GRUNT_2
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_2, SlateportCity_OceanicMuseum_2F_Movement_SecondGruntEnter
	waitmovement 0
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_1, SlateportCity_OceanicMuseum_2F_Movement_FirstGruntApproach
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_2, SlateportCity_OceanicMuseum_2F_Movement_SecondGruntApproach
	waitmovement 0
	call_if_eq VAR_FACING, DIR_SOUTH, SlateportCity_OceanicMuseum_2F_EventScript_PlayerFaceGrunts
	call_if_eq VAR_FACING, DIR_EAST, SlateportCity_OceanicMuseum_2F_EventScript_PlayerFaceGrunts
	msgbox SlateportCity_OceanicMuseum_2F_Text_WellTakeThoseParts, MSGBOX_DEFAULT
	call_if_ne VAR_FACING, DIR_EAST, SlateportCity_OceanicMuseum_2F_EventScript_SternFaceGrunts
	msgbox SlateportCity_OceanicMuseum_2F_Text_SternWhoAreYou, MSGBOX_DEFAULT
	msgbox SlateportCity_OceanicMuseum_2F_Text_WereTeamAqua, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_2, SlateportCity_OceanicMuseum_2F_Movement_GruntApproachToBattle
	waitmovement 0
	call_if_eq VAR_FACING, DIR_SOUTH, SlateportCity_OceanicMuseum_2F_EventScript_PlayerApproachGruntSouth
	call_if_eq VAR_FACING, DIR_WEST, SlateportCity_OceanicMuseum_2F_EventScript_PlayerApproachGruntWest
	trainerbattle_no_intro TRAINER_GRUNT_MUSEUM_1, SlateportCity_OceanicMuseum_2F_Text_Grunt1Defeat
	msgbox SlateportCity_OceanicMuseum_2F_Text_BossGoingToBeFurious, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_2, SlateportCity_OceanicMuseum_2F_Movement_GruntDefeated
	waitmovement 0
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_1, SlateportCity_OceanicMuseum_2F_Movement_GruntApproachToBattle
	waitmovement 0
	msgbox SlateportCity_OceanicMuseum_2F_Text_LetMeTakeCareOfThis, MSGBOX_DEFAULT
	trainerbattle_no_intro TRAINER_GRUNT_MUSEUM_2, SlateportCity_OceanicMuseum_2F_Text_Grunt2Defeat
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_1, SlateportCity_OceanicMuseum_2F_Movement_GruntDefeated
	waitmovement 0
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_1, Common_Movement_WalkInPlaceFasterDown
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_2, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox SlateportCity_OceanicMuseum_2F_Text_MeddlingKid, MSGBOX_DEFAULT
	closemessage
	delay 35
	addobject LOCALID_OCEANIC_MUSEUM_2F_ARCHIE
	applymovement LOCALID_OCEANIC_MUSEUM_2F_ARCHIE, SlateportCity_OceanicMuseum_2F_Movement_ArchieEnter
	applymovement LOCALID_OCEANIC_MUSEUM_2F_GRUNT_2, SlateportCity_OceanicMuseum_2F_Movement_GruntMoveForArchie
	waitmovement 0
	msgbox SlateportCity_OceanicMuseum_2F_Text_CameToSeeWhatsTakingSoLong, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_OCEANIC_MUSEUM_2F_ARCHIE, SlateportCity_OceanicMuseum_2F_Movement_ArchieApproachPlayer
	waitmovement 0
	msgbox SlateportCity_OceanicMuseum_2F_Text_ArchieWarning, MSGBOX_DEFAULT
	closemessage
	savebgm MUS_DUMMY
	fadedefaultbgm
	fadescreen FADE_TO_BLACK
	removeobject LOCALID_OCEANIC_MUSEUM_2F_ARCHIE
	removeobject LOCALID_OCEANIC_MUSEUM_2F_GRUNT_1
	removeobject LOCALID_OCEANIC_MUSEUM_2F_GRUNT_2
	fadescreen FADE_FROM_BLACK
	delay 30
	setflag FLAG_HIDE_SLATEPORT_CITY_OCEANIC_MUSEUM_AQUA_GRUNTS
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	msgbox SlateportCity_OceanicMuseum_2F_Text_SternThankYouForSavingUs, MSGBOX_DEFAULT
	setvar VAR_0x8004, ITEM_DEVON_GOODS
	call Common_EventScript_PlayerHandedOverTheItem
	msgbox SlateportCity_OceanicMuseum_2F_Text_SternIveGotToGo, MSGBOX_DEFAULT
	closemessage
	fadescreen FADE_TO_BLACK
	playfanfare MUS_HEAL
	waitfanfare
	special HealPlayerParty
	removeobject LOCALID_OCEANIC_MUSEUM_2F_CAPT_STERN
	setflag FLAG_HIDE_ROUTE_110_TEAM_AQUA
	call_if_eq VAR_REGISTER_BIRCH_STATE, 0, SlateportCity_OceanicMuseum_2F_EventScript_ReadyRegisterBirch
	setflag FLAG_DELIVERED_DEVON_GOODS
	clearflag FLAG_HIDE_ROUTE_116_DEVON_EMPLOYEE
	setflag FLAG_HIDE_RUSTBORO_CITY_DEVON_CORP_3F_EMPLOYEE
	setvar VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE, 1
	fadescreen FADE_FROM_BLACK
	release
	end

SlateportCity_OceanicMuseum_2F_EventScript_ReadyRegisterBirch::
	setvar VAR_REGISTER_BIRCH_STATE, 1
	return

SlateportCity_OceanicMuseum_2F_EventScript_PlayerFaceGrunts::
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	return

SlateportCity_OceanicMuseum_2F_EventScript_SternFaceGrunts::
	applymovement LOCALID_OCEANIC_MUSEUM_2F_CAPT_STERN, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	return

SlateportCity_OceanicMuseum_2F_EventScript_PlayerApproachGruntSouth::
	applymovement LOCALID_PLAYER, SlateportCity_OceanicMuseum_2F_Movement_PlayerApproachGruntSouth
	waitmovement 0
	return

SlateportCity_OceanicMuseum_2F_EventScript_PlayerApproachGruntWest::
	applymovement LOCALID_PLAYER, SlateportCity_OceanicMuseum_2F_Movement_PlayerApproachGruntWest
	waitmovement 0
	return

SlateportCity_OceanicMuseum_2F_Movement_PlayerApproachGruntSouth:
	walk_left
	walk_down
	walk_in_place_faster_left
	step_end

SlateportCity_OceanicMuseum_2F_Movement_PlayerApproachGruntWest:
	walk_up
	walk_left
	walk_left
	walk_down
	walk_in_place_faster_left
	step_end

SlateportCity_OceanicMuseum_2F_Movement_Unused:
	walk_up
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_up
	walk_up
	walk_left
	walk_left
	walk_up
	walk_up
	delay_8
	step_end

SlateportCity_OceanicMuseum_2F_Movement_ArchieApproachPlayer:
	walk_right
	step_end

SlateportCity_OceanicMuseum_2F_Movement_ArchieEnter:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

SlateportCity_OceanicMuseum_2F_Movement_GruntApproachToBattle:
	walk_right
	step_end

SlateportCity_OceanicMuseum_2F_Movement_FirstGruntEnter:
	walk_down
	walk_right
	walk_in_place_faster_down
	step_end

SlateportCity_OceanicMuseum_2F_Movement_FirstGruntApproach:
	walk_down
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	step_end

SlateportCity_OceanicMuseum_2F_Movement_GruntDefeated:
	lock_facing_direction
	walk_left
	unlock_facing_direction
	step_end

SlateportCity_OceanicMuseum_2F_Movement_SecondGruntEnter:
	walk_down
	step_end

SlateportCity_OceanicMuseum_2F_Movement_SecondGruntApproach:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

SlateportCity_OceanicMuseum_2F_Movement_GruntMoveForArchie:
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_8
	walk_fast_down
	walk_in_place_faster_up
	step_end

SlateportCity_OceanicMuseum_2F_EventScript_WaterQualitySample1::
	msgbox SlateportCity_OceanicMuseum_2F_Text_WaterQualitySample1, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_WaterQualitySample2::
	msgbox SlateportCity_OceanicMuseum_2F_Text_WaterQualitySample2, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_PressureExperiment::
	msgbox SlateportCity_OceanicMuseum_2F_Text_PressureExperiment, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_HoennModel::
	msgbox SlateportCity_OceanicMuseum_2F_Text_HoennModel, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_DeepSeawaterDisplay::
	msgbox SlateportCity_OceanicMuseum_2F_Text_DeepSeawaterDisplay, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_SurfaceSeawaterDisplay::
	msgbox SlateportCity_OceanicMuseum_2F_Text_SurfaceSeawaterDisplay, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_SSTidalReplica::
	msgbox SlateportCity_OceanicMuseum_2F_Text_SSTidalReplica, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_SubmarineReplica::
	msgbox SlateportCity_OceanicMuseum_2F_Text_SubmarineReplica, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_SubmersibleReplica::
	msgbox SlateportCity_OceanicMuseum_2F_Text_SumbersibleReplica, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_SSAnneReplica::
	msgbox SlateportCity_OceanicMuseum_2F_Text_SSAnneReplica, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_2F_EventScript_MuseumPatron1::
	msgbox SlateportCity_OceanicMuseum_2F_Text_RemindsMeOfAbandonedShip, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_2F_EventScript_MuseumPatron2::
	msgbox SlateportCity_OceanicMuseum_2F_Text_DontRunInMuseum, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_2F_EventScript_MuseumPatron3::
	lock
	faceplayer
	msgbox SlateportCity_OceanicMuseum_2F_Text_WantToRideSubmarine, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	release
	end

SlateportCity_OceanicMuseum_2F_Text_ThankYouForTheParts:
	.string "Sim? Se está procurando o STERN,\n"
	.string "sou eu mesmo.\p"
	.string "Ah! Devem ser as peças que encomendei\n"
	.string "do SR. STONE, da DEVON.\p"
	.string "Obrigado! Que ótimo!\n"
	.string "Agora podemos preparar a expedição.$"

SlateportCity_OceanicMuseum_2F_Text_WellTakeThoseParts:
	.string "Hehehe, alto lá!\n"
	.string "Essas peças são nossas!$"

SlateportCity_OceanicMuseum_2F_Text_SternWhoAreYou:
	.string "CAP. STERN: O-o quê?\n"
	.string "Quem são vocês?$"

SlateportCity_OceanicMuseum_2F_Text_WereTeamAqua:
	.string "Somos a EQUIPE AQUA!\p"
	.string "O nosso CHEFE quer essas peças!\n"
	.string "Fica quieto e passa pra cá!$"

SlateportCity_OceanicMuseum_2F_Text_Grunt1Defeat:
	.string "Aaaaaah!\n"
	.string "Uma criança me venceu!$"

SlateportCity_OceanicMuseum_2F_Text_BossGoingToBeFurious:
	.string "Ai, que desastre…\n"
	.string "O CHEFE vai ficar furioso…$"

SlateportCity_OceanicMuseum_2F_Text_LetMeTakeCareOfThis:
	.string "Hunf, seu bunda-mole chorão!\n"
	.string "Deixa que eu cuido disso!$"

SlateportCity_OceanicMuseum_2F_Text_Grunt2Defeat:
	.string "O quê?!\n"
	.string "Eu também perdi!$"

SlateportCity_OceanicMuseum_2F_Text_MeddlingKid:
	.string "E agora? Se a gente não pegar as peças,\n"
	.string "vai sobrar pra gente!\p"
	.string "Argh, não contava com uma criança\n"
	.string "enxerida se metendo!$"

SlateportCity_OceanicMuseum_2F_Text_CameToSeeWhatsTakingSoLong:
	.string "Vim ver por que estava demorando tanto\n"
	.string "pra pegar umas peças, e vocês,\l"
	.string "molengas, travados por uma criança?$"

SlateportCity_OceanicMuseum_2F_Text_ArchieWarning:
	.string "Nós somos a EQUIPE AQUA,\n"
	.string "e amamos o mar!\p"
	.string "E eu sou o líder da EQUIPE AQUA,\n"
	.string "ARCHIE!\p"
	.string "Por que você se mete com a gente?\p"
	.string "…Não! Não pode ser!\n"
	.string "Você é da EQUIPE MAGMA?\p"
	.string "Espera, isso não faz sentido.\n"
	.string "Essa roupa não é de lá.\p"
	.string "… … … … … …\n"
	.string "… … … … … …\p"
	.string "POKéMON, pessoas… Toda a vida\n"
	.string "depende do mar.\p"
	.string "Por isso, a EQUIPE AQUA se dedica\n"
	.string "a expandir o mar.\p"
	.string "Não concorda? O que fazemos é\n"
	.string "uma obra magnífica.\p"
	.string "Ah, tudo bem…\n"
	.string "Você ainda é jovem demais pra entender\l"
	.string "o nosso nobre objetivo.\p"
	.string "Mas eu aviso: nem pense em\n"
	.string "atrapalhar os nossos planos de novo.\p"
	.string "As consequências vão sair\n"
	.string "caro pra você!\p"
	.string "E não se esqueça disso!$"

SlateportCity_OceanicMuseum_2F_Text_SternThankYouForSavingUs:
	.string "CAP. STERN: Você é…\n"
	.string "Ah, claro, você é {PLAYER}{KUN}…\p"
	.string "Enfim, que situação tensa!\n"
	.string "Obrigado por nos salvar!\p"
	.string "Ah, é, quase esqueci que você\n"
	.string "ainda trouxe as peças da DEVON!$"

SlateportCity_OceanicMuseum_2F_Text_SternIveGotToGo:
	.string "CAP. STERN: Opa!\n"
	.string "Não há tempo a perder!\p"
	.string "Temos que partir pra nossa expedição\n"
	.string "ao fundo do mar logo, logo.\p"
	.string "Obrigado de novo, mas agora\n"
	.string "preciso ir!\p"
	.string "Mas fique à vontade pra passear e\n"
	.string "conhecer as nossas instalações.$"

SlateportCity_OceanicMuseum_2F_Text_RemindsMeOfAbandonedShip:
	.string "Vi uma maquete de navio aqui.\p"
	.string "Me lembrou do NAVIO ABANDONADO\n"
	.string "perto da CIDADE DE DEWFORD…$"

SlateportCity_OceanicMuseum_2F_Text_DontRunInMuseum:
	.string "Nem pense em sair correndo dentro\n"
	.string "do MUSEU!$"

SlateportCity_OceanicMuseum_2F_Text_WantToRideSubmarine:
	.string "Uau, o submarino é demais!\n"
	.string "Quero dar uma volta nele!$"

SlateportCity_OceanicMuseum_2F_Text_WaterQualitySample1:
	.string "“AMOSTRA DE ÁGUA 1”, diz a\n"
	.string "etiqueta.\p"
	.string "O mar é todo ligado, mas a água\n"
	.string "parece mudar de região pra região.$"

SlateportCity_OceanicMuseum_2F_Text_WaterQualitySample2:
	.string "“AMOSTRA DE ÁGUA 2”, diz a\n"
	.string "etiqueta.\p"
	.string "Será que o sal do mar também muda\n"
	.string "de região pra região?$"

SlateportCity_OceanicMuseum_2F_Text_PressureExperiment:
	.string "Uma bola de borracha estica e\n"
	.string "encolhe.\p"
	.string "“No mar, o próprio peso da água\n"
	.string "faz pressão.\p"
	.string "“Em águas rasas, a pressão não\n"
	.string "é muito forte.\p"
	.string "“Mas, em águas muito profundas, a\n"
	.string "pressão chega a dezenas de milhares\l"
	.string "de toneladas numa área pequena.”$"

SlateportCity_OceanicMuseum_2F_Text_HoennModel:
	.string "“MAQUETE DA REGIÃO DE HOENN”\p"
	.string "É uma miniatura da região\n"
	.string "de HOENN.\p"
	.string "Onde fica a CIDADE DE LITTLEROOT aqui?$"

SlateportCity_OceanicMuseum_2F_Text_DeepSeawaterDisplay:
	.string "É uma exposição sobre o fluxo da água\n"
	.string "do mar.\p"
	.string "“Perto do fundo do mar, a água corre\n"
	.string "por causa de diferenças de\l"
	.string "temperatura e salinidade.”$"

SlateportCity_OceanicMuseum_2F_Text_SurfaceSeawaterDisplay:
	.string "É uma exposição sobre o fluxo da água\n"
	.string "do mar.\p"
	.string "“Perto da superfície, a água do mar\n"
	.string "corre em correntes levadas pelo vento.”$"

SlateportCity_OceanicMuseum_2F_Text_SSTidalReplica:
	.string "“A BALSA S.S. TIDAL\p"
	.string "“Réplica em escala do navio em\n"
	.string "construção no ESTALEIRO DO STERN.”$"

SlateportCity_OceanicMuseum_2F_Text_SubmarineReplica:
	.string "“SUBMARINO EXPLORADOR 1\p"
	.string "“Réplica do submarino potente\n"
	.string "para explorar o fundo do mar.”$"

SlateportCity_OceanicMuseum_2F_Text_SumbersibleReplica:
	.string "“CÁPSULA SUBMERSÍVEL\p"
	.string "“Réplica de uma cápsula compacta e não\n"
	.string "tripulada para explorar o fundo do mar.”$"

SlateportCity_OceanicMuseum_2F_Text_SSAnneReplica:
	.string "“S.S. ANNE\p"
	.string "“Réplica do navio de luxo que dá\n"
	.string "a volta ao mundo.”$"

