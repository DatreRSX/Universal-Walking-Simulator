#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <string>
#include <locale>
#include <format>
#include <iostream>
#include <chrono>
#include <thread>

#include "other.h"
#include "xorstr.hpp" 
#include <regex>
#include "patterns.h" // we need this for the #ifndef BEFORE_SEASONEIGHT

using namespace std::chrono;

#define INL __forceinline

static inline void (*ToStringO)(struct FName*, class FString&);
static inline void* (*ProcessEventO)(void*, void*, void*);

std::string FN_Version = "0.0";
int Engine_Version;

static struct FChunkedFixedUObjectArray* ObjObjects;
static struct FFixedUObjectArray* OldObjects;

namespace FMemory
{
	void (*Free)(void* Original);
	void* (*Realloc)(void* Original, SIZE_T Count, uint32_t Alignment /* = DEFAULT_ALIGNMENT */);
}

struct Timer
{
	time_point<std::chrono::steady_clock> start, end;
	duration<float> dura;

	Timer()
	{
		start = high_resolution_clock::now();
	}

	~Timer()
	{
		end = high_resolution_clock::now();
		dura = end - start;

		float ns = dura.count() * 10000.0f;
		std::cout << _("Took ") << ns << _("ns \n");
	}

	// You would do "Timer* t = new Timer;" and then delete at the end of the function.
};

template <class ElementType>
class TArray // https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Source/Runtime/Core/Public/Containers/Array.h#L305
{
	friend class FString;
protected:
	ElementType* Data = nullptr;
	int32_t ArrayNum = 0;
	int32_t ArrayMax = 0;
public:
	void Free()
	{
		if (FMemory::Free)
			FMemory::Free(Data);

		Data = nullptr;

		ArrayNum = 0;
		ArrayMax = 0;
	}

	INL auto Num() const { return ArrayNum;  }

	INL ElementType& operator[](int Index) const { return Data[Index]; }

	INL ElementType& At(int Index) const { return Data[Index]; }

	INL int32_t Slack() const
	{
		return ArrayMax - ArrayNum;
	}

	void Reserve(int Number, int Size = sizeof(ElementType))
	{
		/* if (!FMemory::Realloc)
		{
			MessageBoxA(0, _("How are you expecting to reserve with no Realloc?"), _("Universal Walking Simulator"), MB_ICONERROR);
			return;
		} */

		// if (Number > ArrayMax)
		{
			// Data = (ElementType*)realloc(Data, Size * (ArrayNum + 1));
			Data = Slack() >= Number ? Data : (ElementType*)FMemory::Realloc(Data, (ArrayMax = ArrayNum + Number) * Size, 0);
		}
	}

	int Add(const ElementType& New, int Size = sizeof(ElementType))
	{
		Reserve(1, Size);
		if (Data)
		{
			Data[ArrayNum] = New;
			++ArrayNum;
			return ArrayNum; // - 1;
		}
		std::cout << _("Invalid Data when adding!\n");

		/*
		
		if (Data)
		{
			Data = (ElementType*)realloc(Data, sizeof(ElementType) * (ArrayNum + 1));
			Data[ArrayNum] = New;
			++ArrayNum;
			return ArrayNum - 1;
		}

		*/

		return -1;
	};


	void RemoveAtSwapImpl(int Index, int Count = 1, bool bAllowShrinking = true)
	{
		if (Count)
		{
			// CheckInvariants();
			// checkSlow((Count >= 0) & (Index >= 0) & (Index + Count <= ArrayNum));

			// DestructItems(GetData() + Index, Count);

			// Replace the elements in the hole created by the removal with elements from the end of the array, so the range of indices used by the array is contiguous.
			const int NumElementsInHole = Count;
			const int NumElementsAfterHole = ArrayNum - (Index + Count);
			const int NumElementsToMoveIntoHole = min(NumElementsInHole, NumElementsAfterHole);
			if (NumElementsToMoveIntoHole)
			{
				memcpy(// FMemory::Memcpy(
					(uint8_t*)Data + (Index) * sizeof(ElementType),
					(uint8_t*)Data + (ArrayNum - NumElementsToMoveIntoHole) * sizeof(ElementType),
					NumElementsToMoveIntoHole * sizeof(ElementType)
				);
			}
			ArrayNum -= Count;

			if (bAllowShrinking)
			{
				// ResizeShrink();
			}
		}
	}

	inline void RemoveAtSwap(int Index)
	{
		RemoveAtSwapImpl(Index, 1, true);
	}

	inline bool RemoveAt(const int Index) // NOT MINE
	{
		if (Index < ArrayNum)
		{
			if (Index != ArrayNum - 1)
				Data[Index] = Data[ArrayNum - 1];

			--ArrayNum;

			return true;
		}
		return false;
	};

	INL auto GetData() const { return Data; }
};

class FString // https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Source/Runtime/Core/Public/Containers/UnrealString.h#L59
{
public:
	TArray<TCHAR> Data;

	void Set(const wchar_t* NewStr) // by fischsalat
	{
		if (!NewStr || std::wcslen(NewStr) == 0) return;

		Data.ArrayMax = Data.ArrayNum = *NewStr ? (int)std::wcslen(NewStr) + 1 : 0;

		if (Data.ArrayNum)
			Data.Data = const_cast<wchar_t*>(NewStr);
	}

	std::string ToString() const
	{
		auto length = std::wcslen(Data.Data);
		std::string str(length, '\0');
		std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data.Data, Data.Data + length, '?', &str[0]);

