#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>

enum class EventSource {
    Unknown,
    RF,
    IR,
    Kaku,
    Storage,
    WebUI,
    VirtualColor
};

inline const char* toString(EventSource source) {
    switch (source) {
        case EventSource::RF: return "RF";
        case EventSource::IR: return "IR";
        case EventSource::Kaku: return "KAKU";
        case EventSource::Storage: return "STORAGE";
        case EventSource::WebUI: return "WEBUI";
        case EventSource::VirtualColor: return "VIRTUAL_COLOR";
        case EventSource::Unknown:
        default: return "UNKNOWN";
    }
}

// The system is intentionally event-driven:
// - radio/IR/storage inputs publish a SystemEvent
// - ScriptManager matches it against triggers and executes actions
// - the Core layer mainly logs and coordinates startup wiring
// This design keeps the runtime logic understandable and easy to extend.
struct SystemEvent {
    String source;      // Legacy string form kept for compatibility with existing code.
    String identifier;  // e.g., "HOUSE_A_BTN_1", "RED", "SCENE_PURPLE"
    String rawData;     // e.g., "0xFF1AE5" (optional, for debugging)
    EventSource sourceType = EventSource::Unknown;

    bool hasSource(EventSource expected) const {
        return sourceType == expected || source.equalsIgnoreCase(toString(expected));
    }
};

// Callback type for anyone listening to events
typedef std::function<void(const SystemEvent&)> EventCallback;

class EventBus {
public:
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    // Subscribe to ALL events
    void subscribe(EventCallback callback) {
        _listeners.push_back(callback);
    }

    // Publish an event to everyone
    void publish(const SystemEvent& event) {
        for (auto& listener : _listeners) {
            listener(event);
        }
    }

private:
    EventBus() {} // Singleton
    std::vector<EventCallback> _listeners;
};   