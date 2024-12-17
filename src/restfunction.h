#pragma once

#include <nap/resource.h>
#include <nap/resourceptr.h>
#include <apivalue.h>

#include "restresponse.h"
#include "restvalue.h"

namespace nap
{
    using RestValueMap = std::unordered_map<std::string, std::unique_ptr<APIBaseValue>>;

    /**
     * Represents a rest call
     * The address is the path of the rest call
     * The values are the values that can be extracted from the request
     */
    class NAPAPI RestFunction : public Resource
    {
        friend class RestServer;
    RTTI_ENABLE(Resource)
    public:
        std::string mAddress; ///< Property : 'Address' The address of the rest call
        std::vector<ResourcePtr<RestBaseValue>> mValueDescriptions; ///< Property : 'Values' The values of the rest call
    protected:
        /**
         * Extracts a value from the values map
         * @tparam T the value type
         * @param name name of the value
         * @param values reference to values map
         * @param value the value in which to store the extracted value
         * @param errorState contains the error state
         * @return true on success
         */
        template<typename T>
        bool extractValue(const std::string& name, const RestValueMap& values, T& value, utility::ErrorState& errorState);

        /**
         * The function to call when the rest call is made
         * Note: this function is called from a server worker thread
         * @param values reference to values map
         * @return RestResponse the response to the call, will be sent back to the client
         */
        virtual RestResponse call(const RestValueMap& values) = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    //// RestFunction Template Definitions
    //////////////////////////////////////////////////////////////////////////

    template<typename T>
    bool RestFunction::extractValue(const std::string& name,
                                    const RestValueMap& values,
                                    T& value,
                                    utility::ErrorState& errorState)
    {
        auto it = values.find(name);
        if(it == values.end())
        {
            errorState.fail("Value not found: " + name);
            return false;
        }

        auto val = rtti_cast<APIValue<T>>(it->second.get());
        if(val == nullptr)
        {
            errorState.fail("Value is not of the correct type: " + name);
            return false;
        }

        value = val->mValue;
        return true;
    }

    /**
     * A simple rest function that echoes the values it receives
     */
    class NAPAPI RestEchoFunction : public RestFunction
    {
    RTTI_ENABLE(RestFunction)
    protected:
        /**
         * The function to call when the rest call is made
         * @param values reference to values map
         * @return RestResponse the response to the call, will be sent back to the client
         */
        virtual RestResponse call(const RestValueMap& values) override;
    private:
    };
}