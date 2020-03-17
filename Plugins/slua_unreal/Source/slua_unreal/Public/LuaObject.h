// Tencent is pleased to support the open source community by making sluaunreal available.

// Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the BSD 3-Clause License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at

// https://opensource.org/licenses/BSD-3-Clause

// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#pragma once
#define LUA_LIB
#include "CoreMinimal.h"
#include "lua/lua.hpp"
#include "UObject/UnrealType.h"
#include "UObject/WeakObjectPtr.h"
#include "Blueprint/UserWidget.h"
#include "SluaUtil.h"
#include "LuaArray.h"
#include "LuaMap.h"
#include "Runtime/Launch/Resources/Version.h"

#ifndef SLUA_CPPINST
#define SLUA_CPPINST "__cppinst"
#endif 

// checkUD will report error if ud had been freed
// ���UD
#define CheckUD(Type,L,P) auto UD = LuaObject::checkUD<Type>(L,P);
// UD may be freed by Engine, so skip it in gc phase
// ���UD,���п�
#define CheckUDGC(Type,L,P) auto UD = LuaObject::checkUD<Type>(L,P,false); \
	if(!UD) return 0;

// ע��Ԫ����
#define RegMetaMethodByName(L,NAME,METHOD) \
    lua_pushcfunction(L,METHOD); \
    lua_setfield(L,-2,NAME);

// ע��Ԫ����
#define RegMetaMethod(L,METHOD) RegMetaMethodByName(L,#METHOD,METHOD)

// �½�һ��UD
#define NewUD(T, v, f) auto ud = lua_newuserdata(L, sizeof(UserData<T*>)); \
	if (!ud) luaL_error(L, "out of memory to new ud"); \
	auto udptr = reinterpret_cast< UserData<T*>* >(ud); \
	udptr->parent = nullptr; \
	udptr->ud = const_cast<T*>(v); \
    udptr->flag = f;

// �������,��������self,��Ҫ����UE4����
#define CheckSelf(T) \
	auto udptr = reinterpret_cast<UserData<T*>*>(lua_touserdata(L, 1)); \
	if(!udptr) luaL_error(L, "self ptr missing"); \
	if (udptr->flag & UD_HADFREE) \
		luaL_error(L, "checkValue error, obj parent has been freed"); \
	auto self = udptr->ud

// �Ƿ���out����
// ����: ��OutParm,����ConstParm,����BlueprintReadOnly
#define IsRealOutParam(propflag) ((propflag&CPF_OutParm) && !(propflag&CPF_ConstParm) && !(propflag&CPF_BlueprintReadOnly))

class ULatentDelegate;

namespace NS_SLUA {

    class LuaVar;

	// �Զ��ָ�ջ
    struct AutoStack {
        AutoStack(lua_State* l) {
            this->L = l;
            this->top = lua_gettop(L);
        }
        ~AutoStack() {
            lua_settop(this->L,this->top);
        }
        lua_State* L;
        int top;
    };

	// Luaʹ��UE4�ṹ��
    struct SLUA_UNREAL_API LuaStruct : public FGCObject {
        uint8* buf;
        uint32 size;
        UScriptStruct* uss;

        LuaStruct(uint8* buf,uint32 size,UScriptStruct* uss);
        ~LuaStruct();

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

#if (ENGINE_MINOR_VERSION>=20) && (ENGINE_MAJOR_VERSION>=4)
		virtual FString GetReferencerName() const override
		{
			return "LuaStruct";
		}
#endif
    };

	// ��FLAG���	
	#define UD_NOFLAG 0
	// �Զ�GC
	// ��ָ��Ҫ��������gc������ʱ�����
	#define UD_AUTOGC 1 // flag userdata should call __gc and maintain by lua
	// �Ѿ����ͷŵ�
	#define UD_HADFREE 1<<2 // flag userdata had been freed

	/*
	 * TSharedPtr
	 * ����ָ�룺����ָ��ӵ�������õĶ��󣬷�ֹ�ö���ɾ��
	 * �����޹���ָ���������������ʱ��ɾ���ö���
	 * ����ָ���Ϊ�գ���ζ�䲻�����κζ���
	 * �ǿչ���ָ������ɹ�������
	 */

	/*
	 * TSharedRef
	 * �������ã����Ƿǿչ���ָ�룩���빲��ָ�����ƣ��̶����÷ǿն���
	 */
	
	#define UD_SHAREDPTR 1<<3 // it's a TSharedPtr in userdata instead of raw pointer
	#define UD_SHAREDREF 1<<4 // it's a TSharedRef in userdata instead of raw pointer
	// �̰߳�ȫ�Ĺ���ָ��
	#define UD_THREADSAFEPTR 1<<5 // it's a TSharedptr with thread-safe mode in userdata instead of raw pointer
	// UObject
	#define UD_UOBJECT 1<<6 // flag it's an UObject
	// UStruct
	#define UD_USTRUCT 1<<7 // flag it's an UStruct

	/*
	 * std::weak_ptr ��һ������ָ��
	 * ���Ա� std::shared_ptr �����Ķ�����ڷ�ӵ����("��")����
	 * �ڷ��������õĶ���ָ��ǰ������ת��Ϊ std::shared_ptr
	 * ��Ҫ������ʾ��ʱ����Ȩ����ĳ���������ʱ����Ҫ������
	 * ת��Ϊshared_ptr�Ĺ��̵��ڶ����shared_ptr �����ü�����һ
	 * ��������ʹ��weak_ptr�������Ȩ�Ĺ�����,ԭ����shared_ptr������
	 * ��ö���������ڻᱻ�ӳ��������ʱ�� std::shared_ptr ������Ϊֹ
	 * weak_ptr�����Ա��� std::shared_ptr ��ѭ������ 
	 */
	
