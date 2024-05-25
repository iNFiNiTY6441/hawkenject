/**
 * hawkenject - Copyright (c) 2023 atom0s [atom0s@live.com]
 *
 * Contact: https://www.atom0s.com/
 * Contact: https://discord.gg/UmXNvjq
 * Contact: https://github.com/atom0s
 * Support: https://paypal.me/atom0s
 * Support: https://patreon.com/atom0s
 * Support: https://github.com/sponsors/atom0s
 *
 * This file is part of hawkenject.
 *
 * hawkenject is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hawkenject is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with hawkenject.  If not, see <https://www.gnu.org/licenses/>.
 */

#define NOMINMAX

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Ws2_32.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <Windows.h>
#include <algorithm>
#include <format>
#include <ranges>
#include <string>
#include <Psapi.h>

#include "objects.hpp"
#include "utils.hpp"

#include "detours.h"
#include "SDK.hpp"
#include "string"
#include "stdio.h"

/**
 * DEBUG_MODE
 *
 * Enables the debug mode features of hawkenject.
 *
 * When enabled, hawkenject will hook additional functions, that are not generally needed otherwise, for the purpose
 * of debugging and other helpful information. Most of these features will output additional information to the
 * systems debug stream. Use a tool such as DebugView or DbgView++ to view said information.
 */
#define DEBUG_MODE 0

/**
 * Detour Prototypes
 */
extern "C"
{
    // Functions mainly relevant for server / UWorld::Listen reimplementation
    auto Real_ParseParam                        = static_cast<uint32_t(__cdecl*)(const wchar_t*, const wchar_t*, uint32_t)>(nullptr);
    auto Real_TArray_USeqAct_Latent_AddItem     = static_cast<uint32_t(__thiscall*)(int32_t, int32_t)>(nullptr);
    auto Real_UGameEngine_ConstructNetDriver    = static_cast<int32_t(__thiscall*)(int32_t)>(nullptr);
    auto Real_UGameEngine_SpawnServerActors     = static_cast<void(__thiscall*)(int32_t)>(nullptr);
    auto Real_UNetDriver_InitListen             = static_cast<int32_t(__thiscall*)(int32_t, int32_t, int32_t, int32_t)>(nullptr);
    auto Real_UTcpNetDriver_InitListen          = static_cast<int32_t(__thiscall*)(int32_t, atomic::objects::FNetworkNotify*, int32_t, int32_t)>(nullptr);
    auto Real_UPackageMap_AddNetPackages        = static_cast<void(__thiscall*)(int32_t)>(nullptr);
    auto Real_UWorld_GetGameInfo                = static_cast<int32_t(__thiscall*)(int32_t)>(nullptr);
    auto Real_UWorld_SetGameInfo                = static_cast<void(__thiscall*)(int32_t, int32_t)>(nullptr);
    auto Real_UWorld_GetWorldInfo               = static_cast<int32_t(__thiscall*)(int32_t, int32_t)>(nullptr);

    auto Real_ProcessEvent                      = static_cast<void(__thiscall*)(int32_t, int32_t)>(nullptr);

    ////////////////////////////////////////////////////////////
    // Infinity: Recent hooked stuff for testing & fixes      //
    ////////////////////////////////////////////////////////////

    // GameEngine Browse: Prevent server travel to network URLs: Remove local host IP from beginning of TravelURL
    auto Real_UGameEngine_Browse   = static_cast<BOOL(__thiscall*)( int32_t, Classes::FURL, int32_t )>(nullptr);

    // FUrl Constructor
    auto Real_FURL_Base_URL_Traveltype_Constructor  = static_cast<void(__thiscall*)(int32_t, const wchar_t *, Classes::ETravelType )>(nullptr);

    // MeteorOSS Functions
    auto Real_UOnlineSubsystemMeteor_IsServerListingCreated         = static_cast<BOOL(__thiscall*)(int32_t)>(nullptr);
    auto Real_UOnlineSubsystemMeteor_ReadGameItemInstanceCollection = static_cast<void(__thiscall*)(int32_t,int32_t,int32_t)>(nullptr);

    // Player Validation Status Functions
    auto Real_AR_PlayerController_UpdateOverallStatus               = static_cast<void(__thiscall*)(int32_t)>(nullptr);
    auto Real_AR_PlayerController_UpdateMechInstanceExpiredStatus   = static_cast<void(__thiscall*)(int32_t)>(nullptr);

    // Mechsetup Functions
    auto Real_UR_MechSetup_CreateMechPresetsFromInstances           = static_cast<void(__thiscall*)(int32_t, int32_t)>(nullptr);
    auto Real_UR_MechSetup_SetupMechPresetFromInstance              = static_cast<BOOL(__thiscall*)(int32_t, int32_t, int32_t)>(nullptr);
    auto Real_UR_MechSetup_IsPresetIndexValid                       = static_cast<BOOL(__thiscall*)(int32_t, int32_t)>(nullptr);
    auto Real_UR_MechSetup_CheckMechPresetIntegrity                 = static_cast<BOOL(__thiscall*)(int32_t, int32_t, int32_t )>(nullptr);

    // PlayerReplicationInfo sync status
    auto Real_PRI_IsOnlinePlayerAccountSynced                       = static_cast<BOOL(__thiscall*)()>(nullptr);

    // ReadOnlinePlayerAccounts at client on deathmatch join
    auto Real_AR_Deathmatch_ReadOnlinePlayerAccounts                = static_cast<void(__thiscall*)()>(nullptr); 

    ////////////////////////////////////////////////////////////


    // Object Pointers
    auto Real_PTR_GCmdLine                   = static_cast<uint32_t*>(nullptr);
    auto Real_PTR_GUseSeekFreePackageMap     = static_cast<uint32_t*>(nullptr);
    auto Real_PTR_UGameEngine                = static_cast<uint32_t*>(nullptr);
    auto Real_PTR_UPackage_NetObjectNotifies = static_cast<uint32_t*>(nullptr);

#if defined(DEBUG_MODE) && (DEBUG_MODE == 1)
    // Functions (Debugging)
    auto Real_FAsyncPackage_CreateExports = static_cast<BOOL(__thiscall*)(int32_t)>(nullptr);
    auto Real_ULinkerLoad_PreLoad         = static_cast<void(__thiscall*)(int32_t, int32_t)>(nullptr);
    auto Real_UObject_LoadPackage         = static_cast<int32_t(__cdecl*)(int32_t, const wchar_t*, int32_t)>(nullptr);
    auto Real_FSocketWin_RecvFrom         = static_cast<int32_t(__thiscall*)(int32_t, uint8_t*, int32_t, int32_t*, struct sockaddr*)>(nullptr);
#endif
}

