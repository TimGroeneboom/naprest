#include "restserver.h"
#include "httplibwrapper.h"
#include "restutils.h"

#include <nap/logger.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::RestServer)
    RTTI_CONSTRUCTOR(nap::RestService&)
    RTTI_PROPERTY("Functions", &nap::RestServer::mRestFunctions, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Port", &nap::RestServer::mPort, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Host", &nap::RestServer::mHost, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Verbose", &nap::RestServer::mVerbose, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("MaxConcurrentRequests", &nap::RestServer::mMaxConcurrentRequests, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{
    ////////////////////////////////////////////////////////////////////////////
    //// Utility functions forwarded declarations
    ////////////////////////////////////////////////////////////////////////////

    template<typename T>
    T extractValue(const std::string& value_str);

    template<typename T>
    static std::unique_ptr<APIBaseValue> createValue(const std::string& name, const std::string& value_str);

    static std::unordered_map<rtti::TypeInfo, std::function<std::unique_ptr<APIBaseValue>(const std::string&, const std::string&)>> sValueCreators =
    {
        {RTTI_OF(int),          createValue<int>},
        {RTTI_OF(float),        createValue<float>},
        {RTTI_OF(std::string),  createValue<std::string>},
        {RTTI_OF(bool),         createValue<bool>},
        {RTTI_OF(double),       createValue<double>},
        {RTTI_OF(long),         createValue<long>}
    };

    ////////////////////////////////////////////////////////////////////////////
    //// RestServer::Impl
    ////////////////////////////////////////////////////////////////////////////

    struct RestServer::Impl
    {
        httplib::Server mServer;
    };

    ////////////////////////////////////////////////////////////////////////////
    //// RestServer
    ////////////////////////////////////////////////////////////////////////////

    RestServer::RestServer(RestService& service) : Device(), mService(service)
    {
    }


    RestServer::~RestServer(){}


    bool RestServer::init(nap::utility::ErrorState& errorState)
    {
        mImpl = std::make_unique<Impl>();

        int maxConcurrentRequest = mMaxConcurrentRequests;
        if (maxConcurrentRequest > 0)
            mImpl->mServer.new_task_queue = [maxConcurrentRequest] { return new httplib::ThreadPool(maxConcurrentRequest); };

        return true;
    }


    bool RestServer::start(nap::utility::ErrorState& errorState)
    {
        if(!mRunning.load())
        {
            mRunning.store(true);
            std::vector<RestFunction*> calls;
            for(auto c : mRestFunctions)
            {
                calls.emplace_back(c.get());
            }

            mThread = std::thread(&RestServer::run, this, calls, mHost, mPort, mVerbose);
        }

        return true;
    }


    void RestServer::stop()
    {
        if(mRunning.load())
        {
            mRunning.store(false);
            mImpl->mServer.stop();
            mThread.join();
        }
    }


    void RestServer::onDestroy()
    {
        stop();
    }


    void RestServer::run(const std::vector<RestFunction*>& functions,
                         const std::string& host, int port, bool verbose)
    {
        if(verbose)
        {
            mImpl->mServer.set_logger([this](const httplib::Request& req, const httplib::Response& res)
                                      {
                                          nap::Logger::info(*this, "%s %s %i", req.method.c_str(), req.path.c_str(), res.status);
                                      });
        }

        mImpl->mServer.set_error_handler([](const httplib::Request& req, httplib::Response& res)
                                         {
                                             const auto response = utility::generateErrorResponse("Not Found");
                                             res.set_content(response.mData, rest::contenttypes::json);
                                         });


        for(auto& function : functions)
        {
            // Add function callback to server
            mImpl->mServer.Get(function->mAddress, [this, &function](const httplib::Request& req, httplib::Response& res)
            {
                // Create map of values
                std::unordered_map<std::string, std::unique_ptr<APIBaseValue>> values;

                // Create the response object
                RestResponse response = { "", rest::contenttypes::text};

                // Check if all required values are present
                bool valid_params = true;

                // Extract values from request and add to map
                for(auto& val_description : function->mValueDescriptions)
                {
                    // Check if the value is present
                    if(req.has_param(val_description->mName))
                    {
                        // Try to extract the value
                        auto val_str = req.get_param_value(val_description->mName);
                        if(sValueCreators.find(val_description->getRepresentedType()) == sValueCreators.end())
                        {
                            nap::Logger::warn(*this, utility::stringFormat("Unsupported value type: %s, ignoring", val_description->getRepresentedType().get_name().to_string().c_str()));
                            continue;
                        }

                        // Create the value and add it to the map
                        values.emplace(val_description->mName, sValueCreators[val_description->getRepresentedType()](val_description->mName, val_str));
                    }else
                    {
                        // If the value is required, return a bad request
                        if(val_description->mRequired)
                        {
                            response = utility::generateErrorResponse(utility::stringFormat("Error : Missing required parameter %s", val_description->mName.c_str()));
                            valid_params = false;
                            break;
                        }
                    }
                }

                // Call the function, get the response data
                if(valid_params)
                    response = function->call(values);

                // Serve the response
                res.set_content(response.mData, response.mContentType);
            });
        }

        mImpl->mServer.listen(host, port);
    }

    ////////////////////////////////////////////////////////////////////////////
    //// Utility functions
    ////////////////////////////////////////////////////////////////////////////

    template<typename T>
    T extractValue(const std::string& value_str)
    {
        T value;
        std::istringstream(value_str) >> value;
        return value;
    }


    template<typename T>
    static std::unique_ptr<APIBaseValue> createValue(const std::string& name, const std::string& value_str)
    {
        T value = extractValue<T>(value_str);
        return std::make_unique<APIValue<T>>(name, value);
    }
}