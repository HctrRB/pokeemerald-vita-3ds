const u8 gEasyChatWord_Meets[] = _("ENCONTRA");
const u8 gEasyChatWord_Concede[] = _("CEDE");
const u8 gEasyChatWord_Give[] = _("DAR");
const u8 gEasyChatWord_Gives[] = _("DÁ");
const u8 gEasyChatWord_Played[] = _("JOGOU");
const u8 gEasyChatWord_Plays[] = _("JOGA");
const u8 gEasyChatWord_Collect[] = _("JUNTAR");
const u8 gEasyChatWord_Walking[] = _("ANDANDO");
const u8 gEasyChatWord_Walks[] = _("ANDA");
const u8 gEasyChatWord_Says[] = _("DIZ");
const u8 gEasyChatWord_Went[] = _("FOI");
const u8 gEasyChatWord_Said[] = _("DISSE");
const u8 gEasyChatWord_WakeUp[] = _("ACORDAR");
const u8 gEasyChatWord_WakesUp[] = _("ACORDA");
const u8 gEasyChatWord_Angers[] = _("IRRITA");
const u8 gEasyChatWord_Teach[] = _("ENSINAR");
const u8 gEasyChatWord_Teaches[] = _("ENSINA");
const u8 gEasyChatWord_Please[] = _("POR FAVOR");
const u8 gEasyChatWord_Learn[] = _("APRENDER");
const u8 gEasyChatWord_Change[] = _("MUDAR");
const u8 gEasyChatWord_Story[] = _("HISTÓRIA");
const u8 gEasyChatWord_Trust[] = _("CONFIAR");
const u8 gEasyChatWord_Lavish[] = _("ESBANJAR");
const u8 gEasyChatWord_Listens[] = _("ESCUTA");
const u8 gEasyChatWord_Hearing[] = _("OUVINDO");
const u8 gEasyChatWord_Trains[] = _("TREINA");
const u8 gEasyChatWord_Choose[] = _("ESCOLHER");
const u8 gEasyChatWord_Come[] = _("VIR");
const u8 gEasyChatWord_Came[] = _("VEIO");
const u8 gEasyChatWord_Search[] = _("PROCURAR");
const u8 gEasyChatWord_Make[] = _("FAZER");
const u8 gEasyChatWord_Cause[] = _("CAUSAR");
const u8 gEasyChatWord_Know[] = _("SABER");
const u8 gEasyChatWord_Knows[] = _("SABE");
const u8 gEasyChatWord_Refuse[] = _("RECUSAR");
const u8 gEasyChatWord_Stores[] = _("GUARDA");
const u8 gEasyChatWord_Brag[] = _("GABAR");
const u8 gEasyChatWord_Ignorant[] = _("IGNORANTE");
const u8 gEasyChatWord_Thinks[] = _("ACHA");
const u8 gEasyChatWord_Believe[] = _("ACREDITAR");
const u8 gEasyChatWord_Slide[] = _("DESLIZAR");
const u8 gEasyChatWord_Eats[] = _("COME");
const u8 gEasyChatWord_Use[] = _("USAR");
const u8 gEasyChatWord_Uses[] = _("USA");
const u8 gEasyChatWord_Using[] = _("USANDO");
const u8 gEasyChatWord_Couldnt[] = _("NÃO PÔDE");
const u8 gEasyChatWord_Capable[] = _("CAPAZ");
const u8 gEasyChatWord_Disappear[] = _("SUMIR");
const u8 gEasyChatWord_Appear[] = _("APARECER");
const u8 gEasyChatWord_Throw[] = _("JOGAR");
const u8 gEasyChatWord_Worry[] = _("PREOCUPAR");
const u8 gEasyChatWord_Slept[] = _("DORMIU");
const u8 gEasyChatWord_Sleep[] = _("DORMIR");
const u8 gEasyChatWord_Release[] = _("SOLTAR");
const u8 gEasyChatWord_Drinks[] = _("BEBE");
const u8 gEasyChatWord_Runs[] = _("CORRE");
const u8 gEasyChatWord_Run[] = _("CORRER");
const u8 gEasyChatWord_Works[] = _("TRABALHA");
const u8 gEasyChatWord_Working[] = _("TRABALHANDO");
const u8 gEasyChatWord_Talking[] = _("FALANDO");
const u8 gEasyChatWord_Talk[] = _("FALAR");
const u8 gEasyChatWord_Sink[] = _("AFUNDAR");
const u8 gEasyChatWord_Smack[] = _("PANCADA");
const u8 gEasyChatWord_Pretend[] = _("FINGIR");
const u8 gEasyChatWord_Praise[] = _("ELOGIAR");
const u8 gEasyChatWord_Overdo[] = _("EXAGERAR");
const u8 gEasyChatWord_Show[] = _("MOSTRAR");
const u8 gEasyChatWord_Looks[] = _("PARECE");
const u8 gEasyChatWord_Sees[] = _("VÊ");
const u8 gEasyChatWord_Seek[] = _("BUSCAR");
const u8 gEasyChatWord_Own[] = _("TER");
const u8 gEasyChatWord_Take[] = _("PEGAR");
const u8 gEasyChatWord_Allow[] = _("DEIXAR");
const u8 gEasyChatWord_Forget[] = _("ESQUECER");
const u8 gEasyChatWord_Forgets[] = _("ESQUECE");
const u8 gEasyChatWord_Appears[] = _("APARECE");
const u8 gEasyChatWord_Faint[] = _("DESMAIAR");
const u8 gEasyChatWord_Fainted[] = _("DESMAIOU");