/**
 * Helper function to properly call UPackage::NetObjectNotifies.AddItem(..)
 *
 * @param {auto} driver - The NetDriver object.
 * @note
 *
 *      This call is used to implement a proper call for the following:
 *
 *          TArray<FNetObjectNotify*> UPackage::NetObjectNotifies;
 *          UPackage::NetObjectNotifies.AddItem(NetDriver);
 *
 *      Due to how C++ class inheritance works, when the 'AddItem' call is made on this container,
 *      it will automatically it will expect the object being passed to inherit from 'FNetObjectNotify'.
 *
 *      When this happens, the object pointer will be adjusted and aligned to where the 'FNetObjectNotify'
 *      VTable begins within the parent object. Using raw pointers will not cause this to happen, thus we
 *      need to reimplement this functionality ourselves for the time being.
 *
 * @todo: Eventually write out full proper implementations of the Unreal objects to not need this.
 */
void UPackage_NetObjectNotifies_AddItem(auto driver)
{
    int32_t temp = 0;

    __asm
    {
        // Preserve flags and registers..
        pushad;
        pushfd;

        // Ensure the driver is valid..
        mov eax, driver;
        cmp eax, 0;
        jz null_driver;

        // Adjust the driver offset to its 'FNetObjectNotify' VTable start..
        add eax, 0x40;
        mov [temp], eax;
        jmp do_call;

        // Driver was 0..
null_driver:
        mov [temp], 0;
        jmp do_call;

        // Call TArray<FNetObjectNotify*>::AddItem..
do_call:
        lea edx, [temp];
        push edx;
        mov ecx, [Real_PTR_UPackage_NetObjectNotifies];
        call [Real_TArray_USeqAct_Latent_AddItem];

        // Restore flags and registers..
        popfd;
        popad;
    }
}


/**
 * UNUSED: Originally intended to correct server trying to attempt to travel to network urls ( 0.0.0.0/MapName?args ) by correcting things in the constructed FUrl
 * Ended up hooking UGameEngine::Browse() instead for more predictable & limited behavior.
 * 
 * Left here in case it is needed - cleanup will come later
*/
VOID __fastcall Mine_FURL_Base_URL_Traveltype_Constructor( void* pThis, void* edx, int32_t base, const wchar_t* texturl, Classes::ETravelType traveltype ) 
{   
    UNREFERENCED_PARAMETER(pThis);
    UNREFERENCED_PARAMETER(edx);
    ::OutputDebugStringA("\r\n ========================== URL CONSTRUCTED ==========================");
    Real_FURL_Base_URL_Traveltype_Constructor( base, texturl, traveltype );
}

/**
 * INVESTIGATE: Seems to be the initial kickoff to populate the PlayerReplicationInfo.MechPreset from player account instances
 * 
 * Left here in case it is needed - cleanup will come later
*/

VOID __fastcall Mine_UR_MechSetup_CreateMechPresetsFromInstances( int32_t MechSetup, int32_t pEdx, int32_t GameItemInstanceCollection )
{
    UNREFERENCED_PARAMETER( MechSetup );
    UNREFERENCED_PARAMETER( pEdx );
    UNREFERENCED_PARAMETER( GameItemInstanceCollection );

    //Classes::UOnlineGameItemInstanceCollection* cGameItemInstanceCollection = reinterpret_cast<Classes::UOnlineGameItemInstanceCollection*>(GameItemInstanceCollection);

    int num_items = 9999;//cGameItemInstanceCollection->NumItems();

    char message[255];
    sprintf_s( message, "Mine_UR_MechSetup_CreateMechPresetsFromInstances: NumItems: %d", num_items);

    Real_UR_MechSetup_CreateMechPresetsFromInstances( MechSetup, GameItemInstanceCollection );
}

/**
 * INVESTIGATE: Method name is self-explanatory. However it seems to only ever create from server-owned instances. Even on-join of a client that uses a different account / inventory.
 * Maybe just before this this whole process is being called on a specific netid? Could it be stuck at the first netid because what we have is a client?
*/
BOOL __fastcall Mine_UR_MechSetup_SetupMechPresetFromInstance( int32_t MechSetup, int32_t pEdx, int32_t presetIdx, int32_t ItemInstance )
{
    UNREFERENCED_PARAMETER( MechSetup );
    UNREFERENCED_PARAMETER( pEdx );
    UNREFERENCED_PARAMETER( ItemInstance );

    Classes::UOnlineGameItemInstance* cItemInstance = reinterpret_cast<Classes::UOnlineGameItemInstance*>(ItemInstance);


    //Classes::UR_MechSetup* cMechSetup = reinterpret_cast<Classes::UR_MechSetup*>(MechSetup);

    //auto test = cMechSetup->G

    char message[255];
    sprintf_s( message, "Mine_UR_MechSetup_SetupMechPresetFromInstance: PRESET %d", presetIdx);
    ::OutputDebugStringA( message );
    ::OutputDebugStringW( cItemInstance->InstanceID.c_str() );
    ::OutputDebugStringW( cItemInstance->ItemName.c_str() );
    ::OutputDebugStringA("..............................");

    //BOOL result = 

    return Real_UR_MechSetup_SetupMechPresetFromInstance( MechSetup, presetIdx, ItemInstance );;
}

/**
 * INVESTIGATE: Called near every tick when selecting / deploying. Preset for foreign clients always seems to be 0 - something fishy is going on here.
*/
BOOL __fastcall Mine_UR_MechSetup_CheckMechPresetIntegrity( int32_t MechSetup, int32_t pEdx, int32_t presetIdx, int32_t ItemInstance )
{
    UNREFERENCED_PARAMETER( MechSetup );
    UNREFERENCED_PARAMETER( pEdx );
    UNREFERENCED_PARAMETER( ItemInstance );


    Classes::UOnlineGameItemInstance* cItemInstance = reinterpret_cast<Classes::UOnlineGameItemInstance*>(ItemInstance);

    auto test = cItemInstance->InstanceID.c_str();
    auto test2 = cItemInstance->ItemName.c_str();
    
    char message[255];
    sprintf_s( message, "Mine_UR_MechSetup_CheckMechPresetIntegrity: PRESET %d", presetIdx);
    ::OutputDebugStringA(message);
    ::OutputDebugStringW( test );
    ::OutputDebugStringW( test2 );
    ::OutputDebugStringA("..............................");
    return Real_UR_MechSetup_CheckMechPresetIntegrity( MechSetup, presetIdx, ItemInstance );
}

/**
 * INVESTIGATE: Called near every tick when selecting / deploying. Preset for foreign clients always seems to be 0 - something fishy is going on here.
*/
BOOL __fastcall Mine_UR_MechSetup_IsPresetIndexValid( int32_t MechSetup, int32_t pEdx, int32_t presetIdx )
{
    UNREFERENCED_PARAMETER( MechSetup );
    UNREFERENCED_PARAMETER( pEdx );
    UNREFERENCED_PARAMETER( presetIdx );


    Classes::UR_MechSetup* cMechSetup = reinterpret_cast<Classes::UR_MechSetup*>(MechSetup);

    BOOL result = Real_UR_MechSetup_IsPresetIndexValid( MechSetup, presetIdx );
    //char message[63];
    //sprintf_s(message, "Mine_UR_MechSetup_IsPresetIndexValid: Preset (%d) spoofed to TRUE", presetIdx);
    char message[255];
    sprintf_s( message, "Mine_UR_MechSetup_IsPresetIndexValid: Status is %s for preset %d \r\n -CurrentPresetIndex: %d", 
        result ? "TRUE" : "FALSE", 
        presetIdx,
        cMechSetup->CurrentPresetIndex
    );

    //::OutputDebugStringA(message);
    //::OutputDebugStringA("Mine_UR_MechSetup_IsPresetIndexValid: Spoofed to TRUE");
    return result;
}

