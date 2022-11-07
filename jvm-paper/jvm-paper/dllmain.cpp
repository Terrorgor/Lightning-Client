#include "common.h"

#include "CSymbol.hpp"
#include "CInstanceKlass.hpp"
#include "CArray.hpp"
#include "CMethod.hpp"
#include "CJavaHook.hpp"
#include "CCheat.hpp"
#include "CPatch.hpp"

#include <sstream>
#include <fstream>
#include <filesystem>

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

typedef void(__cdecl* __i2i_entry)();
__i2i_entry ORIG_i2i_entry;

CMethod* hooked = nullptr;
CMethod* sendChatMessageMethod = nullptr;

unsigned char* __Method_ptr = 0x0;
unsigned char* __JavaThread_ptr = 0x0;
unsigned char* __MethodData_ptr = 0x0;
unsigned char* __InstanceKlass_ptr_r10 = 0x0;
unsigned char* __InstanceKlass_ptr_rdi = 0x0;
unsigned char* __InstanceKlass_ptr_r14 = 0x0;
unsigned char* __InstanceKlass_ptr_rax = 0x0; //caller
unsigned char* __Args_ptr_rbp = 0x0;

// Created with ReClass.NET 1.2 by KN4CK3R

class Args
{
public:
	char pad_0000[56]; //0x0000
	class N00000084* _args; //0x0038
}; //Size: 0x0040

class N00000084
{
public:
	char pad_0000[40]; //0x0000
	char _body[1]; //0x0028
	char pad_0030[24]; //0x0030
}; //Size: 0x0048

void __declspec(naked) __cdecl __int3() {
	__asm int3
}

void __fastcall do_shit() {
	CMethod* method;
	__asm {
		mov method, qword ptr [rsp+8]
	}
	
}

void __declspec(naked) __cdecl HOOKED_i2i_entry() {
	__asm {
		mov __Method_ptr, rbx
		mov __JavaThread_ptr, r15
		mov __MethodData_ptr, rsi
		mov __InstanceKlass_ptr_r10, r10
		mov __InstanceKlass_ptr_rdi, rdi
		mov __InstanceKlass_ptr_r14, r14
		mov __InstanceKlass_ptr_rax, rax
		mov __Args_ptr_rbp, rbp
		; cmp rbx, sendChatMessageMethod
		; je __int3
		push rbx
		call do_shit
		pop rbx
		push ORIG_i2i_entry
		retn
	}
}

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
#ifndef _DEBUG
			CCheat* cheat = new CCheat(env, javavm);
			CMinecraft* mc = cheat->theMinecraft;
			while (true) {
				mc->runTick();
				Sleep(50);
			}
#else
			jclass a = env->FindClass("bud");
			if (a) {
				CInstanceKlass* klass = GetClass(a);
				if (klass) {
					for (CMethod* md : GetMethods(klass)) {
						if (klass->_constant_pool->symbol_at(md->_constMethod->_name_index)->as_C_string()._Equal("g")) {
							sendChatMessageMethod = md;
						}
					}
				}
			}
			jclass playerKlass = env->FindClass("bib");
			printf("0x%p\n", playerKlass);
			if (playerKlass) {
				printf("[DEBUG] Found bib as jclass!\n");
				CInstanceKlass* klass = GetClass(playerKlass);
				if (klass) {
					printf("[DEBUG] Successfully got InstanceKlass from jclass! addr: 0x%p\n", klass);
					for (CMethod* md : GetMethods(klass)) {
						if (!md || !klass || !klass->_constant_pool) continue;
						if (klass->_constant_pool->symbol_at(md->_constMethod->_name_index)->as_C_string()._Equal("t")) {
							printf("[DEBUG] name matches\n");
							if (klass->_constant_pool->symbol_at(md->_constMethod->_signature_index)->as_C_string()._Equal("()V")) {
								printf("[DEBUG] signature matches\naddr: 0x%p\n", md);
								hooked = md;
								//MH_CreateHook(&printf, &HOOKED_printf, reinterpret_cast<void**>(&ORIG_printf));
								MH_STATUS status = MH_CreateHook(reinterpret_cast<void*>(md->_i2i_entry),
									HOOKED_i2i_entry, reinterpret_cast<void**>(&ORIG_i2i_entry));
								printf("0x%p\n", ORIG_i2i_entry);
								if (status == MH_OK) {
									printf("enabling hook\n");
									status = MH_EnableHook(MH_ALL_HOOKS);
									if (status == MH_OK) {
										printf("chitty chitty bang bang :sunglasses:\n");
									} else {
										printf("%s\n", MH_StatusToString(status));
									}
								} else {
									printf("%s\n", MH_StatusToString(status));
								}
								break;
							}
						}
					}
					while (true) {
						if (__Method_ptr) {
							CMethod* md = (CMethod*)__Method_ptr;
							if (md) {
								CConstMethod* constMethod = md->_constMethod;
								if (constMethod) {
									CConstantPool* constantPool = constMethod->_constants;
									if (constantPool) {
										CInstanceKlass* holder = constantPool->_pool_holder;
										if (holder) {
											//if (__JavaThread_ptr) {
											//	printf("0x%p\n", __JavaThread_ptr);
											//}
											if (holder->_name->as_C_string()._Equal("bud")) {
												if (constantPool->symbol_at(constMethod->_name_index)->as_C_string()._Equal("g")) {
													if (constantPool->symbol_at(constMethod->_signature_index)->as_C_string()._Equal("(Ljava/lang/String;)V")) {
														printf("sendChatMessage called! javathread: 0x%p\n", __JavaThread_ptr);
														if (__Args_ptr_rbp) {
															printf("0x%p\n", __Args_ptr_rbp);
															//Args* args = *(Args**)((uintptr_t)__Args_ptr_rbp - 0x20);
															//if (args->_args->_body) printf("%s\n", args->_args->_body);
														}
														//__asm int3
													}
												} else {
													printf("An interpreted method from bud class has been called! Name: %s, signature: %s\n",
														constantPool->symbol_at(constMethod->_name_index)->as_C_string().c_str(),
														constantPool->symbol_at(constMethod->_signature_index)->as_C_string().c_str());
												}
											} else {
												//printf("An interpreted method from %s class has been called! Name: %s, signature: %s\n",
													//holder->_name->as_C_string().c_str(),
													//constantPool->symbol_at(constMethod->_name_index)->as_C_string().c_str(),
													//constantPool->symbol_at(constMethod->_signature_index)->as_C_string().c_str());
											}
										}
									}
								}
							}
						}
					}
				}
			}
#endif //_DEBUG
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