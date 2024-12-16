// Local Includes
#include "restservice.h"

// External Includes
#include <nap/core.h>
#include <nap/resourcemanager.h>
#include <nap/logger.h>
#include <iostream>

#include "restserver.h"
#include "restclient.h"

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::RestService)
	RTTI_CONSTRUCTOR(nap::ServiceConfiguration*)
RTTI_END_CLASS

namespace nap
{
    //////////////////////////////////////////////////////////////////////////
    //// RestService
    //////////////////////////////////////////////////////////////////////////

    void RestService::registerObjectCreators(rtti::Factory &factory)
    {
        factory.addObjectCreator(std::make_unique<RestServerObjectCreator>(*this));
        factory.addObjectCreator(std::make_unique<RestClientObjectCreator>(*this));
    }


    void RestService::update(double deltaTime)
    {
        for(auto client : mClients)
        {
            client->update(deltaTime);
        }
    }


    void RestService::registerRestClient(RestClient &client)
    {
        mClients.emplace_back(&client);
    }


    void RestService::removeRestClient(RestClient &client)
    {
        auto it = std::find(mClients.begin(), mClients.end(), &client);
        if(it != mClients.end())
        {
            mClients.erase(it);
        }
    }
}