		return str;
	}

	INL void operator=(const std::string& str) { Set(std::wstring(str.begin(), str.end()).c_str()); }

	void FreeString()
	{
		Data.Free();
	}

	FString()
	{
		// const wchar_t* Lmaoo = new wchar_t[1000]();
		// Set(Lmaoo);
	}

	~FString()
	{
		// FreeString();
	}

	/*

	template <typename StrType>
	FORCEINLINE FString& operator+=(StrType&& Str)	{ return Append(Forward<StrType>(Str)); }

	FORCEINLINE FString& operator+=(ANSICHAR Char) { return AppendChar(Char); }
	FORCEINLINE FString& operator+=(WIDECHAR Char) { return AppendChar(Char); }
	FORCEINLINE FString& operator+=(UCS2CHAR Char) { return AppendChar(Char); }

	*/

	// Ill add setting and stuff soon, check out argon's FString if you need.
};

struct FName // https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/Core/Public/UObject/NameTypes.h#L403
{
	uint32_t ComparisonIndex;
	uint32_t Number;

	INL std::string ToString()
	{
		FString temp;

		ToStringO(this, temp);

		auto Str = temp.ToString();

		temp.FreeString();

		return Str;
	}
};

template <typename Fn>
inline Fn GetVFunction(const void* instance, std::size_t index)
{
	auto vtable = *reinterpret_cast<const void***>(const_cast<void*>(instance));
	return reinterpret_cast<Fn>(vtable[index]);
}

struct UObject // https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectBase.h#L20
{
	void** VFTable;
	int32_t ObjectFlags;
	int32_t InternalIndex;
	UObject* ClassPrivate; // Keep it an object because the we will have to cast it to the correct type depending on the version.
	FName NamePrivate;
	UObject* OuterPrivate;

	INL std::string GetName() { return NamePrivate.ToString(); }

	INL std::string GetFullName()
	{
		std::string temp;

		for (auto outer = OuterPrivate; outer; outer = outer->OuterPrivate)
			temp = std::format("{}.{}", outer->GetName(), temp);

		return std::format("{} {}{}", ClassPrivate->GetName(), temp, this->GetName());
	}

	INL std::string GetFullNameT()
	{
		std::string temp;

		for (auto outer = OuterPrivate; outer; outer = outer->OuterPrivate)
			temp = std::format("{}.{}", outer->GetName(), temp);

		return temp + this->GetName();
	}

	template <typename MemberType>
	INL MemberType* Member(const std::string& MemberName, int extraOffset = 0);

	bool IsA(UObject* cmp) const;

	INL struct UFunction* Function(const std::string& FuncName);

	INL auto ProcessEvent(UObject* Function, void* Params = nullptr)
	{
		return ProcessEventO(this, Function, Params);
	}

	INL void* ProcessEvent(const std::string& FuncName, void* Params = nullptr)
	{
		auto fn = this->Function(FuncName); // static?
		if (!fn)
		{
			std::cout << _("[ERROR] Unable to find ") << FuncName << '\n';
			return nullptr;
		}
		return ProcessEvent((UObject*)fn, Params);
	}

	UObject* CreateDefaultObject()
	{
		auto FnVerDouble = std::stod(FN_Version);

		static auto Index = 0;

		if (Index == 0)
		{
			if (Engine_Version < 421)
				Index = 101;
			else if (Engine_Version > 420 && FnVerDouble < 7.40)
				Index = 102;
			else if (FnVerDouble >= 7.40)
				Index = 103; // VERIFIED FOR 7.40T
			else
				std::cout << _("Unable to determine CreateDefaultObject Index!\n");
		}

		if (Index != 0)
			return GetVFunction<UObject* (*)(UObject*)>(this, Index)(this);
		else
			std::cout << _("Unable to create default object because Index is 0!\n");
		return nullptr;
	}
};

struct UFunction : UObject {}; // TODO: Add acutal stuff to this

struct FUObjectItem // https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectArray.h#L26
{
	UObject* Object;
	int32_t Flags;
	int32_t ClusterRootIndex;
	int32_t SerialNumber;
	// int pad_01;
};

struct FFixedUObjectArray
{
	FUObjectItem* Objects;
	int32_t MaxElements;
	int32_t NumElements;

	INL const int32_t Num() const { return NumElements; }

	INL const int32_t Capacity() const { return MaxElements; }

	INL bool IsValidIndex(int32_t Index) const { return Index < Num() && Index >= 0; }

	INL UObject* GetObjectById(int32_t Index) const
	{
		return Objects[Index].Object;
	}
};

struct FChunkedFixedUObjectArray // https://github.com/EpicGames/UnrealEngine/blob/7acbae1c8d1736bb5a0da4f6ed21ccb237bc8851/Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectArray.h#L321
{
	enum
	{
		NumElementsPerChunk = 64 * 1024,
	};

	FUObjectItem** Objects;
	FUObjectItem* PreAllocatedObjects;
	int32_t MaxElements;
	int32_t NumElements;
	int32_t MaxChunks;
	int32_t NumChunks;

	INL const int32_t Num() const { return NumElements; }

	INL const int32_t Capacity() const { return MaxElements; }

	INL UObject* GetObjectById(int32_t Index) const
	{
		if (Index > NumElements || Index < 0) return nullptr;

		const int32_t ChunkIndex = Index / NumElementsPerChunk;
		const int32_t WithinChunkIndex = Index % NumElementsPerChunk;

		if (ChunkIndex > NumChunks) return nullptr;
		FUObjectItem* Chunk = Objects[ChunkIndex];
		if (!Chunk) return nullptr;

		auto obj = (Chunk + WithinChunkIndex)->Object;

		return obj;
	}
};

static UObject* (*StaticFindObjectO)(
	UObject* Class,
	UObject* InOuter,
	const TCHAR* Name,
	bool ExactClass);

