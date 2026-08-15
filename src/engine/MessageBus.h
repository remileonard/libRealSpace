#include <cstdint>
#include <functional>
#include <vector>
#include <mutex>
#include <memory>
#include <queue>

#include "InputMessage.h"
#include "EventMessage.h"



class MessageBus {
public:
    using SubscriptionId = std::uint64_t;
    using InputCallback = std::function<void(const InputMessage&)>;
    using EventCallback = std::function<void(const EventMessage&)>;

private:
    MessageBus() = default;
    ~MessageBus() = default;
    struct InputSubscription {
        SubscriptionId id;
        InputCallback callback;
    };

    struct EventSubscription {
        SubscriptionId id;
        EventCallback callback;
    };

    std::vector<InputSubscription> input_callbacks;
    std::vector<EventSubscription> event_callbacks;
    SubscriptionId next_subscription_id = 1;
    std::mutex subscriptions_mutex;

    std::queue<std::unique_ptr<InputMessage>> input_messages;
    std::queue<std::unique_ptr<EventMessage>> event_messages;
    std::mutex messages_mutex;

public:

    SubscriptionId subscribe(InputCallback callback);
    SubscriptionId subscribe(EventCallback callback);
    void unsubscribe(SubscriptionId id);

    void publish(std::unique_ptr<InputMessage> message);
    void publish(std::unique_ptr<EventMessage> message);

    void processInput();
    void processEvents();

    static MessageBus& instance();

    MessageBus(const MessageBus&) = delete;
    MessageBus& operator=(const MessageBus&) = delete;

};