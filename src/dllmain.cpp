#include "pch.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#define REAL_EVENTS_VERSION "0.1.0"

#ifndef REAL_EVENTS_DEBUG_TOOLS
#define REAL_EVENTS_DEBUG_TOOLS 0
#endif

// ============================================================
// CONFIGURACION GENERAL
// ============================================================

// Dinero
constexpr uintptr_t MERCHANTS_BASE_PTR = 0x006DE518;
constexpr uintptr_t PLAYER_MERCHANT_OFFSET = 0xE340;
#if REAL_EVENTS_DEBUG_TOOLS
constexpr int MONEY_INCREMENT = 100000;
#endif

// Mundo
constexpr uintptr_t GAME_WORLD = 0x006DE4A0;
constexpr uintptr_t DAY_OF_MONTH_OFFSET = 0x00;
constexpr uintptr_t MONTH_OFFSET = 0x01;
constexpr uintptr_t YEAR_OFFSET = 0x02;
constexpr uintptr_t DAY_OF_YEAR_OFFSET = 0x04;
constexpr uintptr_t TOWN_COUNT_OFFSET = 0x10;
constexpr uintptr_t GAMETIME_OFFSET = 0x14;
constexpr uintptr_t TOWNS_PTR_OFFSET = 0x68;

constexpr uint32_t TICKS_PER_DAY = 256;
constexpr uint32_t TICKS_PER_YEAR = 93440;

constexpr uintptr_t TOWN_SIZE = 0x9F8;
constexpr uintptr_t TOWN_INDEX_OFFSET = 0x2C0;
constexpr uintptr_t TOWN_RAW_ID_OFFSET = 0x2C1;
constexpr uintptr_t TOWN_NAMES_PTR = 0x006DDA00;

constexpr uintptr_t PLAYER_HOMETOWN_OFFSET = 0x19;
constexpr uintptr_t PLAYER_FAMILY_NAME_PTR_OFFSET = 0xE4;
constexpr uintptr_t PLAYER_NAME_PTR_OFFSET = 0xE8;

constexpr uint8_t LUEBECK_RAW_ID = 0x0E;

#if REAL_EVENTS_DEBUG_TOOLS
// Evento manual de F11 en Luebeck:
// OFF -> CRISIS -> BOOM -> OFF
constexpr int MANUAL_CRISIS_PERCENT = 50;
constexpr int MANUAL_BOOM_PERCENT = 150;
#endif

enum ProductionEventType
{
    EVENT_NONE = 0,
    EVENT_CRISIS = 1,
    EVENT_BOOM = 2
};

// Ponderación de los eventos automáticos.
// Las crisis son algo más frecuentes que los booms.
// Preparado para añadir más tipos en el futuro.
constexpr int CRISIS_EVENT_WEIGHT = 60;
constexpr int BOOM_EVENT_WEIGHT = 40;

// EventManager automatico
constexpr int MIN_ACTIVE_CRISES = 1;
constexpr int MAX_ACTIVE_CRISES = 5;

constexpr int MIN_CRISIS_MONTHS = 1;
constexpr int MAX_CRISIS_MONTHS = 12;

// De momento tratamos un mes como 30 dias de juego.
// Mas adelante podremos sustituirlo por calendario exacto.
constexpr int DAYS_PER_MONTH = 30;

// CRISIS: queda entre el 40% y el 60% de la producción normal.
constexpr int MIN_PRODUCTION_PERCENT = 40;
constexpr int MAX_PRODUCTION_PERCENT = 60;

// BOOM: aumenta la producción entre un 40% y un 60%.
constexpr int MIN_BOOM_PRODUCTION_PERCENT = 140;
constexpr int MAX_BOOM_PRODUCTION_PERCENT = 160;

// Cada 30-90 dias el EventManager decide si incorpora
// otra crisis, siempre respetando el maximo de 5.
constexpr int MIN_EVENT_INTERVAL_DAYS = 30;
constexpr int MAX_EVENT_INTERVAL_DAYS = 90;

// ============================================================
// HOOKS YA CONFIRMADOS
// ============================================================

// Dispatcher privado / Trader
constexpr uintptr_t PRIVATE_ENTRY_HOOK_RVA = 0x000D3798;
constexpr uintptr_t PRIVATE_EXIT_HOOK_RVA = 0x000D3AE3;

// Dispatcher Town
constexpr uintptr_t TOWN_ENTRY_HOOK_RVA = 0x001101D0;
constexpr uintptr_t TOWN_EXIT1_HOOK_RVA = 0x0011077F;
constexpr uintptr_t TOWN_EXIT2_HOOK_RVA = 0x001107B4;
constexpr uintptr_t TOWN_EXIT3_HOOK_RVA = 0x00110876;


// Informer / taberna.
// Confirmado experimentalmente:
// 005D138F mov eax,[esi+216C]  -> puntero al texto final vanilla
// 005D1395 lea ecx,[esp+18]
// 005D1399 push eax
// 005D139A push 006C66BC       -> "\f1_%s"
// 005D139F push ecx
constexpr uintptr_t INFORMER_TEXT_HOOK_RVA = 0x001D1399;

// ============================================================
// NOMBRES DE CIUDADES POR RAW TOWN ID
// Fuente: Patrician3.CT
// ============================================================

static const char* const RAW_TOWN_NAMES[0x28] =
{
    "Edinburgh",   // 00
    "Newcastle",   // 01
    "Scarborough", // 02
    "Boston",      // 03
    "London",      // 04
    "Bruges",      // 05
    "Haarlem",     // 06
    "Harlingen",   // 07
    "Groningen",   // 08
    "Cologne",     // 09
    "Bremen",      // 0A
    "Ripen",       // 0B
    "Hamburg",     // 0C
    "Flensburg",   // 0D
    "Luebeck",     // 0E
    "Rostock",     // 0F
    "Bergen",      // 10
    "Stavanger",   // 11
    "Toensberg",   // 12
    "Oslo",        // 13
    "Aalborg",     // 14
    "Goeteborg",   // 15
    "Naestved",    // 16
    "Malmoe",      // 17
    "Ahus",        // 18
    "Stockholm",   // 19
    "Visby",       // 1A
    "Helsinki",    // 1B
    "Stettin",     // 1C
    "Ruegenwald",  // 1D
    "Gdansk",      // 1E
    "Torun",       // 1F
    "Koenigsberg", // 20
    "Memel",       // 21
    "Windau",      // 22
    "Riga",        // 23
    "Pernau",      // 24
    "Reval",       // 25
    "Ladoga",      // 26
    "Novgorod"     // 27
};

// ============================================================
// ESTADO GLOBAL
// ============================================================

#if REAL_EVENTS_DEBUG_TOOLS
// F11: EVENT_NONE / EVENT_CRISIS / EVENT_BOOM
static volatile LONG g_manualLuebeckMode = EVENT_NONE;
#endif

// Direcciones / indices
static uintptr_t g_luebeckAddress = 0;
static volatile LONG g_luebeckTownIndex = -1;

// Hook Informer
static volatile LONG g_informerHookInstalled = 0;
static volatile LONG g_informerHits = 0;
static volatile LONG g_informerRumorsAdded = 0;
static volatile LONG g_informerLastTownIndex = -1;
static volatile LONG g_informerLastEventType = EVENT_NONE;

// Valores usados por el trampoline ASM.
static uintptr_t g_informerHookReturnAddress = 0;
static DWORD g_informerFormatString = 0x006C66BC;

// Hooks privado
static volatile LONG g_privateEntryHookInstalled = 0;
static volatile LONG g_privateExitHookInstalled = 0;

static volatile LONG g_privateHits = 0;
static volatile LONG g_privateAffectedHits = 0;
static volatile LONG g_privateModifiedHits = 0;
static volatile LONG g_privateRestoredHits = 0;
static volatile LONG g_privateStackOverflows = 0;

static uintptr_t g_privateEntryReturnAddress = 0;
static uintptr_t g_privateExitReturnAddress = 0;

// Hooks Town
static volatile LONG g_townEntryHookInstalled = 0;
static volatile LONG g_townExitHooksInstalled = 0;

static volatile LONG g_townHits = 0;
static volatile LONG g_townAffectedHits = 0;
static volatile LONG g_townModifiedHits = 0;
static volatile LONG g_townRestoredHits = 0;
static volatile LONG g_townStackOverflows = 0;

static uintptr_t g_townEntryReturnAddress = 0;

// EventManager
static volatile LONG g_eventManagerInitialized = 0;
static volatile LONG g_daysProcessed = 0;
static volatile LONG g_lastGameDaySerial = -1;
static volatile LONG g_nextAutoEventGameDay = 0;

static volatile LONG g_stateDirty = 0;
static volatile LONG g_persistenceLoaded = 0;
static volatile LONG g_persistenceValid = 0;

static uint32_t g_rngState = 0xA341316Cu;
static uint32_t g_campaignHash = 0;

static HMODULE g_moduleHandle = nullptr;

static uintptr_t g_cachedTownsBase = 0;
static uint32_t g_cachedTownCount = 0;

// ============================================================
// EVENTOS AUTOMATICOS DE PRODUCCION
// ============================================================