template <typename ReturnType = UObject>
static ReturnType* StaticFindObject(const std::string& str)
{
	auto Name = std::wstring(str.begin(), str.end()).c_str();
	return (ReturnType*)StaticFindObjectO(nullptr, nullptr, Name, false);
}

template <typename ReturnType = UObject>
static ReturnType* FindObjectOld(const std::string& str, bool bIsEqual = false, bool bIsName = false)
{
	if (bIsName) bIsEqual = true;

	for (int32_t i = 0; i < (ObjObjects ? ObjObjects->Num() : OldObjects->Num()); i++)
	{
		auto Object = ObjObjects ? ObjObjects->GetObjectById(i) : OldObjects->GetObjectById(i);

		if (!Object) continue;

		auto ObjectName = bIsName ? Object->GetName() : Object->GetFullName();

		// cant we do like if ((bIsEqual) ? ObjectName == str : ObjectName.contains(str))
		if (bIsEqual)
		{
			if (ObjectName == str)
				return (ReturnType*)Object;
		}
		else
		{
			if (ObjectName.contains(str))
				return (ReturnType*)Object;
		}
	}

	return nullptr;
}

template <typename ReturnType = UObject>
static ReturnType* FindObject(const std::string& str, bool bIsEqual = false, bool bIsName = false, bool bDoNotUseStaticFindObject = false, bool bSkipIfSFOFails = true)
{
	if (StaticFindObjectO && !bDoNotUseStaticFindObject)
	{
		auto Object = StaticFindObject<ReturnType>(str.substr(str.find(" ") + 1));
		if (Object)
		{
			// std::cout << _("Found SFO!\n");
			return Object;
		}
		// std::cout << _("[WARNING] Failed to find object with SFO named: ") << str << " (if you're game doesn't crash soon, it means it's fine)\n";

		if (bSkipIfSFOFails)
			return nullptr;
	}

	return FindObjectOld<ReturnType>(str, bIsEqual, bIsName);
}

// Here comes the version changing and makes me want to die I need to find a better way to do this

struct UField : UObject
{
	UField* Next;
};

struct UFieldPadding : UObject
{
	UField* Next;
	void* pad_01;
	void* pad_02;
};

struct UProperty_UE : public UField // Default UProperty for UE, >4.20.
{
	int32_t ArrayDim;
	int32_t ElementSize;
	uint64_t PropertyFlags;
	uint16_t RepIndex;
	TEnumAsByte<ELifetimeCondition> BlueprintReplicationCondition;
	int32_t Offset_Internal;
	FName RepNotifyFunc;
	UProperty_UE* PropertyLinkNext;
	UProperty_UE* NextRef;
	UProperty_UE* DestructorLinkNext;
	UProperty_UE* PostConstructLinkNext;
};

struct UStruct_FT : public UField // >4.20
{
	UStruct_FT* SuperStruct;
	UField* ChildProperties; // Children
	int32_t PropertiesSize;
	int32_t MinAlignment;
	TArray<uint8_t> Script;
	UProperty_UE* PropertyLink;
	UProperty_UE* RefLink;
	UProperty_UE* DestructorLink;
	UProperty_UE* PostConstructLink;
	TArray<UObject*> ScriptObjectReferences;
};

struct UProperty_FTO : UField
{
	uint32_t ArrayDim; // 0x30
	uint32_t ElementSize; // 0x34
	uint64_t PropertyFlags; // 0x38
	char pad_40[4]; // 0x40
	uint32_t Offset_Internal; // 0x44
	char pad_48[0x70 - 0x48];
};

struct UStruct_FTO : public UField // 4.21 only
{
	UStruct_FTO* SuperStruct;
	UField* ChildProperties; // Children
	int32_t PropertiesSize;
	int32_t MinAlignment;
	UProperty_FTO* PropertyLink;
	UProperty_FTO* RefLink;
	UProperty_FTO* DestructorLink;
	UProperty_FTO* PostConstructLink;
	TArray<UObject*> ScriptObjectReferences;
};

struct UStruct_FTT : UField // 4.22-4.24
{
	void* Pad;
	void* Pad2;
	UStruct_FTT* SuperStruct; // 0x30
	UField* ChildProperties; // 0x38
	uint32_t PropertiesSize; // 0x40
	char pad_44[0x88 - 0x30 - 0x14];
};

struct FField
{
	void** VFT;
	void* ClassPrivate;
	void* Owner;
	void* pad;
	FField* Next;
	FName Name;
	EObjectFlags FlagsPrivate;

	std::string GetName()
	{
		return Name.ToString();
	}
};

struct FProperty : public FField
{
	int32_t	ArrayDim;
	int32_t	ElementSize;
	EPropertyFlags PropertyFlags;
	uint16_t RepIndex;
	TEnumAsByte<ELifetimeCondition> BlueprintReplicationCondition;
	int32_t	Offset_Internal;
	FName RepNotifyFunc;
	FProperty* PropertyLinkNext;
	FProperty* NextRef;
	FProperty* DestructorLinkNext;
	FProperty* PostConstructLinkNext;
};

class UStruct_CT : public UFieldPadding
{
public:
	UStruct_CT* SuperStruct;
	UFieldPadding* Children;
	FField* ChildProperties;
	int32_t PropertiesSize;
	int32_t MinAlignment;
	TArray<uint8_t> Script;
	FProperty* PropertyLink;
	FProperty* RefLink;
	FProperty* DestructorLink;
	FProperty* PostConstructLink;
	TArray<UObject*> ScriptAndPropertyObjectReferences;
};

