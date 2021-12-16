/*************************************************************************/
/*  breakpad.cpp                                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "breakpad.h"

#include "core/bind/core_bind.h"
#include "core/os/os.h"

#ifdef _WIN32
#include <thirdparty/breakpad/src/client/windows/handler/exception_handler.h>
#else
#include <thirdparty/breakpad/src/client/linux/handler/exception_handler.h>
#endif

static google_breakpad::ExceptionHandler *breakpad_handler = nullptr;

#ifdef _WIN32
static bool dump_callback(const wchar_t* dump_path, const wchar_t* minidump_id, void* context,
   EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded) {
   wprintf(L"Crash dump created at: %s/%s.dmp\n", dump_path, minidump_id);
   fwprintf(stderr, L"Crash dump created at: %s/%s.dmp\n", dump_path, minidump_id);
#else
static bool dump_callback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) {
    printf("Crash dump created at: %s\n", descriptor.path());
#endif
    return succeeded;
}

void initialize_breakpad(bool register_handlers) {
    if(breakpad_handler != nullptr) {
        // TODO: error?
        return;
    }

#ifdef _WIN32
    String crash_folder;

    wchar_t tempPath[MAX_PATH + 1];

    if(GetTempPathW(MAX_PATH + 1, tempPath) < 1){
        crash_folder = tempPath;
    } else {
        crash_folder = L"C:/temp";
    }

    // Automatic register to the exception handlers can be disabled when Godot crash handler listens to them
    breakpad_handler = new google_breakpad::ExceptionHandler(crash_folder.c_str(), nullptr, dump_callback, nullptr,
        register_handlers ? google_breakpad::ExceptionHandler::HANDLER_ALL : google_breakpad::ExceptionHandler::HANDLER_NONE);
#else
    google_breakpad::MinidumpDescriptor descriptor("/tmp");

    breakpad_handler = new google_breakpad::ExceptionHandler(descriptor, nullptr, dump_callback, nullptr, register_handlers, -1);
#endif
}

void disable_breakpad() {
    if(breakpad_handler == nullptr)
        return;

    delete breakpad_handler;
    breakpad_handler = nullptr;
}

void report_user_data_dir_usable() {
    if(breakpad_handler == nullptr)
        return;

    String crash_folder = OS::get_singleton()->get_user_data_dir() + "/crashes";
	_Directory dir;
	if (!dir.dir_exists(crash_folder)) {
        dir.make_dir_recursive(crash_folder);
	}

#ifdef _WIN32
    breakpad_handler->set_dump_path(crash_folder.c_str());
#else
    google_breakpad::MinidumpDescriptor descriptor(crash_folder.utf8().get_data());

    breakpad_handler->set_minidump_descriptor(descriptor);
#endif
}

void breakpad_handle_signal(int signal) {
    if(breakpad_handler == nullptr)
        return;

#ifndef _WIN32
    // TODO: Should this use HandleSignal(int sig, siginfo_t* info, void* uc) instead?
    // would require changing to sigaction in crash_handler_x11.cpp
    breakpad_handler->SimulateSignalDelivery(signal);
#endif
}

void breakpad_handle_exception_pointers(void *exinfo) {
    if(breakpad_handler == nullptr)
        return;

#ifdef _WIN32
    breakpad_handler->WriteMinidumpForException(static_cast<EXCEPTION_POINTERS*>(exinfo));
#endif
}