struct AutoCrisisSlot
{
    volatile LONG townIndex;
    volatile LONG rawTownId;
    volatile LONG eventType;
    volatile LONG productionPercent;
    volatile LONG startGameDay;
    volatile LONG endGameDay;
};

static AutoCrisisSlot g_autoCrises[MAX_ACTIVE_CRISES] =
{
    { -1, -1, EVENT_NONE, 100, 0, 0 },
    { -1, -1, EVENT_NONE, 100, 0, 0 },
    { -1, -1, EVENT_NONE, 100, 0, 0 },
    { -1, -1, EVENT_NONE, 100, 0, 0 },
    { -1, -1, EVENT_NONE, 100, 0, 0 }
};

// ============================================================
// PILAS TLS DE LOS HOOKS
// ============================================================

struct ProductionFrame
{
    uintptr_t entry;
    uint32_t originalFactor;
    bool modified;
};


#pragma pack(push, 1)
struct FacilityLayout
{
    uint32_t efficiency;       // +0x00
    uint16_t employees;        // +0x04
    uint8_t  facilityType;     // +0x06
    uint8_t  townIndex;        // +0x07
    uint16_t field08;          // +0x08
    uint16_t field0A;          // +0x0A
    uint16_t employeesMax;     // +0x0C
    uint16_t field0E;          // +0x0E
    uint32_t field10;          // +0x10
};
#pragma pack(pop)

static_assert(sizeof(FacilityLayout) == 0x14,
    "FacilityLayout debe medir 0x14 bytes");

constexpr int HOOK_FRAME_CAPACITY = 32;

__declspec(thread)
static ProductionFrame g_privateFrames[HOOK_FRAME_CAPACITY];

__declspec(thread)
static int g_privateDepth = 0;

__declspec(thread)
static ProductionFrame g_townFrames[HOOK_FRAME_CAPACITY];

__declspec(thread)
static int g_townDepth = 0;


// Buffer temporal del texto del Informer.
// Es TLS porque el render puede ejecutarse en un hilo distinto al EventManager.
constexpr size_t INFORMER_TEXT_BUFFER_SIZE = 4096;

__declspec(thread)
static char g_informerTextBuffer[INFORMER_TEXT_BUFFER_SIZE];

// ============================================================
// HELPERS ATOMICOS
// ============================================================

LONG AtomicRead(volatile LONG* value)
{
    return InterlockedCompareExchange(value, 0, 0);
}

// ============================================================
// LECTURA DEL MUNDO
// ============================================================