/**
 * FIX:
 * Spoof status of whether the OSS has created the dedicated server listing.
 * The actual creation seems to not happen - so in normal execution this always returns false.
 * During the PendingMatch state, the game server waits until this is true before allowing players to ready up.
 * So this prevents ready-up if not spoofed to true
 * 
*/
BOOL Mine_UOnlineSubsystemMeteor_IsServerListingCreated( int32_t OSSMeteor )
{
    UNREFERENCED_PARAMETER(OSSMeteor);

    ::OutputDebugStringA("\r\n ========================== Mine_UOnlineSubsystemMeteor_IsServerListingCreated ==========================\r\n Spoofing to TRUE");
    return TRUE;
}



VOID __fastcall Mine_UOnlineSubsystemMeteor_ReadGameItemInstanceCollection( int32_t pMeteorOSS, int32_t pEdx, int32_t pNetID, int32_t pInstanceCollection )
{
    UNREFERENCED_PARAMETER( pMeteorOSS );
    UNREFERENCED_PARAMETER( pEdx );
    UNREFERENCED_PARAMETER( pNetID );
    UNREFERENCED_PARAMETER( pInstanceCollection );


    Classes::FUniqueNetId* cNetID = reinterpret_cast<Classes::FUniqueNetId*>(pNetID);

    char char_netid[255];
    sprintf_s( char_netid, "%d %d %d %d", cNetID->Uid.A, cNetID->Uid.B, cNetID->Uid.C, cNetID->Uid.D );

    ::OutputDebugStringA("-=-=-=-=-=--=-=-=-=-=--=-=-=-=-=--=-=-=-=-=--=-=");
    ::OutputDebugStringA("-=-=-=-=-=- READ INSTANCE COLLECTION -=-=-=-=-=-");
    ::OutputDebugStringA( char_netid );
    Real_UOnlineSubsystemMeteor_ReadGameItemInstanceCollection( pMeteorOSS, pNetID, pInstanceCollection );
}

/**
 * INVESTIGATE: Called near every tick when selecting / deploying. Preset for foreign clients always seems to be 0 - something fishy is going on here.
*/
BOOL __fastcall Mine_UGameEngine_Browse( int32_t game_engine, int32_t pEdx, Classes::FURL url, int32_t err ) 
{   
    //UNREFERENCED_PARAMETER(ptr_gameengine);
    UNREFERENCED_PARAMETER(pEdx);
    //UNREFERENCED_PARAMETER(err);
    // auto CGameEngine             = reinterpret_cast<Classes::UGameEngine*>(gameengine);
    // auto CUrl                    = reinterpret_cast<Classes::FURL*>(url);
    //auto CErrorString            = reinterpret_cast<Classes::FString*>(ref_errorstring);

    ::OutputDebugStringA("[HOOK] UGameEngine_Browse CALLED");


    char message[63];
    sprintf_s(message, "ptr_param_game_engine: %d", game_engine);

    char message2[63];
    sprintf_s(message2, "ptr_Real_UGameEngine: %d", *Real_PTR_UGameEngine);

    char message3[63];
    sprintf_s(message3, "ptr_err: %d", err);

    ::OutputDebugStringA( message );
    ::OutputDebugStringA( message2 );
    ::OutputDebugStringA( message3 );

    ::OutputDebugStringA("\r\n\r\n");
    ::OutputDebugStringA("[HOOK] [BROWSE] PROTOCOL:");
    ::OutputDebugStringW( url.Protocol.c_str() );

    ::OutputDebugStringA("\r\n\r\n");
    ::OutputDebugStringA("[HOOK] [BROWSE] HOST:");
    ::OutputDebugStringW( url.Host.c_str() );

    ::OutputDebugStringA("\r\n\r\n");
    ::OutputDebugStringA("[HOOK] [BROWSE] MAP:");
    ::OutputDebugStringW( url.Map.c_str() );


    url.Host = Classes::FString(L"");

    //Classes::FString* ptr = &myerrortext;

    return Real_UGameEngine_Browse( game_engine, url, err );
}

/**
 * WORKAROUND: Override status here to have the client limp through to spawn, results in broken behavior. The client will have the servers preset 0 mech forcefully replicated.
 * This does not affect weapons, so you can get an Assault ( server enforced mech preset ) with Rocketeer weapons ( Local client mech selection weapons )
*/
VOID __fastcall Mine_AR_PlayerController_UpdateMechInstanceExpiredStatus( int32_t PC )
{

    bool override_status = true;

    char dbg_printout[127];
    sprintf_s( dbg_printout, "[Mine_AR_PlayerController_UpdateMechInstanceExpiredStatus]: INTERCEPT (%s)", override_status ? "No override" : "OVERRIDE to SVS_Succeeded");

    if ( override_status ) {

        Classes::AR_PlayerController* cPlayerController = reinterpret_cast<Classes::AR_PlayerController*>(PC);

        //cPlayerController->ValidationStatus_ItemInstanceCollection    = Classes::ESelectionValidationStatus::SVS_Succeeded;
        //cPlayerController->ValidationStatus_PresetLoaded              = Classes::ESelectionValidationStatus::SVS_Succeeded;
        cPlayerController->ValidationStatus_MechInstance              = Classes::ESelectionValidationStatus::SVS_Succeeded;
        //cPlayerController->ValidationStatus_SelectedPrimaryWeapon     = Classes::ESelectionValidationStatus::SVS_Succeeded;
        //cPlayerController->ValidationStatus_AchievementsLoaded        = Classes::ESelectionValidationStatus::SVS_Succeeded;
        //cPlayerController->OverallSelectionValidationStatus           = Classes::ESelectionValidationStatus::SVS_Succeeded;
        cPlayerController->SelectionValidationResponse                = Classes::ESelectionValidationResponse::SVR_Valid;
    }

    ::OutputDebugStringA( dbg_printout );
    Real_AR_PlayerController_UpdateMechInstanceExpiredStatus( PC );
}