const struct EasyChatWordInfo gEasyChatGroup_Actions[] = {
    [EC_INDEX(EC_WORD_MEETS)] =
    {
        .text = gEasyChatWord_Meets,
        .alphabeticalOrder = EC_INDEX(EC_WORD_THINKS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_CONCEDE)] =
    {
        .text = gEasyChatWord_Concede,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WAKES_UP),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_GIVE)] =
    {
        .text = gEasyChatWord_Give,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WAKE_UP),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_GIVES)] =
    {
        .text = gEasyChatWord_Gives,
        .alphabeticalOrder = EC_INDEX(EC_WORD_BELIEVE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_PLAYED)] =
    {
        .text = gEasyChatWord_Played,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SINK),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_PLAYS)] =
    {
        .text = gEasyChatWord_Plays,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WALKS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_COLLECT)] =
    {
        .text = gEasyChatWord_Collect,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WALKING),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WALKING)] =
    {
        .text = gEasyChatWord_Walking,
        .alphabeticalOrder = EC_INDEX(EC_WORD_APPEARS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WALKS)] =
    {
        .text = gEasyChatWord_Walks,
        .alphabeticalOrder = EC_INDEX(EC_WORD_APPEAR),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SAYS)] =
    {
        .text = gEasyChatWord_Says,
        .alphabeticalOrder = EC_INDEX(EC_WORD_LEARN),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WENT)] =
    {
        .text = gEasyChatWord_Went,
        .alphabeticalOrder = EC_INDEX(EC_WORD_DRINKS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SAID)] =
    {
        .text = gEasyChatWord_Said,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SEEK),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WAKE_UP)] =
    {
        .text = gEasyChatWord_WakeUp,
        .alphabeticalOrder = EC_INDEX(EC_WORD_CAPABLE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WAKES_UP)] =
    {
        .text = gEasyChatWord_WakesUp,
        .alphabeticalOrder = EC_INDEX(EC_WORD_CAUSE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_ANGERS)] =
    {
        .text = gEasyChatWord_Angers,
        .alphabeticalOrder = EC_INDEX(EC_WORD_CONCEDE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TEACH)] =
    {
        .text = gEasyChatWord_Teach,
        .alphabeticalOrder = EC_INDEX(EC_WORD_EATS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TEACHES)] =
    {
        .text = gEasyChatWord_Teaches,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TRUST),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_PLEASE)] =
    {
        .text = gEasyChatWord_Please,
        .alphabeticalOrder = EC_INDEX(EC_WORD_RUNS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_LEARN)] =
    {
        .text = gEasyChatWord_Learn,
        .alphabeticalOrder = EC_INDEX(EC_WORD_RUN),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_CHANGE)] =
    {
        .text = gEasyChatWord_Change,
        .alphabeticalOrder = EC_INDEX(EC_WORD_GIVES),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_STORY)] =
    {
        .text = gEasyChatWord_Story,
        .alphabeticalOrder = EC_INDEX(EC_WORD_GIVE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TRUST)] =
    {
        .text = gEasyChatWord_Trust,
        .alphabeticalOrder = EC_INDEX(EC_WORD_ALLOW),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_LAVISH)] =
    {
        .text = gEasyChatWord_Lavish,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SLIDE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_LISTENS)] =
    {
        .text = gEasyChatWord_Listens,
        .alphabeticalOrder = EC_INDEX(EC_WORD_FAINT),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_HEARING)] =
    {
        .text = gEasyChatWord_Hearing,
        .alphabeticalOrder = EC_INDEX(EC_WORD_FAINTED),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TRAINS)] =
    {
        .text = gEasyChatWord_Trains,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SAID),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_CHOOSE)] =
    {
        .text = gEasyChatWord_Choose,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SAYS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_COME)] =
    {
        .text = gEasyChatWord_Come,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SLEEP),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_CAME)] =
    {
        .text = gEasyChatWord_Came,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SLEPT),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SEARCH)] =
    {
        .text = gEasyChatWord_Search,
        .alphabeticalOrder = EC_INDEX(EC_WORD_PRAISE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_MAKE)] =
    {
        .text = gEasyChatWord_Make,
        .alphabeticalOrder = EC_INDEX(EC_WORD_MEETS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_CAUSE)] =
    {
        .text = gEasyChatWord_Cause,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TEACHES),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_KNOW)] =
    {
        .text = gEasyChatWord_Know,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TEACH),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_KNOWS)] =
    {
        .text = gEasyChatWord_Knows,
        .alphabeticalOrder = EC_INDEX(EC_WORD_LAVISH),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_REFUSE)] =
    {
        .text = gEasyChatWord_Refuse,
        .alphabeticalOrder = EC_INDEX(EC_WORD_CHOOSE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_STORES)] =
    {
        .text = gEasyChatWord_Stores,
        .alphabeticalOrder = EC_INDEX(EC_WORD_LISTENS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_BRAG)] =
    {
        .text = gEasyChatWord_Brag,
        .alphabeticalOrder = EC_INDEX(EC_WORD_FORGETS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_IGNORANT)] =
    {
        .text = gEasyChatWord_Ignorant,
        .alphabeticalOrder = EC_INDEX(EC_WORD_FORGET),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_THINKS)] =
    {
        .text = gEasyChatWord_Thinks,
        .alphabeticalOrder = EC_INDEX(EC_WORD_OVERDO),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_BELIEVE)] =
    {
        .text = gEasyChatWord_Believe,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TALKING),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SLIDE)] =
    {
        .text = gEasyChatWord_Slide,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TALK),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_EATS)] =
    {
        .text = gEasyChatWord_Eats,
        .alphabeticalOrder = EC_INDEX(EC_WORD_MAKE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_USE)] =
    {
        .text = gEasyChatWord_Use,
        .alphabeticalOrder = EC_INDEX(EC_WORD_PRETEND),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_USES)] =
    {
        .text = gEasyChatWord_Uses,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WENT),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_USING)] =
    {
        .text = gEasyChatWord_Using,
        .alphabeticalOrder = EC_INDEX(EC_WORD_BRAG),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_COULDN_T)] =
    {
        .text = gEasyChatWord_Couldnt,
        .alphabeticalOrder = EC_INDEX(EC_WORD_STORES),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_CAPABLE)] =
    {
        .text = gEasyChatWord_Capable,
        .alphabeticalOrder = EC_INDEX(EC_WORD_STORY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_DISAPPEAR)] =
    {
        .text = gEasyChatWord_Disappear,
        .alphabeticalOrder = EC_INDEX(EC_WORD_IGNORANT),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_APPEAR)] =
    {
        .text = gEasyChatWord_Appear,
        .alphabeticalOrder = EC_INDEX(EC_WORD_ANGERS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_THROW)] =
    {
        .text = gEasyChatWord_Throw,
        .alphabeticalOrder = EC_INDEX(EC_WORD_PLAYS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WORRY)] =
    {
        .text = gEasyChatWord_Worry,
        .alphabeticalOrder = EC_INDEX(EC_WORD_THROW),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SLEPT)] =
    {
        .text = gEasyChatWord_Slept,
        .alphabeticalOrder = EC_INDEX(EC_WORD_PLAYED),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SLEEP)] =
    {
        .text = gEasyChatWord_Sleep,
        .alphabeticalOrder = EC_INDEX(EC_WORD_COLLECT),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_RELEASE)] =
    {
        .text = gEasyChatWord_Release,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SHOW),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_DRINKS)] =
    {
        .text = gEasyChatWord_Drinks,
        .alphabeticalOrder = EC_INDEX(EC_WORD_CHANGE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_RUNS)] =
    {
        .text = gEasyChatWord_Runs,
        .alphabeticalOrder = EC_INDEX(EC_WORD_COULDN_T),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_RUN)] =
    {
        .text = gEasyChatWord_Run,
        .alphabeticalOrder = EC_INDEX(EC_WORD_HEARING),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WORKS)] =
    {
        .text = gEasyChatWord_Works,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SMACK),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_WORKING)] =
    {
        .text = gEasyChatWord_Working,
        .alphabeticalOrder = EC_INDEX(EC_WORD_LOOKS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TALKING)] =
    {
        .text = gEasyChatWord_Talking,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TAKE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TALK)] =
    {
        .text = gEasyChatWord_Talk,
        .alphabeticalOrder = EC_INDEX(EC_WORD_PLEASE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SINK)] =
    {
        .text = gEasyChatWord_Sink,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WORRY),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SMACK)] =
    {
        .text = gEasyChatWord_Smack,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SEARCH),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_PRETEND)] =
    {
        .text = gEasyChatWord_Pretend,
        .alphabeticalOrder = EC_INDEX(EC_WORD_REFUSE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_PRAISE)] =
    {
        .text = gEasyChatWord_Praise,
        .alphabeticalOrder = EC_INDEX(EC_WORD_KNOWS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_OVERDO)] =
    {
        .text = gEasyChatWord_Overdo,
        .alphabeticalOrder = EC_INDEX(EC_WORD_KNOW),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SHOW)] =
    {
        .text = gEasyChatWord_Show,
        .alphabeticalOrder = EC_INDEX(EC_WORD_RELEASE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_LOOKS)] =
    {
        .text = gEasyChatWord_Looks,
        .alphabeticalOrder = EC_INDEX(EC_WORD_DISAPPEAR),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SEES)] =
    {
        .text = gEasyChatWord_Sees,
        .alphabeticalOrder = EC_INDEX(EC_WORD_OWN),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_SEEK)] =
    {
        .text = gEasyChatWord_Seek,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WORKS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_OWN)] =
    {
        .text = gEasyChatWord_Own,
        .alphabeticalOrder = EC_INDEX(EC_WORD_WORKING),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_TAKE)] =
    {
        .text = gEasyChatWord_Take,
        .alphabeticalOrder = EC_INDEX(EC_WORD_TRAINS),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_ALLOW)] =
    {
        .text = gEasyChatWord_Allow,
        .alphabeticalOrder = EC_INDEX(EC_WORD_USES),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_FORGET)] =
    {
        .text = gEasyChatWord_Forget,
        .alphabeticalOrder = EC_INDEX(EC_WORD_USING),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_FORGETS)] =
    {
        .text = gEasyChatWord_Forgets,
        .alphabeticalOrder = EC_INDEX(EC_WORD_USE),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_APPEARS)] =
    {
        .text = gEasyChatWord_Appears,
        .alphabeticalOrder = EC_INDEX(EC_WORD_SEES),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_FAINT)] =
    {
        .text = gEasyChatWord_Faint,
        .alphabeticalOrder = EC_INDEX(EC_WORD_CAME),
        .enabled = TRUE,
    },
    [EC_INDEX(EC_WORD_FAINTED)] =
    {
        .text = gEasyChatWord_Fainted,
        .alphabeticalOrder = EC_INDEX(EC_WORD_COME),
        .enabled = TRUE,
    },
};