	#define UD_WEAKUPTR 1<<8 // flag it's a weak UObject ptr
	// ������
	#define UD_REFERENCE 1<<9

	struct UDBase {
		uint32 flag;
		void* parent;
	};

	// Memory layout of GenericUserData and UserData should be same
	// �ڴ�һ��
	struct GenericUserData : public UDBase {
		void* ud;
	};

	template<class T>
	struct UserData : public UDBase {
		// ģ�����͵Ĵ�СҪ����ָ���С
		static_assert(sizeof(T) == sizeof(void*), "Userdata type should size equal to sizeof(void*)");
		T ud; 
	};

    DefTypeName(LuaArray);
    DefTypeName(LuaMap);

    template<typename T>
    struct LuaOwnedPtr {
        T* ptr;
        LuaOwnedPtr(T* p):ptr(p) {}
    	// ���� ָ��ṹ���Ա�����
		T* operator->() const {
			return ptr;
		}
    };

	// ref
	template<typename T,ESPMode mode>
	struct SharedRefUD {
	private:
		TSharedRef<T, mode> ref;
	public:
		typedef T type;
		SharedRefUD(const TSharedRef<T, mode>& other) :ref(other) {}
		
		T* get(lua_State *L) {
			return &ref.Get();
		}
	};

	// shared_ptr
	template<typename T, ESPMode mode>
	struct SharedPtrUD {
	private:
		TSharedPtr<T, mode> ptr;
	public:
		typedef T type;
		SharedPtrUD(const TSharedPtr<T, mode>& other) :ptr(other) {}

		T* get(lua_State *L) {
			return ptr.Get();
		}
	};

	// weak_ptr
	struct WeakUObjectUD {
		FWeakObjectPtr ud;
		WeakUObjectUD(FWeakObjectPtr ptr):ud(ptr) {

		}

		bool isValid() {
			return ud.IsValid();
		}

		UObject* get() {
			if(isValid()) return ud.Get();
			else return nullptr;
		}
	};

	// ��ؼ��Ĳ���
    class SLUA_UNREAL_API LuaObject
    {
    private:
		// checkfree�ǵ��������ĺ����ĺ�������
#define CHECK_UD_VALID(ptr) if (ptr && ptr->flag&UD_HADFREE) { \
		if (checkfree) \
			luaL_error(L, "arg %d had been freed(%p), can't be used", lua_absindex(L, p), ptr->ud); \
		else \
			return nullptr; \
		}

    	// �����ǲ���ud��
		template<typename T>
		static T* maybeAnUDTable(lua_State* L, int p,bool checkfree) {
			if(lua_istable(L, p)) {
                AutoStack as(L);
				lua_getfield(L, p, SLUA_CPPINST);
				if (lua_type(L, -1) == LUA_TUSERDATA)
					return checkUD<T>(L, lua_absindex(L, -1), checkfree);
			}
			return nullptr;
		}

        // testudata, if T is base of uobject but isn't uobject, try to  cast it to T
    	// �̳���UObject�Ҳ���UObject
        template<typename T>
        static typename std::enable_if<std::is_base_of<UObject,T>::value && !std::is_same<UObject,T>::value, T*>::type 
		testudata(lua_State* L,int p, bool checkfree=true) {
            UserData<UObject*>* ptr = (UserData<UObject*>*)luaL_testudata(L,p,"UObject");
			CHECK_UD_VALID(ptr);
			T* t = nullptr;
			// if it's a weak UObject, rawget it
			// �����weak_ptr,���WeakUObjectUDȥȡֵ
			if (ptr && ptr->flag&UD_WEAKUPTR) {
				auto wptr = (UserData<WeakUObjectUD*>*)ptr;
				t = Cast<T>(wptr->ud->get());
			}
			else t = ptr?Cast<T>(ptr->ud):nullptr;

			if (!t && lua_isuserdata(L, p)) {

				/*
				 * int luaL_getmetafield (lua_State *L, int obj, const char *e)
				 * ������ obj �������Ԫ���� e ���ֵѹջ
				 * ����ö���û��Ԫ��
				 * ���Ǹ�Ԫ��û�������
				 * �˺���ʲôҲ����ѹջ������ LUA_TNIL
				 */
				
				luaL_getmetafield(L, p, "__name");
				if (lua_isnil(L, -1)) {
					lua_pop(L, 1);
					return t;
				}
				FString clsname(lua_tostring(L, -1));
				lua_pop(L, 1);
				// skip first char may be 'U' or 'A'
				// ��������ǲ��� U ���� A ��ͷ
				if (clsname.Find(T::StaticClass()->GetName()) == 1) {
					UserData<T*>* tptr = (UserData<T*>*) lua_touserdata(L, p);
					t = tptr ? tptr->ud : nullptr;
				}
			}
			else if (!t)
				return maybeAnUDTable<T>(L, p,checkfree);
            return t;
        }

        // testudata, if T is uobject
    	// ģ���� UObject
        template<typename T>
        static typename std::enable_if<std::is_same<UObject,T>::value, T*>::type 
		testudata(lua_State* L,int p, bool checkfree=true) {
            auto ptr = (UserData<T*>*)luaL_testudata(L,p,"UObject");
			CHECK_UD_VALID(ptr);
			if (!ptr) return maybeAnUDTable<T>(L, p, checkfree);
			// if it's a weak UObject ptr
			if (ptr->flag&UD_WEAKUPTR) {
				auto wptr = (UserData<WeakUObjectUD*>*)ptr;
				return wptr->ud->get();
			}
			else
				return ptr?ptr->ud:nullptr;
        }