VOID __fastcall Mine_AR_PlayerController_UpdateOverallStatus( int32_t PC ) 
{


    // if ( cplayer ) {

    //     cplayer->ValidationStatus_ItemInstanceCollection    = Classes::ESelectionValidationStatus::SVS_Succeeded;
    //     cplayer->ValidationStatus_PresetLoaded              = Classes::ESelectionValidationStatus::SVS_Succeeded;
    //     cplayer->ValidationStatus_MechInstance              = Classes::ESelectionValidationStatus::SVS_Succeeded;
    //     cplayer->ValidationStatus_SelectedPrimaryWeapon     = Classes::ESelectionValidationStatus::SVS_Succeeded;
    //     cplayer->ValidationStatus_AchievementsLoaded        = Classes::ESelectionValidationStatus::SVS_Succeeded;
    //     cplayer->OverallSelectionValidationStatus           = Classes::ESelectionValidationStatus::SVS_Succeeded;
        
    //     //Classes::UObject::GetName((UObject *)this);
    //     //if ( cplayer->IsInCombat(false) ) OutputDebugStringA("NOT IN COMBAT");
    //     //const Classes::UObject* playerObject = (Classes::UObject *) cplayer;
    //     //const std::string playername = cplayer->GetName();
    //     //::OutputDebugStringA(playername.c_str());
    //     //OutputDebugStringA( )
    //     //UNREFERENCED_PARAMETER(playername);
        
    // }

    
    
    ::OutputDebugStringA("UPDATEOVERALL ======================================/////////////////////////");

    Classes::AR_PlayerController* cplayer = reinterpret_cast<Classes::AR_PlayerController*>(PC);

    cplayer->ValidationStatus_ItemInstanceCollection    = Classes::ESelectionValidationStatus::SVS_Succeeded;
    cplayer->ValidationStatus_PresetLoaded              = Classes::ESelectionValidationStatus::SVS_Succeeded;
    cplayer->ValidationStatus_MechInstance              = Classes::ESelectionValidationStatus::SVS_Succeeded;
    cplayer->ValidationStatus_SelectedPrimaryWeapon     = Classes::ESelectionValidationStatus::SVS_Succeeded;
    cplayer->ValidationStatus_AchievementsLoaded        = Classes::ESelectionValidationStatus::SVS_Succeeded;
    cplayer->OverallSelectionValidationStatus           = Classes::ESelectionValidationStatus::SVS_Succeeded;
    cplayer->SelectionValidationResponse                = Classes::ESelectionValidationResponse::SVR_Valid;

    Real_AR_PlayerController_UpdateOverallStatus(PC);
    //UNREFERENCED_PARAMETER(ARPlayerController);

} 
// infinity: bypass CheckStartMatch() countdown "player not synced" issue
BOOL __fastcall Mine_PRI_IsOnlinePlayerAccountSynced()
{

        // const auto pri = reinterpret_cast<Classes::AR_PlayerReplicationInfo*>(AR_PlayerReplicationInfo);
    // const Classes::UR_OnlinePlayerStats_UnrankedGameplay stats = pri->OnlinePlayerStats_UnrankedGameplay;
    // const Classes::TEnumAsByte<Classes::EOnlinePlayerStats_State> ss = stats.StatsState
    //::OutputDebugStringA("[IsOnlinePlayerAccountSynced] Spoofing to TRUE\n");
    //::OutputDebugStringA("Trying the funny shit !!!\n");

    //::OutputDebugStringA("[IsOnlinePlayerAccountSynced] Spoofing to TRUE\n");
    //::OutputDebugStringA("Trying the funny shit !!!\n");
    // if ( justonce == false) {
        //Real_AR_Deathmatch_ReadOnlinePlayerAccounts();
        //justonce = true;
    //}
    return TRUE;
}

VOID __fastcall Mine_AR_Deathmatch_ReadOnlinePlayerAccounts()
{
    ::OutputDebugStringA("[AR_Deathmatch_ReadOnlinePlayerAccounts] !!!\n");
    Real_AR_Deathmatch_ReadOnlinePlayerAccounts();
}

/**
 * Reimplementation of the 'UWorld::Listen' function. (Removed from Hawken 2014+ clients.)
 *
 * @param {int32_t} world - The main UWorld object.
 * @param {int32_t} url - The URL associated with this world.
 * @return {BOOL} TRUE on success, FALSE otherwise.
 * @note
 *
 *      The contents of this reimplementation are based on the reversed code of the old PAX Hawken client
 *      and various code implementations of this function from UE3/4. The comments within this function
 *      show what the original code of this function looks like from UE3.
 */
