#include "common.h"

#include "CSymbol.hpp"
#include "CInstanceKlass.hpp"
#include "CArray.hpp"
#include "CMethod.hpp"
#include "CJavaHook.hpp"
#include "CCheat.hpp"

typedef long(__stdcall* _JNI_GetCreatedJavaVMs)(JavaVM**, long, long*);
_JNI_GetCreatedJavaVMs ORIG_JNI_GetCreatedJavaVMs;

bool GetModuleInfo(void* moduleHandle, void** moduleBase, size_t* moduleSize) {
	if (!moduleHandle) return false;

	MODULEINFO info;
	GetModuleInformation(GetCurrentProcess(), reinterpret_cast<HMODULE>(moduleHandle), &info, sizeof(info));

	if (moduleBase)
		*moduleBase = info.lpBaseOfDll;

	if (moduleSize)
		*moduleSize = (size_t)info.SizeOfImage;

	return true;
}

bool GetModuleInfo(const std::string& moduleName, void** moduleHandle, void** moduleBase, size_t* moduleSize) {
	HMODULE handle = GetModuleHandleA(moduleName.c_str());
	auto ret = GetModuleInfo(handle, moduleBase, moduleSize);

	if (ret && moduleHandle)
		*moduleHandle = handle;

	return ret;
}

CInstanceKlass* GetClass(jclass klass) {
	uintptr_t* inner = *(uintptr_t**)klass;
	uintptr_t offset = ((DWORD)inner + 0x48);
	int64_t ptr = *reinterpret_cast<int64_t*>(offset);
	
	return reinterpret_cast<CInstanceKlass*>(ptr);
}

std::vector<CMethod*> GetMethods(CInstanceKlass* klass) {
	std::vector<CMethod*> result;
	for (int idx = 0; idx < klass->_methods->_length; idx++) {
		CMethod* md = klass->_methods->at(idx);
		if (!md) continue;
		result.push_back(md);
	}

	return result;
}

//overwrite pentest
JNIEXPORT void JNICALL HOOKED_swingHand(JNIEnv* env, jobject thiz, jobject hand) {
	printf("0x%p\n", hand);
}

typedef void(__cdecl* _N000000D1__swingHand)(void**, void**);
_N000000D1__swingHand ORIG_swingHand;

//append pentest
//a2 - JavaThread**, i'm lazy to recreate the whole JavaThread in reclass :c
void __cdecl HOOKED_swingHand_APPEND(void** a1, void** a2) {
	//ne nado pojalusta rascommenchivat' ibo crashnet cherez paru minut
	//printf("swingHand called! yahoo!\na1: 0x%p, a2: 0x%p\n", a1, a2);
	printf("shutting down!\n");
	return ORIG_swingHand(a1, a2);
}

JNIEnv* env;

/*jclass playerKlass = env->FindClass("bud");
			printf("0x%p\n", playerKlass);
			if (playerKlass) {
				printf("[DEBUG] Found bib as jclass!\n");
				CInstanceKlass* klass = GetClass(playerKlass);
				if (klass) {
					printf("[DEBUG] Successfully got InstanceKlass from jclass! addr: 0x%p\n", klass);
					for (CMethod* md : GetMethods(klass)) {
						if (!md || !klass || !klass->_constant_pool) continue;
						if (klass->_constant_pool->symbol_at(md->_constMethod->_name_index)->as_C_string()._Equal("a")) {
							printf("[DEBUG] name matches\n");
							if (klass->_constant_pool->symbol_at(md->_constMethod->_signature_index)->as_C_string()._Equal("(Lub;)V")) {
								printf("[DEBUG] signature matches\naddr: 0x%p\n", md);
								JavaHook detour(md, CHookType::OVERWRITE, reinterpret_cast<void**>(&HOOKED_swingHand), nullptr);
								jmethodID method = env->GetMethodID(playerKlass, "h", "()V");
								auto target = reinterpret_cast<_N000000D1__swingHand>(*(unsigned long long*)(*(unsigned long long*)method + 0x30));
								MH_STATUS status = MH_CreateHook(reinterpret_cast<void**>(target), &HOOKED_swingHand_APPEND, reinterpret_cast<void**>(&ORIG_swingHand));
								if (status != MH_OK) {
									printf("Failed to create detours for compiled entry: %s\n", MH_StatusToString(status));
								}
								else {
									//MH_EnableHook(_Hooked);
									// to be sure if we are enabling our hook - xwhitey
									status = MH_EnableHook(MH_ALL_HOOKS);
									if (status != MH_OK) {
										printf("Failed to enable detours for compiled entry: %s\n", MH_StatusToString(status));
									}
									else {
										printf("chitty chitty bang bang :sunglasses:\n");
									}
								}
break;
							}
						}
					}
				}
			}*/

void start() {
	AllocConsole();
	FILE* in;
	FILE* out;
	freopen_s(&in, "conin$", "r", stdin);
	freopen_s(&out, "conout$", "w", stdout);
	void* handle;
	void* base;
	size_t size;
	if (GetModuleInfo("jvm.dll", &handle, &base, &size)) {
		MH_Initialize();
		ORIG_JNI_GetCreatedJavaVMs = reinterpret_cast<_JNI_GetCreatedJavaVMs>(GetProcAddress(reinterpret_cast<HMODULE>(handle), "JNI_GetCreatedJavaVMs"));
		JavaVM* javavm;
		ORIG_JNI_GetCreatedJavaVMs(&javavm, 1, nullptr);
		javavm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr);
		jvmtiEnv* jvmti_env;
		javavm->GetEnv(reinterpret_cast<void**>(&jvmti_env), JVMTI_VERSION);
		if (env) {
			CCheat* cheat = new CCheat(env, javavm);
			CMinecraft* mc = cheat->theMinecraft;
			while (true) {
				mc->runTick();
				Sleep(50);
			}
		}
	}
}

DWORD WINAPI DllMain(_In_ void* _DllHandle, _In_ unsigned long _Reason, _In_opt_ void** _Unused) {
	if (_Reason == DLL_PROCESS_ATTACH) {
		CreateThread(NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(start), NULL, NULL, NULL);
		return 1;
	}
	
	return 0;
}