struct UClass_FT : public UStruct_FT {}; // >4.20
struct UClass_FTO : public UStruct_FTO {}; // 4.21
struct UClass_FTT : public UStruct_FTT {}; // 4.22-4.24
struct UClass_CT : public UStruct_CT {}; // C2 to before C3

template <typename ClassType, typename PropertyType, typename ReturnValue = PropertyType>
auto GetMembers(UObject* Object)
{
	std::vector<ReturnValue*> Members;

	if (Object)
	{
		for (auto CurrentClass = (ClassType*)Object->ClassPrivate; CurrentClass; CurrentClass = (ClassType*)CurrentClass->SuperStruct)
		{
			auto Property = CurrentClass->ChildProperties;

			if (Property)
			{
				auto Next = Property->Next;

				if (Next)
				{
					while (Property)
					{
						Members.push_back((ReturnValue*)Property);

						Property = Property->Next;
					}
				}
			}

		}
	}

	return Members;
}

template <typename ClassType, typename PropertyType, typename ReturnValue = PropertyType>
auto GetMembersFProperty(UObject* Object, bool bOnlyMembers = false, bool bOnlyFunctions = false)
{
	std::vector<ReturnValue*> Members;

	if (Object)
	{
		for (auto CurrentClass = (ClassType*)Object->ClassPrivate; CurrentClass; CurrentClass = (ClassType*)CurrentClass->SuperStruct)
		{
			auto Property = CurrentClass->ChildProperties;
			auto Child = CurrentClass->Children;

			if ((!bOnlyFunctions && bOnlyMembers) || (!bOnlyFunctions && !bOnlyMembers)) // Only members
			{
				if (Property)
				{
					Members.push_back((ReturnValue*)Property);

					auto Next = Property->Next;

					if (Next)
					{
						while (Property)
						{
							Members.push_back((ReturnValue*)Property);

							Property = Property->Next;
						}
					}
				}
			}

			if ((!bOnlyMembers && bOnlyFunctions) || (!bOnlyMembers && !bOnlyFunctions)) // Only functions
			{
				if (Child)
				{
					Members.push_back((ReturnValue*)Child);

					auto Next = Child->Next;

					if (Next)
					{
						while (Child)
						{
							Members.push_back((ReturnValue*)Child);

							Child = decltype(Child)(Child->Next);
						}
					}
				}
			}

		}
	}

	return Members;
}

auto GetMembersAsObjects(UObject* Object, bool bOnlyMembers = false, bool bOnlyFunctions = false)
{
	std::vector<UObject*> Members;

	if (Engine_Version <= 420)
		Members = GetMembers<UClass_FT, UProperty_UE, UObject>(Object);

	else if (Engine_Version == 421) // && Engine_Version <= 424)
		Members = GetMembers<UClass_FTO, UProperty_FTO, UObject>(Object);

	else if (Engine_Version >= 422 && Engine_Version <= 424)
		Members = GetMembers<UClass_FTT, UProperty_FTO, UObject>(Object);

	else if (Engine_Version >= 425 && Engine_Version < 500)
		Members = GetMembersFProperty<UClass_CT, FProperty, UObject>(Object, bOnlyMembers, bOnlyFunctions);

	else if (Engine_Version >= 500)
		Members = GetMembersFProperty<UClass_CT, FProperty, UObject>(Object, bOnlyMembers, bOnlyFunctions);

	return Members;
}

std::vector<std::string> GetMemberNames(UObject* Object, bool bOnlyMembers = false, bool bOnlyFunctions = false)
{
	std::vector<std::string> Names;
	std::vector<UObject*> Members = GetMembersAsObjects(Object, bOnlyMembers, bOnlyFunctions);

	for (auto Member : Members)
		Names.push_back(Member->GetName());

	return Names;
}

UFunction* FindFunction(const std::string& Name, UObject* Object) // might as well pass in object because what else u gon use a func for.
{
	for (auto Member : GetMembersAsObjects(Object, false, true))
	{
		if (Member->GetName() == Name) // dont use IsA cuz slower
			return (UFunction*)Member;
	}
	
	return nullptr;
}

template <typename ClassType, typename PropertyType>
int LoopMembersAndFindOffset(UObject* Object, const std::string& MemberName, int offset = 0)
{
	// We loop through the whole class hierarchy to find the offset.

	for (auto Member : GetMembers<ClassType, PropertyType>(Object))
	{
		if (Member)
		{
			if (Member->GetName() == MemberName)
			{
				if (!offset)
					return ((PropertyType*)Member)->Offset_Internal;
				else
					return *(int*)(__int64(Member) + offset);
			}
		}
	}

	return 0;
}

static int GetOffset(UObject* Object, const std::string& MemberName)
{
	if (Object && !MemberName.contains(_(" ")))
	{
		if (Engine_Version <= 420)
			return LoopMembersAndFindOffset<UClass_FT, UProperty_UE>(Object, MemberName);

		else if (Engine_Version == 421) // && Engine_Version <= 424)
			return LoopMembersAndFindOffset<UClass_FTO, UProperty_FTO>(Object, MemberName);

		else if (Engine_Version >= 422 && Engine_Version <= 424)
			return LoopMembersAndFindOffset<UClass_FTT, UProperty_FTO>(Object, MemberName);

		else if (Engine_Version >= 425 && Engine_Version < 500)
			return LoopMembersAndFindOffset<UClass_CT, FProperty>(Object, MemberName);

		else if (std::stod(FN_Version) >= 19)
			return LoopMembersAndFindOffset<UClass_CT, FProperty>(Object, MemberName, 0x44);
	}
	else
	{
		// std::cout << std::format(_("Either invalid object or MemberName. MemberName {} Object {}"), MemberName, __int64(Object));
	}

	return 0;
}

