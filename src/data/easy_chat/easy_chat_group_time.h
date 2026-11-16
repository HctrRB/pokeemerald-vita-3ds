const u8 gEasyChatWord_Fall[] = _("OUTONO");
const u8 gEasyChatWord_Morning[] = _("MANHÃ");
const u8 gEasyChatWord_Tomorrow[] = _("AMANHÃ");
const u8 gEasyChatWord_Last[] = _("ÚLTIMO");
const u8 gEasyChatWord_Day[] = _("DIA");
const u8 gEasyChatWord_Sometime[] = _("ALGUM DIA");
const u8 gEasyChatWord_Always[] = _("SEMPRE");
const u8 gEasyChatWord_Current[] = _("ATUAL");
const u8 gEasyChatWord_Forever[] = _("PARA SEMPRE");
const u8 gEasyChatWord_Days[] = _("DIAS");
const u8 gEasyChatWord_End[] = _("FIM");
const u8 gEasyChatWord_Tuesday[] = _("TERÇA");
const u8 gEasyChatWord_Yesterday[] = _("ONTEM");
const u8 gEasyChatWord_Today[] = _("HOJE");
const u8 gEasyChatWord_Friday[] = _("SEXTA");
const u8 gEasyChatWord_Monday[] = _("SEGUNDA");
const u8 gEasyChatWord_Later[] = _("DEPOIS");
const u8 gEasyChatWord_Earlier[] = _("MAIS CEDO");
const u8 gEasyChatWord_Another[] = _("OUTRO");
const u8 gEasyChatWord_Time[] = _("TEMPO");
const u8 gEasyChatWord_Finish[] = _("TERMINAR");
const u8 gEasyChatWord_Wednesday[] = _("QUARTA");
const u8 gEasyChatWord_Soon[] = _("LOGO");
const u8 gEasyChatWord_Start[] = _("COMEÇAR");
const u8 gEasyChatWord_Month[] = _("MÊS");
const u8 gEasyChatWord_Stop[] = _("PARAR");
const u8 gEasyChatWord_Now[] = _("AGORA");
const u8 gEasyChatWord_Final[] = _("FINAL");
const u8 gEasyChatWord_Next[] = _("PRÓXIMO");
const u8 gEasyChatWord_Age[] = _("IDADE");
const u8 gEasyChatWord_Saturday[] = _("SÁBADO");
const u8 gEasyChatWord_Summer[] = _("VERÃO");
const u8 gEasyChatWord_Sunday[] = _("DOMINGO");
const u8 gEasyChatWord_Beginning[] = _("COMEÇO");
const u8 gEasyChatWord_Spring[] = _("PRIMAVERA");
const u8 gEasyChatWord_Daytime[] = _("DE DIA");
const u8 gEasyChatWord_Winter[] = _("INVERNO");
const u8 gEasyChatWord_Daily[] = _("DIÁRIO");
const u8 gEasyChatWord_Olden[] = _("ANTIGAMENTE");
const u8 gEasyChatWord_Almost[] = _("QUASE");
const u8 gEasyChatWord_Nearly[] = _("POR POUCO");
const u8 gEasyChatWord_Thursday[] = _("QUINTA");
const u8 gEasyChatWord_Nighttime[] = _("À NOITE");
const u8 gEasyChatWord_Night[] = _("NOITE");
const u8 gEasyChatWord_Week[] = _("SEMANA");

