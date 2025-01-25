#include "Body.h"

#include <Helpers/Macro.h>

#include "../../Helpers/Translations.h"

DEFINE_HOOK(502E66, CTriggerOption_OnInitDialog_RepeatTypeFix, 7)
{
    GET(CTriggerOption*, pThis, ESI);

    pThis->CCBHouse.DeleteAllStrings();
    pThis->CCBRepeatType.DeleteAllStrings();

    pThis->CCBRepeatType.AddString(Translations::TranslateOrDefault("TriggerRepeatType.OneTimeOr", "0 - 任一关联对象满足条件，执行一次"));
    pThis->CCBRepeatType.AddString(Translations::TranslateOrDefault("TriggerRepeatType.OneTimeAnd", "1 - 所有关联对象满足条件，执行一次"));
    pThis->CCBRepeatType.AddString(Translations::TranslateOrDefault("TriggerRepeatType.RepeatingOr", "2 - 任一关联对象满足条件，重复执行"));

    return 0;
}
