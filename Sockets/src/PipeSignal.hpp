#pragma once

#include <memory>
#include <string>

namespace Sockets {

    class PipeSignal {
    public:
        PipeSignal();
        bool Initialize();
        void Set();
        void Clear();
        bool IsSet() const;
        int GetSelectHandle() const;
    private:
        struct Impl;
        std::shared_ptr< Impl > impl_;
    };

}
