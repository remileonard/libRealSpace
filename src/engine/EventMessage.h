#pragma once

#include "Message.h"

class EventMessage : public Message {
public:
    virtual ~EventMessage() = default;
};