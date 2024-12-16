#pragma once

// External Includes
#include <nap/service.h>

namespace nap
{
    // forward declarations
    class RestClient;

	class NAPAPI RestService : public Service
	{
        friend class RestClient;

		RTTI_ENABLE(Service)
	public:
		// Default Constructor
		RestService(ServiceConfiguration* configuration) : Service(configuration)	{ }

        /**
         * Register object creators for the service
         * @param factory the factory to register the object creators to
         */
        void registerObjectCreators(rtti::Factory &factory) override;

        void update(double deltaTime) override;
    private:
        void registerRestClient(RestClient& client);

        void removeRestClient(RestClient& client);

        std::vector<RestClient*> mClients;
	};
}
