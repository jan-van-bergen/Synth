#pragma once
#include <type_traits>

namespace meta {
	template<typename ... Ts>
	struct TypeList;

	template<typename T>
	struct TypeList<T> {
		using Head = T;
//		using Tail = nullptr_t;

		static constexpr auto size = 1;
	};

	template<typename T, typename ... Ts>
	struct TypeList<T, Ts ...> {
		using Head = T;
		using Tail = TypeList<Ts ...>;

		static constexpr auto size = 1 + sizeof ... (Ts);
	};

	template<typename ComponentList, typename Target>
	struct TypeListContains;

	template<typename Target, typename T>
	struct TypeListContains<TypeList<T>, Target> {
		static constexpr auto value = std::is_same<Target, T>();
	};

	template<typename Target, typename T, typename ... Ts>
	struct TypeListContains<TypeList<T, Ts ...>, Target> {
		static constexpr auto value = std::is_same<Target, T>() || TypeListContains<TypeList<Ts ...>, Target>::value;
	};
}