template <typename ClassType>
bool IsA_(const UObject* cmpto, UObject* cmp)
{
	for (auto super = (ClassType*)cmpto->ClassPrivate; super; super = (ClassType*)super->SuperStruct)
	{
		if (super == cmp)
			return true;
	}

	return false;
}

bool UObject::IsA(UObject* cmp) const
{
	if (Engine_Version <= 420)
		return IsA_<UClass_FT>(this, cmp);

	else if (Engine_Version == 421) // && Engine_Version <= 424)
		return IsA_<UClass_FTO>(this, cmp);

	else if (Engine_Version >= 422 && Engine_Version < 425)
		return IsA_<UClass_FTT>(this, cmp);

	else if (Engine_Version >= 425)
		return IsA_<UClass_CT>(this, cmp);

	return false;
}

INL UFunction* UObject::Function(const std::string& FuncName)
{
	return FindFunction(FuncName, this);
}

FString(*GetEngineVersion)();

// TODO: There is this 1.9 function, 48 8D 05 D9 51 22 03. It has the CL and stuff. We may be able to determine the version using the CL.
// There is also a string for the engine version and fortnite version, I think it's for every version its like "engineversion=". I will look into it when I find time.

struct FActorSpawnParameters
{
	unsigned char Unk00[0x40];
};

static UObject* (*SpawnActorO)(UObject* World, UObject* Class, FVector* Position, FRotator* Rotation, const FActorSpawnParameters& SpawnParameters);

uint64_t ToStringAddr = 0;
uint64_t ProcessEventAddr = 0;
uint64_t ObjectsAddr = 0;
uint64_t FreeMemoryAddr = 0;

static int ServerReplicateActorsOffset = 0x53; // UE4.20