    	// ��SharedPtrUD�л�ȡֵ
		template<class T>
		static typename std::enable_if<!std::is_base_of<TSharedFromThis<T>,T>::value, T*>::type
		unboxSharedUD(lua_State* L, UserData<T*>* ptr) {
			// ptr may be a ThreadSafe sharedref or NotThreadSafe sharedref
			// but we don't care about it, we just Get raw ptr from it
			static_assert(sizeof(SharedPtrUD<T, ESPMode::NotThreadSafe>) == sizeof(SharedPtrUD<T, ESPMode::ThreadSafe>), "unexpected static assert");
			auto sptr = (UserData<SharedPtrUD<T, ESPMode::NotThreadSafe>*>*)ptr;
			// unbox shared ptr to rawptr
			return sptr->ud->get(L);
		}

		template<class T>
		static typename std::enable_if<std::is_base_of<TSharedFromThis<T>, T>::value, T*>::type
		unboxSharedUD(lua_State* L, UserData<T*>* ptr) {
			// never run here
			ensureMsgf(false, TEXT("You cannot use a TSharedPtr of one mode with a type which inherits TSharedFromThis of another mode."));
			return nullptr;
		}

		// ��SharedRefUD�л�ȡֵ
		template<class T>
		static T* unboxSharedUDRef(lua_State* L, UserData<T*>* ptr) {
			// ptr may be a ThreadSafe sharedref or NotThreadSafe sharedref
			// but we don't care about it, we just Get raw ptr from it
			static_assert(sizeof(SharedRefUD<T, ESPMode::NotThreadSafe>) == sizeof(SharedRefUD<T, ESPMode::ThreadSafe>), "unexpected static assert");
			auto sptr = (UserData<SharedRefUD<T, ESPMode::NotThreadSafe>*>*)ptr;
			// unbox shared ptr to rawptr
			return sptr->ud->get(L);
		}

    	// ����Uobject
        // testudata, if T isn't uobject
        template<typename T>
        static typename std::enable_if<!std::is_base_of<UObject,T>::value && !std::is_same<UObject,T>::value, T*>::type 
		testudata(lua_State* L,int p,bool checkfree=true) {
            auto ptr = (UserData<T*>*)luaL_testudata(L,p,TypeName<T>::value().c_str());
			CHECK_UD_VALID(ptr);
			// ptr is boxed shared ptr?
			if (ptr) {
				if (ptr->flag&UD_SHAREDPTR) {
					return unboxSharedUD<T>(L,ptr);
				}
				else if (ptr->flag&UD_SHAREDREF) {
					return unboxSharedUDRef<T>(L,ptr);
				}
			}
			if (!ptr) return maybeAnUDTable<T>(L, p, checkfree);
            return ptr?ptr->ud:nullptr;
        }

    public:

        typedef int (*PushPropertyFunction)(lua_State* L,UProperty* prop,uint8* parms,bool ref);
    	typedef int (*CheckPropertyFunction)(lua_State* L,UProperty* prop,uint8* parms,int i);

		// UProperty�ڲ�����UClass
        static CheckPropertyFunction getChecker(UClass* prop);
        static PushPropertyFunction getPusher(UProperty* prop);
        static CheckPropertyFunction getChecker(UProperty* cls);
        static PushPropertyFunction getPusher(UClass* cls);

    	// tn		=>	������
    	// noprefix	=>	ǰ׺
		// �����Ƿ�ƥ��
    	static bool matchType(lua_State* L, int p, const char* tn, bool noprefix=false);

    	// class��__indexԪ����
		static int classIndex(lua_State* L);
    	// class��__newindexԪ����
		static int classNewindex(lua_State* L);

    	// �½�һ������
		static void newType(lua_State* L, const char* tn);
    	// �½�һ������,ͬʱָ������
        static void newTypeWithBase(lua_State* L, const char* tn, std::initializer_list<const char*> bases);
		// ���ӷ���,name�Ƿ�����
    	// isInstance => �Ƿ���ʵ������
    	static void addMethod(lua_State* L, const char* name, lua_CFunction func, bool isInstance = true);
		// ����ȫ�ַ���,û������
    	static void addGlobalMethod(lua_State* L, const char* name, lua_CFunction func);
		// �����ֶ�,�ṩget��set����
    	static void addField(lua_State* L, const char* name, lua_CFunction getter, lua_CFunction setter, bool isInstance = true);
		// ���ز�����	=>	����Ԫ����
    	static void addOperator(lua_State* L, const char* name, lua_CFunction func);
		// �������ɷ���,GC�ص����ܹ�ת��Ϊstring������
    	static void finishType(lua_State* L, const char* tn, lua_CFunction ctor, lua_CFunction gc, lua_CFunction strHint=nullptr);
		// Ϊ����������,�Թ���һ����������
    	static void fillParam(lua_State* L, int i, UFunction* func, uint8* params);
		// ������ֵѹջ
    	static int returnValue(lua_State* L, UFunction* func, uint8* params);

