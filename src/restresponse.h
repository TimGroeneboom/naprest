#pragma once

#include <nap/core.h>

namespace nap
{
    /**
     * Represents a response from a REST call.
     */
    struct NAPAPI RestResponse
    {
        /**
         * Constructor
         * @param data the data
         * @param contentType the content type
         */
        RestResponse(std::string data, std::string contentType) : mData(std::move(data)), mContentType(std::move(contentType)) { }
        RestResponse() = default;

        std::string mData = "";
        std::string mContentType = "";
    };
}