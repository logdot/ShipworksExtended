#include <cstdint>
#include <cstring>

// --- Game Function Definitions ---
// Addresses derived from original ASM hooks

typedef void* (*New_t)(size_t size);
New_t New = (New_t)0x140334094;

typedef void* (*CreateLibraryEx_t)(void* obj);
CreateLibraryEx_t CreateLibraryEx = (CreateLibraryEx_t)0x14027f200;

typedef void (*MakeGlobalEx_t)(void* arg1, void* obj);
MakeGlobalEx_t MakeGlobalEx = (MakeGlobalEx_t)0x140194de0;

typedef void (*CopyStringEx_t)(void* dest, char* src, size_t len);
CopyStringEx_t CopyStringEx = (CopyStringEx_t)0x1400056e0;

typedef void* (*DefinePartEx_t)(void* parts, const char* id, void* lib, int flag);
DefinePartEx_t DefinePartEx = (DefinePartEx_t)0x14021b190;

// --- Replacement Functions ---

extern "C" {

    void DefinePart(void** editorParts, const char* moid, void** editorLibrary) {
        void* result = DefinePartEx(*editorParts, moid, *editorLibrary, 1);
        if (result) {
            *(int*)((char*)result + 0x7a8) = 0x3E7;
        }
    }

    void CreateLibrary(void** libraryPtr, char* libraryName) {
        void* newObj = New(0x128);
        if (!newObj) return;

        void* initializedObj = CreateLibraryEx(newObj);

        MakeGlobalEx((char*)libraryPtr - 8, initializedObj);

        void* destString = (char*)(*libraryPtr) + 0xC8;
        CopyStringEx(destString, libraryName, 0x13);
    }
}