    	// ����UFunction
		static void callUFunction(lua_State* L, UObject* obj, UFunction* func, uint8* params);
		// create new enum type to lua, see DefEnum macro
    	// �½�һ��ö������
		template<class T>
		static void newEnum(lua_State* L, const char* tn, const char* keys, std::initializer_list<T> values) {
			FString fkey(keys);
			// remove namespace prefix
			// �Ƴ������ռ�
			fkey.ReplaceInline(*FString::Format(TEXT("{0}::"), { UTF8_TO_TCHAR(tn) }), TEXT(""));
			// remove space
			// �Ƴ��ո�
			fkey.ReplaceInline(TEXT(" "),TEXT(""));
			// create enum table
			// ������
			createTable(L, tn);

			// ����ö��
			FString key, right;
			for (size_t i = 0; i < values.size() && strSplit(fkey, ",", &key, &right); ++i) {
				lua_pushinteger(L, (int)(*(values.begin() + i)));
				lua_setfield(L, -2, TCHAR_TO_UTF8(*key));
				fkey = right;
			}
			// pop t
			lua_pop(L, 1);
		}
    	// ��ʼ��,ע��ÿ�����͵�push������check����
        static void init(lua_State* L);

        // check arg at p is exported lua class named __name in field 
        // of metable of the class, if T is base of class or class is T, 
        // return the pointer of class, otherwise return nullptr
        // �������ǲ�����__nameԪ��������Ԫ�� ȡ�� class
        // ���ģ���Ǽ̳���class������������class,����classָ��
        // ���򷵻�nullptr
        template<typename T>
        static T* checkUD(lua_State* L,int p,bool checkfree=true) {
        	// nil��ֱ�ӷ���
			if (lua_isnil(L, p))
			{
				return nullptr;
			}

        	// ����ǲ���UD,������ģ��ָ��
            T* ret = testudata<T>(L,p, checkfree);
            if(ret) return ret;

        	/*
        	 * int luaL_getmetafield (lua_State *L, int obj, const char *e)
        	 * ������ obj �������Ԫ���� e ���ֵѹջ
        	 * ����ö���û��Ԫ�������Ǹ�Ԫ��û�������
        	 * �˺���ʲôҲ����ѹջ������ LUA_TNIL
        	 */
        	
            const char *typearg = nullptr;
            if (luaL_getmetafield(L, p, "__name") == LUA_TSTRING)
                typearg = lua_tostring(L, -1);

            lua_pop(L,1);

            if(checkfree && !typearg)
                luaL_error(L,"expect userdata at %d",p);

        	// ��������Ƿ�ƥ��
			if (LuaObject::isBaseTypeOf(L, typearg, TypeName<T>::value().c_str())) {
				UserData<T*> *udptr = reinterpret_cast<UserData<T*>*>(lua_touserdata(L, p));
				CHECK_UD_VALID(udptr);
				return udptr->ud;
			}
            if(checkfree) 
				luaL_error(L,"expect userdata %s, but got %s",TypeName<T>::value().c_str(),typearg);
            return nullptr;
        }

    	// ����ƥ��,��ȥ�жϻ������ͺ�UObject,���ж�UD���������
		template<class T>
		static T checkValueOpt(lua_State* L, int p, const T& defaultValue=T()) {
			if (!typeMatched<T>(lua_type(L,p))) {
				return defaultValue;
			} else {
				return checkValue<T>(L, p);
			}
		}

    	// ��鷵��ֵ,ģ����ָ��
		template<class T>
		static typename std::enable_if< std::is_pointer<T>::value,T >::type checkReturn(lua_State* L, int p) {
			UserData<T> *udptr = reinterpret_cast<UserData<T>*>(lua_touserdata(L, p));
        	if (udptr->flag & UD_HADFREE)
				luaL_error(L, "checkValue error, obj parent has been freed");
			// if it's an UStruct, check struct type name is matched
			if (udptr->flag & UD_USTRUCT) {
				UserData<LuaStruct*> *structptr = reinterpret_cast<UserData<LuaStruct*>*>(udptr);
				LuaStruct* ls = structptr->ud;
				// skip first prefix like 'F','U','A'
				// ȥ��ǰ׺,�Ƚ��������ǲ���һ��
				if (sizeof(typename std::remove_pointer<T>::type) == ls->size && strcmp(TypeName<T>::value().c_str()+1, TCHAR_TO_UTF8(*ls->uss->GetName())) == 0)
					return (T)(ls->buf);
				else
					luaL_error(L, "checkValue error, type dismatched, expect %s", TypeName<T>::value().c_str());
			}
			return udptr->ud;
		}

		// if T isn't pointer, we should assume UserData box a pointer and dereference it to return
		// ��鷵��ֵ,����ָ������
    	template<class T>
		static typename std::enable_if< !std::is_pointer<T>::value,T >::type checkReturn(lua_State* L, int p) {
			UserData<T*> *udptr = reinterpret_cast<UserData<T*>*>(lua_touserdata(L, p));
			if (udptr->flag & UD_HADFREE)
				luaL_error(L, "checkValue error, obj parent has been freed");
			// if it's an UStruct, check struct type name is matched
			if (udptr->flag & UD_USTRUCT) {
				UserData<LuaStruct*> *structptr = reinterpret_cast<UserData<LuaStruct*>*>(udptr);
				LuaStruct* ls = structptr->ud;
				// skip first prefix like 'F','U','A'
				if (sizeof(T) == ls->size && strcmp(TypeName<T>::value().c_str() + 1, TCHAR_TO_UTF8(*ls->uss->GetName())) == 0)
					return *((T*)(ls->buf));
				else
					luaL_error(L, "checkValue error, type dismatched, expect %s", TypeName<T>::value().c_str());
			}
			return *(udptr->ud);
		}