bool Setup(/* void* ProcessEventHookAddr */)
{
	auto SpawnActorAddr = FindPattern(_("40 53 56 57 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 0F 28 1D ? ? ? ? 0F 57 D2 48 8B B4 24 ? ? ? ? 0F 28 CB"));

	if (!SpawnActorAddr)
	{
		MessageBoxA(0, _("Failed to find SpawnActor function."), _("Universal Walking Simulator"), MB_OK);
		return 0;
	}

	SpawnActorO = decltype(SpawnActorO)(SpawnActorAddr);

	bool bOldObjects = false;

	GetEngineVersion = decltype(GetEngineVersion)(FindPattern(_("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 8B C8 41 B8 04 ? ? ? 48 8B D3")));

	std::string FullVersion;
	FString toFree;

	if (!GetEngineVersion)
	{
		auto VerStr = FindPattern(_("2B 2B 46 6F 72 74 6E 69 74 65 2B 52 65 6C 65 61 73 65 2D ? ? ? ?"));

		if (!VerStr)
		{
			MessageBoxA(0, _("Failed to find fortnite version!"), _("Universal Walking Simulator"), MB_ICONERROR);
			return false;
		}

		FullVersion = decltype(FullVersion.c_str())(VerStr);
		Engine_Version = 500;
	}

	else
	{
		toFree = GetEngineVersion();
		FullVersion = toFree.ToString();
	}

	std::string FNVer = FullVersion;
	std::string EngineVer = FullVersion;

	if (!FullVersion.contains(_("Live")) && !FullVersion.contains(_("Next")) && !FullVersion.contains(_("Cert")))
	{
		if (GetEngineVersion)
		{
			FNVer.erase(0, FNVer.find_last_of(_("-"), FNVer.length() - 1) + 1);
			EngineVer.erase(EngineVer.find_first_of(_("-"), FNVer.length() - 1), 40);

			if (EngineVer.find_first_of(".") != EngineVer.find_last_of(".")) // this is for 4.21.0 and itll remove the .0
				EngineVer.erase(EngineVer.find_last_of(_(".")), 2);

			Engine_Version = std::stod(EngineVer) * 100;
		}

		else
		{
			const std::regex base_regex(_("-([0-9.]*)-"));
			std::cmatch base_match;

			std::regex_search(FullVersion.c_str(), base_match, base_regex);

			FNVer = base_match[1];
		}

		FN_Version = FNVer;

		auto FnVerDouble = std::stod(FN_Version);

		if (FnVerDouble >= 16.00 && FnVerDouble < 18.40)
			Engine_Version = 427; // 4.26.1;
	}

	else
	{
		Engine_Version = 419;
		FN_Version = _("2.69");
	}

	if (Engine_Version >= 416 && Engine_Version <= 420)
	{
		ObjectsAddr = FindPattern(_("48 8B 05 ? ? ? ? 48 8D 1C C8 81 4B ? ? ? ? ? 49 63 76 30"), false, 7, true);

		if (!ObjectsAddr)
			ObjectsAddr = FindPattern(_("48 8B 05 ? ? ? ? 48 8D 14 C8 EB 03 49 8B D6 8B 42 08 C1 E8 1D A8 01 0F 85 ? ? ? ? F7 86 ? ? ? ? ? ? ? ?"), false, 7, true);

		if (Engine_Version == 420)
			ToStringAddr = FindPattern(_("48 89 5C 24 ? 57 48 83 EC 40 83 79 04 00 48 8B DA 48 8B F9 75 23 E8 ? ? ? ? 48 85 C0 74 19 48 8B D3 48 8B C8 E8 ? ? ? ? 48"));
		else
		{
			ToStringAddr = FindPattern(_("40 53 48 83 EC 40 83 79 04 00 48 8B DA 75 19 E8 ? ? ? ? 48 8B C8 48 8B D3 E8 ? ? ? ?"));

			if (!ToStringAddr) // This means that we are in season 1 (i think).
			{
				ToStringAddr = FindPattern(_("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 20 48 8B DA 4C 8B F1 E8 ? ? ? ? 4C 8B C8 41 8B 06 99"));

				if (ToStringAddr)
					Engine_Version = 416;
			}
		}

		FreeMemoryAddr = FindPattern(_("48 85 C9 74 1D 4C 8B 05 ? ? ? ? 4D 85 C0 0F 84 ? ? ? ? 49"));

		if (!FreeMemoryAddr)
			FreeMemoryAddr = FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9 48 8B 0D ? ? ? ? 48 85 C9 75 0C E8 ? ? ? ? 48 8B 0D ? ? ? ? 48"));

		ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 48 63 41 0C 45 33 F6"));

		bOldObjects = true;
	}

	if (Engine_Version >= 421 && Engine_Version <= 424)
	{
		ToStringAddr = FindPattern(_("48 89 5C 24 ? 57 48 83 EC 30 83 79 04 00 48 8B DA 48 8B F9"));
		ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? ? ? ? 45 33 F6"));
	}

	if (Engine_Version >= 425 && Engine_Version < 500)
	{
		ToStringAddr = FindPattern(_("48 89 5C 24 ? 55 56 57 48 8B EC 48 83 EC 30 8B 01 48 8B F1 44 8B 49 04 8B F8 C1 EF 10 48 8B DA 0F B7 C8 89 4D 24 89 7D 20 45 85 C9"));
		ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 8B 41 0C 45 33 F6"));
	}

	auto FnVerDouble = std::stod(FN_Version);

	if (Engine_Version == 421 && FnVerDouble >= 5 && FnVerDouble < 6)
		ServerReplicateActorsOffset = 0x54;
	else if (Engine_Version == 421 || (Engine_Version >= 422 && Engine_Version < 425))
		ServerReplicateActorsOffset = 0x56;
	if (FnVerDouble == 7.40 || FnVerDouble < 8.40)
		ServerReplicateActorsOffset = 0x57;
	else if (Engine_Version >= 425)
		ServerReplicateActorsOffset = 0x5D;

	if (FnVerDouble >= 5)
	{
		ObjectsAddr = FindPattern(_("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8D 04 D1 EB 03 48 8B ? 81 48 08 ? ? ? 40 49"), false, 7, true);
		FreeMemoryAddr = FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9"));
		bOldObjects = false;

		if (!ObjectsAddr)
			ObjectsAddr = FindPattern(_("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8B 04 D1"), true, 3);

		if (!ObjectsAddr)
			ObjectsAddr = FindPattern(_("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8D 04 D1"), true, 3); // stupid 5.41
	}

	if (FnVerDouble >= 16.00) // 4.26.1
	{
		FreeMemoryAddr = FindPattern(_("48 85 C9 0F 84 ? ? ? ? 48 89 5C 24 ? 57 48 83 EC 20 48 8B 3D ? ? ? ? 48 8B D9 48"));

		if (FnVerDouble < 19.00)
		{
			ToStringAddr = FindPattern(_("48 89 5C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 8B 19 48 8B F2 0F B7 FB 4C 8B F1 E8 ? ? ? ? 44 8B C3 8D 1C 3F 49 C1 E8 10 33 FF 4A 03 5C C0 ? 41 8B 46 04"));
			ProcessEventAddr = FindPattern(_("40 55 53 56 57 41 54 41 56 41 57 48 81 EC"));

			if (!ToStringAddr)
				ToStringAddr = FindPattern(_("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 8B 19 33 ED 0F B7 01 48 8B FA C1 EB 10 4C"));
		}
	}

	// if (Engine_Version >= 500)
	if (FnVerDouble >= 19.00)
	{
		ToStringAddr = FindPattern(_("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 8B"));
		ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 45 33 ED"));

		if (!FreeMemoryAddr)
			FreeMemoryAddr = FindPattern(_("48 85 C9 0F 84 ? ? ? ? 53 48 83 EC 20 48 89 7C 24 ? 48 8B D9 48 8B 3D ? ? ? ? 48 85 FF"));

		// C3 S3

		if (!ToStringAddr)
			ToStringAddr = FindPattern(_("48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 8B 01 48 8B F2 8B"));

		if (!ProcessEventAddr)
			ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 45"));
	}

	if (!FreeMemoryAddr)
	{
		MessageBoxA(0, _("Failed to find FMemory::Free"), _("Universal Walking Simulator"), MB_OK);
		return false;
	}

	FMemory::Free = decltype(FMemory::Free)(FreeMemoryAddr);

	toFree.FreeString();

	if (!ToStringAddr)
	{
		MessageBoxA(0, _("Failed to find FName::ToString"), _("Universal Walking Simulator"), MB_OK);
		return false;
	}

	ToStringO = decltype(ToStringO)(ToStringAddr);

	if (!ProcessEventAddr)
	{
		MessageBoxA(0, _("Failed to find UObject::ProcessEvent"), _("Universal Walking Simulator"), MB_OK);
		return false;
	}

	ProcessEventO = decltype(ProcessEventO)(ProcessEventAddr);

	if (!ObjectsAddr)
	{
		MessageBoxA(0, _("Failed to find FUObjectArray::ObjObjects"), _("Universal Walking Simulator"), MB_OK);
		return false;
	}

	if (bOldObjects)
		OldObjects = decltype(OldObjects)(ObjectsAddr);
	else
		ObjObjects = decltype(ObjObjects)(ObjectsAddr);

	return true;
}

UObject* GetEngine()
{
	static auto Engine = FindObjectOld(_("FortEngine_"));

	return Engine;
}

