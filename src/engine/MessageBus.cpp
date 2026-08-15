#include "MessageBus.h"


MessageBus::SubscriptionId MessageBus::subscribe(InputCallback callback) {
    if (!callback) {
        throw std::invalid_argument("Input callback cannot be empty");
    }
    std::lock_guard<std::mutex> lock(subscriptions_mutex);
    const SubscriptionId id = next_subscription_id++;
    input_callbacks.push_back({id, std::move(callback)});
    return id;
}

MessageBus::SubscriptionId MessageBus::subscribe(EventCallback callback) {
    if (!callback) {
        throw std::invalid_argument("Event callback cannot be empty");
    }
    std::lock_guard<std::mutex> lock(subscriptions_mutex);
    const SubscriptionId id = next_subscription_id++;
    event_callbacks.push_back({id, std::move(callback)});
    return id;
}

void MessageBus::unsubscribe(SubscriptionId id) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex);
    for (auto it = input_callbacks.begin(); it != input_callbacks.end(); ++it) {
        if (it->id == id) {
            input_callbacks.erase(it);
            return;
        }
    }
    for (auto it = event_callbacks.begin(); it != event_callbacks.end(); ++it) {
        if (it->id == id) {
            event_callbacks.erase(it);
            return;
        }
    }
}
void MessageBus::publish(std::unique_ptr<InputMessage> message) {
    if (!message) {
        throw std::invalid_argument("Input message cannot be null");
    }

    std::lock_guard<std::mutex> lock(messages_mutex);
    input_messages.push(std::move(message));
}

void MessageBus::publish(std::unique_ptr<EventMessage> message) {
    if (!message) {
        throw std::invalid_argument("Event message cannot be null");
    }

    std::lock_guard<std::mutex> lock(messages_mutex);
    event_messages.push(std::move(message));
}

void MessageBus::processEvents() {
    std::queue<std::unique_ptr<EventMessage>> messages;
    std::vector<EventSubscription> subscriptions;

    {
        std::lock_guard<std::mutex> lock(messages_mutex);
        messages.swap(event_messages);
    }

    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex);
        subscriptions = event_callbacks;
    }

    while (!messages.empty()) {
        const EventMessage& message = *messages.front();

        for (const EventSubscription& subscription : subscriptions) {
            subscription.callback(message);
        }

        messages.pop();
    }
}

void MessageBus::processInput() {
    std::queue<std::unique_ptr<InputMessage>> messages;
    std::vector<InputSubscription> subscriptions;

    {
        std::lock_guard<std::mutex> lock(messages_mutex);
        messages.swap(input_messages);
    }

    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex);
        subscriptions = input_callbacks;
    }

    while (!messages.empty()) {
        const InputMessage& message = *messages.front();

        for (const InputSubscription& subscription : subscriptions) {
            subscription.callback(message);
        }

        messages.pop();
    }
}
MessageBus& MessageBus::instance() {
    static MessageBus bus;
    return bus;
}