    	// ���ֵ
        template<class T>
		static T checkValue(lua_State* L, int p) {
        	// ָ��
            if(std::is_pointer<T>::value && lua_isnil(L,p))
                return T();

            using TT = typename remove_cr<T>::type;
			// ����ָ��
        	if(std::is_class<TT>::value && std::is_default_constructible<TT>::value && lua_isnil(L,p))
                return TT();

			static_assert(!std::is_same<wchar_t*, typename remove_ptr_const<T>::type>::value,
				"checkValue does not support parameter const TCHAR*, use FString instead");

			if (!lua_isuserdata(L, p))
				luaL_error(L, "expect userdata at arg %d", p);

			return checkReturn<T>(L, p);
		}

    	// out����ֵ
		template<class T>
		bool checkValue(lua_State* L, int p, T& out) {
			if (!typeMatched<T>(lua_type(L, p)))
				return false;
			out = checkValue<T>(L, p);
			return true;
		}

		// check value if it's enum
    	// ö��,�ȿ��ǲ�������,Ȼ������ת��
		template<typename T>
		static T checkEnumValue(lua_State* L, int p) {
			return static_cast<T>(luaL_checkinteger(L, p));
		}

        // check value if it's TArray
    	// LuaArray ת��Ϊ TArray
    	// ElementType TArray�洢������
        template<class T>
		static T checkTArray(lua_State* L, int p) {
            CheckUD(LuaArray,L,p);
			return UD->asTArray<typename T::ElementType>(L);
		}

		// check value if it's TMap
    	// LuaMap ת��Ϊ TMap
		template<class T>
		static T checkTMap(lua_State* L, int p) {
			CheckUD(LuaMap, L, p);
			using KeyType = typename TPairTraits<typename T::ElementType>::KeyType;
			using ValueType = typename TPairTraits<typename T::ElementType>::ValueType;
			return UD->asTMap<KeyType, ValueType>(L);
		}

    	// UObject
        template<class T>
        static UObject* checkUObject(lua_State* L,int p) {
            UserData<UObject*>* ud = reinterpret_cast<UserData<UObject*>*>(luaL_checkudata(L, p,"UObject"));
            if(!ud) luaL_error(L, "checkValue error at %d",p);
            return Cast<T>(ud->ud);
        }

    	// תvoid * const�汾
        template<typename T>
        static void* void_cast( const T* v ) {
            return reinterpret_cast<void *>(const_cast< T* >(v));
        }

		// תvoid * ��const�汾
        template<typename T>
        static void* void_cast( T* v ) {
            return reinterpret_cast<void *>(v);
        }

    	// push obj ������cacheObj
    	// �����obj������UObject����������������
		template<class T>
		static int push(lua_State* L, const char* fn, const T* v, uint32 flag = UD_NOFLAG) {
            if(getFromCache(L,void_cast(v),fn)) return 1;
            luaL_getmetatable(L,fn);
			// if v is the UnrealType
			UScriptStruct* uss = nullptr;
        	// �ж��ǲ���UE�Ľṹ��
			if (lua_isnil(L, -1) && isUnrealStruct(fn, &uss)) {
				lua_pop(L, 1); // pop nil
				// UScriptStruct�µ�MinAlignmentΪ1
				uint32 size = uss->GetStructureSize() ? uss->GetStructureSize() : 1;
				ensure(size == sizeof(T));
				uint8* buf = (uint8*)FMemory::Malloc(size);
				uss->InitializeStruct(buf);
				uss->CopyScriptStruct(buf, v);
				cacheObj(L, void_cast(v));
				return push(L, new LuaStruct(buf, size, uss));
			}
        	// �½�һ��UD
			NewUD(T, v, flag);
			lua_pushvalue(L, -2);
			lua_setmetatable(L, -2);
			lua_remove(L, -2); // remove metatable of fn
            cacheObj(L,void_cast(v));
            return 1;
		}

    	// �Ƴ�����
		static void releaseLink(lua_State* L, void* prop);
		// ��������
    	static void linkProp(lua_State* L, void* parent, void* prop);

    	// push������������
		template<class T>
		static int pushAndLink(lua_State* L, const void* parent, const char* tn, const T* v) {
			if (getFromCache(L, void_cast(v), tn)) return 1;
			NewUD(T, v, UD_NOFLAG);
			luaL_getmetatable(L, tn);
			lua_setmetatable(L, -2);
			cacheObj(L, void_cast(v));
			linkProp(L, void_cast(parent), void_cast(udptr));
			return 1;
		}

    	// ����Ԫ������
        typedef void SetupMetaTableFunc(lua_State* L,const char* tn,lua_CFunction setupmt,lua_CFunction gc);

    	// push����,�����ô�����gc����
        template<class T, bool F = IsUObject<T>::value >
        static int pushType(lua_State* L,T cls,const char* tn,lua_CFunction setupmt=nullptr,lua_CFunction gc=nullptr) {
            if(!cls) {
                lua_pushnil(L);
                return 1;
            }
            UserData<T>* ud = reinterpret_cast< UserData<T>* >(lua_newuserdata(L, sizeof(UserData<T>)));
			ud->parent = nullptr;
            ud->ud = cls;
            ud->flag = gc!=nullptr?UD_AUTOGC:UD_NOFLAG;
        	// ָ��ΪUObject
			if (F) ud->flag |= UD_UOBJECT;
        	// ����Ԫ��,�󶨴���������gc����
            setupMetaTable(L,tn,setupmt,gc);
            return 1;
        }

		// for weak UObject
		// �ͷ�weak_ptr
		static int gcWeakUObject(lua_State* L) {
			luaL_checktype(L, 1, LUA_TUSERDATA);
			UserData<WeakUObjectUD*>* ud = reinterpret_cast<UserData<WeakUObjectUD*>*>(lua_touserdata(L, 1));
			ensure(ud->flag&UD_WEAKUPTR);
			ud->flag |= UD_HADFREE;
			SafeDelete(ud->ud);
			return 0;
		}

