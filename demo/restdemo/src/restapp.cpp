// Local Includes
#include "restapp.h"

// External Includes
#include <utility/fileutils.h>
#include <nap/logger.h>
#include <inputrouter.h>
#include <rendergnomoncomponent.h>
#include <perspcameracomponent.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <imgui_internal.h>

namespace nap 
{    
    bool CoreApp::init(utility::ErrorState& error)
    {
		// Retrieve services
		mRenderService	= getCore().getService<nap::RenderService>();
		mSceneService	= getCore().getService<nap::SceneService>();
		mInputService	= getCore().getService<nap::InputService>();
		mGuiService		= getCore().getService<nap::IMGuiService>();

		// Fetch the resource manager
        mResourceManager = getCore().getResourceManager();

		// Get the render window
		mRenderWindow = mResourceManager->findObject<nap::RenderWindow>("Window");
		if (!error.check(mRenderWindow != nullptr, "unable to find render window with name: %s", "Window"))
			return false;

		// Get the scene that contains our entities and components
		mScene = mResourceManager->findObject<Scene>("Scene");
		if (!error.check(mScene != nullptr, "unable to find scene with name: %s", "Scene"))
			return false;

    	// Get the rest client
    	mRestClient = mResourceManager->findObject<RestClient>("RestClient");
    	if (!error.check(mRestClient != nullptr, "unable to find rest client with name: %s", "RestClient"))
    		return false;

		// All done!
        return true;
    }


    // Render app
    void CoreApp::render()
    {
		// Signal the beginning of a new frame, allowing it to be recorded.
		// The system might wait until all commands that were previously associated with the new frame have been processed on the GPU.
		// Multiple frames are in flight at the same time, but if the graphics load is heavy the system might wait here to ensure resources are available.
		mRenderService->beginFrame();

		// Begin recording the render commands for the main render window
		if (mRenderService->beginRecording(*mRenderWindow))
		{
			// Begin render pass
			mRenderWindow->beginRendering();

			// Draw GUI elements
			mGuiService->draw();

			// Stop render pass
			mRenderWindow->endRendering();

			// End recording
			mRenderService->endRecording();
		}

		// Proceed to next frame
		mRenderService->endFrame();
    }


    void CoreApp::windowMessageReceived(WindowEventPtr windowEvent)
    {
		mRenderService->addEvent(std::move(windowEvent));
    }


    void CoreApp::inputMessageReceived(InputEventPtr inputEvent)
    {
		// If we pressed escape, quit the loop
		if (inputEvent->get_type().is_derived_from(RTTI_OF(nap::KeyPressEvent)))
		{
			nap::KeyPressEvent* press_event = static_cast<nap::KeyPressEvent*>(inputEvent.get());
			if (press_event->mKey == nap::EKeyCode::KEY_ESCAPE)
				quit();

			if (press_event->mKey == nap::EKeyCode::KEY_f)
				mRenderWindow->toggleFullscreen();
		}
		mInputService->addEvent(std::move(inputEvent));
    }


    int CoreApp::shutdown()
    {
		return 0;
    }


	// Update app
    void CoreApp::update(double deltaTime)
    {
		// Use a default input router to forward input events (recursively) to all input components in the scene
		// This is explicit because we don't know what entity should handle the events from a specific window.
		nap::DefaultInputRouter input_router(true);
		mInputService->processWindowEvents(*mRenderWindow, input_router, { &mScene->getRootEntity() });

    	// The following windows construct a REST API call
    	if (ImGui::Begin("Rest Client"))
    	{
            const auto& palette = mGuiService->getPalette();
            ImGui::TextColored(palette.mHighlightColor2, "RestClient");
            ImGui::Text("This demonstrates a simple call to a REST API served by this application using a RestClient device");
            ImGui::Text("Alternatively, open the browser and navigate to the URL below to see the API response");
            ImGui::Spacing(); ImGui::Spacing();
            std::string url = utility::stringFormat("%s%s?intValue=%i&floatValue=%.2f&stringValue=%s",
                                                    mRestClient->mURL.c_str(),
                                                    mAddressString.c_str(),
                                                    mIntInput,
                                                    mFloatInput,
                                                    mStringInput.c_str());
    		ImGui::TextColored(palette.mHighlightColor2, "%s", url.c_str());
            if(ImGui::Button("Copy URL To Clipboard"))
                ImGui::SetClipboardText(url.c_str());
            ImGui::Spacing(); ImGui::Spacing();
    		ImGui::InputText("Address", &mAddressString);
    		ImGui::InputText("Text", &mStringInput);
    		ImGui::InputInt("Int", &mIntInput);
    		ImGui::InputFloat("Float", &mFloatInput);
    		if (ImGui::Button("Send"))
    		{
    			std::vector<std::unique_ptr<APIBaseValue>> params;
    			params.emplace_back(std::make_unique<APIInt>("intValue", mIntInput));
    			params.emplace_back(std::make_unique<APIFloat>("floatValue", mFloatInput));
    			params.emplace_back(std::make_unique<APIString>("stringValue", mStringInput));
				mRestClient->get(mAddressString, params, [this](const RestResponse& response)
				{
					mResponseText = response.mData;
				},
				[this](const utility::ErrorState& error)
				{
					nap::Logger::error(error.toString());
				});
    		}
    		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    		ImGui::InputTextMultiline("Response", &mResponseText);
    		ImGui::PopItemFlag();
    	}
    	ImGui::End();
    }
}
