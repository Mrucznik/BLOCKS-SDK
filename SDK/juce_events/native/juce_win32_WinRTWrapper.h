/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

   -----------------------------------------------------------------------------

   To release a closed-source product which uses other parts of JUCE not
   licensed under the ISC terms, commercial licenses are available: visit
   www.juce.com for more information.

  ==============================================================================
*/

#pragma once

class WinRTWrapper :   public DeletedAtShutdown
{
public:
    juce_DeclareSingleton_SingleThreaded (WinRTWrapper, true)

    class ScopedHString
    {
    public:
        ScopedHString (String str)
        {
            if (WinRTWrapper::getInstance()->isInitialised())
                WinRTWrapper::getInstance()->createHString (str.toWideCharPointer(),
                                                            static_cast<uint32_t> (str.length()),
                                                            &hstr);
        }

        ~ScopedHString()
        {
            if (WinRTWrapper::getInstance()->isInitialised() && hstr != nullptr)
                WinRTWrapper::getInstance()->deleteHString (hstr);
        }

        HSTRING get() const noexcept
        {
            return hstr;
        }

    private:
        HSTRING hstr = nullptr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopedHString)
    };

    ~WinRTWrapper()
    {
        if (winRTHandle != nullptr)
            ::FreeLibrary (winRTHandle);
    }

    String hStringToString (HSTRING hstr)
    {
        const wchar_t* str = nullptr;
        if (isInitialised())
        {
            str = getHStringRawBuffer (hstr, nullptr);
            if (str != nullptr)
                return String (str);
        }

        return {};
    }

    bool isInitialised() const noexcept
    {
        return initialised;
    }

    template <class ComClass>
    ComSmartPtr<ComClass> getWRLFactory (const wchar_t* runtimeClassID)
    {
        ComSmartPtr<ComClass> comPtr;

        if (isInitialised())
        {
            ScopedHString classID (runtimeClassID);
            if (classID.get() != nullptr)
                roGetActivationFactory (classID.get(), __uuidof (ComClass), (void**) comPtr.resetAndGetPointerAddress());
        }

        return comPtr;
    }

private:
    WinRTWrapper()
    {
        winRTHandle = ::LoadLibraryA ("api-ms-win-core-winrt-l1-1-0");
        if (winRTHandle == nullptr)
            return;

        roInitialize           = (RoInitializeFuncPtr)              ::GetProcAddress (winRTHandle, "RoInitialize");
        createHString          = (WindowsCreateStringFuncPtr)       ::GetProcAddress (winRTHandle, "WindowsCreateString");
        deleteHString          = (WindowsDeleteStringFuncPtr)       ::GetProcAddress (winRTHandle, "WindowsDeleteString");
        getHStringRawBuffer    = (WindowsGetStringRawBufferFuncPtr) ::GetProcAddress (winRTHandle, "WindowsGetStringRawBuffer");
        roGetActivationFactory = (RoGetActivationFactoryFuncPtr)    ::GetProcAddress (winRTHandle, "RoGetActivationFactory");

        if (roInitialize == nullptr || createHString == nullptr || deleteHString == nullptr
         || getHStringRawBuffer == nullptr || roGetActivationFactory == nullptr)
            return;

        HRESULT status = roInitialize (1);
        initialised = ! (status != S_OK && status != S_FALSE && status != 0x80010106L);
    }

    HMODULE winRTHandle = nullptr;
    bool initialised = false;

    typedef HRESULT (WINAPI* RoInitializeFuncPtr) (int);
    typedef HRESULT (WINAPI* WindowsCreateStringFuncPtr) (LPCWSTR, UINT32, HSTRING*);
    typedef HRESULT (WINAPI* WindowsDeleteStringFuncPtr) (HSTRING);
    typedef PCWSTR  (WINAPI* WindowsGetStringRawBufferFuncPtr) (HSTRING, UINT32*);
    typedef HRESULT (WINAPI* RoGetActivationFactoryFuncPtr) (HSTRING, REFIID, void**);

    RoInitializeFuncPtr roInitialize = nullptr;
    WindowsCreateStringFuncPtr createHString = nullptr;
    WindowsDeleteStringFuncPtr deleteHString = nullptr;
    WindowsGetStringRawBufferFuncPtr getHStringRawBuffer = nullptr;
    RoGetActivationFactoryFuncPtr roGetActivationFactory = nullptr;
};
