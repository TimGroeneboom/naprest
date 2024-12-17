#pragma once

#include <nap/device.h>
#include <atomic>
#include <thread>

#include "restresponse.h"
#include "concurrentqueue.h"
#include <apivalue.h>
#include "utility/autoresetevent.h"

namespace nap
{
    // Forward declarations
    class RestService;

    /**
     * Represents a header for a request
     */
    struct NAPAPI RestHeader
    {
        std::string key; ///< Property : 'Key' The key of the header
        std::string value; ///< Property : 'Value' The value of the header
    };

    /**
     * A RestClient can make an http request.
     * It has its own worker thread to handle requests. Requests will be put into a queue as long as the previous request has not returned
     * Success and Error callbacks will always be called on the main thread from the RestService update loop
     */
    class NAPAPI RestClient final : public Device
    {
        friend class RestService;
    RTTI_ENABLE(Device)
    public:
        /**
         * Constructor
         * @param service reference to the rest service
         */
        RestClient(RestService& service);

        // default destructor
        ~RestClient();

        /**
         * Initializes the client
         * @param errorState contains the error state
         * @return true on success
         */
        bool init(nap::utility::ErrorState& errorState) final;

        /**
         * Starts the worker thread
         * Registers the client with the service
         * @param errorState contains the error state
         * return true on success
         */
        bool start(nap::utility::ErrorState& errorState) final;

        /**
         * Stops the worker thread
         * Removes the client from the service
         */
        void stop() final;

        /**
         * Sends a get request, non-blocking, callbacks are executed on the main thread during the update loop of RestService
         * @param address the address to send the request to
         * @param params the parameters to send with the request
         * @param onSuccess on success callback
         * @param onError on error callback
         */
        void get(const std::string& address,
                 const std::vector<std::unique_ptr<APIBaseValue>>& params,
                 std::function<void(const RestResponse& response)> onSuccess,
                 std::function<void(const utility::ErrorState&)> onError);

        /**
         * Sends a blocking get request
         * @param address the address to send the request to
         * @param params the parameters to send with the request
         * @param response response object to fill with the response
         * @param errorState contains the error state
         * @return true on success
         */
        bool getBlocking(const std::string& address,
                         const std::vector<std::unique_ptr<APIBaseValue>>& params,
                         RestResponse& response,
                         utility::ErrorState& errorState);

        // Properties
        std::string mURL; ///< Property : 'URL' The URL to which the client sends requests
        std::string mCertPath = "/etc/ssl/certs"; ///< Property : 'CertPath' The path to the certificate file
        std::string mArraySeparator = ","; ///< Property : 'ArraySeparator' The separator used for arrays in the URL
        std::vector<RestHeader> mHeaders = {{"User-Agent", "NAP/1.0"}}; ///< Property : 'Headers' The headers to send with the request
        int mTimeOutSeconds = 0; ///< Property : 'TimeOutSeconds' The timeout in seconds for the request

        // Signals
        // Progress signal is dispatched on main thread when progress is reported, first int is bytes received second int is total bytes to receive
        Signal<uint64_t, uint64_t> mProgressSignal;
    private:
        // Called by the service
        void update(double deltaTime);

        // Threading
        std::atomic_bool mRunning = {false};
        void run();

        std::thread mThread;
        moodycamel::ConcurrentQueue<std::function<void()>> mCallbackQueue;
        moodycamel::ConcurrentQueue<std::function<void()>> mRequestQueue;
        utility::AutoResetEvent mRequestEvent;

        // httplib implementation
        struct Impl;
        std::unique_ptr<Impl> mImpl;

        // rest service reference
        RestService& mService;

        std::unordered_map<std::string, std::vector<std::unique_ptr<APIBaseValue>>> mRequestParamsCache;
    };

    using RestClientObjectCreator = rtti::ObjectCreator<RestClient, RestService>;
}