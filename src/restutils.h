#pragma once

#include "restresponse.h"

namespace nap
{
    namespace utility
    {
        /**
         * Generate an error response
         * An error response is a JSON object with a status field set to "error" and a message field set to the provided message
         * @param message the error message
         * @return the error response
         */
        RestResponse NAPAPI generateErrorResponse(const std::string& message);
    }
}
