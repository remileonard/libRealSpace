#pragma once
#include <type_traits>

// Emplacement de création d'un rôle d'activité : create() rend le type enregistré par
// registerType<T>(), ou Base si rien n'a été enregistré.
template <class Base>
class Creatable {
    inline static Base *(*s_factory)() = nullptr;

    template <class T>
    static Base *instantiate() { return new T(); }

public:
    static Base *create() {
        return s_factory != nullptr ? s_factory() : new Base();
    }

    template <class T>
    static void registerType() {
        static_assert(std::is_base_of_v<Base, T>, "T doit dériver de Base");
        s_factory = &instantiate<T>;
    }
};
