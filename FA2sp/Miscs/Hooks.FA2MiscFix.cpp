#include <Helpers/Macro.h>
#include "../RunTime.h"

static char InfantryListBuffer[600000];

#define DEFINE_REG_HELPER(to, from) \
void __declspec(naked) to##_##from() \
{ __asm {lea to, [MapPreviewBuffer + from]} }

#define DEFINE_ZERO_HELPER(to) \
void __declspec(naked) to##_##0() \
{ __asm {lea to, [MapPreviewBuffer]} }

#define DEFINE_ZERO_VAR_HELPER(to,var) \
void __declspec(naked) to##_##var() \
{ __asm {lea to, [var]} }

#define DEFINE_PUSH_HELPER(to) \
void __declspec(naked) push##_##to() \
{ __asm {push to} }



namespace PreviewFixDetails
{
    DEFINE_PUSH_HELPER(0x5CA700); // 0
    DEFINE_PUSH_HELPER(0x5CDE60); // -1
    DEFINE_PUSH_HELPER(0x5CF344); // \pics\*.bmp
    DEFINE_PUSH_HELPER(0x0927C0); // 10000*60
    DEFINE_ZERO_VAR_HELPER(edx, InfantryListBuffer);

    static constexpr byte NOP = 0x90;

    inline void DoZero(unsigned long addr, void* fn)
    {
        RunTime::ResetMemoryContentAt(addr - 2, fn, 6);
    }

    inline void DoPush(unsigned long addr, void* fn)
    {
        RunTime::ResetMemoryContentAt(addr - 1, fn, 5);
    }

    inline void DoReg(unsigned long addr, void* fn)
    {
        RunTime::ResetMemoryContentAt(addr - 3, fn, 6);
        RunTime::ResetMemoryContentAt(addr - 3 + 6, &NOP, 1);
    }
}

#undef DEFINE_ZERO_HELPER
#undef DEFINE_ZERO_VAR_HELPER
#undef DEFINE_REG_HELPER
#undef DEFINE_PUSH_HELPER

DEFINE_HOOK(537129, ExeRun_FA2MiscFix, 9)
{
    using namespace PreviewFixDetails;

    // fix check .bmp file in pics2
    //DoPush(0x47AC96, push_0x5CF344);

    // fix InfantryDatas crash due to unable to push_back beyond 100
    //DoPush(0x4A2B63, push_0x0927C0);
    //DoZero(0x4A2BCA, edx_InfantryListBuffer);

    // change recruitA&B default to 0
    DoPush(0x4B024F, push_0x5CA700);
    DoPush(0x4B025D, push_0x5CA700);
    DoPush(0x4B0D60, push_0x5CA700);
    DoPush(0x4B0D6E, push_0x5CA700);

    // change aircraft group default to -1
    DoPush(0x4B0241, push_0x5CDE60);

    const char* MapImporterFilter = "All files|*.yrm;*.mpr;*.map;*.bmp|Multi maps|*.yrm;*.mpr|Single maps|*.map|Windows bitmaps|*.bmp|";
    RunTime::ResetStaticCharAt(0x5D026C, MapImporterFilter);

    // select main executive
    //const char* filter = "RA2 Mix Files|*.mix|";
    //RunTime::ResetStaticCharAt(0x5D23D8, filter);
    //const char* display = "ra2md.mix";
    //RunTime::ResetStaticCharAt(0x5CC160, display);

    return 0;
}
