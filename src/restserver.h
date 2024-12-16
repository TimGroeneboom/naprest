#pragma once

#include <nap/resource.h>
#include <nap/resourceptr.h>
#include <apiargument.h>
#include <nap/device.h>
#include <utility/autoresetevent.h>

#include <thread>
#include <atomic>

#include "restservice.h"
#include "restcontenttypes.h"
#include "restresponse.h"
#include "restfunction.h"

namespace nap
{
    /**
     * RestServer is a device that listens for incoming rest calls and routes them to the appropriate RestFunction.
     * Each rest call is a new thread, you can limit the number of concurrent requests and the total number of requests.
     * A RestCall is defined by a RestFunction and a list of RestFunctionValues that can be extracted from the request.
     */
    class NAPAPI RestServer final : public Device
    {
    RTTI_ENABLE(Device)
    public:
        /**
         * Constructor
         * @param service reference to the rest service
         */
        RestServer(RestService& service);

        /**
         * Creates the implementation
         * @param errorState returns the error state
         * @return true on success
         */
        bool init(nap::utility::ErrorState& errorState) final;

        /**
         * Starts the server
         * @param errorState returns the error state
         * @return true on success
         */
        bool start(nap::utility::ErrorState& errorState) final;

        /**
         * Stops the server
         */
        void stop() final;

        /**
         * Destroys the server implementation
         */
        void onDestroy() final;

        std::vector<ResourcePtr<RestFunction>> mRestFunctions; ///< Property : 'RestCalls' The rest calls that are handled by this server
        int mPort = 8080; ///< Property : 'Port' The port on which the server listens
        std::string mHost = "localhost"; ///< Property : 'Host' The host on which the server listens
        bool mVerbose = true; ///< Property : 'Verbose' If the server should be verbose
        int mMaxConcurrentRequests = 0; ///< Property : 'MaxConcurrentRequests' The maximum number of concurrent requests, 0 means unlimited
    private:
        // The main server loop
        std::atomic_bool mRunning = {false};
        void run(const std::vector<RestFunction*>& functions, const std::string& host, int port, bool verbose);
        std::thread mThread;

        // RestService
        RestService& mService;

        // httplib implmentation
        struct Impl;
        std::unique_ptr<Impl> mImpl;

        // Request queue
        std::vector<std::function<void()>> mRequestQueue;
    };

    using RestServerObjectCreator = rtti::ObjectCreator<RestServer, RestService>;
}
