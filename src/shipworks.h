#pragma once
#include <basetsd.h>

typedef DWORD64 pointer;

static const DWORD64 EditorParts = 0x143a139f0;

enum EditorLibraries : DWORD64 {
	HullLibrary = 0x1403bd400,
	ArmorLibrary = 0x1403bd418,
	FuelLibrary = 0x1403bd430,
	AmmoLibrary = 0x1403bd448,
	LegLibrary = 0x1403bd460,
	EngineLibrary = 0x1403bd478,
	GunLibrary = 0x1403bd490,
	SensorLibrary = 0x1403bd4a8,
	ExpendablesLibrary = 0x1403bd4c0,
	CraftLibrary = 0x1403bd4d8,
	DefencesLibrary = 0x1403bd4f0,
	GeneratorLibrary = 0x1403bd508,
	QuartersLibrary = 0x1403bd520,
	EmergencyLibrary = 0x1403bd538
};

extern "C" {
	void DefinePart(pointer editorParts, const char* moid, pointer editorLibrary);
	void CreateLibrary(EditorLibraries library, const char* libraryName);
}

void CreateLibraries();

void DefineVanillaHulls();
void DefineVanillaArmor();
void DefineVanillaFuel();
void DefineVanillaAmmo();
void DefineVanillaLegs();
void DefineVanillaEngines();
void DefineVanillaGuns();
void DefineVanillaSensors();
void DefineVanillaExpendables();
void DefineVanillaCrafts();
void DefineVanillaDefences();
void DefineVanillaGenerators();
void DefineVanillaQuarters();
void DefineVanillaEmergencies();

void DefineVanillaParts();