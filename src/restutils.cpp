#include "restutils.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

namespace nap
{
    namespace utility
    {
        RestResponse generateErrorResponse(const std::string& message)
        {
            rapidjson::Document document(rapidjson::kObjectType);
            document.AddMember("status", "error", document.GetAllocator());
            document.AddMember("message", rapidjson::Value(message.c_str(), document.GetAllocator()), document.GetAllocator());

            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            document.Accept(writer);

            RestResponse response;
            response.mData = buffer.GetString();
            response.mContentType = "application/json";

            return response;
        }
    }
}