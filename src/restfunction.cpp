#include "restfunction.h"
#include "restcontenttypes.h"
#include "nap/logger.h"

#include <thread>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::RestFunction)
    RTTI_PROPERTY("Address", &nap::RestFunction::mAddress, nap::rtti::EPropertyMetaData::Default)
        RTTI_PROPERTY("ValueDescriptions", &nap::RestFunction::mValueDescriptions, nap::rtti::EPropertyMetaData::Embedded)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::RestEchoFunction)
RTTI_END_CLASS

namespace nap
{
    //////////////////////////////////////////////////////////////////////////
    //// RestEchoFunction
    //////////////////////////////////////////////////////////////////////////

    RestResponse RestEchoFunction::call(const RestValueMap& values)
    {
        // Construct JSON
        rapidjson::Document data(rapidjson::kObjectType);
        for(const auto& [key, value] : values)
        {
            if(value->getRepresentedType() == RTTI_OF(int))
            {
                int val = static_cast<const APIInt&>(*value.get()).mValue;
                data.AddMember(rapidjson::StringRef(value->mName.c_str()), val, data.GetAllocator());
                continue;
            }

            if(value->getRepresentedType() == RTTI_OF(float))
            {
                float val = static_cast<const APIFloat&>(*value.get()).mValue;
                data.AddMember(rapidjson::StringRef(value->mName.c_str()), val, data.GetAllocator());
                continue;
            }

            if(value->getRepresentedType() == RTTI_OF(std::string))
            {
                const char* val = static_cast<const APIString&>(*value.get()).mValue.c_str();
                data.AddMember(rapidjson::StringRef(value->mName.c_str()), rapidjson::StringRef(val), data.GetAllocator());
                continue;
            }

            if(value->getRepresentedType() == RTTI_OF(bool))
            {
                bool val = static_cast<const APIBool&>(*value.get()).mValue;
                data.AddMember(rapidjson::StringRef(value->mName.c_str()), val, data.GetAllocator());
                continue;
            }

            if(value->getRepresentedType() == RTTI_OF(double))
            {
                double val = static_cast<const APIDouble&>(*value.get()).mValue;
                data.AddMember(rapidjson::StringRef(value->mName.c_str()), val, data.GetAllocator());
                continue;
            }

            if(value->getRepresentedType() == RTTI_OF(long))
            {
                long val = static_cast<const APILong&>(*value.get()).mValue;
                data.AddMember(rapidjson::StringRef(value->mName.c_str()), val, data.GetAllocator());
                continue;
            }

            nap::Logger::warn(*this, "Unsupported value type: %s, ignoring", value->getRepresentedType().get_name().to_string().c_str());
        }

        // Serialize the response
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetMaxDecimalPlaces(4);
        data.Accept(writer);

        return { buffer.GetString(), rest::contenttypes::json };
    }
}