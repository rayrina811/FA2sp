#include "Body.h"

#include "../../Helpers/Translations.h"

#include <Helpers/Macro.h>
#include <CINI.h>

DEFINE_HOOK(4DBC5C, CSingleplayerSettings_Translate, 5)
{
    GET(CSingleplayerSettings*, pThis, ESI);

    Translations::TranslateItem(pThis, 1349, "SingleplayerParTimeEasy");
    Translations::TranslateItem(pThis, 1350, "SingleplayerParTimeMedium");
    Translations::TranslateItem(pThis, 1351, "SingleplayerParTimeHard");
    Translations::TranslateItem(pThis, 1352, "SingleplayerOverParTitle");
    Translations::TranslateItem(pThis, 1353, "SingleplayerOverParMessage");
    Translations::TranslateItem(pThis, 1354, "SingleplayerUnderParTitle");
    Translations::TranslateItem(pThis, 1355, "SingleplayerUnderParMessage");
    Translations::TranslateItem(pThis, 1363, "SingleplayerRanking");
    Translations::TranslateItem(pThis, 1364, "SingleplayerGeneral");
    Translations::TranslateItem(pThis, 1365, "SingleplayerCampaignMoneyDeltaEasy");
    Translations::TranslateItem(pThis, 1367, "SingleplayerCampaignMoneyDeltaHard");
    Translations::TranslateItem(pThis, 1369, "SingleplayerSpyMoneyStealPercent");
    Translations::TranslateItem(pThis, 1371, "SingleplayerTeamDelays");
    Translations::TranslateItem(pThis, 1373, "SingleplayerPrismSupportModifier");
    Translations::TranslateItem(pThis, 1375, "SingleplayerDefaultMirageDisguises");
    Translations::TranslateItem(pThis, 1400, "SingleplayerSave");

    return 0;
}

DEFINE_HOOK(4DA1A6, CSingleplayerSettings_UpdateDialog, 5)
{
    GET(CSingleplayerSettings*, pThis, EDI);

    pThis->SetDlgItemText(1356, CINI::CurrentDocument->GetString("Ranking", "ParTimeEasy"));
    pThis->SetDlgItemText(1357, CINI::CurrentDocument->GetString("Ranking", "ParTimeMedium"));
    pThis->SetDlgItemText(1358, CINI::CurrentDocument->GetString("Ranking", "ParTimeHard"));
    pThis->SetDlgItemText(1359, CINI::CurrentDocument->GetString("Ranking", "OverParTitle"));
    pThis->SetDlgItemText(1360, CINI::CurrentDocument->GetString("Ranking", "OverParMessage"));
    pThis->SetDlgItemText(1361, CINI::CurrentDocument->GetString("Ranking", "UnderParTitle"));
    pThis->SetDlgItemText(1362, CINI::CurrentDocument->GetString("Ranking", "UnderParMessage"));
    pThis->SetDlgItemText(1366, CINI::CurrentDocument->GetString("General", "CampaignMoneyDeltaEasy"));
    pThis->SetDlgItemText(1368, CINI::CurrentDocument->GetString("General", "CampaignMoneyDeltaHard"));
    pThis->SetDlgItemText(1370, CINI::CurrentDocument->GetString("General", "SpyMoneyStealPercent"));
    pThis->SetDlgItemText(1372, CINI::CurrentDocument->GetString("General", "TeamDelays"));
    pThis->SetDlgItemText(1374, CINI::CurrentDocument->GetString("General", "PrismSupportModifier"));
    pThis->SetDlgItemText(1376, CINI::CurrentDocument->GetString("General", "DefaultMirageDisguises"));

    return 0;
}