#include "qcffmpegplayecommons.h"

#ifdef Q_OS_WIN
    #include "dshow.h"
    #include "atlbase.h"
    #include "windows.h"
    #define __T(x)      L ## x
    #pragma comment(lib, "strmiids.lib")
    void DeviceNamesGet(QVector<Interface> &QVInterfaces) {
        QVInterfaces.clear();
        ICreateDevEnum *pICreateDevEnum= nullptr;
        IEnumMoniker *pIEnumMoniker= nullptr;
        IMoniker *pMoniker= nullptr;
        HRESULT Result= CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(Result)) {
            Result= CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, reinterpret_cast<void**>(&pICreateDevEnum));
            if (SUCCEEDED(Result)) {
                Result= pICreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pIEnumMoniker, 0);
                if (SUCCEEDED(Result) && pIEnumMoniker) {
                    while (pIEnumMoniker->Next(1, &pMoniker, NULL)== S_OK) {
                        IPropertyBag* pPropBag;
                        Result= pMoniker->BindToStorage(NULL, NULL, IID_IPropertyBag, reinterpret_cast<void**>(&pPropBag));
                        if (SUCCEEDED(Result)) {
                            VARIANT varName, varPath;
                            VariantInit(&varName);
                            VariantInit(&varPath);
                            Result= pPropBag->Read(L"FriendlyName", &varName, 0);
                            if (SUCCEEDED(Result)) {
                                pPropBag->Read(L"DevicePath", &varPath, 0);
                                std::wstring wsName(varName.bstrVal);
                                std::wstring wsPath(varPath.bstrVal);
                                std::string deviceName(wsName.begin(), wsName.end());
                                std::string devicePath(wsPath.begin(), wsPath.end());
                                Interface aInterface;
                                aInterface.index= QVInterfaces.length();
                                aInterface.Name= QString::fromStdString(deviceName).trimmed();
                                aInterface.Path= QString::fromStdString(deviceName).trimmed();
                                aInterface.Version= QString::fromStdString(devicePath).trimmed();
                                QVInterfaces.append(aInterface);
                                VariantClear(&varName);
                                VariantClear(&varPath);
                            }
                            pPropBag->Release();
                        }
                        pMoniker->Release();
                    }
                }
            }
            if (pIEnumMoniker) pIEnumMoniker->Release();
            if (pICreateDevEnum) pICreateDevEnum->Release();
            CoUninitialize();
        }
    }
    bool Enumerate(WORD index, QString &Name, QString &Version) {
        typedef BOOL WINAPI capGetDriverDescription(WORD wDriverIndex, LPTSTR lpszName, INT cbName, LPTSTR lpszVer, INT cbVer);
        capGetDriverDescription* capGetDriverDescriptionFunction= 0;
        HINSTANCE Avicap32Dll= 0;
        Avicap32Dll= LoadLibraryW(__T("avicap32.dll"));
        if (Avicap32Dll) {
            capGetDriverDescriptionFunction= (capGetDriverDescription*)GetProcAddress(Avicap32Dll, "capGetDriverDescriptionA");
            if (capGetDriverDescriptionFunction!= 0) {
                char lpszName[80];
                char lpszVer[80];
                if (capGetDriverDescriptionFunction(index, LPTSTR(lpszName), 80, LPTSTR(lpszVer), 80)) {
                    Name= QString(lpszName);
                    Version= QString(lpszVer);
                    FreeLibrary(Avicap32Dll);
                    return true;
                }
            }
            FreeLibrary(Avicap32Dll);
        }
        return false;
    }
#endif
#ifdef Q_OS_LINUX
    #include <fcntl.h>
    #include <linux/v4l2-subdev.h>
    #include <sys/ioctl.h>
    #include <unistd.h>
    bool Enumerate(int index, QString &Name, QString &Version) {
        int Handle= open(QString("/dev/video"+ QString::number(index)).toLatin1(), O_RDONLY);
        if (Handle> 0) {
            v4l2_capability Capability;
            if (ioctl(Handle, VIDIOC_QUERYCAP, &Capability)== 0) {
                Name= QString((char*)Capability.card);
                Version= QString::number(Capability.version);
            }
            close(Handle);
            return true;
        }
        return false;
    }
#endif
#ifdef Q_OS_MAC
    #include <CoreMediaIO/CMIOHardware.h>
    void DeviceNamesGet(QVector<Interface> &QVInterfaces) {
        CMIOObjectID ObjectID= kCMIOObjectSystemObject;
        CMIOObjectPropertyAddress Address;
        Address.mSelector= kCMIOHardwarePropertyDevices;
        Address.mScope= kCMIOObjectPropertyScopeGlobal;
        Address.mElement= kCMIOObjectPropertyElementMaster;
        UInt32 Size= 0;
        OSStatus Result= CMIOObjectGetPropertyDataSize(ObjectID, &Address, 0, nullptr, &Size);
        if (Result== noErr) {
            UInt32 deviceCount= Size / sizeof(CMIOObjectID);
            std::vector<CMIOObjectID> devices(deviceCount);
            Result= CMIOObjectGetPropertyData(ObjectID, &Address, 0, nullptr, Size, &Size, devices.data());
            if (Result== noErr) {
                Address.mSelector= kCMIOObjectPropertyName;
                Address.mScope= kCMIOObjectPropertyScopeGlobal;
                Address.mElement= kCMIOObjectPropertyElementMaster;
                for (CMIOObjectID devId : devices) {
                    CFStringRef deviceName= nullptr;
                    Size= sizeof(CFStringRef);
                    Result= CMIOObjectGetPropertyData(devId, &Address, 0, nullptr, Size, &Size, &deviceName);
                    if (Result== noErr && deviceName) {
                        Interface aInterface;
                        aInterface.index= QVInterfaces.length();
                        aInterface.Name= QString::fromCFString(deviceName);
                        aInterface.Path= QString::fromCFString(deviceName);
                        QVInterfaces.append(aInterface);
                        CFRelease(deviceName);
                    }
                }
            }
        }
    }
#endif

bool InterfacesList(QVector<Interface> &QVInterfaces) {
    #ifdef Q_OS_LINUX
        for (int count= 0; count< 99; count++) {
            if (QFileInfo::exists("/dev/video"+ QString::number(count))) {
                QString Name, Version;
                if (Enumerate(count, Name, Version)) {
                    Interface aInterface;
                    aInterface.index= count;
                    aInterface.Name= Name;
                    aInterface.Path= "/dev/video"+ QString::number(count);
                    aInterface.Version= Version;
                    QVInterfaces.append(aInterface);
                }
            } else break;
        }
    #endif
    #ifdef Q_OS_WIN
        DeviceNamesGet(QVInterfaces);
    #endif
    #ifdef Q_OS_MAC
        DeviceNamesGet(QVInterfaces);
    #endif
    return true;
}