    	// push weak_type
		static int pushWeakType(lua_State* L, WeakUObjectUD* cls) {
			UserData<WeakUObjectUD*>* ud = reinterpret_cast<UserData<WeakUObjectUD*>*>(lua_newuserdata(L, sizeof(UserData<WeakUObjectUD*>)));
			ud->parent = nullptr;
			ud->ud = cls;
			ud->flag = UD_WEAKUPTR | UD_AUTOGC;
			setupMetaTable(L, "UObject", setupInstanceMT, gcWeakUObject);
			return 1;
		}

		// for TSharePtr version
		// �ͷ�SharedPtr
		template<class T, ESPMode mode>
		static int gcSharedUD(lua_State* L) {
			luaL_checktype(L, 1, LUA_TUSERDATA);
			UserData<T*>* ud = reinterpret_cast<UserData<T*>*>(lua_touserdata(L, 1));
			ud->flag |= UD_HADFREE;
			SafeDelete(ud->ud);
			return 0;
		}

    	// push shareptr
		template<class BOXPUD, ESPMode mode, bool F>
		static int pushSharedType(lua_State* L, BOXPUD* cls, const char* tn, int flag) {
			UserData<BOXPUD*>* ud = reinterpret_cast<UserData<BOXPUD*>*>(lua_newuserdata(L, sizeof(UserData<BOXPUD*>)));
			ud->parent = nullptr;
			ud->ud = cls;
			ud->flag = UD_AUTOGC | flag;
			if (F) ud->flag |= UD_UOBJECT;
			if (mode == ESPMode::ThreadSafe) ud->flag |= UD_THREADSAFEPTR;
			setupMetaTable(L, tn, gcSharedUD<BOXPUD, mode>);
			return 1;
		}

    	// push SharedPtrUD
		template<class T,ESPMode mode, bool F = IsUObject<T>::value>
		static int pushType(lua_State* L, SharedPtrUD<T, mode>* cls, const char* tn) {
			if (!cls) {
				lua_pushnil(L);
				return 1;
			}
			using BOXPUD = SharedPtrUD<T, mode>;
			return pushSharedType<BOXPUD,mode,F>(L, cls, tn, UD_SHAREDPTR);
		}

    	// push SharedRefUD
		template<class T, ESPMode mode, bool F = IsUObject<T>::value>
		static int pushType(lua_State* L, SharedRefUD<T, mode>* cls, const char* tn) {
			if (!cls) {
				lua_pushnil(L);
				return 1;
			}
			using BOXPUD = SharedRefUD<T, mode>;
			return pushSharedType<BOXPUD, mode,F>(L, cls, tn, UD_SHAREDREF);
		}

    	// ���ӵ�����
        static void addRef(lua_State* L,UObject* obj, void* ud, bool ref);
		// �Ƴ�����
    	static void removeRef(lua_State* L,UObject* obj);

    	// push GCObject ������������,GC�������Ƿ�����
        template<typename T>
        static int pushGCObject(lua_State* L,T obj,const char* tn,lua_CFunction setupmt,lua_CFunction gc,bool ref) {
            if(getFromCache(L,obj,tn)) return 1;
            lua_pushcclosure(L,gc,0);
            int f = lua_gettop(L);
            int r = pushType<T>(L,obj,tn,setupmt,f);
            lua_remove(L,f); // remove wraped gc function
			if (r) {
				addRef(L, obj, lua_touserdata(L, -1), ref);
				cacheObj(L, obj);
			}
            return r;
        }

    	// push Object
        template<typename T>
        static int pushObject(lua_State* L,T obj,const char* tn,lua_CFunction setupmt=nullptr) {
            if(getFromCache(L,obj,tn)) return 1;
            int r = pushType<T>(L,obj,tn,setupmt,nullptr);
            if(r) cacheObj(L,obj);
            return r;
        }

    	// ����self��__indexԪ��
		static int setupMTSelfSearch(lua_State* L);

    	// push Class
        static int pushClass(lua_State* L,UClass* cls);
		// push Struct
        static int pushStruct(lua_State* L,UScriptStruct* cls);
		// push Enum
		static int pushEnum(lua_State* L, UEnum* e);
    	// push �Ƿ�ԭʼ���Ƿ�����
		static int push(lua_State* L, UObject* obj, bool rawpush=false, bool ref=true);
    	// const�汾
		inline static int push(lua_State* L, const UObject* obj) {
			return push(L, const_cast<UObject*>(obj));
		}
    	// push����
		static int push(lua_State* L, FWeakObjectPtr ptr);
		static int push(lua_State* L, FScriptDelegate* obj);
		static int push(lua_State* L, LuaStruct* ls);
		static int push(lua_State* L, double v);
        static int push(lua_State* L, int64 v);
        static int push(lua_State* L, uint64 v);
        static int push(lua_State* L, int8 v);
        static int push(lua_State* L, uint8 v);
        static int push(lua_State* L, int16 v);
        static int push(lua_State* L, uint16 v);
		static int push(lua_State* L, float v);
		static int push(lua_State* L, int v);
		static int push(lua_State* L, bool v);
		static int push(lua_State* L, uint32 v);
		static int push(lua_State* L, void* v);
		static int push(lua_State* L, const FText& v);
		static int push(lua_State* L, const FString& str);
		static int push(lua_State* L, const FName& str);
		static int push(lua_State* L, const char* str);
		static int push(lua_State* L, const LuaVar& v);
        static int push(lua_State* L, UFunction* func, UClass* cls=nullptr);
		static int push(lua_State* L, const LuaLString& lstr);
		static int push(lua_State* L, UProperty* up, uint8* parms, bool ref=true);
		static int push(lua_State* L, UProperty* up, UObject* obj, bool ref=true);