BOOL __stdcall Mine_UWorld_Listen(const int32_t world, const int32_t url)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // if (NetDriver)
    // {
    //     Error = LocalizeError(TEXT("NetAlready"), TEXT("Engine"));
    //     return FALSE;
    // }
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    // Offset pulled from: UWorld::IsServer
    auto NetDriver = reinterpret_cast<int32_t*>(world + 0x00D8);
    if (*NetDriver)
        return FALSE;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // NetDriver = GEngine->ConstructNetDriver();
    // if (NetDriver == NULL)
    // {
    //     debugf(TEXT("Failed to create Net Driver"));
    //     return FALSE;
    // }
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    *NetDriver = Real_UGameEngine_ConstructNetDriver(*Real_PTR_UGameEngine);
    if (*NetDriver == NULL)
        return FALSE;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // if (!NetDriver->InitListen(this, InURL, Error))
    // {
    //     debugf(TEXT("Failed to listen: %s"), *Error);
    //     NetDriver = NULL;
    //     return FALSE;
    // }
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    const auto cworld = reinterpret_cast<atomic::objects::UWorld*>(world);

    Classes::FString error;
    if (!Real_UTcpNetDriver_InitListen(*NetDriver, cworld, url, reinterpret_cast<int32_t>(&error)))
    {
        *NetDriver = NULL;
        return FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // static UBOOL LanPlay = ParseParam(appCmdLine(), TEXT("lanplay"));
    // if (!LanPlay && (NetDriver->MaxInternetClientRate < NetDriver->MaxClientRate) && (NetDriver->MaxInternetClientRate > 2500))
    // {
    //     NetDriver->MaxClientRate = NetDriver->MaxInternetClientRate;
    // }
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    // Offsets pulled from: UNetDriver::StaticConstructor
    auto NetDriver_MasterMap             = reinterpret_cast<uint32_t*>(*NetDriver + 0x0058);
    auto NetDriver_ServerTravelPause     = reinterpret_cast<float*>(*NetDriver + 0x0074);
    auto NetDriver_MaxClientRate         = reinterpret_cast<uint32_t*>(*NetDriver + 0x0078);
    auto NetDriver_MaxInternetClientRate = reinterpret_cast<uint32_t*>(*NetDriver + 0x007C);

    static BOOL LanPlay = Real_ParseParam(reinterpret_cast<const wchar_t*>(Real_PTR_GCmdLine), L"lanplay", 0);
    if (!LanPlay && (*NetDriver_MaxInternetClientRate < *NetDriver_MaxClientRate) && (*NetDriver_MaxInternetClientRate > 2500))
        *NetDriver_MaxClientRate = *NetDriver_MaxInternetClientRate;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // if (GetGameInfo() && GetGameInfo()->MaxPlayers > 16)
    // {
    //     NetDriver->MaxClientRate = ::Min(NetDriver->MaxClientRate, 10000);
    // }
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    const auto game_info = reinterpret_cast<Classes::AGameInfo*>(Real_UWorld_GetGameInfo(world));
    if (game_info && game_info->MaxPlayers > 16)
        *NetDriver_MaxClientRate = std::min(*NetDriver_MaxClientRate, 10000u);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // if (!GUseSeekFreePackageMap)
    // {
    //     BuildServerMasterMap();
    // }
    // else
    // {
    //     UPackage::NetObjectNotifies.AddItem(NetDriver);
    // }
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    if (!*Real_PTR_GUseSeekFreePackageMap)
    {
        Real_UPackageMap_AddNetPackages(*NetDriver_MasterMap);
        UPackage_NetObjectNotifies_AddItem(*NetDriver);
    }
    else
        UPackage_NetObjectNotifies_AddItem(*NetDriver);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // GEngine->SpawnServerActors();
    //
    // GetWorldInfo()->NetMode             = GEngine->Client ? NM_ListenServer : NM_DedicatedServer;
    // GetWorldInfo()->NextSwitchCountdown = NetDriver->ServerTravelPause;
    // debugf(TEXT("NetMode is now %d"), GetWorldInfo()->NetMode);
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    Real_UGameEngine_SpawnServerActors(*Real_PTR_UGameEngine);

    const auto world_info = reinterpret_cast<Classes::AWorldInfo*>(Real_UWorld_GetWorldInfo(world, 0));
    if (world_info)
    {
        //const auto engine = reinterpret_cast<Classes::UGameEngine*>(*Real_PTR_UGameEngine);

        //world_info->NetMode             = engine->Client ? Classes::ENetMode::NM_ListenServer : Classes::ENetMode::NM_DedicatedServer;
        //world_info->NetMode             = Classes::ENetMode::NM_ListenServer;
        

        const auto oldNextURL = (world_info->NextURL).c_str();

        ::OutputDebugStringA("========= OLD WORLDINFO ========");
        ::OutputDebugStringA(reinterpret_cast<LPCSTR>(oldNextURL) );
        ::OutputDebugStringA("--------------------------------");

        world_info->NetMode             = Classes::ENetMode::NM_DedicatedServer;

        //world_info->NextTravelType = Classes::ETravelType::TRAVEL_Absolute;

        // Infinity: Fix server trying to travel to 0.0.0.0/MapUrl and disconnecting all clients as a result
        world_info->NextTravelType = Classes::ETravelType::TRAVEL_Absolute;
        //world_info->bBegunPlay = false;

        // world_info->bOfflineApproved = 1;
        //world_info->bStartup = 1;

        // game_info->NumBots = 12;
        // game_info->bAlwaysTick = 1;
        // game_info->bDebug = 1;
        // game_info->bDelayedStart = 1;
        game_info->bUseSeamlessTravel = 0;
        ::OutputDebugStringA("SET worldinfo");

        const auto game_engine = reinterpret_cast<Classes::UGameEngine*>(Real_PTR_UGameEngine);


        game_engine->TravelType = Classes::ETravelType::TRAVEL_Absolute;

        const auto last_url_valid = game_engine->LastURL.Valid;
        if ( last_url_valid ) {
            ::OutputDebugStringA("LASTURL WAS VALID: ");
            //::OutputDebugStringW( last_url );
        }
        
        //const auto svOpts = game_info->GameName.c_str();

        const auto nextLevel = world_info->NextURL.c_str();
        ::OutputDebugStringA("NEXT LEVEL:");
        ::OutputDebugStringW(nextLevel);
        //::OutputDebugStringW(svOpts);

        //const auto* UW = reinterpret_cast<Classes::UWorld*>(cworld);
       // const auto u = new Classes::FString(L"VS-Andromeda");
        //UW->ServerTravel( u, false, false);
        //world_info->ServerTravel( *u, false, false );

        world_info->NextSwitchCountdown = *NetDriver_ServerTravelPause;
    }

    return TRUE;
}

/**
 * UWorld::SetGameInfo detour hook callback.
 *
 * @param {int32_t} world - The current UWorld object used for this call.
 * @param {int32_t} pEdx - The value of the EDX register. (ignored)
 * @param {int32_t} url - The URL associated with this world.
 */

bool justonce = false;


bool only_listen_once = false;

void __fastcall Mine_UWorld_SetGameInfo(int32_t world, int32_t pEdx, int32_t url)
{
    UNREFERENCED_PARAMETER(pEdx);

    ::OutputDebugStringW(L"[SETGAMEINFO]");

    // Allow the original call to happen first..
    Real_UWorld_SetGameInfo(world, url);

    //if( only_listen_once == true ) return;

    //only_listen_once = true;

    //if ( justonce == true ) return;

    __asm pushad;
    __asm pushfd;

    // Reimplement the removed UWorld::Listen call..
    Mine_UWorld_Listen(world, url);
    //if ( justonce == false ) justonce = true;

    __asm popfd;
    __asm popad;
}

#if defined(DEBUG_MODE) && (DEBUG_MODE == 1)

/**
 * FAsyncPackage::CreateExports detour hook callback.
 *
 * @param {int32_t} package - The FAsyncPackage object used for this call.
 * @param {int32_t} pEdx - The value of the EDX register. (ignored)
 */
BOOL __fastcall Mine_FAsyncPackage_CreateExports(int32_t package, int32_t pEdx)
{
    UNREFERENCED_PARAMETER(pEdx);

    __asm pushad;
    __asm pushfd;

    const auto str = reinterpret_cast<Classes::FString*>(package + 0x04);
    if (str && str->IsValid() && str->c_str()[0] != 0)
        ::OutputDebugStringW(std::format(L"[FAsyncPackage::CreateExports] {}\n", str->c_str()).c_str());
    else
        ::OutputDebugStringW(L"[FAsyncPackage::CreateExports] (unknown)\n");

    __asm popfd;
    __asm popad;

    return Real_FAsyncPackage_CreateExports(package);
}

/**
 * ULinkerLoad::PreLoad detour hook callback.
 *
 * @param {int32_t} linker_load - The ULinkerLoad object used for this call.
 * @param {int32_t} pEdx - The value of the EDX register. (ignored)
 * @param {int32_t} obj - The UObject used for this call.
 */
void __fastcall Mine_ULinkerLoad_PreLoad(int32_t linker_load, int32_t pEdx, int32_t obj)
{
    UNREFERENCED_PARAMETER(pEdx);

    __asm pushad;
    __asm pushfd;

    const auto o = reinterpret_cast<Classes::UObject*>(obj);
    if (o == nullptr)
        ::OutputDebugStringA(std::format("[ULinkerLoad::PreLoad] (null object)\n").c_str());
    else
    {
        const auto name = Classes::FName::GNames->GetByIndex(o->Name.Index);
        if (name && name->AnsiName[0] != 0)
            ::OutputDebugStringA(std::format("[ULinkerLoad::PreLoad] {}\n", name->AnsiName).c_str());
        else
            ::OutputDebugStringA("[ULinkerLoad::PreLoad] (unknown object - invalid object name)\n");
    }

    __asm popfd;
    __asm popad;

    Real_ULinkerLoad_PreLoad(linker_load, obj);
}