const struct EasyChatWordInfo gEasyChatGroup_Time[] = {
    [EC_INDEX(EC_WORD_FALL)] =
    {
        .text = gEasyChatWord_Fall,
        .alphabeticalOrder = EC_INDEX(EC_WORD_NIGHTTIME),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_MORNING)] =
    {
        .text = gEasyChatWord_Morning,
        .alphabeticalOrder = EC_INDEX(EC_WORD_NOW),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TOMORROW)] =
    {
        .text = gEasyChatWord_Tomorrow,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SOMETIME),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_LAST)] =
    {
        .text = gEasyChatWord_Last,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TOMORROW),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_DAY)] =
    {
        .text = gEasyChatWord_Day,
        .alphabeticalOrder = EC_INDEX(EC_WORD_OLDEN),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SOMETIME)] =
    {
        .text = gEasyChatWord_Sometime,
        .alphabeticalOrder = EC_INDEX(EC_WORD_CURRENT),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_ALWAYS)] =
    {
        .text = gEasyChatWord_Always,
        .alphabeticalOrder = EC_INDEX(EC_WORD_START),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_CURRENT)] =
    {
        .text = gEasyChatWord_Current,
        .alphabeticalOrder = EC_INDEX(EC_WORD_BEGINNING),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_FOREVER)] =
    {
        .text = gEasyChatWord_Forever,
        .alphabeticalOrder = EC_INDEX(EC_WORD_DAYTIME),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_DAYS)] =
    {
        .text = gEasyChatWord_Days,
        .alphabeticalOrder = EC_INDEX(EC_WORD_LATER),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_END)] =
    {
        .text = gEasyChatWord_End,
        .alphabeticalOrder = EC_INDEX(EC_WORD_DAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TUESDAY)] =
    {
        .text = gEasyChatWord_Tuesday,
        .alphabeticalOrder = EC_INDEX(EC_WORD_DAILY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_YESTERDAY)] =
    {
        .text = gEasyChatWord_Yesterday,
        .alphabeticalOrder = EC_INDEX(EC_WORD_DAYS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TODAY)] =
    {
        .text = gEasyChatWord_Today,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SUNDAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_FRIDAY)] =
    {
        .text = gEasyChatWord_Friday,
        .alphabeticalOrder = EC_INDEX(EC_WORD_END),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_MONDAY)] =
    {
        .text = gEasyChatWord_Monday,
        .alphabeticalOrder = EC_INDEX(EC_WORD_FINAL),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_LATER)] =
    {
        .text = gEasyChatWord_Later,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TODAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_EARLIER)] =
    {
        .text = gEasyChatWord_Earlier,
        .alphabeticalOrder = EC_INDEX(EC_WORD_AGE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_ANOTHER)] =
    {
        .text = gEasyChatWord_Another,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WINTER),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TIME)] =
    {
        .text = gEasyChatWord_Time,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SOON),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_FINISH)] =
    {
        .text = gEasyChatWord_Finish,
        .alphabeticalOrder = EC_INDEX(EC_WORD_EARLIER),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WEDNESDAY)] =
    {
        .text = gEasyChatWord_Wednesday,
        .alphabeticalOrder = EC_INDEX(EC_WORD_MORNING),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SOON)] =
    {
        .text = gEasyChatWord_Soon,
        .alphabeticalOrder = EC_INDEX(EC_WORD_MONTH),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_START)] =
    {
        .text = gEasyChatWord_Start,
        .alphabeticalOrder = EC_INDEX(EC_WORD_NIGHT),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_MONTH)] =
    {
        .text = gEasyChatWord_Month,
        .alphabeticalOrder = EC_INDEX(EC_WORD_YESTERDAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_STOP)] =
    {
        .text = gEasyChatWord_Stop,
        .alphabeticalOrder = EC_INDEX(EC_WORD_FALL),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_NOW)] =
    {
        .text = gEasyChatWord_Now,
        .alphabeticalOrder = EC_INDEX(EC_WORD_ANOTHER),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_FINAL)] =
    {
        .text = gEasyChatWord_Final,
        .alphabeticalOrder = EC_INDEX(EC_WORD_FOREVER),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_NEXT)] =
    {
        .text = gEasyChatWord_Next,
        .alphabeticalOrder = EC_INDEX(EC_WORD_STOP),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_AGE)] =
    {
        .text = gEasyChatWord_Age,
        .alphabeticalOrder = EC_INDEX(EC_WORD_NEARLY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SATURDAY)] =
    {
        .text = gEasyChatWord_Saturday,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SPRING),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SUMMER)] =
    {
        .text = gEasyChatWord_Summer,
        .alphabeticalOrder = EC_INDEX(EC_WORD_NEXT),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SUNDAY)] =
    {
        .text = gEasyChatWord_Sunday,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WEDNESDAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_BEGINNING)] =
    {
        .text = gEasyChatWord_Beginning,
        .alphabeticalOrder = EC_INDEX(EC_WORD_ALMOST),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SPRING)] =
    {
        .text = gEasyChatWord_Spring,
        .alphabeticalOrder = EC_INDEX(EC_WORD_THURSDAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_DAYTIME)] =
    {
        .text = gEasyChatWord_Daytime,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SATURDAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WINTER)] =
    {
        .text = gEasyChatWord_Winter,
        .alphabeticalOrder = EC_INDEX(EC_WORD_MONDAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_DAILY)] =
    {
        .text = gEasyChatWord_Daily,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WEEK),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_OLDEN)] =
    {
        .text = gEasyChatWord_Olden,
        .alphabeticalOrder = EC_INDEX(EC_WORD_ALWAYS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_ALMOST)] =
    {
        .text = gEasyChatWord_Almost,
        .alphabeticalOrder = EC_INDEX(EC_WORD_FRIDAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_NEARLY)] =
    {
        .text = gEasyChatWord_Nearly,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TIME),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_THURSDAY)] =
    {
        .text = gEasyChatWord_Thursday,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TUESDAY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_NIGHTTIME)] =
    {
        .text = gEasyChatWord_Nighttime,
        .alphabeticalOrder = EC_INDEX(EC_WORD_FINISH),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_NIGHT)] =
    {
        .text = gEasyChatWord_Night,
        .alphabeticalOrder = EC_INDEX(EC_WORD_LAST),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WEEK)] =
    {
        .text = gEasyChatWord_Week,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SUMMER),
        .enabled = TRUE,
    },
};
