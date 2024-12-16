#include "restclient.h"
#include "restservice.h"
#include "httplibwrapper.h"
#include "nap/logger.h"

#include <mathutils.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::RestClient)
    RTTI_CONSTRUCTOR(nap::RestService&)
    RTTI_PROPERTY("URL", &nap::RestClient::mURL, nap::rtti::EPropertyMetaData::Default, "The URL of the REST client")
    RTTI_PROPERTY("CertificatePath", &nap::RestClient::mCertPath, nap::rtti::EPropertyMetaData::Default, "The path to the certificate file")
    RTTI_PROPERTY("ArraySeparator", &nap::RestClient::mArraySeparator, nap::rtti::EPropertyMetaData::Default, "The separator used for arrays of parameters in the URL")
    RTTI_PROPERTY("Headers", &nap::RestClient::mHeaders, nap::rtti::EPropertyMetaData::Default, "The headers to send with the request")
    RTTI_PROPERTY("Timeout", &nap::RestClient::mTimeOutSeconds, nap::rtti::EPropertyMetaData::Default, "The timeout in seconds for the request")
RTTI_END_CLASS

RTTI_BEGIN_STRUCT(nap::RestHeader)
    RTTI_PROPERTY("Key", &nap::RestHeader::key, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Value", &nap::RestHeader::value, nap::rtti::EPropertyMetaData::Default)
RTTI_END_STRUCT

namespace nap
{
    ////////////////////////////////////////////////////////////////////////////
    //// Helper functions forwarded declarations
    ////////////////////////////////////////////////////////////////////////////

    static std::vector<std::unique_ptr<APIBaseValue>> copyParams(const std::vector<std::unique_ptr<APIBaseValue>> &params);

    static httplib::Params toHTTPParams(const std::vector<std::unique_ptr<APIBaseValue>> &params, const std::string& arraySeparator);

    static httplib::Headers toHTTPHeaders(const std::vector<RestHeader>& headers);

    template<typename T>
    static void addValue(std::vector<std::unique_ptr<APIBaseValue>>& copy, APIBaseValue& value);

    ////////////////////////////////////////////////////////////////////////////
    //// RestClient::Impl
    ////////////////////////////////////////////////////////////////////////////

    struct RestClient::Impl
    {
        Impl(const std::string &url) : mClient(httplib::Client(url))
        {}

        httplib::Client mClient;
    };

    ////////////////////////////////////////////////////////////////////////////
    //// RestClient
    ////////////////////////////////////////////////////////////////////////////

    RestClient::RestClient(RestService &service) : mService(service)
    {
    }


    bool RestClient::init(nap::utility::ErrorState &errorState)
    {
        mImpl = std::make_unique<Impl>(mURL);
        mImpl->mClient.set_ca_cert_path("", mCertPath);
        if(mTimeOutSeconds > 0)
        {
            mImpl->mClient.set_connection_timeout(mTimeOutSeconds, 0);
            mImpl->mClient.set_read_timeout(mTimeOutSeconds, 0);
            mImpl->mClient.set_write_timeout(mTimeOutSeconds, 0);
        }


        return true;
    }


    bool RestClient::start(nap::utility::ErrorState &errorState)
    {
        mService.registerRestClient(*this);

        mRunning = true;
        mThread = std::thread(&RestClient::run, this);
        return true;
    }


    void RestClient::stop()
    {
        mService.removeRestClient(*this);

        mRequestEvent.cancelWait();
        mRunning = false;
        mThread.join();
        mImpl->mClient.stop();
    }


    bool RestClient::getBlocking(const std::string &address, const std::vector<std::unique_ptr<APIBaseValue>> &params, nap::RestResponse &response, utility::ErrorState &errorState)
    {
        auto httplib_params = toHTTPParams(params, mArraySeparator);
        auto httplib_headers = toHTTPHeaders(mHeaders);
        httplib::Progress progress = [&](uint64_t current, uint64_t total) -> bool
        {
            // Dispatch progress on main thread
            this->mCallbackQueue.enqueue([this, current, total]()
            {
                mProgressSignal.trigger(current, total);
            });
            return true;
        };
        auto result = mImpl->mClient.Get(address, httplib_params, httplib_headers, progress);

        if(result)
        {
            response.mData = result->body;
            response.mContentType = result->get_header_value("Content-Type");
            return true;
        }

        errorState.fail("Failed to get response from server : %s", to_string(result.error()).c_str());
        return false;
    }


    void RestClient::get(const std::string &address, const std::vector<std::unique_ptr<APIBaseValue>> &params, std::function<void(const RestResponse &)> onSuccess, std::function<void(const utility::ErrorState &)> onError)
    {
        // create deep copy of params
        auto params_copy = copyParams(params);

        // generate uuid
        std::string uuid4 = math::generateUUID();

        // move params
        mRequestParamsCache.emplace(uuid4, std::move(params_copy));

        // Add the request to the queue, request will be processed in the worker thread
        mRequestQueue.enqueue([this, uuid4, address, onSuccess, onError] ()
        {
            // Currently in worker thread

            // Create response object and error state
            RestResponse response;
            utility::ErrorState errorState;

            // Send the request
            if(getBlocking(address, mRequestParamsCache[uuid4], response, errorState))
            {
                // Callback on success, execute on main thread
                mCallbackQueue.enqueue([this, uuid4, onSuccess, response]()
                {
                    // Currently in main thread

                    // callback on success
                    onSuccess(response);

                    // remove params from cache
                    mRequestParamsCache.erase(uuid4);
                });
            }
            else
            {
                // Callback on error, execute on main thread
                mCallbackQueue.enqueue([this, uuid4, onError, errorState]()
                {
                    // Currently in main thread

                    // callback on error
                    onError(errorState);

                    // remove params from cache
                    mRequestParamsCache.erase(uuid4);
                });
            }
        });

        // Signal that a request has been added to the queue
        mRequestEvent.set();
    }


    void RestClient::update(double deltaTime)
    {
        if(mCallbackQueue.size_approx() > 0)
        {
            // Process the callback queue
            std::function<void()> callback;
            while(mCallbackQueue.try_dequeue(callback))
            {
                callback();
            }
        }
    }


    void RestClient::run()
    {
        while(mRunning)
        {
            // Wait for a request to be added to the queue
            mRequestEvent.wait();

            // Process the request queue
            std::function<void()> request;
            while(mRequestQueue.try_dequeue(request))
            {
                request();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    //// Helper functions
    ////////////////////////////////////////////////////////////////////////////

    static std::unordered_map<rtti::TypeInfo, std::function<void(std::vector<std::unique_ptr<APIBaseValue>>&, APIBaseValue&)>> copyValueMap =
    {
        {RTTI_OF(int),          addValue<int>},
        {RTTI_OF(float),        addValue<float>},
        {RTTI_OF(bool),         addValue<bool>},
        {RTTI_OF(std::string),  addValue<std::string>},
        {RTTI_OF(double),       addValue<double>},
        {RTTI_OF(long),         addValue<long>},
        {RTTI_OF(std::vector<int>),         addValue<std::vector<int> >},
        {RTTI_OF(std::vector<float>),       addValue<std::vector<float> >},
        {RTTI_OF(std::vector<bool>),        addValue<std::vector<bool> >},
        {RTTI_OF(std::vector<std::string>), addValue<std::vector<std::string> >},
        {RTTI_OF(std::vector<double>),      addValue<std::vector<double> >},
        {RTTI_OF(std::vector<long>),        addValue<std::vector<long> >},
    };


    static std::vector<std::unique_ptr<APIBaseValue>> copyParams(const std::vector<std::unique_ptr<APIBaseValue>> &params)
    {
        std::vector<std::unique_ptr<APIBaseValue>> copy;
        for(auto &param : params)
        {
            if(copyValueMap.find(param->getRepresentedType()) == copyValueMap.end())
            {
                nap::Logger::warn("Unsupported type for parameter copy: " + param->getRepresentedType().get_name().to_string());
                continue;
            }
            copyValueMap[param->getRepresentedType()](copy, *param);
        }
        return std::move(copy);
    }


    template<typename T>
    static void addValue(std::vector<std::unique_ptr<APIBaseValue>>& copy, APIBaseValue& value)
    {
        copy.emplace_back(std::make_unique<APIValue<T>>(*static_cast<APIValue<T>*>(&value)));
    }


    template<typename T>
    static std::string toHTTPParam(const APIValue<T>& value)
    {
        return std::to_string(value.mValue);
    }


    template<>
    std::string toHTTPParam<std::string>(const APIValue<std::string>& value)
    {
        return value.mValue;
    }


    template<typename T>
    std::string toHTTPParamArray(const APIValue<std::vector<T>>& value, const std::string& arraySeparator)
    {
        std::string result;
        for(const auto& val : value.mValue)
        {
            if(!result.empty())
            {
                result += arraySeparator;
            }
            result += std::to_string(val);
        }
        return result;
    }


    template<>
    std::string toHTTPParamArray<std::string>(const APIValue<std::vector<std::string>>& value, const std::string& arraySeparator)
    {
        std::string result;
        for(const auto& val : value.mValue)
        {
            if(!result.empty())
            {
                result += arraySeparator;
            }
            result += val;
        }
        return result;
    }


    static std::unordered_map<rtti::TypeInfo, std::function<std::string(const APIBaseValue&)>> toHTTPParamMap =
    {
        {RTTI_OF(int),          [](const APIBaseValue& value) { return toHTTPParam(*static_cast<const APIValue<int>*>(&value)); }},
        {RTTI_OF(float),        [](const APIBaseValue& value) { return toHTTPParam(*static_cast<const APIValue<float>*>(&value)); }},
        {RTTI_OF(bool),         [](const APIBaseValue& value) { return toHTTPParam(*static_cast<const APIValue<bool>*>(&value)); }},
        {RTTI_OF(std::string),  [](const APIBaseValue& value) { return toHTTPParam(*static_cast<const APIValue<std::string>*>(&value)); }},
        {RTTI_OF(double),       [](const APIBaseValue& value) { return toHTTPParam(*static_cast<const APIValue<double>*>(&value)); }},
        {RTTI_OF(long),         [](const APIBaseValue& value) { return toHTTPParam(*static_cast<const APIValue<long>*>(&value)); }},
    };

    static std::unordered_map<rtti::TypeInfo, std::function<std::string(const APIBaseValue&, const std::string&)>> toHTTPParamArrayMap =
    {
        {RTTI_OF(std::vector<int>),         [](const APIBaseValue& value, const std::string& arraySeparator) { return toHTTPParamArray(*static_cast<const APIValue<std::vector<int> >*>(&value), arraySeparator); }},
        {RTTI_OF(std::vector<float>),       [](const APIBaseValue& value, const std::string& arraySeparator) { return toHTTPParamArray(*static_cast<const APIValue<std::vector<float> >*>(&value), arraySeparator); }},
        {RTTI_OF(std::vector<bool>),        [](const APIBaseValue& value, const std::string& arraySeparator) { return toHTTPParamArray(*static_cast<const APIValue<std::vector<bool> >*>(&value), arraySeparator); }},
        {RTTI_OF(std::vector<std::string>), [](const APIBaseValue& value, const std::string& arraySeparator) { return toHTTPParamArray(*static_cast<const APIValue<std::vector<std::string> >*>(&value), arraySeparator); }},
        {RTTI_OF(std::vector<double>),      [](const APIBaseValue& value, const std::string& arraySeparator) { return toHTTPParamArray(*static_cast<const APIValue<std::vector<double> >*>(&value), arraySeparator); }},
        {RTTI_OF(std::vector<long>),        [](const APIBaseValue& value, const std::string& arraySeparator) { return toHTTPParamArray(*static_cast<const APIValue<std::vector<long> >*>(&value), arraySeparator); }},
    };


    static httplib::Params toHTTPParams(const std::vector<std::unique_ptr<APIBaseValue>> &params, const std::string& arraySeperator)
    {
        httplib::Params http_params;
        for(const auto& param : params)
        {
            if(toHTTPParamMap.find(param->getRepresentedType()) != toHTTPParamMap.end())
            {
                http_params.emplace(param->mName, toHTTPParamMap[param->getRepresentedType()](*param));
                continue;
            }

            if(toHTTPParamArrayMap.find(param->getRepresentedType()) != toHTTPParamArrayMap.end())
            {
                http_params.emplace(param->mName, toHTTPParamArrayMap[param->getRepresentedType()](*param, arraySeperator));
                continue;
            }

            nap::Logger::warn("Unsupported type for parameter conversion: " + param->getRepresentedType().get_name().to_string());
        }
        return http_params;
    }


    static httplib::Headers toHTTPHeaders(const std::vector<RestHeader>& headers)
    {
        httplib::Headers http_headers;
        for(const auto& header : headers)
        {
            http_headers.emplace(header.key, header.value);
        }
        return http_headers;
    }
}