/**
 * FSocketWin::RecvFrom detour hook callback.
 *
 * @param {int32_t} socket - The socket to receive the data from.
 * @param {int32_t} pEdx - The value of the EDX register. (ignored)
 * @param {int32_t} data - The buffer to hold the read data.
 * @param {int32_t} size - The size of the buffer.
 * @param {int32_t} read - The output value to hold the amount of data read from the socket.
 * @param {int32_t} src - The output value to hold the source address information.
 */
int32_t __fastcall Mine_FSocketWin_RecvFrom(int32_t socket, int32_t pEdx, uint8_t* data, int32_t size, int32_t* read, struct sockaddr* src)
{
    UNREFERENCED_PARAMETER(pEdx);

    const auto ret = Real_FSocketWin_RecvFrom(socket, data, size, read, src);
    if (ret && read && *read > 0)
    {
        const auto sin = reinterpret_cast<const struct sockaddr_in*>(src);

        char addr[INET_ADDRSTRLEN]{};
        ::inet_ntop(AF_INET, reinterpret_cast<const void*>(&sin->sin_addr), addr, INET_ADDRSTRLEN);

        std::stringstream ss;
        ss << std::format("[FSocketWin::RecvFrom] Client: {}:{} | Size: {}", addr, ::htons(sin->sin_port), *read) << std::endl;
        ss << atomic::strings::hexdump<16, true>(data, *read) << std::endl;

        ::OutputDebugStringA(ss.str().c_str());
    }

    return ret;
}

#endif

/**
 * Installs the hawkenject hook into the current process.
 *
 * @return {BOOL} TRUE on success, FALSE otherwise.
 */