        // check tn is base of base
    	// ���̳й�ϵ
        static bool isBaseTypeOf(lua_State* L,const char* tn,const char* base);

    	// ����UObject,Ҳ����LUA_type,ͨ��TypeName��ȡģ������
        template<typename T>
        static int push(lua_State* L,T* ptr,typename std::enable_if<!std::is_base_of<UObject,T>::value && !Has_LUA_typename<T>::value>::type* = nullptr) {
            return push(L, TypeName<T>::value().c_str(), ptr);
        }

		// it's an override for non-uobject, non-ptr, only accept struct or class value
    	// ����UObject
		template<typename T>
		static int push(lua_State* L, const T& v, typename std::enable_if<!std::is_base_of<UObject, T>::value && std::is_class<T>::value>::type* = nullptr) {
			T* newPtr = new T(v);
			return push<T>(L, TypeName<T>::value().c_str(), newPtr, UD_AUTOGC);
		}

		// if T has a member function named LUA_typename,
		// used this branch
		// ����UObject,��LUA_type
		template<typename T>
		static int push(lua_State* L, T* ptr, typename std::enable_if<!std::is_base_of<UObject, T>::value && Has_LUA_typename<T>::value>::type* = nullptr) {
			return push(L, ptr->LUA_typename().c_str(), ptr);
		}

    	// Lua����Ptr
		template<typename T>
		static int push(lua_State* L, LuaOwnedPtr<T> ptr, typename std::enable_if<!std::is_base_of<UObject, T>::value && Has_LUA_typename<T>::value>::type* = nullptr) {
			return push(L, ptr->LUA_typename().c_str(), ptr.ptr, UD_AUTOGC);
		}

		// Lua����Ptr
		template<typename T>
		static int push(lua_State* L, LuaOwnedPtr<T> ptr, typename std::enable_if<!std::is_base_of<UObject, T>::value && !Has_LUA_typename<T>::value>::type* = nullptr) {
			return push(L, TypeName<T>::value().c_str(), ptr.ptr, UD_AUTOGC);
		}

		static int gcSharedPtr(lua_State *L) {
			return 0;
		}

    	// TSharedPtr ��ȡ����,�ٰ�װһ��
		template<typename T, ESPMode mode>
		static int push(lua_State* L, const TSharedPtr<T, mode>& ptr) {
			// get raw ptr from sharedptr
			T* rawptr = ptr.Get();
			// get typename 
			auto tn = TypeName<T>::value();
			if (getFromCache(L, rawptr, tn.c_str())) return 1;
			int r = pushType<T>(L, new SharedPtrUD<T, mode>(ptr), tn.c_str());
			if (r) cacheObj(L, rawptr);
			return r;
		}

    	// TSharedRef ��ȡ����,�ٰ�װһ��
		template<typename T, ESPMode mode>
		static int push(lua_State* L, const TSharedRef<T, mode>& ref) {
			// get raw ptr from sharedptr
			T& rawref = ref.Get();
			// get typename 
			auto tn = TypeName<T>::value();
			if (getFromCache(L, &rawref, tn.c_str())) return 1;
			int r = pushType<T>(L, new SharedRefUD<T, mode>(ref), tn.c_str());
			if (r) cacheObj(L, &rawref);
			return r;
		}

		// for TBaseDelegate
    	// ��������,���ί��
		template<class R, class ...ARGS>
		static int push(lua_State* L, TBaseDelegate<R, ARGS...>& delegate);
		

    	// ö������,ת��Ϊint
        template<typename T>
        static int push(lua_State* L,T v,typename std::enable_if<std::is_enum<T>::value>::type* = nullptr) {
            return push(L,static_cast<int>(v));
        }

    	// TArray
		template<typename T>
		static int push(lua_State* L, const TArray<T>& v) {
			return LuaArray::push(L, v);
		}

    	// TMap
		template<typename K,typename V>
		static int push(lua_State* L, const TMap<K,V>& v) {
			return LuaMap::push(L, v);
		}

		// static int push(lua_State* L, FScriptArray* array);

    	// push nil
        static int pushNil(lua_State* L) {
            lua_pushnil(L);
            return 1;
        }

    	// ���Ӷ���ķ���
		static void addExtensionMethod(UClass* cls, const char* n, lua_CFunction func, bool isStatic = false);
		// ���Ӷ��������
    	static void addExtensionProperty(UClass* cls, const char* n, lua_CFunction getter, lua_CFunction setter, bool isStatic = false);

    	// ��ѯ���淽��
        static UFunction* findCacheFunction(lua_State* L,UClass* cls,const char* fname);
    	// ���ӻ��淽��
        static void cacheFunction(lua_State* L, UClass* cls,const char* fame,UFunction* func);

		// ��ѯ��������
        static UProperty* findCacheProperty(lua_State* L, UClass* cls, const char* pname);
		// ���ӻ�������
    	static void cacheProperty(lua_State* L, UClass* cls, const char* pname, UProperty* property);

    	// ��ѯ�������
        static bool getFromCache(lua_State* L, void* obj, const char* tn, bool check = true);
		// ���ӻ������
    	static void cacheObj(lua_State* L, void* obj);
    	// ɾ���������
		static void removeFromCache(lua_State* L, void* obj);
		static ULatentDelegate* getLatentDelegate(lua_State* L);
    	// ����LuaStruct��װ�Ķ���
		static void deleteFGCObject(lua_State* L,FGCObject* obj);
    private:
    	// ����Ԫ��
        static int setupClassMT(lua_State* L);
        static int setupInstanceMT(lua_State* L);
        static int setupInstanceStructMT(lua_State* L);
        static int setupStructMT(lua_State* L);