uint32_t GetTownCount()
{
    __try
    {
        return *reinterpret_cast<uint16_t*>(
            GAME_WORLD + TOWN_COUNT_OFFSET);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

uintptr_t GetTownsBase()
{
    __try
    {
        return *reinterpret_cast<uint32_t*>(
            GAME_WORLD + TOWNS_PTR_OFFSET);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

int GetDayOfMonth()
{
    __try
    {
        return static_cast<int>(
            *reinterpret_cast<uint8_t*>(
                GAME_WORLD + DAY_OF_MONTH_OFFSET));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

int GetMonth()
{
    __try
    {
        return static_cast<int>(
            *reinterpret_cast<uint8_t*>(
                GAME_WORLD + MONTH_OFFSET));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

int GetYear()
{
    __try
    {
        return static_cast<int>(
            *reinterpret_cast<uint16_t*>(
                GAME_WORLD + YEAR_OFFSET));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

int GetDayOfYear()
{
    __try
    {
        return static_cast<int>(
            *reinterpret_cast<uint16_t*>(
                GAME_WORLD + DAY_OF_YEAR_OFFSET));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

uint32_t GetGametime()
{
    __try
    {
        return *reinterpret_cast<uint32_t*>(
            GAME_WORLD + GAMETIME_OFFSET);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

uint32_t GetGameDaySerial()
{
    return GetGametime() / TICKS_PER_DAY;
}

int GetRawTownIdByIndex(int townIndex)
{
    __try
    {
        const uint32_t townCount = GetTownCount();
        const uintptr_t townsBase = GetTownsBase();

        if (!townsBase ||
            townIndex < 0 ||
            static_cast<uint32_t>(townIndex) >= townCount)
        {
            return -1;
        }

        const uintptr_t town =
            townsBase +
            static_cast<uintptr_t>(townIndex) * TOWN_SIZE;

        return static_cast<int>(
            *reinterpret_cast<uint8_t*>(
                town + TOWN_RAW_ID_OFFSET));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

int GetStoredTownIndexByArrayIndex(int arrayIndex)
{
    __try
    {
        const uint32_t townCount = GetTownCount();
        const uintptr_t townsBase = GetTownsBase();

        if (!townsBase ||
            arrayIndex < 0 ||
            static_cast<uint32_t>(arrayIndex) >= townCount)
        {
            return -1;
        }

        const uintptr_t town =
            townsBase +
            static_cast<uintptr_t>(arrayIndex) * TOWN_SIZE;

        return static_cast<int>(
            *reinterpret_cast<uint8_t*>(
                town + TOWN_INDEX_OFFSET));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

int FindTownIndexByRawId(int rawTownId)
{
    const uint32_t townCount = GetTownCount();

    for (uint32_t i = 0; i < townCount; ++i)
    {
        if (GetRawTownIdByIndex(
                static_cast<int>(i)) == rawTownId)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

const char* GetTownNameByIndex(int townIndex)
{
    __try
    {
        const uint32_t townCount = GetTownCount();

        if (townIndex >= 0 &&
            static_cast<uint32_t>(townIndex) < townCount)
        {
            const uintptr_t namePtr =
                *reinterpret_cast<uint32_t*>(
                    TOWN_NAMES_PTR +
                    static_cast<uintptr_t>(townIndex) * 4);

            if (namePtr)
            {
                const char* name =
                    reinterpret_cast<const char*>(namePtr);

                if (name[0] != '\0')
                    return name;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    const int rawId =
        GetRawTownIdByIndex(townIndex);

    if (rawId >= 0 && rawId < 0x28)
        return RAW_TOWN_NAMES[rawId];

    return "Unknown";
}

void RefreshWorldCacheIfNeeded()
{
    const uintptr_t townsBase = GetTownsBase();
    const uint32_t townCount = GetTownCount();

    if (!townsBase ||
        townCount == 0 ||
        townCount > 100)
    {
        return;
    }

    bool mustRefresh =
        townsBase != g_cachedTownsBase ||
        townCount != g_cachedTownCount;

    const LONG oldLuebeck =
        AtomicRead(&g_luebeckTownIndex);

    if (!mustRefresh && oldLuebeck >= 0)
    {
        if (GetRawTownIdByIndex(
                static_cast<int>(oldLuebeck)) !=
            LUEBECK_RAW_ID)
        {
            mustRefresh = true;
        }
    }

    if (!mustRefresh && oldLuebeck >= 0)
        return;

    uintptr_t newLuebeckAddress = 0;
    LONG newLuebeckIndex = -1;

    __try
    {
        for (uint32_t i = 0; i < townCount; ++i)
        {
            const uintptr_t town =
                townsBase +
                static_cast<uintptr_t>(i) * TOWN_SIZE;

            const uint8_t rawTownId =
                *reinterpret_cast<uint8_t*>(
                    town + TOWN_RAW_ID_OFFSET);

            if (rawTownId == LUEBECK_RAW_ID)
            {
                newLuebeckAddress = town;
                newLuebeckIndex =
                    static_cast<LONG>(i);
                break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return;
    }

    g_cachedTownsBase = townsBase;
    g_cachedTownCount = townCount;
    g_luebeckAddress = newLuebeckAddress;

    InterlockedExchange(
        &g_luebeckTownIndex,
        newLuebeckIndex);
}

void UpdateAddresses()
{
    RefreshWorldCacheIfNeeded();
}

// ============================================================
// DINERO
// ============================================================

#if REAL_EVENTS_DEBUG_TOOLS
uintptr_t GetMoneyAddress()
{
    __try
    {
        uintptr_t merchantsBase =
            *reinterpret_cast<uint32_t*>(
                MERCHANTS_BASE_PTR);

        if (!merchantsBase)
            return 0;

        return merchantsBase +
            PLAYER_MERCHANT_OFFSET;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

bool AddMoney(int amount)
{
    const uintptr_t address =
        GetMoneyAddress();

    if (!address)
        return false;

    __try
    {
        int* money =
            reinterpret_cast<int*>(address);

        *money += amount;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}
#endif

int RandomRange(int minValue, int maxValue);

// ============================================================
// PERSISTENCIA DEL EVENT MANAGER
// ============================================================

constexpr uint32_t STATE_MAGIC = 0x37563350; // P3V7
constexpr uint32_t STATE_VERSION = 2;

#pragma pack(push, 1)
struct PersistedCrisis
{
    int32_t rawTownId;
    int32_t eventType;
    int32_t productionPercent;
    int32_t startGameDay;
    int32_t endGameDay;
};

struct PersistedState
{
    uint32_t magic;
    uint32_t version;
    uint32_t campaignHash;
    uint32_t townCount;
    uint32_t savedGameDay;
    uint32_t nextAutoEventGameDay;
    PersistedCrisis crises[MAX_ACTIVE_CRISES];
};
#pragma pack(pop)

uint32_t HashFNV1aByte(uint32_t hash, uint8_t value)
{
    hash ^= value;
    hash *= 16777619u;
    return hash;
}

uint32_t HashFNV1aData(
    uint32_t hash,
    const void* data,
    size_t size)
{
    const uint8_t* bytes =
        reinterpret_cast<const uint8_t*>(data);

    for (size_t i = 0; i < size; ++i)
        hash = HashFNV1aByte(hash, bytes[i]);

    return hash;
}

uint32_t HashStringPointer(
    uint32_t hash,
    uintptr_t ptr)
{
    __try
    {
        if (!ptr)
            return hash;

        const char* s =
            reinterpret_cast<const char*>(ptr);

        for (int i = 0; i < 64; ++i)
        {
            const uint8_t c =
                static_cast<uint8_t>(s[i]);

            hash = HashFNV1aByte(hash, c);

            if (c == 0)
                break;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return hash;
}

uint32_t ComputeCampaignHash()
{
    uint32_t hash = 2166136261u;

    const uint32_t townCount = GetTownCount();

    hash = HashFNV1aData(
        hash,
        &townCount,
        sizeof(townCount));

    for (uint32_t i = 0; i < townCount; ++i)
    {
        const int raw =
            GetRawTownIdByIndex(
                static_cast<int>(i));

        hash = HashFNV1aData(
            hash,
            &raw,
            sizeof(raw));
    }

    __try
    {
        const uintptr_t merchantsBase =
            *reinterpret_cast<uint32_t*>(
                MERCHANTS_BASE_PTR);

        if (merchantsBase)
        {
            const uintptr_t player =
                merchantsBase +
                PLAYER_MERCHANT_OFFSET;

            const uint8_t hometown =
                *reinterpret_cast<uint8_t*>(
                    player +
                    PLAYER_HOMETOWN_OFFSET);

            hash = HashFNV1aByte(
                hash,
                hometown);

            const uintptr_t familyPtr =
                *reinterpret_cast<uint32_t*>(
                    player +
                    PLAYER_FAMILY_NAME_PTR_OFFSET);

            const uintptr_t namePtr =
                *reinterpret_cast<uint32_t*>(
                    player +
                    PLAYER_NAME_PTR_OFFSET);

            hash = HashStringPointer(
                hash,
                familyPtr);

            hash = HashStringPointer(
                hash,
                namePtr);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return hash;
}

bool BuildStatePath(
    char* buffer,
    size_t capacity,
    const char* suffix)
{
    if (!buffer ||
        capacity == 0 ||
        !g_moduleHandle)
    {
        return false;
    }

    char modulePath[MAX_PATH] = {};

    const DWORD len =
        GetModuleFileNameA(
            g_moduleHandle,
            modulePath,
            MAX_PATH);

    if (len == 0 ||
        len >= MAX_PATH)
    {
        return false;
    }

    char* slash1 =
        strrchr(modulePath, '\\');

    char* slash2 =
        strrchr(modulePath, '/');

    char* slash =
        slash1 > slash2
        ? slash1
        : slash2;

    if (slash)
        *(slash + 1) = '\0';
    else
        modulePath[0] = '\0';

    sprintf_s(
        buffer,
        capacity,
        "%sRealEvents_%08X%s",
        modulePath,
        g_campaignHash,
        suffix ? suffix : ".dat");

    return true;
}

void MarkStateDirty()
{
    InterlockedExchange(
        &g_stateDirty,
        1);
}

void ResetAutoCrisesInMemory()
{
    for (int i = 0;
         i < MAX_ACTIVE_CRISES;
         ++i)
    {
        InterlockedExchange(
            &g_autoCrises[i].townIndex,
            -1);

        InterlockedExchange(
            &g_autoCrises[i].rawTownId,
            -1);

        InterlockedExchange(
            &g_autoCrises[i].eventType,
            EVENT_NONE);

        InterlockedExchange(
            &g_autoCrises[i].productionPercent,
            100);

        InterlockedExchange(
            &g_autoCrises[i].startGameDay,
            0);

        InterlockedExchange(
            &g_autoCrises[i].endGameDay,
            0);
    }
}

bool SavePersistentState()
{
    if (g_campaignHash == 0)
        return false;

    PersistedState state = {};
    state.magic = STATE_MAGIC;
    state.version = STATE_VERSION;
    state.campaignHash = g_campaignHash;
    state.townCount = GetTownCount();
    state.savedGameDay = GetGameDaySerial();

    const LONG nextEventDayForSave =
        AtomicRead(
            &g_nextAutoEventGameDay);

    state.nextAutoEventGameDay =
        nextEventDayForSave > 0
        ? static_cast<uint32_t>(
            nextEventDayForSave)
        : 0u;

    for (int i = 0;
         i < MAX_ACTIVE_CRISES;
         ++i)
    {
        state.crises[i].rawTownId =
            AtomicRead(
                &g_autoCrises[i].rawTownId);

        state.crises[i].eventType =
            AtomicRead(
                &g_autoCrises[i].eventType);

        state.crises[i].productionPercent =
            AtomicRead(
                &g_autoCrises[i].productionPercent);

        state.crises[i].startGameDay =
            AtomicRead(
                &g_autoCrises[i].startGameDay);

        state.crises[i].endGameDay =
            AtomicRead(
                &g_autoCrises[i].endGameDay);
    }

    char finalPath[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};

    if (!BuildStatePath(
            finalPath,
            sizeof(finalPath),
            ".dat") ||
        !BuildStatePath(
            tempPath,
            sizeof(tempPath),
            ".tmp"))
    {
        return false;
    }

    HANDLE file =
        CreateFileA(
            tempPath,
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

    if (file ==
        INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD written = 0;

    const BOOL ok =
        WriteFile(
            file,
            &state,
            sizeof(state),
            &written,
            nullptr);

    FlushFileBuffers(file);
    CloseHandle(file);

    if (!ok ||
        written != sizeof(state))
    {
        DeleteFileA(tempPath);
        return false;
    }

    if (!MoveFileExA(
            tempPath,
            finalPath,
            MOVEFILE_REPLACE_EXISTING |
            MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileA(tempPath);
        return false;
    }

    InterlockedExchange(
        &g_stateDirty,
        0);

    InterlockedExchange(
        &g_persistenceValid,
        1);

    return true;
}

void FlushPersistentStateIfDirty()
{
    if (AtomicRead(
            &g_stateDirty) == 0)
    {
        return;
    }

    SavePersistentState();
}

bool LoadPersistentState()
{
    if (g_campaignHash == 0)
        return false;

    char path[MAX_PATH] = {};

    if (!BuildStatePath(
            path,
            sizeof(path),
            ".dat"))
    {
        return false;
    }

    HANDLE file =
        CreateFileA(
            path,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

    if (file ==
        INVALID_HANDLE_VALUE)
    {
        return false;
    }

    PersistedState state = {};
    DWORD read = 0;

    const BOOL ok =
        ReadFile(
            file,
            &state,
            sizeof(state),
            &read,
            nullptr);

    CloseHandle(file);

    if (!ok ||
        read != sizeof(state) ||
        state.magic != STATE_MAGIC ||
        state.version != STATE_VERSION ||
        state.campaignHash != g_campaignHash ||
        state.townCount != GetTownCount())
    {
        return false;
    }

    const uint32_t currentDay =
        GetGameDaySerial();

    if (state.savedGameDay > currentDay)
        return false;

    ResetAutoCrisesInMemory();

    for (int i = 0;
         i < MAX_ACTIVE_CRISES;
         ++i)
    {
        const PersistedCrisis& p =
            state.crises[i];

        if (p.rawTownId < 0 ||
            (p.eventType != EVENT_CRISIS &&
             p.eventType != EVENT_BOOM) ||
            p.productionPercent < 1 ||
            p.productionPercent > 200 ||
            p.endGameDay <=
                static_cast<int32_t>(
                    currentDay))
        {
            continue;
        }

        const int townIndex =
            FindTownIndexByRawId(
                p.rawTownId);

        if (townIndex < 0)
            continue;

        InterlockedExchange(
            &g_autoCrises[i].rawTownId,
            p.rawTownId);

        InterlockedExchange(
            &g_autoCrises[i].eventType,
            p.eventType);

        InterlockedExchange(
            &g_autoCrises[i].productionPercent,
            p.productionPercent);

        InterlockedExchange(
            &g_autoCrises[i].startGameDay,
            p.startGameDay);

        InterlockedExchange(
            &g_autoCrises[i].endGameDay,
            p.endGameDay);

        InterlockedExchange(
            &g_autoCrises[i].townIndex,
            townIndex);
    }

    LONG nextDay =
        static_cast<LONG>(
            state.nextAutoEventGameDay);

    if (nextDay <=
        static_cast<LONG>(
            currentDay))
    {
        nextDay =
            static_cast<LONG>(
                currentDay) +
            RandomRange(
                MIN_EVENT_INTERVAL_DAYS,
                MAX_EVENT_INTERVAL_DAYS);
    }

    InterlockedExchange(
        &g_nextAutoEventGameDay,
        nextDay);

    InterlockedExchange(
        &g_persistenceLoaded,
        1);

    InterlockedExchange(
        &g_persistenceValid,
        1);

    return true;
}

// ============================================================
// RNG
// ============================================================

uint32_t NextRandom()
{
    // xorshift32
    uint32_t x = g_rngState;

    if (x == 0)
        x = 0x6D2B79F5u;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    g_rngState = x;
    return x;
}

int RandomRange(int minValue, int maxValue)
{
    if (maxValue <= minValue)
        return minValue;

    const uint32_t span =
        static_cast<uint32_t>(
            maxValue - minValue + 1);

    return minValue +
        static_cast<int>(
            NextRandom() % span);
}


const char* GetEventTypeName(int eventType)
{
    switch (eventType)
    {
    case EVENT_CRISIS:
        return "CRISIS";

    case EVENT_BOOM:
        return "BOOM";

    default:
        return "NONE";
    }
}

int ChooseRandomProductionEventType()
{
    const int totalWeight =
        CRISIS_EVENT_WEIGHT +
        BOOM_EVENT_WEIGHT;

    const int roll =
        RandomRange(
            1,
            totalWeight);

    if (roll <= CRISIS_EVENT_WEIGHT)
        return EVENT_CRISIS;

    return EVENT_BOOM;
}

// ============================================================
// EVENT MANAGER - CONSULTAS
// ============================================================

int FindAutoCrisisSlotByTown(int townIndex)
{
    for (int i = 0;
        i < MAX_ACTIVE_CRISES;
        ++i)
    {
        if (AtomicRead(
            &g_autoCrises[i].townIndex) ==
            townIndex)
        {
            return i;
        }
    }

    return -1;
}

int FindFreeAutoCrisisSlot()
{
    for (int i = 0;
        i < MAX_ACTIVE_CRISES;
        ++i)
    {
        if (AtomicRead(
            &g_autoCrises[i].townIndex) < 0)
        {
            return i;
        }
    }

    return -1;
}

int CountAutoEvents()
{
    int count = 0;

    for (int i = 0;
        i < MAX_ACTIVE_CRISES;
        ++i)
    {
        if (AtomicRead(
            &g_autoCrises[i].townIndex) >= 0)
        {
            ++count;
        }
    }

    return count;
}

bool ManualLuebeckIsSeparateEventCity()
{
#if REAL_EVENTS_DEBUG_TOOLS
    if (AtomicRead(
        &g_manualLuebeckMode) == 0)
    {
        return false;
    }

    const int luebeckIndex =
        static_cast<int>(
            AtomicRead(
                &g_luebeckTownIndex));

    if (luebeckIndex < 0)
        return false;

    return FindAutoCrisisSlotByTown(
        luebeckIndex) < 0;
#else
    return false;
#endif
}

int CountActiveEventCities()
{
    int count =
        CountAutoEvents();

    if (ManualLuebeckIsSeparateEventCity())
        ++count;

    return count;
}

// Devuelve el porcentaje REAL que debe producir la ciudad.
// 100 = normal, 50 = produce la mitad.
int GetProductionPercent(int townIndex)
{
#if REAL_EVENTS_DEBUG_TOOLS
    const int luebeckIndex =
        static_cast<int>(
            AtomicRead(
                &g_luebeckTownIndex));

    const LONG manualMode =
        AtomicRead(
            &g_manualLuebeckMode);

    // F11 es una herramienta de debug: si esta activo en Luebeck,
    // el modo manual sustituye temporalmente cualquier AUTO existente
    // para poder comprobar claramente CRISIS y BOOM.
    if (townIndex == luebeckIndex &&
        manualMode != EVENT_NONE)
    {
        return manualMode == EVENT_BOOM
            ? MANUAL_BOOM_PERCENT
            : MANUAL_CRISIS_PERCENT;
    }
#endif

    const int autoSlot =
        FindAutoCrisisSlotByTown(
            townIndex);

    if (autoSlot >= 0)
    {
        const int autoPercent =
            static_cast<int>(
                AtomicRead(
                    &g_autoCrises[
                        autoSlot
                    ].productionPercent));

        if (autoPercent > 0)
            return autoPercent;
    }

    return 100;
}

// ============================================================
// EVENT MANAGER - MODIFICACION DE SLOTS
// ============================================================

void ClearAutoCrisisSlot(int slot)
{
    if (slot < 0 ||
        slot >= MAX_ACTIVE_CRISES)
    {
        return;
    }

    InterlockedExchange(
        &g_autoCrises[slot].townIndex,
        -1);

    InterlockedExchange(
        &g_autoCrises[slot].rawTownId,
        -1);

    InterlockedExchange(
        &g_autoCrises[slot].eventType,
        EVENT_NONE);

    InterlockedExchange(
        &g_autoCrises[slot].productionPercent,
        100);

    InterlockedExchange(
        &g_autoCrises[slot].startGameDay,
        0);

    InterlockedExchange(
        &g_autoCrises[slot].endGameDay,
        0);

    MarkStateDirty();
}

bool AddAutoCrisisForTown(
    int townIndex,
    int eventType,
    int percent,
    int durationDays,
    uint32_t startGameDay)
{
    const uint32_t townCount =
        GetTownCount();

    if (townIndex < 0 ||
        static_cast<uint32_t>(townIndex) >=
            townCount)
    {
        return false;
    }

    if (eventType != EVENT_CRISIS &&
        eventType != EVENT_BOOM)
    {
        return false;
    }

    if (FindAutoCrisisSlotByTown(
            townIndex) >= 0)
    {
        return false;
    }

    if (CountActiveEventCities() >=
        MAX_ACTIVE_CRISES)
    {
        return false;
    }

    const int slot =
        FindFreeAutoCrisisSlot();

    if (slot < 0)
        return false;

    const int rawTownId =
        GetRawTownIdByIndex(
            townIndex);

    if (rawTownId < 0)
        return false;

    const uint32_t endGameDay =
        startGameDay +
        static_cast<uint32_t>(
            durationDays);

    InterlockedExchange(
        &g_autoCrises[slot].rawTownId,
        rawTownId);

    InterlockedExchange(
        &g_autoCrises[slot].eventType,
        eventType);

    InterlockedExchange(
        &g_autoCrises[slot].productionPercent,
        percent);

    InterlockedExchange(
        &g_autoCrises[slot].startGameDay,
        static_cast<LONG>(
            startGameDay));

    InterlockedExchange(
        &g_autoCrises[slot].endGameDay,
        static_cast<LONG>(
            endGameDay));

    InterlockedExchange(
        &g_autoCrises[slot].townIndex,
        townIndex);

    MarkStateDirty();
    return true;
}

bool AddRandomAutoEvent(
    uint32_t gameDay)
{
    if (CountActiveEventCities() >=
        MAX_ACTIVE_CRISES)
    {
        return false;
    }

    const uint32_t townCount =
        GetTownCount();

    if (townCount == 0 ||
        townCount > 100)
    {
        return false;
    }

#if REAL_EVENTS_DEBUG_TOOLS
    const int manualLuebeckIndex =
        static_cast<int>(
            AtomicRead(
                &g_luebeckTownIndex));

    const bool manualActive =
        AtomicRead(
            &g_manualLuebeckMode) != 0;
#endif

    const int start =
        RandomRange(
            0,
            static_cast<int>(
                townCount) - 1);

    for (uint32_t n = 0;
         n < townCount;
         ++n)
    {
        const int candidate =
            (start +
             static_cast<int>(n)) %
            static_cast<int>(townCount);

        if (FindAutoCrisisSlotByTown(
                candidate) >= 0)
        {
            continue;
        }

#if REAL_EVENTS_DEBUG_TOOLS
        if (manualActive &&
            candidate ==
                manualLuebeckIndex)
        {
            continue;
        }
#endif

        const int months =
            RandomRange(
                MIN_CRISIS_MONTHS,
                MAX_CRISIS_MONTHS);

        const int durationDays =
            months *
            DAYS_PER_MONTH;

        const int eventType =
            ChooseRandomProductionEventType();

        const int percent =
            eventType == EVENT_BOOM
            ? RandomRange(
                MIN_BOOM_PRODUCTION_PERCENT,
                MAX_BOOM_PRODUCTION_PERCENT)
            : RandomRange(
                MIN_PRODUCTION_PERCENT,
                MAX_PRODUCTION_PERCENT);

        return AddAutoCrisisForTown(
            candidate,
            eventType,
            percent,
            durationDays,
            gameDay);
    }

    return false;
}

void EnsureMinimumEvents(
    uint32_t gameDay)
{
    while (CountActiveEventCities() <
           MIN_ACTIVE_CRISES)
    {
        if (!AddRandomAutoEvent(
                gameDay))
        {
            break;
        }
    }
}

void RemoveShortestAutoCrisis(
    int protectedTownIndex)
{
    int bestSlot = -1;
    LONG bestEndDay = 0x7FFFFFFF;

    for (int i = 0;
         i < MAX_ACTIVE_CRISES;
         ++i)
    {
        const LONG townIndex =
            AtomicRead(
                &g_autoCrises[i].townIndex);

        if (townIndex < 0 ||
            townIndex ==
                protectedTownIndex)
        {
            continue;
        }

        const LONG endDay =
            AtomicRead(
                &g_autoCrises[i].endGameDay);

        if (endDay < bestEndDay)
        {
            bestEndDay = endDay;
            bestSlot = i;
        }
    }

    if (bestSlot >= 0)
        ClearAutoCrisisSlot(
            bestSlot);
}

// ============================================================
// EVENT MANAGER - CICLO DIARIO
// ============================================================

void ProcessOneGameDay(
    uint32_t gameDay)
{
    InterlockedIncrement(
        &g_daysProcessed);

    for (int i = 0;
         i < MAX_ACTIVE_CRISES;
         ++i)
    {
        if (AtomicRead(
                &g_autoCrises[i].townIndex) < 0)
        {
            continue;
        }

        const LONG endDay =
            AtomicRead(
                &g_autoCrises[i].endGameDay);

        if (endDay <=
            static_cast<LONG>(
                gameDay))
        {
            ClearAutoCrisisSlot(i);
        }
    }

    EnsureMinimumEvents(
        gameDay);

    LONG nextEventDay =
        AtomicRead(
            &g_nextAutoEventGameDay);

    if (nextEventDay <=
        static_cast<LONG>(
            gameDay))
    {
        const int target =
            RandomRange(
                MIN_ACTIVE_CRISES,
                MAX_ACTIVE_CRISES);

        if (CountActiveEventCities() <
                target &&
            CountActiveEventCities() <
                MAX_ACTIVE_CRISES)
        {
            AddRandomAutoEvent(
                gameDay);
        }

        const LONG newNext =
            static_cast<LONG>(
                gameDay) +
            RandomRange(
                MIN_EVENT_INTERVAL_DAYS,
                MAX_EVENT_INTERVAL_DAYS);

        InterlockedExchange(
            &g_nextAutoEventGameDay,
            newNext);

        MarkStateDirty();
    }

    EnsureMinimumEvents(
        gameDay);
}

void EventManagerInitializeIfReady()
{
    if (AtomicRead(
            &g_eventManagerInitialized) != 0)
    {
        return;
    }

    RefreshWorldCacheIfNeeded();

    const uint32_t gameDay =
        GetGameDaySerial();

    const uint32_t townCount =
        GetTownCount();

    if (gameDay == 0 ||
        townCount == 0 ||
        townCount > 100 ||
        !GetTownsBase())
    {
        return;
    }

    g_rngState ^=
        static_cast<uint32_t>(
            GetTickCount64());

    g_rngState ^=
        GetGametime();

    g_campaignHash =
        ComputeCampaignHash();

    InterlockedExchange(
        &g_lastGameDaySerial,
        static_cast<LONG>(
            gameDay));

    const bool loaded =
        LoadPersistentState();

    if (!loaded)
    {
        ResetAutoCrisesInMemory();

        InterlockedExchange(
            &g_nextAutoEventGameDay,
            static_cast<LONG>(
                gameDay) +
            RandomRange(
                MIN_EVENT_INTERVAL_DAYS,
                MAX_EVENT_INTERVAL_DAYS));

        EnsureMinimumEvents(
            gameDay);

        MarkStateDirty();
    }

    InterlockedExchange(
        &g_eventManagerInitialized,
        1);

    EnsureMinimumEvents(
        gameDay);

    FlushPersistentStateIfDirty();
}

void EventManagerPollGameDay()
{
    EventManagerInitializeIfReady();

    if (AtomicRead(
            &g_eventManagerInitialized) == 0)
    {
        return;
    }

    const uint32_t currentGameDay =
        GetGameDaySerial();

    if (currentGameDay == 0)
        return;

    const LONG previous =
        AtomicRead(
            &g_lastGameDaySerial);

    if (previous < 0)
    {
        InterlockedExchange(
            &g_lastGameDaySerial,
            static_cast<LONG>(
                currentGameDay));
        return;
    }

    const int64_t delta =
        static_cast<int64_t>(
            currentGameDay) -
        static_cast<int64_t>(
            previous);

    if (delta == 0)
        return;

    if (delta < 0 ||
        delta > 3650)
    {
        ResetAutoCrisesInMemory();

        InterlockedExchange(
            &g_eventManagerInitialized,
            0);

        InterlockedExchange(
            &g_persistenceLoaded,
            0);

        InterlockedExchange(
            &g_persistenceValid,
            0);

        g_campaignHash = 0;

        EventManagerInitializeIfReady();
        return;
    }

    for (LONG day =
            previous + 1;
         day <=
            static_cast<LONG>(
                currentGameDay);
         ++day)
    {
        ProcessOneGameDay(
            static_cast<uint32_t>(
                day));
    }

    InterlockedExchange(
        &g_lastGameDaySerial,
        static_cast<LONG>(
            currentGameDay));

    FlushPersistentStateIfDirty();
}

// ============================================================
// F11 MANUAL LUEBECK: CRISIS -> BOOM -> OFF
// ============================================================

#if REAL_EVENTS_DEBUG_TOOLS
void ToggleManualLuebeckEvent()
{
    UpdateAddresses();

    const int luebeckIndex =
        static_cast<int>(
            AtomicRead(
                &g_luebeckTownIndex));

    if (luebeckIndex < 0)
        return;

    const LONG currentMode =
        AtomicRead(
            &g_manualLuebeckMode);

    if (currentMode == EVENT_NONE)
    {
        // El evento manual cuenta dentro del maximo global de 5
        // si Luebeck no tiene ya un evento AUTO.
        if (FindAutoCrisisSlotByTown(
                luebeckIndex) < 0 &&
            CountActiveEventCities() >=
                MAX_ACTIVE_CRISES)
        {
            RemoveShortestAutoCrisis(
                luebeckIndex);
        }

        InterlockedExchange(
            &g_manualLuebeckMode,
            EVENT_CRISIS);

        return;
    }

    if (currentMode == EVENT_CRISIS)
    {
        InterlockedExchange(
            &g_manualLuebeckMode,
            EVENT_BOOM);

        return;
    }

    // BOOM -> OFF
    InterlockedExchange(
        &g_manualLuebeckMode,
        EVENT_NONE);

    EnsureMinimumEvents(
        GetGameDaySerial());
}
#endif

// ============================================================
// INFORMER: RUMORES SOBRE EVENTOS DE PRODUCCION
// ============================================================

struct InformerEventCandidate
{
    int townIndex;
    int eventType;
    int productionPercent;
    int remainingDays;
    bool manual;
};

uint32_t HashInformerText(const char* text)
{
    uint32_t hash = 2166136261u;

    if (!text)
        return hash;

    __try
    {
        // Limite defensivo: nunca recorremos una cadena absurda.
        for (size_t i = 0; i < 4096; ++i)
        {
            const uint8_t c =
                static_cast<uint8_t>(
                    text[i]);

            hash ^= c;
            hash *= 16777619u;

            if (c == 0)
                break;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return hash;
}

int CollectInformerEventCandidates(
    InformerEventCandidate* out,
    int capacity)
{
    if (!out || capacity <= 0)
        return 0;

    int count = 0;

    const uint32_t gameDay =
        GetGameDaySerial();

#if REAL_EVENTS_DEBUG_TOOLS
    const int luebeckIndex =
        static_cast<int>(
            AtomicRead(
                &g_luebeckTownIndex));

    const int manualMode =
        static_cast<int>(
            AtomicRead(
                &g_manualLuebeckMode));

    bool luebeckAdded = false;
#endif

    // Primero los eventos AUTO.
    for (int i = 0;
         i < MAX_ACTIVE_CRISES &&
         count < capacity;
         ++i)
    {
        const int townIndex =
            static_cast<int>(
                AtomicRead(
                    &g_autoCrises[i].townIndex));

        if (townIndex < 0)
            continue;

        int eventType =
            static_cast<int>(
                AtomicRead(
                    &g_autoCrises[i].eventType));

        int percent =
            static_cast<int>(
                AtomicRead(
                    &g_autoCrises[i].productionPercent));

        const int endDay =
            static_cast<int>(
                AtomicRead(
                    &g_autoCrises[i].endGameDay));

        int remaining =
            endDay -
            static_cast<int>(
                gameDay);

        if (remaining < 0)
            remaining = 0;

        bool manual = false;

#if REAL_EVENTS_DEBUG_TOOLS
        // Si F11 esta activo en Luebeck, representa el estado efectivo
        // que esta viendo el jugador, incluso si debajo hay un AUTO.
        if (townIndex == luebeckIndex &&
            manualMode != EVENT_NONE)
        {
            eventType = manualMode;
            percent =
                manualMode == EVENT_BOOM
                ? MANUAL_BOOM_PERCENT
                : MANUAL_CRISIS_PERCENT;

            manual = true;
            remaining = -1;
            luebeckAdded = true;
        }
#endif

        if (eventType != EVENT_CRISIS &&
            eventType != EVENT_BOOM)
        {
            continue;
        }

        InformerEventCandidate& c =
            out[count++];

        c.townIndex = townIndex;
        c.eventType = eventType;
        c.productionPercent = percent;
        c.remainingDays = remaining;
        c.manual = manual;
    }

#if REAL_EVENTS_DEBUG_TOOLS
    // F11 puede existir sin AUTO en Luebeck.
    if (count < capacity &&
        manualMode != EVENT_NONE &&
        luebeckIndex >= 0 &&
        !luebeckAdded &&
        FindAutoCrisisSlotByTown(
            luebeckIndex) < 0)
    {
        InformerEventCandidate& c =
            out[count++];

        c.townIndex = luebeckIndex;
        c.eventType = manualMode;
        c.productionPercent =
            manualMode == EVENT_BOOM
            ? MANUAL_BOOM_PERCENT
            : MANUAL_CRISIS_PERCENT;

        c.remainingDays = -1;
        c.manual = true;
    }
#endif

    return count;
}

const char* GetInformerDurationText(
    int eventType,
    int remainingDays,
    bool manual)
{
    if (manual || remainingDays < 0)
    {
        return "How long it will last is difficult to say.";
    }

    if (remainingDays <= 30)
    {
        return eventType == EVENT_BOOM
            ? "The favorable conditions are not expected to last much longer."
            : "The situation is expected to improve soon.";
    }

    if (remainingDays <= 90)
    {
        return eventType == EVENT_BOOM
            ? "The favorable conditions may continue for a few months."
            : "The difficulties may continue for a few months.";
    }

    if (remainingDays <= 180)
    {
        return eventType == EVENT_BOOM
            ? "The favorable conditions are expected to continue for quite some time."
            : "The situation is expected to last for quite some time.";
    }

    return eventType == EVENT_BOOM
        ? "There is no sign of the favorable conditions ending soon."
        : "There is no sign of the difficulties ending soon.";
}

void BuildInformerEventRumor(
    const InformerEventCandidate& event,
    char* output,
    size_t capacity)
{
    if (!output || capacity == 0)
        return;

    output[0] = '\0';

    const char* townName =
        GetTownNameByIndex(
            event.townIndex);

    const char* durationText =
        GetInformerDurationText(
            event.eventType,
            event.remainingDays,
            event.manual);

    if (event.eventType == EVENT_CRISIS)
    {
        const char* severity = nullptr;

        if (event.productionPercent <= 45)
            severity = "a severe production crisis";
        else if (event.productionPercent <= 52)
            severity = "a serious production crisis";
        else
            severity = "a slowdown in production";

        _snprintf_s(
            output,
            capacity,
            _TRUNCATE,
            "Merchants report %s in %s. %s",
            severity,
            townName,
            durationText);

        return;
    }

    const char* strength = nullptr;

    if (event.productionPercent >= 155)
        strength = "an exceptional production boom";
    else if (event.productionPercent >= 148)
        strength = "a strong production boom";
    else
        strength = "unusually high production";

    _snprintf_s(
        output,
        capacity,
        _TRUNCATE,
        "Merchants report %s in %s. %s",
        strength,
        townName,
        durationText);
}

const char* __cdecl BuildInformerTextHelper(
    const char* vanillaText)
{
    InterlockedIncrement(
        &g_informerHits);

    if (!vanillaText)
        return vanillaText;

    InformerEventCandidate candidates[
        MAX_ACTIVE_CRISES
    ] = {};

    const int count =
        CollectInformerEventCandidates(
            candidates,
            MAX_ACTIVE_CRISES);

    if (count <= 0)
        return vanillaText;

    // Seleccion estable mientras se muestre la misma conversacion:
    // mismo texto vanilla + mismo dia de juego = mismo rumor.
    uint32_t selector =
        HashInformerText(
            vanillaText);

    selector ^=
        GetGameDaySerial() *
        0x9E3779B9u;

    const int selectedIndex =
        static_cast<int>(
            selector %
            static_cast<uint32_t>(
                count));

    const InformerEventCandidate& selected =
        candidates[selectedIndex];

    char rumor[512] = {};

    BuildInformerEventRumor(
        selected,
        rumor,
        sizeof(rumor));

    if (rumor[0] == '\0')
        return vanillaText;

    __try
    {
        // El juego ya acepta LF en estos textos.
        // No tocamos el buffer vanilla: construimos uno propio.
        _snprintf_s(
            g_informerTextBuffer,
            INFORMER_TEXT_BUFFER_SIZE,
            _TRUNCATE,
            "%s\n%s",
            vanillaText,
            rumor);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return vanillaText;
    }

    InterlockedIncrement(
        &g_informerRumorsAdded);

    InterlockedExchange(
        &g_informerLastTownIndex,
        selected.townIndex);

    InterlockedExchange(
        &g_informerLastEventType,
        selected.eventType);

    return g_informerTextBuffer;
}

// El hook sustituye las dos instrucciones originales:
// 005D1399  push eax
// 005D139A  push 006C66BC
//
// Al volver a 005D139F, el juego ejecuta su "push ecx" y
// continua por la ruta vanilla.
__declspec(naked)
void InformerTextHook()
{
    __asm
    {
        // ECX se necesita inmediatamente despues del hook
        // (005D139F push ecx), por eso lo preservamos.
        push ecx
        push edx

        push eax
        call BuildInformerTextHelper
        add esp, 4

        pop edx
        pop ecx

        // Reproducir instrucciones originales con EAX ya sustituido.
        push eax
        push dword ptr [g_informerFormatString]

        jmp dword ptr [g_informerHookReturnAddress]
    }
}

// ============================================================
// APLICAR MULTIPLICADOR DE PRODUCCION
// ============================================================

uint32_t ApplyProductionPercent(
    uint32_t original,
    int percent)
{
    if (percent == 100)
        return original;

    if (percent <= 0)
        return 0;

    // 64 bits para evitar overflow.
    const uint64_t scaled =
        static_cast<uint64_t>(
            original) *
        static_cast<uint64_t>(
            percent);

    // Redondeo al entero mas cercano.
    return static_cast<uint32_t>(
        (scaled + 50u) / 100u);
}

// ============================================================
// HOOK PRIVADO / TRADER
// ============================================================

void __cdecl PrivateEnterHelper(
    uintptr_t entry)
{
    InterlockedIncrement(
        &g_privateHits);

    if (g_privateDepth >=
        HOOK_FRAME_CAPACITY)
    {
        InterlockedIncrement(
            &g_privateStackOverflows);

        return;
    }

    ProductionFrame& frame =
        g_privateFrames[
            g_privateDepth++];

    frame.entry = entry;
    frame.originalFactor = 0;
    frame.modified = false;

    __try
    {
        FacilityLayout* facility =
            reinterpret_cast<FacilityLayout*>(
                entry);

        const int townIndex =
            static_cast<int>(
                facility->townIndex);

        const int percent =
            GetProductionPercent(
                townIndex);

        if (percent == 100)
            return;

        InterlockedIncrement(
            &g_privateAffectedHits);

        const uint32_t original =
            facility->efficiency;

        frame.originalFactor =
            original;

        if (original >= 2)
        {
            facility->efficiency =
                ApplyProductionPercent(
                    original,
                    percent);

            frame.modified = true;

            InterlockedIncrement(
                &g_privateModifiedHits);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        frame.modified = false;
    }
}

void __cdecl PrivateExitHelper()
{
    if (g_privateDepth <= 0)
        return;

    ProductionFrame& frame =
        g_privateFrames[
            --g_privateDepth];

    if (!frame.modified ||
        !frame.entry)
    {
        return;
    }

    __try
    {
        reinterpret_cast<FacilityLayout*>(
            frame.entry)->efficiency =
            frame.originalFactor;

        InterlockedIncrement(
            &g_privateRestoredHits);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

__declspec(naked)
void PrivateEntryHook()
{
    __asm
    {
        pushfd
        pushad

        push esi
        call PrivateEnterHelper
        add esp, 4

        popad
        popfd

        // Original:
        // mov al,[esi+06]
        // cmp eax,14
        mov al, [esi + 06h]
        cmp eax, 14h

        jmp dword ptr[g_privateEntryReturnAddress]
    }
}

__declspec(naked)
void PrivateExitHook()
{
    __asm
    {
        pushfd
        pushad

        call PrivateExitHelper

        popad
        popfd

        // Original
        pop edi
        pop esi
        pop ebp
        pop ebx
        pop ecx

        jmp dword ptr[g_privateExitReturnAddress]
    }
}

// ============================================================
// HOOK TOWN
// ============================================================

void __cdecl TownEnterHelper(
    uintptr_t entry)
{
    InterlockedIncrement(
        &g_townHits);

    if (g_townDepth >=
        HOOK_FRAME_CAPACITY)
    {
        InterlockedIncrement(
            &g_townStackOverflows);

        return;
    }

    ProductionFrame& frame =
        g_townFrames[
            g_townDepth++];

    frame.entry = entry;
    frame.originalFactor = 0;
    frame.modified = false;

    __try
    {
        FacilityLayout* facility =
            reinterpret_cast<FacilityLayout*>(
                entry);

        const int townIndex =
            static_cast<int>(
                facility->townIndex);

        const int percent =
            GetProductionPercent(
                townIndex);

        if (percent == 100)
            return;

        InterlockedIncrement(
            &g_townAffectedHits);

        const uint32_t original =
            facility->efficiency;

        frame.originalFactor =
            original;

        if (original >= 2)
        {
            facility->efficiency =
                ApplyProductionPercent(
                    original,
                    percent);

            frame.modified = true;

            InterlockedIncrement(
                &g_townModifiedHits);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        frame.modified = false;
    }
}

void __cdecl TownExitHelper()
{
    if (g_townDepth <= 0)
        return;

    ProductionFrame& frame =
        g_townFrames[
            --g_townDepth];

    if (!frame.modified ||
        !frame.entry)
    {
        return;
    }

    __try
    {
        reinterpret_cast<FacilityLayout*>(
            frame.entry)->efficiency =
            frame.originalFactor;

        InterlockedIncrement(
            &g_townRestoredHits);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

__declspec(naked)
void TownEntryHook()
{
    __asm
    {
        pushfd
        pushad

        push ecx
        call TownEnterHelper
        add esp, 4

        popad
        popfd

        // Original 005101D0-005101D5
        sub esp, 08h
        push esi
        push edi
        mov edi, ecx

        jmp dword ptr[g_townEntryReturnAddress]
    }
}

__declspec(naked)
void TownExitHook()
{
    __asm
    {
        pushfd
        pushad

        call TownExitHelper

        popad
        popfd

        // Original de las 3 salidas
        pop edi
        pop esi
        add esp, 08h
        ret 4
    }
}

// ============================================================
// INSTALADORES DE HOOKS
// ============================================================

bool InstallRelativeHook(
    uintptr_t hookAddress,
    const BYTE* expectedBytes,
    size_t patchSize,
    void* hookFunction,
    uintptr_t* returnAddress)
{
    if (patchSize < 5 ||
        patchSize > 16)
    {
        return false;
    }

    __try
    {
        if (memcmp(
            reinterpret_cast<void*>(
                hookAddress),
            expectedBytes,
            patchSize) != 0)
        {
            return false;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    *returnAddress =
        hookAddress + patchSize;

    DWORD oldProtect = 0;

    if (!VirtualProtect(
        reinterpret_cast<void*>(
            hookAddress),
        patchSize,
        PAGE_EXECUTE_READWRITE,
        &oldProtect))
    {
        return false;
    }

    BYTE patch[16];
    memset(
        patch,
        0x90,
        sizeof(patch));

    patch[0] = 0xE9;

    const intptr_t relativeJump =
        reinterpret_cast<uintptr_t>(
            hookFunction)
        - (hookAddress + 5);

    *reinterpret_cast<int32_t*>(
        &patch[1]) =
        static_cast<int32_t>(
            relativeJump);

    memcpy(
        reinterpret_cast<void*>(
            hookAddress),
        patch,
        patchSize);

    FlushInstructionCache(
        GetCurrentProcess(),
        reinterpret_cast<void*>(
            hookAddress),
        patchSize);

    DWORD dummy = 0;

    VirtualProtect(
        reinterpret_cast<void*>(
            hookAddress),
        patchSize,
        oldProtect,
        &dummy);

    return true;
}

bool InstallTerminalHook(
    uintptr_t hookAddress,
    const BYTE* expectedBytes,
    size_t patchSize,
    void* hookFunction)
{
    if (patchSize < 5 ||
        patchSize > 16)
    {
        return false;
    }

    __try
    {
        if (memcmp(
            reinterpret_cast<void*>(
                hookAddress),
            expectedBytes,
            patchSize) != 0)
        {
            return false;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    DWORD oldProtect = 0;

    if (!VirtualProtect(
        reinterpret_cast<void*>(
            hookAddress),
        patchSize,
        PAGE_EXECUTE_READWRITE,
        &oldProtect))
    {
        return false;
    }

    BYTE patch[16];
    memset(
        patch,
        0x90,
        sizeof(patch));

    patch[0] = 0xE9;

    const intptr_t relativeJump =
        reinterpret_cast<uintptr_t>(
            hookFunction)
        - (hookAddress + 5);

    *reinterpret_cast<int32_t*>(
        &patch[1]) =
        static_cast<int32_t>(
            relativeJump);

    memcpy(
        reinterpret_cast<void*>(
            hookAddress),
        patch,
        patchSize);

    FlushInstructionCache(
        GetCurrentProcess(),
        reinterpret_cast<void*>(
            hookAddress),
        patchSize);

    DWORD dummy = 0;

    VirtualProtect(
        reinterpret_cast<void*>(
            hookAddress),
        patchSize,
        oldProtect,
        &dummy);

    return true;
}

bool InstallInformerTextHook()
{
    HMODULE gameModule =
        GetModuleHandleW(nullptr);

    if (!gameModule)
        return false;

    const uintptr_t base =
        reinterpret_cast<uintptr_t>(
            gameModule);

    const uintptr_t hookAddress =
        base +
        INFORMER_TEXT_HOOK_RVA;

    // 005D1399 push eax
    // 005D139A push 006C66BC
    const BYTE expectedBytes[6] =
    {
        0x50,
        0x68, 0xBC, 0x66, 0x6C, 0x00
    };

    if (!InstallRelativeHook(
            hookAddress,
            expectedBytes,
            sizeof(expectedBytes),
            reinterpret_cast<void*>(
                &InformerTextHook),
            &g_informerHookReturnAddress))
    {
        return false;
    }

    InterlockedExchange(
        &g_informerHookInstalled,
        1);

    return true;
}

bool InstallPrivateProductionHook()
{
    HMODULE gameModule =
        GetModuleHandleW(nullptr);

    if (!gameModule)
        return false;

    const uintptr_t base =
        reinterpret_cast<uintptr_t>(
            gameModule);

    const uintptr_t exitAddress =
        base +
        PRIVATE_EXIT_HOOK_RVA;

    const uintptr_t entryAddress =
        base +
        PRIVATE_ENTRY_HOOK_RVA;

    const BYTE expectedExit[5] =
    {
        0x5F,
        0x5E,
        0x5D,
        0x5B,
        0x59
    };

    if (!InstallRelativeHook(
        exitAddress,
        expectedExit,
        sizeof(expectedExit),
        reinterpret_cast<void*>(
            &PrivateExitHook),
        &g_privateExitReturnAddress))
    {
        return false;
    }

    InterlockedExchange(
        &g_privateExitHookInstalled,
        1);

    const BYTE expectedEntry[6] =
    {
        0x8A, 0x46, 0x06,
        0x83, 0xF8, 0x14
    };

    if (!InstallRelativeHook(
        entryAddress,
        expectedEntry,
        sizeof(expectedEntry),
        reinterpret_cast<void*>(
            &PrivateEntryHook),
        &g_privateEntryReturnAddress))
    {
        return false;
    }

    InterlockedExchange(
        &g_privateEntryHookInstalled,
        1);

    return true;
}

bool InstallTownProductionHook()
{
    HMODULE gameModule =
        GetModuleHandleW(nullptr);

    if (!gameModule)
        return false;

    const uintptr_t base =
        reinterpret_cast<uintptr_t>(
            gameModule);

    const BYTE expectedTownExit[8] =
    {
        0x5F,
        0x5E,
        0x83, 0xC4, 0x08,
        0xC2, 0x04, 0x00
    };

    const uintptr_t exits[3] =
    {
        base +
            TOWN_EXIT1_HOOK_RVA,

        base +
            TOWN_EXIT2_HOOK_RVA,

        base +
            TOWN_EXIT3_HOOK_RVA
    };

    // Primero las tres salidas.
    // Si falla una, no instalamos entrada.
    for (int i = 0;
        i < 3;
        ++i)
    {
        if (!InstallTerminalHook(
            exits[i],
            expectedTownExit,
            sizeof(expectedTownExit),
            reinterpret_cast<void*>(
                &TownExitHook)))
        {
            return false;
        }
    }

    InterlockedExchange(
        &g_townExitHooksInstalled,
        1);

    const uintptr_t entryAddress =
        base +
        TOWN_ENTRY_HOOK_RVA;

    const BYTE expectedTownEntry[7] =
    {
        0x83, 0xEC, 0x08,
        0x56,
        0x57,
        0x8B, 0xF9
    };

    if (!InstallRelativeHook(
        entryAddress,
        expectedTownEntry,
        sizeof(expectedTownEntry),
        reinterpret_cast<void*>(
            &TownEntryHook),
        &g_townEntryReturnAddress))
    {
        return false;
    }

    InterlockedExchange(
        &g_townEntryHookInstalled,
        1);

    return true;
}

// ============================================================
// DEBUG
// ============================================================

#if REAL_EVENTS_DEBUG_TOOLS
int GetManualLuebeckPercent()
{
    const LONG mode =
        AtomicRead(
            &g_manualLuebeckMode);

    if (mode == EVENT_CRISIS)
        return MANUAL_CRISIS_PERCENT;

    if (mode == EVENT_BOOM)
        return MANUAL_BOOM_PERCENT;

    return 100;
}

void AppendText(
    char* buffer,
    size_t capacity,
    size_t& used,
    const char* format,
    ...)
{
    if (!buffer ||
        used >= capacity)
    {
        return;
    }

    va_list args;
    va_start(args, format);

    const int written =
        _vsnprintf_s(
            buffer + used,
            capacity - used,
            _TRUNCATE,
            format,
            args);

    va_end(args);

    if (written > 0)
    {
        used +=
            static_cast<size_t>(
                written);
    }
    else
    {
        used =
            strlen(buffer);
    }
}

void ShowDebugInfo()
{
    UpdateAddresses();
    EventManagerInitializeIfReady();

    char text[6144] = {};
    size_t used = 0;

    const int day =
        GetDayOfMonth();

    const int month =
        GetMonth();

    const int year =
        GetYear();

    const int dayOfYear =
        GetDayOfYear();

    const uint32_t gameDay =
        GetGameDaySerial();

    const int activeCount =
        CountActiveEventCities();

    const LONG nextEventDay =
        AtomicRead(
            &g_nextAutoEventGameDay);

    const LONG daysToNext =
        nextEventDay >
            static_cast<LONG>(gameDay)
        ? nextEventDay -
            static_cast<LONG>(gameDay)
        : 0;

    AppendText(
        text,
        sizeof(text),
        used,
        "REAL EVENTS v" REAL_EVENTS_VERSION "\n"
        "\n"
        "Fecha: %02d/%02d/%04d\n"
        "Dia del ano: %d\n"
        "Game day: %u\n"
        "Dias procesados esta sesion: %ld\n"
        "Eventos activos: %d / %d\n"
        "Proximo evento AUTO: %ld dias\n"
        "Persistencia: %s\n"
        "Campaign hash: %08X\n"
        "\n"
        "EVENTOS ACTIVOS\n",
        day,
        month,
        year,
        dayOfYear,
        gameDay,
        AtomicRead(
            &g_daysProcessed),
        activeCount,
        MAX_ACTIVE_CRISES,
        daysToNext,
        AtomicRead(
            &g_persistenceValid)
            ? (AtomicRead(
                    &g_persistenceLoaded)
                ? "CARGADA"
                : "NUEVA/GUARDADA")
            : "NO DISPONIBLE",
        g_campaignHash);

    bool anyShown = false;

    for (int i = 0;
         i < MAX_ACTIVE_CRISES;
         ++i)
    {
        const int townIndex =
            static_cast<int>(
                AtomicRead(
                    &g_autoCrises[i].townIndex));

        if (townIndex < 0)
            continue;

        const int percent =
            static_cast<int>(
                AtomicRead(
                    &g_autoCrises[i].productionPercent));

        const int eventType =
            static_cast<int>(
                AtomicRead(
                    &g_autoCrises[i].eventType));

        const int endDay =
            static_cast<int>(
                AtomicRead(
                    &g_autoCrises[i].endGameDay));

        const int remainingRaw =
            endDay -
            static_cast<int>(
                gameDay);

        const int remaining =
            remainingRaw > 0
            ? remainingRaw
            : 0;

        const int luebeckIndex =
            static_cast<int>(
                AtomicRead(
                    &g_luebeckTownIndex));

        const bool alsoManual =
            AtomicRead(
                &g_manualLuebeckMode) != 0 &&
            townIndex ==
                luebeckIndex;

        const int effectivePercent =
            GetProductionPercent(
                townIndex);

        const int storedTownIndex =
            GetStoredTownIndexByArrayIndex(
                townIndex);

        AppendText(
            text,
            sizeof(text),
            used,
            "\n[%s%s] %s (idx %d / stored %d / raw %02X)\n"
            "  Produccion: %d%%",
            alsoManual
                ? "AUTO+F11 "
                : "AUTO ",
            GetEventTypeName(
                eventType),
            GetTownNameByIndex(
                townIndex),
            townIndex,
            storedTownIndex,
            GetRawTownIdByIndex(
                townIndex),
            effectivePercent);

        if (alsoManual &&
            effectivePercent != percent)
        {
            AppendText(
                text,
                sizeof(text),
                used,
                " (AUTO %s %d%% / F11 %s %d%%)",
                GetEventTypeName(
                    eventType),
                percent,
                GetEventTypeName(
                    static_cast<int>(
                        AtomicRead(
                            &g_manualLuebeckMode))),
                effectivePercent);
        }

        AppendText(
            text,
            sizeof(text),
            used,
            "\n  Restante AUTO: %d dias (~%d meses)\n",
            remaining,
            (remaining + 29) / 30);

        anyShown = true;
    }

    if (ManualLuebeckIsSeparateEventCity())
    {
        const int luebeckIndex =
            static_cast<int>(
                AtomicRead(
                    &g_luebeckTownIndex));

        const int manualMode =
            static_cast<int>(
                AtomicRead(
                    &g_manualLuebeckMode));

        const int manualPercent =
            GetManualLuebeckPercent();

        AppendText(
            text,
            sizeof(text),
            used,
            "\n[F11 %s] %s (idx %d)\n"
            "  Produccion: %d%%\n"
            "  Duracion: hasta cambiar/desactivar F11\n",
            GetEventTypeName(
                manualMode),
            GetTownNameByIndex(
                luebeckIndex),
            luebeckIndex,
            manualPercent);

        anyShown = true;
    }

    if (!anyShown)
    {
        AppendText(
            text,
            sizeof(text),
            used,
            "\n(ninguna)\n");
    }

    AppendText(
        text,
        sizeof(text),
        used,
        "\n"
        "PRIVADO / TRADER\n"
        "Hook entrada: %s\n"
        "Hook salida: %s\n"
        "Hits totales: %ld\n"
        "Hits en ciudades con crisis: %ld\n"
        "Modificados: %ld\n"
        "Restaurados: %ld\n"
        "Depth: %d\n"
        "Overflows: %ld\n"
        "\n"
        "TOWN\n"
        "Hook entrada: %s\n"
        "Hooks salida (3): %s\n"
        "Hits totales: %ld\n"
        "Hits en ciudades con crisis: %ld\n"
        "Modificados: %ld\n"
        "Restaurados: %ld\n"
        "Depth: %d\n"
        "Overflows: %ld\n"
        "\n"
        "INFORMER\n"
        "Hook: %s\n"
        "Render hits: %ld\n"
        "Rumores anadidos: %ld\n"
        "Ultimo evento: %s / %s\n"
        "\n"
        "DEBUG TOOLS\n"
        "F10 = +100.000\n"
        "F11 = Luebeck: CRISIS -> BOOM -> OFF\n"
        "º = Debug",

        // PRIVADO / TRADER
        AtomicRead(
            &g_privateEntryHookInstalled)
            ? "SI" : "NO",

        AtomicRead(
            &g_privateExitHookInstalled)
            ? "SI" : "NO",

        AtomicRead(
            &g_privateHits),

        AtomicRead(
            &g_privateAffectedHits),

        AtomicRead(
            &g_privateModifiedHits),

        AtomicRead(
            &g_privateRestoredHits),

        g_privateDepth,

        AtomicRead(
            &g_privateStackOverflows),

        // TOWN
        AtomicRead(
            &g_townEntryHookInstalled)
            ? "SI" : "NO",

        AtomicRead(
            &g_townExitHooksInstalled)
            ? "SI" : "NO",

        AtomicRead(
            &g_townHits),

        AtomicRead(
            &g_townAffectedHits),

        AtomicRead(
            &g_townModifiedHits),

        AtomicRead(
            &g_townRestoredHits),

        g_townDepth,

        AtomicRead(
            &g_townStackOverflows),

        // INFORMER
        AtomicRead(
            &g_informerHookInstalled)
            ? "SI" : "NO",

        AtomicRead(
            &g_informerHits),

        AtomicRead(
            &g_informerRumorsAdded),

        GetEventTypeName(
            static_cast<int>(
                AtomicRead(
                    &g_informerLastEventType))),

        GetTownNameByIndex(
            static_cast<int>(
                AtomicRead(
                    &g_informerLastTownIndex))));

    MessageBoxA(
        nullptr,
        text,
        "Patrician III - Real Events v" REAL_EVENTS_VERSION,
        MB_OK |
        MB_ICONINFORMATION);
}
#endif

// ============================================================
// THREAD
// ============================================================

DWORD WINAPI ModThread(LPVOID)
{
#if REAL_EVENTS_DEBUG_TOOLS
    bool previousF10 = false;
    bool previousF11 = false;
    bool previousDebug = false;
#endif

    while (true)
    {
        UpdateAddresses();
        EventManagerPollGameDay();
        FlushPersistentStateIfDirty();

#if REAL_EVENTS_DEBUG_TOOLS
        const bool currentF10 =
            (GetAsyncKeyState(
                VK_F10) &
                0x8000) != 0;

        if (currentF10 &&
            !previousF10)
        {
            AddMoney(
                MONEY_INCREMENT);
        }

        previousF10 =
            currentF10;

        const bool currentF11 =
            (GetAsyncKeyState(
                VK_F11) &
                0x8000) != 0;

        if (currentF11 &&
            !previousF11)
        {
            ToggleManualLuebeckEvent();
        }

        previousF11 =
            currentF11;

        const bool currentDebug =
            (GetAsyncKeyState(
                VK_OEM_5) &
                0x8000) != 0;

        if (currentDebug &&
            !previousDebug)
        {
            ShowDebugInfo();
        }

        previousDebug =
            currentDebug;
#endif

        Sleep(30);
    }

    return 0;
}

// ============================================================
// START
// ============================================================

extern "C"
__declspec(dllexport)
int start()
{
    UpdateAddresses();

    InstallPrivateProductionHook();
    InstallTownProductionHook();
    InstallInformerTextHook();

    HANDLE thread =
        CreateThread(
            nullptr,
            0,
            ModThread,
            nullptr,
            0,
            nullptr);

    if (!thread)
        return 2;

    CloseHandle(thread);

    return 0;
}

// ============================================================
// DLLMAIN
// ============================================================

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD reason,
    LPVOID)
{
    if (reason ==
        DLL_PROCESS_ATTACH)
    {
        g_moduleHandle = hModule;

        DisableThreadLibraryCalls(
            hModule);
    }

    return TRUE;
}
