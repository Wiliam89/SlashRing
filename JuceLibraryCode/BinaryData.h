/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   BD_CL_Telocastme_wav;
    const int            BD_CL_Telocastme_wavSize = 60020;

    extern const char*   BD_HV_Creamback3_mixed_wav;
    const int            BD_HV_Creamback3_mixed_wavSize = 54804;

    extern const char*   BD_LD_HairApparently_wav;
    const int            BD_LD_HairApparently_wavSize = 42911;

    extern const char*   BD_RH_GatesOfHell_wav;
    const int            BD_RH_GatesOfHell_wavSize = 41147;

    extern const char*   Marshall21960G12M25R121_wav;
    const int            Marshall21960G12M25R121_wavSize = 66194;

    extern const char*   Marshall34x12sm576in0c_wav;
    const int            Marshall34x12sm576in0c_wavSize = 66516;

    extern const char*   Marshall44x12i55in1_5c_wav;
    const int            Marshall44x12i55in1_5c_wavSize = 83892;

    extern const char*   Marshall_Creamback_wav;
    const int            Marshall_Creamback_wavSize = 54804;

    extern const char*   Marshall1960G12M25SM57_wav;
    const int            Marshall1960G12M25SM57_wavSize = 66194;

    extern const char*   Metal1_wav;
    const int            Metal1_wavSize = 28880;

    extern const char*   Metal2_wav;
    const int            Metal2_wavSize = 28880;

    extern const char*   marshall_cab_wav;
    const int            marshall_cab_wavSize = 54804;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 12;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