__declspec(dllexport) BOOL __stdcall install(void)
{
    MODULEINFO minfo{};
    if (!::GetModuleInformation(::GetCurrentProcess(), ::GetModuleHandleA(nullptr), &minfo, sizeof(MODULEINFO)))
        return FALSE;

    std::vector<std::string> names;

#define PTR_CHECK(p)  \
    if (p == nullptr) \
        names.push_back(#p);

    const auto base = reinterpret_cast<uint32_t>(minfo.lpBaseOfDll);
    const auto size = minfo.SizeOfImage;

    // Update the function pointers..
    Real_ParseParam                     = atomic::memory::find<decltype(Real_ParseParam)>(base, size, "538B5C240866833B0055565774??8B7C24188D43025750E8", 0, 0);
    Real_TArray_USeqAct_Latent_AddItem  = atomic::memory::find<decltype(Real_TArray_USeqAct_Latent_AddItem)>(base, size, "568BF18B4E08578B7E048D47018946043BC17E??6A045150E8????????8B0E83C40C89460885C975??85C074??03C06A0803C05051E8????????83C40C89068B0E8D04B985C074??8B54240C8B0A89088BC75F5EC20400", 0, 0);
    Real_UGameEngine_ConstructNetDriver = atomic::memory::find<decltype(Real_UGameEngine_ConstructNetDriver)>(base, size, "833D????????0053565775??68????????E8????????83C404A3????????E8????????A1????????6A0068002000006A0068????????6A0050", 0, 0);
    Real_UGameEngine_SpawnServerActors  = atomic::memory::find<decltype(Real_UGameEngine_SpawnServerActors)>(base, size, "6AFF68????????64A1000000005081EC2C020000A1", 0, 0);
    Real_UNetDriver_InitListen          = atomic::memory::find<decltype(Real_UNetDriver_InitListen)>(base, size, "568BF18B068B904C010000FFD28B442408894654B8010000005EC20C00", 0, 0);
    Real_UTcpNetDriver_InitListen       = atomic::memory::find<decltype(Real_UTcpNetDriver_InitListen)>(base, size, "6AFF68????????64A1000000005083EC1853555657A1????????33C4508D44242C64A3000000008BF18B5C24448B7C24408B6C243C535755E8", 0, 0);
    Real_UPackageMap_AddNetPackages     = atomic::memory::find<decltype(Real_UPackageMap_AddNetPackages)>(base, size, "6AFF68????????64A1000000005083EC1053555657A1????????33C4508D44242464A3000000008BF9897C24148D773C", 0, 0);
    Real_UWorld_GetGameInfo             = atomic::memory::find<decltype(Real_UWorld_GetGameInfo)>(base, size, "568B71508B464085C07F??74??68????????683F02000068????????68????????E8????????83C4108B463C8B088B81B40400005EC3", 0, 0);
    Real_UWorld_SetGameInfo             = atomic::memory::find<decltype(Real_UWorld_SetGameInfo)>(base, size, "6AFF68????????64A1000000005081EC64020000A1????????33C48984246002000053555657A1????????33C4508D84247802000064A3000000008BAC248802", 0, 0);
    Real_UWorld_GetWorldInfo            = atomic::memory::find<decltype(Real_UWorld_GetWorldInfo)>(base, size, "568B71508B464085C07F??74??68????????683F02000068????????68????????E8????????83C410837C2408008B463C8B300F??????????83", 0, 0);


    //PE-INDEX: Old, stolen from the SDKGen
    Real_ProcessEvent                   = atomic::memory::find_ptr<decltype(Real_ProcessEvent)>(base, size, "74??83C00783E0F8E8????????8BC4",200,0);
    PTR_CHECK(Real_ProcessEvent);

    /////////////////////////////////////////////////////////////////////////////
    // UOnlineSubsystemMeteor
    ////////////////////////////////////////////////////////////////////////////

    Real_UOnlineSubsystemMeteor_ReadGameItemInstanceCollection = atomic::memory::find<decltype(Real_UOnlineSubsystemMeteor_ReadGameItemInstanceCollection)>(base, size, "6AFF??????????64A1????????505156??????????33C450????????64A3????????8BF16A08687425??????????????83C408????????????????????????85C0??????????????????????8D8E5003????51????????52518BC8",0,0);
    PTR_CHECK(Real_UOnlineSubsystemMeteor_ReadGameItemInstanceCollection);

    Real_UOnlineSubsystemMeteor_IsServerListingCreated = atomic::memory::find<decltype(Real_UOnlineSubsystemMeteor_IsServerListingCreated)>(base, size, "568BF18B??????????85C9??????????????83F801????B801??????5EC38B8E240100008B018B5020FFD284C0",0,0);
    PTR_CHECK(Real_UOnlineSubsystemMeteor_IsServerListingCreated)

    //-------------------------------------------------------------------------//

    /////////////////////////////////////////////////////////////////////////////
    // AR_PlayerController
    ////////////////////////////////////////////////////////////////////////////

    Real_AR_PlayerController_UpdateMechInstanceExpiredStatus = atomic::memory::find<decltype(Real_AR_PlayerController_UpdateOverallStatus)>(base, size, "6AFF??????????64A1????????5083EC1C53555657??????????33C450????????64A3????????8BF180BE7D0D000001????????????80BE7B0D000003????????????80BE800D000007????????????8B86DC01000050",0,0);
    PTR_CHECK(Real_AR_PlayerController_UpdateMechInstanceExpiredStatus);

    Real_AR_PlayerController_UpdateOverallStatus = atomic::memory::find<decltype(Real_AR_PlayerController_UpdateOverallStatus)>(base, size, "6AFF68????????64A1????????5083EC18",0,0);
    PTR_CHECK(Real_AR_PlayerController_UpdateOverallStatus);

    //-------------------------------------------------------------------------//


    // PlayerReplicationInfo: IsAccountSynced
    Real_PRI_IsOnlinePlayerAccountSynced     = atomic::memory::find<decltype(Real_PRI_IsOnlinePlayerAccountSynced)>(base, size, "8B??????????85C074??8A????3C0174??3C0375??8B??????????85C074??B2023850??75??8B81????????85C074??38????75??38????75??B801??????C3", 0, 0);
    PTR_CHECK(Real_PRI_IsOnlinePlayerAccountSynced);


    /////////////////////////////////////////////////////////////////////////////
    // UR_MechSetup
    ////////////////////////////////////////////////////////////////////////////

    Real_UR_MechSetup_CreateMechPresetsFromInstances = atomic::memory::find<decltype(Real_UR_MechSetup_CreateMechPresetsFromInstances)>(base, size, "6AFF??????????64A1????????5083EC6053555657A1????????33C450????????64A3????????8BD9??????????????????50??????????33ED83C4043BC5????????????8B80980100008B7848????????3BFD??????????????????????????51??????????83C408??????????",0,0);
    PTR_CHECK(Real_UR_MechSetup_CreateMechPresetsFromInstances);

    Real_UR_MechSetup_SetupMechPresetFromInstance = atomic::memory::find<decltype(Real_UR_MechSetup_SetupMechPresetFromInstance)>(base, size, "6AFF??????????64A1????????5081ECE001000053555657A1????????33C450??????????????64A3????????8BF9??????????????????50??????????83C404????????85C0??????????????????????????85DB????????????3B9FE001??????????????????????????????85F6????????????8B87E00100003BD8????85DB????85C0????",0,0);
    PTR_CHECK(Real_UR_MechSetup_SetupMechPresetFromInstance);

    Real_UR_MechSetup_CheckMechPresetIntegrity = atomic::memory::find<decltype(Real_UR_MechSetup_CheckMechPresetIntegrity)>(base, size, "6AFF??????????64A1????????5083EC4853555657A1????????33C450????????64A3????????8BF1??????????50??????????83C404????????85C0????????????8B??????85DB????????????3B9EE001??????????????????????????????????????8B86E00100003BD8????85DB????85C0????",0,0);
    PTR_CHECK(Real_UR_MechSetup_CheckMechPresetIntegrity);

    Real_UR_MechSetup_IsPresetIndexValid = atomic::memory::find<decltype(Real_UR_MechSetup_IsPresetIndexValid)>(base, size, "8B??????85C0????3B81E001????????B801??????C204??33C0C204??",0,0);
    PTR_CHECK(Real_UR_MechSetup_IsPresetIndexValid);

    //-------------------------------------------------------------------------//


    Real_FURL_Base_URL_Traveltype_Constructor = atomic::memory::find<decltype(Real_FURL_Base_URL_Traveltype_Constructor)>(base, size, "558BEC6AFF??????????64A1????????5083EC74A1????????33C5??????53565750??????64A3????????8BD9??????A1??????????????????????????????????89430489430885C0????6A0803C0506A00??????????83C40C8903",0,0);
    PTR_CHECK(Real_FURL_Base_URL_Traveltype_Constructor);

    Real_UGameEngine_Browse = atomic::memory::find<decltype(Real_UGameEngine_Browse)>(base, size, "6AFF??????????64A1????????5083EC7853555657A1????????33C450??????????????64A3????????8BF9??????????????33DB",0,0);
    PTR_CHECK(Real_UGameEngine_Browse)

    Real_AR_Deathmatch_ReadOnlinePlayerAccounts = atomic::memory::find<decltype(Real_AR_Deathmatch_ReadOnlinePlayerAccounts)>(base, size, "558BE9F6????????????0F??????????8B??????????5356576A00E8????????8BD88B??????????33FF39??????????0F??????????8B??????????8D??????85FF78??8B??????????3BF87C??85FF75??85C074??68????????683F02000068????????68????????E8????????8B??????????83C4108B??????????8B????85F674??85C975??68????????E8????????83C404A3????????E8????????8B??????????85C974??8B????85C074??3BC174??8B????85C075??EB??8B????8B??????????568BCDFFD08B??????????8B??????????473B??????????0F??????????5F5E5B5DC3", 0, 0);
    PTR_CHECK(Real_AR_Deathmatch_ReadOnlinePlayerAccounts);

    PTR_CHECK(Real_ParseParam);
    PTR_CHECK(Real_TArray_USeqAct_Latent_AddItem);
    PTR_CHECK(Real_UGameEngine_ConstructNetDriver);
    PTR_CHECK(Real_UGameEngine_SpawnServerActors);
    PTR_CHECK(Real_UNetDriver_InitListen);
    PTR_CHECK(Real_UTcpNetDriver_InitListen);
    PTR_CHECK(Real_UPackageMap_AddNetPackages);
    PTR_CHECK(Real_UWorld_GetGameInfo);
    PTR_CHECK(Real_UWorld_SetGameInfo);
    PTR_CHECK(Real_UWorld_GetWorldInfo);
    PTR_CHECK(Real_AR_PlayerController_UpdateOverallStatus);

    // Update object pointers..
    Real_PTR_GCmdLine                   = atomic::memory::find_ptr<decltype(Real_PTR_GCmdLine)>(base, size, "68????????895C2428E8????????83C41485C0", 1, 0);
    Real_PTR_GUseSeekFreePackageMap     = atomic::memory::find_ptr<decltype(Real_PTR_GUseSeekFreePackageMap)>(base, size, "833D????????000F??????????8B4E188B018B90600100005355", 2, 0);
    Real_PTR_UGameEngine                = atomic::memory::find_ptr<decltype(Real_PTR_UGameEngine)>(base, size, "A1????????3BC30F??????????3998E00400000F", 1, 0);
    Real_PTR_UPackage_NetObjectNotifies = atomic::memory::find_ptr<decltype(Real_PTR_UPackage_NetObjectNotifies)>(base, size, "8B15????????8B0CBA8B018B500456FFD2A1", 2, 0);

    PTR_CHECK(Real_PTR_GCmdLine);
    PTR_CHECK(Real_PTR_GUseSeekFreePackageMap);
    PTR_CHECK(Real_PTR_UGameEngine);
    PTR_CHECK(Real_PTR_UPackage_NetObjectNotifies);

#if defined(DEBUG_MODE) && (DEBUG_MODE == 1)
    // Update debug features..
    Real_FAsyncPackage_CreateExports = atomic::memory::find<decltype(Real_FAsyncPackage_CreateExports)>(base, size, "558BEC83E4F883EC085355568BF18B46288B4E3C573B88", 0, 0);
    Real_ULinkerLoad_PreLoad         = atomic::memory::find<decltype(Real_ULinkerLoad_PreLoad)>(base, size, "6AFF68????????64A1000000005083EC4053555657A1????????33C4508D44245464A3000000008BF98B74246433ED89", 0, 0);
    Real_UObject_LoadPackage         = atomic::memory::find<decltype(Real_UObject_LoadPackage)>(base, size, "558BEC6AFF68????????64A1000000005083EC74A1????????33C58945EC535657", 0, 0);
    Real_FSocketWin_RecvFrom         = atomic::memory::find<decltype(Real_FSocketWin_RecvFrom)>(base, size, "51538B5C2418558B6C241056578D44241050538BF18B4C24248B56146A005155", 0, 0);

    PTR_CHECK(Real_FAsyncPackage_CreateExports);
    PTR_CHECK(Real_ULinkerLoad_PreLoad);
    PTR_CHECK(Real_UObject_LoadPackage);
    PTR_CHECK(Real_FSocketWin_RecvFrom);
#endif

    // Update the SDK pointers..
    Classes::FName::GNames     = atomic::memory::find_ptr<decltype(Classes::FName::GNames)>(base, size, "8B0185C078??3B05????????7D??8B0D????????833C810074", 16, 0);
    Classes::UObject::GObjects = atomic::memory::find_ptr<decltype(Classes::UObject::GObjects)>(base, size, "8B44240485C078??3B05????????7D??8B0D????????8B0481C333C0C3", 18, 0);

    // Check for any pattern scanning failures..
    if (!names.empty())
    {
        const auto msg = std::ranges::views::transform(names, [](const auto str) { return str + "\n"; }) |
                         std::ranges::views::join |
                         std::ranges::to<std::string>();

        ::MessageBoxA(0, msg.c_str(), "hawkenject :: Failed to locate required pattern(s)!", MB_ICONERROR | MB_OK);
        return FALSE;
    }

    // Hook the needed functions with detours..
    ::DetourTransactionBegin();
    ::DetourUpdateThread(::GetCurrentThread());
    ::DetourAttach(&(PVOID&)Real_UWorld_SetGameInfo, Mine_UWorld_SetGameInfo);

    // infinity: bypass CheckStartMatch() countdown "player not synced" issue
    //::DetourAttach(&(PVOID&)Real_PRI_IsOnlinePlayerAccountSynced, Mine_PRI_IsOnlinePlayerAccountSynced);

    //HOOK_BROWSE
    ::DetourAttach(&(PVOID&)Real_UGameEngine_Browse, Mine_UGameEngine_Browse);

    ::DetourAttach(&(PVOID&)Real_UOnlineSubsystemMeteor_IsServerListingCreated, Mine_UOnlineSubsystemMeteor_IsServerListingCreated);
    ::DetourAttach(&(PVOID&)Real_UOnlineSubsystemMeteor_ReadGameItemInstanceCollection, Mine_UOnlineSubsystemMeteor_ReadGameItemInstanceCollection);
    
    ::DetourAttach(&(PVOID&)Real_UR_MechSetup_CheckMechPresetIntegrity, Mine_UR_MechSetup_CheckMechPresetIntegrity);
    ::DetourAttach(&(PVOID&)Real_UR_MechSetup_IsPresetIndexValid, Mine_UR_MechSetup_IsPresetIndexValid);
    ::DetourAttach(&(PVOID&)Real_UR_MechSetup_CreateMechPresetsFromInstances, Mine_UR_MechSetup_CreateMechPresetsFromInstances);
    ::DetourAttach(&(PVOID&)Real_UR_MechSetup_SetupMechPresetFromInstance, Mine_UR_MechSetup_SetupMechPresetFromInstance);
    //::DetourAttach(&(PVOID&)Real_FURL_Base_URL_Traveltype_Constructor, Mine_FURL_Base_URL_Traveltype_Constructor);

    //::DetourAttach(&(PVOID&)Real_AR_PlayerController_UpdateOverallStatus, Mine_AR_PlayerController_UpdateOverallStatus); 

    ::DetourAttach(&(PVOID&)Real_AR_PlayerController_UpdateMechInstanceExpiredStatus, Mine_AR_PlayerController_UpdateMechInstanceExpiredStatus); 

    //::DetourAttach(&(PVOID&)Real_AR_Deathmatch_ReadOnlinePlayerAccounts, Mine_AR_Deathmatch_ReadOnlinePlayerAccounts);

#if defined(DEBUG_MODE) && (DEBUG_MODE == 1)
    ::DetourAttach(&(PVOID&)Real_ULinkerLoad_PreLoad, Mine_ULinkerLoad_PreLoad);
    ::DetourAttach(&(PVOID&)Real_FAsyncPackage_CreateExports, Mine_FAsyncPackage_CreateExports);
    ::DetourAttach(&(PVOID&)Real_FSocketWin_RecvFrom, Mine_FSocketWin_RecvFrom);
#endif

    ::DetourTransactionCommit();

    return TRUE;
}

/**
 * Uninstalls the hawkenject hook from the current process.
 *
 * @return {BOOL} TRUE on success, FALSE otherwise.
 */
__declspec(dllexport) BOOL __stdcall uninstall(void)
{
    ::DetourTransactionBegin();
    ::DetourUpdateThread(::GetCurrentThread());
    ::DetourDetach(&(PVOID&)Real_UWorld_SetGameInfo, Mine_UWorld_SetGameInfo);

#if defined(DEBUG_MODE) && (DEBUG_MODE == 1)
    ::DetourDetach(&(PVOID&)Real_ULinkerLoad_PreLoad, Mine_ULinkerLoad_PreLoad);
    ::DetourDetach(&(PVOID&)Real_FAsyncPackage_CreateExports, Mine_FAsyncPackage_CreateExports);
    ::DetourDetach(&(PVOID&)Real_FSocketWin_RecvFrom, Mine_FSocketWin_RecvFrom);
#endif

    ::DetourTransactionCommit();

    return TRUE;
}

/**
 * Module entry point.
 *
 * @param {HINSTANCE} hinstDLL - A handle to the DLL module.
 * @param {DWORD} fdwReason - The reason code that indicates why the DLL entry-point function is being called.
 * @param {LPVOID} lpvReserved - Reserved.
 * @return {BOOL} TRUE on success, FALSE otherwise.
 */
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            ::DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            uninstall();
            break;
        default:
            break;
    }

    return TRUE;
}