struct FURL
{
	FString                                     Protocol;                                                 // 0x0000(0x0010) (ZeroConstructor)
	FString                                     Host;                                                     // 0x0010(0x0010) (ZeroConstructor)
	int                                         Port;                                                     // 0x0020(0x0004) (ZeroConstructor, IsPlainOldData)
	int                                         Valid;                                                    // 0x0024(0x0004) (ZeroConstructor, IsPlainOldData)
	FString                                     Map;                                                      // 0x0028(0x0010) (ZeroConstructor)
	FString                                     RedirectUrl;                                              // 0x0038(0x0010) (ZeroConstructor)
	TArray<FString>                             Op;                                                       // 0x0048(0x0010) (ZeroConstructor)
	FString                                     Portal;                                                   // 0x0058(0x0010) (ZeroConstructor)
};

struct FLevelCollection
{
	unsigned char                                      UnknownData00[0x8];                                       // 0x0000(0x0008) MISSED OFFSET
	UObject* GameState;                                                // 0x0008(0x0008) (ZeroConstructor, IsPlainOldData)
	UObject* NetDriver;                                                // 0x0010(0x0008) (ZeroConstructor, IsPlainOldData)
	UObject* DemoNetDriver;                                            // 0x0018(0x0008) (ZeroConstructor, IsPlainOldData)
	UObject* PersistentLevel;                                          // 0x0020(0x0008) (ZeroConstructor, IsPlainOldData)
	unsigned char                                      UnknownData01[0x50];  // TSet<ULevel*> Levels;
};

template <typename ClassType, typename FieldType, typename Prop>
int FindOffsetStructAh(const std::string& ClassName, const std::string& MemberName, int offset = 0)
{
	auto Class = FindObject<ClassType>(ClassName, true);

	if (Class)
	{
		if (FieldType* Next = Class->ChildProperties)
		{
			auto PropName = Class->ChildProperties->GetName();

			while (Next)
			{
				if (PropName == MemberName)
				{
					if (!offset)
						return ((Prop*)Next)->Offset_Internal;
					else
						return *(int*)(__int64(Next) + offset);
				}
				else
				{
					Next = Next->Next;

					if (Next)
					{
						PropName = Next->GetName();
					}
				}
			}
		}
	}

	return 0;
}

int FindOffsetStruct(const std::string& ClassName, const std::string& MemberName)
{
	if (Engine_Version <= 420)
		return FindOffsetStructAh<UClass_FT, UField, UProperty_UE>(ClassName, MemberName);

	else if (Engine_Version == 421) // && Engine_Version <= 424)
		return FindOffsetStructAh<UClass_FTO, UField, UProperty_FTO>(ClassName, MemberName);

	else if (Engine_Version >= 422 && Engine_Version <= 424)
		return FindOffsetStructAh<UClass_FTT, UField, UProperty_FTO>(ClassName, MemberName);

	else if (Engine_Version >= 425 && Engine_Version < 500)
		return FindOffsetStructAh<UClass_CT, FField, FProperty>(ClassName, MemberName);

	else if (std::stod(FN_Version) >= 19)
		return FindOffsetStructAh<UClass_CT, FField, FProperty>(ClassName, MemberName, 0x44);

	return 0;
}

template <typename MemberType>
INL MemberType* UObject::Member(const std::string& MemberName, int extraOffset)
{
	// MemberName.erase(0, MemberName.find_last_of(".", MemberName.length() - 1) + 1); // This would be getting the short name of the member if you did like ObjectProperty /Script/stuff

	// if (!bIsStruct)
	return (MemberType*)(__int64(this) + (GetOffset(this, MemberName) - extraOffset));
	// else
		// return (MemberType*)(__int64(this) + FindOffsetStruct(this->GetFullName(), MemberName));
}

struct FFastArraySerializerItem
{
	int                                                ReplicationID;                                            // 0x0000(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, RepNotify, Interp, NonTransactional, EditorOnly, NoDestructor, AutoWeak, ContainsInstancedReference, AssetRegistrySearchable, SimpleDisplay, AdvancedDisplay, Protected, BlueprintCallable, BlueprintAuthorityOnly, TextExportTransient, NonPIEDuplicateTransient, ExposeOnSpawn, PersistentInstance, UObjectWrapper, HasGetValueTypeHash, NativeAccessSpecifierPublic, NativeAccessSpecifierProtected, NativeAccessSpecifierPrivate)
	int                                                ReplicationKey;                                           // 0x0004(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, RepNotify, Interp, NonTransactional, EditorOnly, NoDestructor, AutoWeak, ContainsInstancedReference, AssetRegistrySearchable, SimpleDisplay, AdvancedDisplay, Protected, BlueprintCallable, BlueprintAuthorityOnly, TextExportTransient, NonPIEDuplicateTransient, ExposeOnSpawn, PersistentInstance, UObjectWrapper, HasGetValueTypeHash, NativeAccessSpecifierPublic, NativeAccessSpecifierProtected, NativeAccessSpecifierPrivate)
	int                                                MostRecentArrayReplicationKey;                            // 0x0008(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, RepNotify, Interp, NonTransactional, EditorOnly, NoDestructor, AutoWeak, ContainsInstancedReference, AssetRegistrySearchable, SimpleDisplay, AdvancedDisplay, Protected, BlueprintCallable, BlueprintAuthorityOnly, TextExportTransient, NonPIEDuplicateTransient, ExposeOnSpawn, PersistentInstance, UObjectWrapper, HasGetValueTypeHash, NativeAccessSpecifierPublic, NativeAccessSpecifierProtected, NativeAccessSpecifierPrivate)
};

struct FFastArraySerializer
{
#ifndef BEFORE_SEASONEIGHT
	
	char ItemMap[0x50];

