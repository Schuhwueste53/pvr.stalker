/*
 *  Copyright (C) 2015-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 1015, 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <thread>
#include <functional>

#include "SAPI.h"

namespace SC {
    class CWatchdog {
    public:
        CWatchdog(unsigned int interval, SAPI *api, std::function<void(SError)> errorCallback);

        virtual ~CWatchdog();

        virtual void Start();

        virtual void Stop();

    private:
        void Process();

        unsigned int m_interval;
        SAPI *m_api;
        std::function<void(SError)> m_errorCallback;
        bool m_threadActive;
        std::thread m_thread;
    };
}

