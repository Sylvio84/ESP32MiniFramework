#include <Managers/ConfigurationManager.h>
#include <Devices/RelayDevice.h>
#include <Managers/EventManager.h>

RelayDevice::RelayDevice(String id, FrameworkContext& ctx) : OnOffDevice(id, ctx)
{
    name = "Relay";
    pin = 12;  // Default pin for relay
    invertedLogic = false;  // Default: normal logic (can be changed)
    debug("RelayDevice constructed with default pin " + String(pin), 1);
}

void RelayDevice::init()
{
    // Load pin configuration using new system
    pin = loadPin("pin", pin);
    
    // Register pin for listing
    registerPin(pin, "output", "Relay control pin");
    
    // Try to load other configuration
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (configMgr) {
        // Load inverted logic setting from configuration
        bool configInverted = configMgr->getPreference(id + "_inverted", invertedLogic);
        if (configInverted != invertedLogic) {
            invertedLogic = configInverted;
            debug("Inverted logic loaded from configuration: " + String(invertedLogic ? "Yes" : "No"), 1);
        }
    }
    
    // Then call parent init
    OnOffDevice::init();
}

void RelayDevice::registerSpecificCommands()
{
    // Register relay-specific command for inverted logic
    registerDeviceCommand("setinverted", "Set inverted logic (0=normal, 1=inverted)", 
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return String("ERROR: Usage: relay:setinverted <0|1>");
            }
            
            bool newInverted = args[0].toInt() != 0;
            setInvertedLogic(newInverted);
            
            // Save to configuration
            auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
            if (configMgr) {
                configMgr->setPreference(id + "_inverted", newInverted);
                debug("Inverted logic saved to configuration", 1);
            } else {
                debug("Could not save inverted logic to configuration", 2);
            }
            
            // Apply the new logic immediately
            writePin(state);
            
            return String("Inverted logic set to: " + String(newInverted ? "Yes" : "No"));
        });
    
    // Additional aliases for relay
    auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
    if (cmdMgr) {
        cmdMgr->registerAlias("relay_on", "relay:on");
        cmdMgr->registerAlias("relay_off", "relay:off");
        cmdMgr->registerAlias("relay_toggle", "relay:toggle");
        cmdMgr->registerAlias("relay_status", "relay:state");
    }
}