	int32_t IDCounter;
	int32_t ArrayReplicationKey;

	char GuidReferencesMap[0x50];
	char GuidReferencesMap_StructDelta[0x50];

	int32_t CachedNumItems;
	int32_t CachedNumItemsToConsiderForWriting;
	EFastArraySerializerDeltaFlags DeltaFlags;

#else

	// TMap<int32_t, int32_t> ItemMap;
	char ItemMap[0x50];
	int32_t IDCounter;
	int32_t ArrayReplicationKey;

	char GuidReferencesMap[0x50];

	int32_t CachedNumItems;
	int32_t CachedNumItemsToConsiderForWriting;
#endif

	void MarkItemDirty(FFastArraySerializerItem* Item)
	{
		if (Item->ReplicationID == -1)
		{
			Item->ReplicationID = ++IDCounter;
			if (IDCounter == -1)
				IDCounter++;
		}

		Item->ReplicationKey++;
		MarkArrayDirty();
	}

	void MarkAllItemsDirty() // This is my function, not ue.
	{
		
	}

	void MarkArrayDirty()
	{
		// ItemMap.Reset();		// This allows to clients to add predictive elements to arrays without affecting replication.
		IncrementArrayReplicationKey();

		// Invalidate the cached item counts so that they're recomputed during the next write
		CachedNumItems = -1;
		CachedNumItemsToConsiderForWriting = -1;
	}

	void IncrementArrayReplicationKey()
	{
		ArrayReplicationKey++;
		if (ArrayReplicationKey == -1)
			ArrayReplicationKey++;
	}
};

int32_t GetSizeOfStruct(UObject* Struct)
{
	if (Engine_Version <= 420)
		return ((UClass_FT*)Struct)->PropertiesSize;

	else if (Engine_Version == 421)
		return ((UClass_FTO*)Struct)->PropertiesSize;

	else if (Engine_Version >= 422 && Engine_Version <= 424)
		return ((UClass_FTT*)Struct)->PropertiesSize;

	else if (Engine_Version >= 425)
		return ((UClass_CT*)Struct)->PropertiesSize;

	return 0;
}

template <typename T>
T* Get(int offset, uintptr_t addr)
{
	return (T*)(__int64(addr) + offset);
}

// TODO: REMAKME

struct FWeakObjectPtr
{
public:
	inline bool SerialNumbersMatch(FUObjectItem* ObjectItem) const
	{
		return ObjectItem->SerialNumber == ObjectSerialNumber;
	}

	int32_t ObjectIndex;
	int32_t ObjectSerialNumber;
};

template<class T, class TWeakObjectPtrBase = FWeakObjectPtr>
struct TWeakObjectPtr : public TWeakObjectPtrBase
{
public:
};

template<typename TObjectID>
class TPersistentObjectPtr
{
public:
	FWeakObjectPtr WeakPtr;
	int32_t TagAtLastTest;
	TObjectID ObjectID;
};

struct FSoftObjectPath
{
	FName AssetPathName;
	FString SubPathString;
};

class FSoftObjectPtr : public TPersistentObjectPtr<FSoftObjectPath>
{

};

class TSoftObjectPtr : FSoftObjectPtr
{

};

namespace EAbilityGenericReplicatedEvent
{
	enum Type
	{
		/** A generic confirmation to commit the ability */
		GenericConfirm = 0,
		/** A generic cancellation event. Not necessarily a canellation of the ability or targeting. Could be used to cancel out of a channelling portion of ability. */
		GenericCancel,
		/** Additional input presses of the ability (Press X to activate ability, press X again while it is active to do other things within the GameplayAbility's logic) */
		InputPressed,
		/** Input release event of the ability */
		InputReleased,
		/** A generic event from the client */
		GenericSignalFromClient,
		/** A generic event from the server */
		GenericSignalFromServer,
		/** Custom events for game use */
		GameCustom1,
		GameCustom2,
		GameCustom3,
		GameCustom4,
		GameCustom5,
		GameCustom6,
		MAX
	};
}

template <class ObjectType>
class TSharedRef
{
public:
	ObjectType* Object;

	int SharedReferenceCount;
	int WeakReferenceCount;

	inline ObjectType* Get()
	{
		return Object;
	}
	inline ObjectType* Get() const
	{
		return Object;
	}
	inline ObjectType& operator*()
	{
		return *Object;
	}
	inline const ObjectType& operator*() const
	{
		return *Object;
	}
	inline ObjectType* operator->()
	{
		return Object;
	}
};

template <class ObjectType>
class TSharedPtr
{
public:
	ObjectType* Object;

	int SharedReferenceCount;
	int WeakReferenceCount;

	inline ObjectType* Get()
	{
		return Object;
	}
	inline ObjectType* Get() const
	{
		return Object;
	}
	inline ObjectType& operator*()
	{
		return *Object;
	}
	inline const ObjectType& operator*() const
	{
		return *Object;
	}
	inline ObjectType* operator->()
	{
		return Object;
	}

	inline TSharedRef<ObjectType> ToSharedRef()
	{
		return TSharedRef<ObjectType>(Object);
	}
};

template <typename KeyType, typename ValueType>
class TPair
{
public:
	KeyType First;
	ValueType Second;

	TPair(KeyType Key, ValueType Value)
		: First(Key)
		, Second(Value)
	{
	}

	inline KeyType& Key()
	{
		return First;
	}
	inline const KeyType& Key() const
	{
		return First;
	}
	inline ValueType& Value()
	{
		return Second;
	}
	inline const ValueType& Value() const
	{
		return Second;
	}
};

struct FAbilityReplicatedData
{
	bool bTriggered;
	FVector VectorPayload;
	unsigned char Pad[24];
};