#pragma once

#include <nap/resource.h>

namespace nap
{
    /**
     * RestBaseValue describes a value that can be extracted from a rest call
     */
    class NAPAPI RestBaseValue : public Resource
    {
    RTTI_ENABLE(Resource)
    public:
        RestBaseValue(rtti::TypeInfo type) : mRepresentedType(type)
        { }

        /**
         * @return the actual type (int, float, etc..) represented by this Rest value
         */
        rtti::TypeInfo getRepresentedType() const { return mRepresentedType; }

        // properties
        std::string mName; ///< Property : 'Name' The name of the value
        bool mRequired = false; ///< Property : 'Required' If the value is required
    private:
        rtti::TypeInfo mRepresentedType;
    };

    /**
     * RestValue is a RestBaseValue that represents a specific type
     * @tparam T the type of the value
     */
    template<typename T>
    class NAPAPI RestValue : public RestBaseValue
    {
    RTTI_ENABLE(RestBaseValue)
    public:
        RestValue();
    };


    template<typename T>
    RestValue<T>::RestValue() : RestBaseValue(RTTI_OF(T))
    { }

    // Type aliases for common types
    using RestValueInt      = RestValue<int>;
    using RestValueFloat    = RestValue<float>;
    using RestValueString   = RestValue<std::string>;
    using RestValueBool     = RestValue<bool>;
    using RestValueDouble   = RestValue<double>;
    using RestValueLong     = RestValue<long>;
}