    	// gc
        static int gcObject(lua_State* L);
        static int gcClass(lua_State* L);
        static int gcStructClass(lua_State* L);
		static int gcStruct(lua_State* L);
    	// ����ת�ַ���
        static int objectToString(lua_State* L);

    	// ����gc����
        static void setupMetaTable(lua_State* L,const char* tn,lua_CFunction setupmt,lua_CFunction gc);
		static void setupMetaTable(lua_State* L, const char* tn, lua_CFunction setupmt, int gc);
		static void setupMetaTable(lua_State* L, const char* tn, lua_CFunction gc);

    	// ����RPC����
		static void callRpc(lua_State* L, UObject* obj, UFunction* func, uint8* params);

    	// push ����
        template<class T, bool F = IsUObject<T>::value>
        static int pushType(lua_State* L,T cls,const char* tn,lua_CFunction setupmt,int gc) {
            if(!cls) {
                lua_pushnil(L);
                return 1;
            }
                
            UserData<T>* ud = reinterpret_cast< UserData<T>* >(lua_newuserdata(L, sizeof(UserData<T>)));
			ud->parent = nullptr;
            ud->ud = cls;
            ud->flag = F|UD_AUTOGC;
			if (F) ud->flag |= UD_UOBJECT;
            setupMetaTable(L,tn,setupmt,gc);
            return 1;
        }
    	// ����һ��ȫ�ֱ�
		static void createTable(lua_State* L, const char* tn);
    };
	
	// ���UD������
    template<>
    inline UClass* LuaObject::checkValue(lua_State* L, int p) {
        CheckUD(UClass, L, p);
        return UD;
    }

    template<>
    inline UObject* LuaObject::checkValue(lua_State* L, int p) {
		CheckUD(UObject, L, p);
		return UD;
    }

    template<>
    inline UScriptStruct* LuaObject::checkValue(lua_State* L, int p) {
        CheckUD(UScriptStruct, L, p);
        return UD;
    }

    template<>
    inline LuaStruct* LuaObject::checkValue(lua_State* L, int p) {
        CheckUD(LuaStruct, L, p);
        return UD;
    }

    template<>
    inline const char* LuaObject::checkValue(lua_State* L, int p) {
        return luaL_checkstring(L, p);
    }

    template<>
    inline float LuaObject::checkValue(lua_State* L, int p) {
        return (float)luaL_checknumber(L, p);
    }

    template<>
    inline double LuaObject::checkValue(lua_State* L, int p) {
        return luaL_checknumber(L, p);
    }

    template<>
    inline int LuaObject::checkValue(lua_State* L, int p) {
        return luaL_checkinteger(L, p);
    }

    template<>
    inline uint32 LuaObject::checkValue(lua_State* L, int p) {
        return (uint32) luaL_checkinteger(L, p);
    }

    template<>
    inline int8 LuaObject::checkValue(lua_State* L, int p) {
        return (int8) luaL_checkinteger(L, p);
    }

    template<>
    inline uint8 LuaObject::checkValue(lua_State* L, int p) {
        return (uint8) luaL_checkinteger(L, p);
    }

    template<>
    inline int16 LuaObject::checkValue(lua_State* L, int p) {
        return (int16) luaL_checkinteger(L, p);
    }

    template<>
    inline uint16 LuaObject::checkValue(lua_State* L, int p) {
        return (uint16) luaL_checkinteger(L, p);
    }

    template<>
    inline int64 LuaObject::checkValue(lua_State* L, int p) {
        return luaL_checkinteger(L, p);
    }

    template<>
    inline uint64 LuaObject::checkValue(lua_State* L, int p) {
        return luaL_checkinteger(L, p);
    }

    template<>
    inline bool LuaObject::checkValue(lua_State* L, int p) {
        luaL_checktype(L, p, LUA_TBOOLEAN);
        return !!lua_toboolean(L, p);
    }

    template<>
    inline FText LuaObject::checkValue(lua_State* L, int p) {
        const char* s = luaL_checkstring(L, p);
        return FText::FromString(UTF8_TO_TCHAR(s));
    }

    template<>
    inline FString LuaObject::checkValue(lua_State* L, int p) {
        const char* s = luaL_checkstring(L, p);
        return FString(UTF8_TO_TCHAR(s));
    }

    template<>
    inline FName LuaObject::checkValue(lua_State* L, int p) {
        const char* s = luaL_checkstring(L, p);
        return FName(UTF8_TO_TCHAR(s));
    }

    template<>
    inline void* LuaObject::checkValue(lua_State* L, int p) {
        luaL_checktype(L,p,LUA_TLIGHTUSERDATA);
        return lua_touserdata(L,p);
    }

	// push ����
	template<>
	inline int LuaObject::pushType<LuaStruct*, false>(lua_State* L, LuaStruct* cls,
		const char* tn, lua_CFunction setupmt, lua_CFunction gc) {
		if (!cls) {
			lua_pushnil(L);
			return 1;
		}
		UserData<LuaStruct*>* ud = reinterpret_cast<UserData<LuaStruct*>*>(lua_newuserdata(L, sizeof(UserData<LuaStruct*>)));
		ud->parent = nullptr;
		ud->ud = cls;
		ud->flag = gc != nullptr ? UD_AUTOGC : UD_NOFLAG;
		ud->flag |= UD_USTRUCT;
		setupMetaTable(L, tn, setupmt, gc);
		return 1;
	}
}