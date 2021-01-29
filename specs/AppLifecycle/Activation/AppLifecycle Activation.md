# AppLifecycle - Rich Activation

Reunion introduces a new AppLifecycle component that provides a core set of
functionality related to app activation and lifecycle. Much of this
functionality is directly equivalent - or very similar - to existing
functionality within the platform. However, there is value in abstracting this
functionality so that all app types can use it, delivering it out-of-band from
the OS, and making it available as open-source. The totality of AppLifecycle
features in v1 is as follows (additional features may be added in later
releases):

- Rich activation objects
- Selective single/multi-instancing
- System state and power notifications.

This spec addresses the rich activation APIs in the component.

## Background

Traditional Win32 apps expect to get their arguments passed into WinMain in the
form of a string array. They can also call the Win32
[GetCommandLineW](https://docs.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-getcommandlinew)
API. Windows Forms apps expect to call
[Environment.GetCommandLineArgs](https://docs.microsoft.com/en-us/dotnet/api/system.environment.getcommandlineargs)
for the same purpose. This is true regardless of the "activation kind" - in
fact, for unpackaged apps, all activations are managed in the same way, only
varying by the arguments passed in to WinMain. Win32 apps are familiar with 3
activation kinds: regular launch (whether by command-line, tile/shortcut or
startup), file-type and protocol association.

Conversely, UWP supports
[44 different kinds of activation](https://docs.microsoft.com/en-us/uwp/api/Windows.ApplicationModel.Activation.ActivationKind),
each represented by a different IActivatedEventArgs class, where the arguments
are passed in to the app's Launched or Activated event handlers, as properties
on a specific XXXActivatedEventArgs object. In addition, multi-instanced UWP and
Desktop Bridge apps can call the
[AppInstance.GetActivatedEventArgs](https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.appinstance.getactivatedeventargs)
API.

Of all the rich activation kinds, some require some form of prior information
registered with the system so that it knows how to activate the target app. In
the case of file-type, protocol and startup activation, Win32 apps can do this
registration by writing regkeys (either in their installer or at runtime).
Packaged (UWP and Desktop Bridge) apps do this via entries in their manifest
which the platform registers during install. Toast activation is unusual in that
the developer requirements are different between UWP and Desktop Bridge. For
UWP, it requires only very minimal "registration" - in that the system must be
told the ID of the app to use for activation. For Desktop Bridge, it does
require manifest declarations.

<br>

## Description

This part of Reunion AppLifecycle makes rich UWP activation functionality
available to non-MSIX-packaged Win32 app types. There are 2 main aspects to
consider:

- Which activation kinds will unpackaged apps be able to use?
- How does the app tell the system which activation kinds it wants to use?

### Supported activation kinds

The first release focuses on the most commonly-used rich activation kinds. The
AppLifecycle component will provide a converged GetActivatedEventArgs API which
will get all args, regardless of activation kind, effectively merging in the
functionality of GetCommandLineArgs/GetCommandLineW. It will also be decoupled
from multi-instancing.

Of the 44 UWP activation kinds, 4 of the most commonly-used kinds are also
supported for Desktop Bridge and Windows
[reg-free WinRT](https://blogs.windows.com/windowsdeveloper/2019/04/30/enhancing-non-packaged-desktop-apps-using-windows-runtime-components/)
apps. The AppLifecycle implementation is based on this set. Some activation
kinds require prior registration, others do not. The target list for v1 is as
follows:

| Activation kind | Description                                                                                                                                      |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| Launch          | Launch from a Start tile/shortcut, from a command window, or programmatically via ShellExecute/CreateProcess.                                    |
| File            | The app is activated when the user double-clicks a registered file, or when a caller calls ShellExecute or LaunchFileAsync on a registered file. |
| Protocol        | Activated via ShellExecute or LaunchUriAsync, or by specifying a protocol string on a command-line.                                              |
| StartupTask     | The app starts on user log-in - either registered in the registry, or via shortcuts in a well-known startup folder.                              |

### Activation registration

Apps can start using supported modern activation phases by:

- Calling a method to register activation.
- Calling a method in their entrypoint to handle activation.

From a Win32 app perspective, the 2 activation kinds that do not require any
registration (Shortcut/Tile Launch and CommandLine) are effectively the same in
terms of activation payload. Indeed, it is not possible to distinguish a launch
activation of a Win32 app from a commandline activation. Therefore, the
AppLifecycle implementation will merge Launch and CommandLine into one (Launch)
activation kind.

UWP apps can only register for activation kinds via manifest declarations, and
therefore this happens during app install. Conversely, traditional app types can
write registry entries at any time.

A key benefit is to enable unpackaged Win32 apps to use the set of rich
activations using familiar designs, without any additional friction.
Specifically, AppLifecycle will provide APIs which the app can call to register
its interest in rich activations. The app is not required to have an MSIX
manifest, nor an MSIX package. Using the API approach rather than the manifest
approach has the added benefit that it also addresses a related customer request
for dynamic activation registrations even for UWP apps.

Currently all registered activation points are managed per-user. If your app is
installed for multiple users via an MSI or elevated installer your app will need
to re-register launch points for every user. For v1, the platform will support
only per-user registration.

 <br>

## Examples

### Register for rich activation

Providing APIs for activation registration means that an app can register at any
time. The most common case would be very early in the app?s main function. The
app can register for any of the supported activation kinds. For most activation
kinds, the app can register multiple options (for example, multiple filetypes or
dataformats, or verbs) - and could therefore call the RegisterXXX function
multiple times, if required. For example, an app could register for ".jpg" and
".bmp" filetype activations at one point, and then later also register for
".png".

Each registered filetype must be unique for the app. To allow for the case where
the app determines the combination of options at runtime (and potentially
changes the combinations at a later time), where an app registers the same
filetype multiple times, the latest registration is honored and overwrites any
previous registrations.

For startup activation, only one of each of these is allowed. If an app attempts
to register either of these more than once, the latest registration is honored
and overwrites any previous registrations.

```c++
int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    // Registering for rich activation kinds can be done in the
    // app's installer or in the app itself.
    RegisterForActivation();

    // When the app starts, it can get its activated eventargs, and perform
    // any required operations based on the activation kind and payload.
    RespondToActivation();

    ///////////////////////////////////////////////////////////////////////////
    // Standard Win32 window configuration/creation and message pump:
    // ie, whatever the app would normally do - nothing new here.
    RegisterClassAndStartMessagePump(hInstance, nCmdShow);
    return 1;
}

void RegisterForActivation()
{
    // Register one or more supported image filetypes, an icon,
    // and zero or more verbs for the File Explorer context menu.
    ActivationRegistrationManager::RegisterForFileTypeActivation(
        new string[]{ ".jpg", ".png", ".bmp" },
        "MyResources.dll",
        12,
        new string[]{ "view", "edit" });

    // Register another set of filetypes, with a different
    // icon, and no specified verbs.
    ActivationRegistrationManager::RegisterForFileTypeActivation(
        new string[]{ ".mov", ".wmv", ".mp3" },
        "MyResources.dll",
        18);

    // Register some URI schemes for protocol activation.
    ActivationRegistrationManager::RegisterForProtocolActivation("foo");
    ActivationRegistrationManager::RegisterForProtocolActivation("com.contoso.list");
    ActivationRegistrationManager::RegisterForProtocolActivation("foo-bar");

    // Register for startup activation.
    ActivationRegistrationManager::RegisterForStartupActivation(
        "MyTaskId", true, "Contoso Photo Viewer");
}
```

### Get rich activation objects

A Win32 app would typically get its command-line arguments very early in its
WinMain, and we therefore expect the app to call the new
AppLifecycle::GetActivatedEventArgs in the same place, instead of using the
supplied lpCmdLine parameter or using GetCommandLineW. From there, the app can
determine the specific ActivationKind, query the object for the specific
XXXActivatedEventArgs type, and then access the properties and methods of that
type.

```c++
void RespondToActivation()
{
    IActivatedEventArgs args = AppLifecycle::GetActivatedEventArgs();

    if (args)
    {
        ActivationKind kind = args.Kind();
        if (kind == ActivationKind::Launch)
        {
            auto launchArgs = args.as<LaunchActivatedEventArgs>();
            DoSomethingWithLaunchArgs(launchArgs.Arguments());
        }
        else if (kind == ActivationKind::File)
        {
            auto fileArgs = args.as<FileActivatedEventArgs>();
            DoSomethingWithFileArgs(fileArgs.Files());
        }
        else if (kind == ActivationKind::Protocol)
        {
            auto protocolArgs = args.as<ProtocolActivatedEventArgs>();
            DoSomethingWithProtocolArgs(protocolArgs.Uri());
        }
        else if (kind == ActivationKind::StartupTask)
        {
            auto startupArgs = args.as<StartupTaskActivatedEventArgs>();
            DoSomethingWithStartupArgs(startupArgs.TaskId());
        }
    }
}
```

### Unregister

The primary requirement is to be able to register for the supported activation
kinds dynamically. A secondary requirement is to be able to unregister for any
or all of these.

For filetype associations, the app can register for multiple filetypes. We will
provide the ability to unregister one or more filetypes. For protocol
registrations, the app can register for multiple protocols individually, so the
API will also enable the app to unregister for each of these individually. For
the other supported activation kinds, there can be at most one registration for
the app. Therefore, the app does not need to pass any parameters in order to
unregister for these activation kinds.

```c++
void UnregisterForActivation()
{
    // Unregister one or more registered filetypes.
    ActivationRegistrationManager::UnregisterForFileTypeActivation(
        new string[]{ ".jpg", ".png", ".bmp" });

    ActivationRegistrationManager::UnregisterForProtocolActivation("foo");
    ActivationRegistrationManager::UnregisterForStartupActivation();
}
```

<br>

## API Details

### Top-level types

This spec proposes 6 new classes and one enum:

- An AppLifecycle class to encapsulate the GetActivatedEventArgs method.
- An XXXActivatedEventArgs class for each activation kind, modeled directly on
  existing XXXActivatedEventArgs classes in the
  Windows.ApplicationModel.Activation namespace.
- The ActivationKind enum, cloned from
  Windows.ApplicationModel.Activation.ActivationKind.

| API                                                                      | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| ------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| AppLifecycle<br> {<br> IActivatedEventArgs GetActivatedEventArgs()<br> } | New class. This is the major new class exposed in the undocked AppLifecycle component. This spec only covers activation aspects here. Additional AppLifecycle properties, methods and events are specified in other specs. GetActivatedEventArgs provides the same behavior as the existing [AppInstance.GetActivatedEventArgs](https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.appinstance.getactivatedeventargs), in that it returns the XXXActivatedEventArgs object for the current activation. The initial release of GetActivatedEventArgs must support the XXXActivatedEventArgs and corresponding ActivationKinds listed below. The Reunion implementation must not require coupling with the UWP multi-instancing behavior. |
| class LaunchActivatedEventArgs                                           | Based on the existing [LaunchActivatedEventArgs](https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.activation.launchactivatedeventargs).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| class FileActivatedEventArgs                                             | Based on the existing [FileActivatedEventArgs](https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.activation.fileactivatedeventargs).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| class ProtocolActivatedEventArgs                                         | Based on the existing [ProtocolActivatedEventArgs](https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.activation.protocolactivatedeventargs).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| class StartupTaskActivatedEventArgs                                      | Based on the existing [StartupTaskActivatedEventArgs](https://docs.microsoft.com/en-us/uwp/api/windows.applicationmodel.activation.startuptaskactivatedeventargs).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| enum ActivationKind                                                      | Based on the existing [ActivationKind](https://docs.microsoft.com/en-us/uwp/api/Windows.ApplicationModel.Activation.ActivationKind).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |

### Properties, methods and sub-types

Most of the existing XXXActivatedEventArgs types include properties such as the
User, SplashScreen, PreviousExecutionState and ActivationViewSwitcher for
example. These are low-priority for v1, as they are less-commonly used.

In defining an API that is usable by both unpackged Win32 apps and UWP apps,
inevitably there will be some features that are not applicable to one or other
type of consumer. However, it is important to maximize consistency with the
existing platform APIs. The approach here is to implement only those UWP
interfaces in v1 that make sense for Win32 apps. For example, in UWP,
ProtocolActivatedEventArgs implements 5 interfaces, of which only 2 define
members that are interesting to Win32 apps - so for v1, the proposal is to
implement only those 2 interfaces; and defer consideration of the additional
interfaces to a later release.

The following table lists the existing APIs that will be implemented in
AppLifecycle in Reunion v1.

| API                                                                                                                                                    | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| ------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| interface IActivatedEventArgs<br> {<br> ActivationKind Kind,<br> ApplicationExecutionState PreviousExecutionState,<br> SplashScreen SplashScreen<br> } | All XXXActivatedEventArgs types implement this interface.<br> **Kind**: enum value indicating the type of this activation. <br>**PreviousExecutionState**: in UWP, this gets the execution state of the app before this activation. The app can use this information to determine whether it should restore saved state. For Win32, the value will be ApplicationExecutionState.NotRunning. <br>**SplashScreen**: in UWP, this gets the execution state of the app before this activation. The app can use this information to determine whether it should restore saved state. For Win32, the value will be ApplicationExecutionState. NotRunning. |
| class LaunchActivatedEventArgs                                                                                                                         | Implements IActivatedEventArgs, plus the following:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| interface ILaunchActivatedEventArgs<br> {<br> string Arguments;<br> string TileId; <br> }<br>                                                          | **Arguments** is the minimum required property for this activation type. <br>**TileId** is not applicable for Win32, and will return an empty string.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| class FileActivatedEventArgs                                                                                                                           | Implements IActivatedEventArgs, plus the following:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| interface IFileActivatedEventArgs<br> {<br> IVectorView<IStorageItem> Files;<br> string Verb;<br> } <br>                                               | **Files** is the minimum required property for this activation type. <br>**Verb** is an arbitrary string that indicates the action associated with the activated file (open, delete, etc).                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| class ProtocolActivatedEventArgs                                                                                                                       | Implements IActivatedEventArgs, plus the following:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| interface IProtocolActivatedEventArgs<br> {<br> Uri Uri;<br> }<br>                                                                                     | **Uri** is the minimum required property for this activation type.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| class StartupTaskActivatedEventArgs                                                                                                                    | Implements IActivatedEventArgs, plus the following:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| interface IStartupTaskActivatedEventArgs<br> {<br> string TaskId;<br> }<br>                                                                            | **TaskId** is an app-defined value; the app can use this later in the StartupTask API to request enabling or disabling of the startup behavior.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |

Also see the related API Spec AppLifecycle - Single/Multi-instancing, which
describes the new Activated event. An app can use GetActivatedEventArgs even if
it is not choosing to use multi-instance redirection. If it is using
multi-instance redirection, then it can use GetActivatedEventArgs and also
handle the Activated event.

### Activation registration

For most activation kinds, the optional attributes that are available for the
traditional registry-based activation registrations are roughly equivalent to
those available for UWP manifest-based registration. The exception to this is
file-type association, where traditional registry activation offers 5 optional
attributes, whereas UWP offers 37 optional attributes. Most of the UWP optional
attributes are very rarely used, and in some cases there are no apps in the
Store that use a given parameter. For v1, the platform will support all the
registry-based optional attributes - further UWP attributes may be included in a
later release.

The new RegisterForXXXActivation methods will be exposed as static methods from
a new ActivationRegistrationManager class in the AppLifecycle component.

The aim is that the app would call any of these RegisterForXXXActivation methods
at any time dynamically from the app itself. For v1, this API does not support
calling from the app's installer. For filetype and protocol, the app can call
the RegisterForXXXActivation method multiple times, each time providing
different filetype names or different protocol names. These registrations would
be cumulative, so long as the parameters are different. If the app tries to
register for the same filetype name or the same protocol multiple times, this is
idempotent (there's only one registration per unique filetype name or protocol
name; no error for multiple identical registrations, any new values supplied for
the same registration overwrite any old values).

Similarly, if the app registers multiple times for startup, this is also
idempotent (a single registration, no error, any new values supplied overwrite
any old values).

| API                                                                                                                                              | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| ------------------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| class ActivationRegistrationManager                                                                                                              | New class to encapsulate the activation RegisterForXXXActivation and UnregisterForXXXActivation methods.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| static void RegisterForFileTypeActivation(<br> string supportedFileTypes[],<br> string logo,<br> int? index,<br> string? supportedVerbs[]<br> ); | **supportedFileTypes**: one or more supported filetypes, specified by the file extension including the leading ".", eg ".docx".<br> **logo**: used in File Explorer for this filetype if this app is the default handler. In UWP this is a package-relative path to an image resource file. In Win32 registry-based filetype registrations, it's a filepath to a binary file plus a resource index. The caller can pass either.<br> **index**: optional - if this is not supplied, we will interpret the logo string as a package-relative path to an image resource file. If index is supplied, we will interpret the logo string as a binary filepath, and the index as the resource index in that binary.<br> **supportedVerbs**: optional - zero or more app-defined verbs. Each verb is added to the File Explorer context menu when a registered file is right-clicked; and the selected verb is passed to the app as the FileActivatedEventArgs::Verb property. |
| static void RegisterForProtocolActivation(<br> string name<br> );                                                                                | **name**: the protocol identifier, eg "foo" or "https".                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| static void RegisterForStartupActivation(<br> string taskId,<br> bool isEnabled,<br> string displayName<br> );                                   | **taskId**: a string identifier which the app can use in the StartupTask API to request enabling or disabling of the startup behavior.<br> **isEnabled**: enables the startup behavior without a user prompt, so long as the user has activated the app at least once. Consistent with existing Desktop Bridge behavior.<br> **displayName**: used in the Settings - Apps - Startup page.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |

### Activation unregistration

The new API will include UnregisterForXXXActivation methods for each of the
supported activation kinds.

| API                                                                        | Description                                                                   |
| -------------------------------------------------------------------------- | ----------------------------------------------------------------------------- |
| static void UnregisterForFileTypeActivation(<br> string fileTypes[]<br> ); | **fileTypes**: one or more filetypes that the caller wants to unregister.     |
| static void UnregisterForProtocolActivation(<br> string name<br> );        | **name**: the previously-registered protocol identifier, eg "foo" or "https". |
| static void UnregisterForStartupActivation();                              |                                                                               |

<br>
<br>
